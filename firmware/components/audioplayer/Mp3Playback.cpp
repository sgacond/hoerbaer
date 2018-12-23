#include <iostream>
#include <cstring>
#include "esp_log.h"

#include "mad.h"
#include "../../Configuration.h"
#include "Mp3Playback.h"

// The theoretical maximum frame size is 2881 bytes,
// MPEG 2.5 Layer II, 8000 Hz @ 160 kbps, with a padding slot plus 8 byte MAD_BUFFER_GUARD.
#define MAX_FRAME_SIZE (2889)

// The theoretical minimum frame size of 24 plus 8 byte MAD_BUFFER_GUARD.
#define MIN_FRAME_SIZE (32)

static const char* LOG_TAG = "MP3";

Mp3Playback::Mp3Playback(std::shared_ptr<Storage> storage, i2s_port_t i2sPort, uint32_t * bytesPlayed) {
    this->storage = storage;
    this->fp = 0;
    this->i2sPort = i2sPort;
    this->bytesPlayed = bytesPlayed;
}

Mp3Playback::~Mp3Playback() {
    if(this->fp)
        this->storage->Close(this->fp);
    ESP_LOGD(LOG_TAG, "Playback closed. File closed.");
}

AudioFileInfo Mp3Playback::Load(std::string filename) {

    // OPEN File
    this->fp = this->storage->OpenRead(filename);

    /* default MAD buffer format */
    this->madBufferFormat.sample_rate = 44100;
    this->madBufferFormat.bit_depth = I2S_BITS_PER_SAMPLE_16BIT;
    this->madBufferFormat.num_channels = 2;
    this->madBufferFormat.buffer_format = PCM_LEFT_RIGHT;

    return (AudioFileInfo) { 44100, 16, 2, 0, MAX_FRAME_SIZE };
}

void Mp3Playback::stop() {
    if(this->fp && !this->Eof()) {
        fseek(this->fp, 0, SEEK_END);
        vTaskDelay(2000/portTICK_RATE_MS); // give the playback task the chance to stop
    }
}

void Mp3Playback::SeekToSamples(uint32_t samples) {
}

bool Mp3Playback::Eof() {
    if(!this->fp) throw std::runtime_error("Operation failed. Load file first.");
    return feof(this->fp);
}

void Mp3Playback::run(void* data) {
    int ret;

    struct mad_stream *stream = (mad_stream*) malloc(sizeof(struct mad_stream));
    struct mad_frame *frame = (mad_frame*) malloc(sizeof(struct mad_frame));
    struct mad_synth *synth = (mad_synth*) malloc(sizeof(struct mad_synth));
    auto buffer = (unsigned char*) malloc(MAX_FRAME_SIZE);

    if (stream==NULL) { ESP_LOGE(LOG_TAG, "malloc(stream) failed\n"); return; }
    if (synth==NULL) { ESP_LOGE(LOG_TAG, "malloc(synth) failed\n"); return; }
    if (frame==NULL) { ESP_LOGE(LOG_TAG, "malloc(frame) failed\n"); return; }
    // if (buf==NULL) { ESP_LOGE(LOG_TAG, "buf_create() failed\n"); return; }

    ESP_LOGI(LOG_TAG, "decoder start");

    //Initialize mp3 parts
    mad_stream_init(stream);
    mad_frame_init(frame);
    mad_synth_init(synth);

    // int64_t t1 = 0, t2 = 0;
    size_t curBufLength = 0;

    while(!this->Eof()) {

        // consume and swap buffers (input equivalent)

        // IRGENDWAS STIMMT DA NOCH GAR NICHT:
        // http://www.beamreach.org/soft/Xbat-win-Matlab/XBAT_PRE_R3/Core/Sound/Formats/MP3/MEX/decode.c

        fread(buffer, MAX_FRAME_SIZE, 1, this->fp);

        // backwards seek to last 0xFFE0 bits set.
        unsigned char *lastFrameEnd = buffer + MAX_FRAME_SIZE;
        while(lastFrameEnd > buffer) {
            lastFrameEnd--;
            if(*lastFrameEnd == 0xFF) // && (*(lastFrameEnd+1) & 0xF0) == 0xF0)
                break;
        }
        
        curBufLength = lastFrameEnd - buffer;

        // rewind file pointer to this
        fseek(this->fp, (curBufLength - MAX_FRAME_SIZE), SEEK_CUR);

        mad_stream_buffer(stream, buffer, curBufLength);
        printf("R: %d\n", curBufLength);

        // decode frames until MAD complains
        while(1) {
            // returns 0 or -1
            // t1 = esp_timer_get_time();
            ret = mad_frame_decode(frame, stream);
            // t2 = esp_timer_get_time();
            // printf("TIME: mad_frame_decode used %jd micros\n", (t2-t1));

            if (ret == -1) {
                if (!MAD_RECOVERABLE(stream->error)) {
                    //We're most likely out of buffer and need to call input() again
                    // ESP_LOGE(LOG_TAG, "dec err 0x%04x (%s) - NOT RECOVERABLE", stream->error, mad_stream_errorstr(stream));
                    break;
                }
                // ESP_LOGE(LOG_TAG, "dec err 0x%04x (%s) - recoverable", stream->error, mad_stream_errorstr(stream));
                continue;
            }
            // t1 = esp_timer_get_time();
            mad_synth_frame(synth, frame);
            // t2 = esp_timer_get_time();
            // printf("TIME: mad_synth_frame used %jd micros\n", (t2-t1));
        }

        // ESP_LOGI(LOG_TAG, "RAM left %d", esp_get_free_heap_size());
        // ESP_LOGI(LOG_TAG, "MAD decoder stack: %d\n", uxTaskGetStackHighWaterMark(NULL));

    }

    ESP_LOGI(LOG_TAG, "STOPPED, cleaning up.");
    size_t written = 0;
    std::memset(buffer, 0, MAX_FRAME_SIZE);
    i2s_write(this->i2sPort, buffer, MAX_FRAME_SIZE, &written, 100);

    free(synth);
    free(frame);
    free(stream);
    heap_caps_free(buffer);
    Task::stop();
}

extern "C" {
    void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels);
    void set_dac_sample_rate(int rate);
}

/* render callback for the libmad synth */
void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels)
{
    // pointer to left / right sample position
    char *ptr_l = (char*)sample_buff_ch0;
    char *ptr_r = (char*)sample_buff_ch1;
    uint8_t stride = sizeof(short);

    if (num_channels == 1) {
        ptr_r = ptr_l;
    }

    size_t bytes_written = 0;
    TickType_t max_wait = 20 / portTICK_PERIOD_MS; // portMAX_DELAY = bad idea
    for (int i = 0; i < num_samples; i++) {
        /* low - high / low - high */
        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]};
        i2s_write(I2S_NUM, (const char *)&samp32, sizeof(samp32), &bytes_written, max_wait);

        // DMA buffer full - retry
        if (bytes_written == 0) {
            i--;
        } else {
            ptr_r += stride;
            ptr_l += stride;
        }
    }
}

// Called by the NXP modifications of libmad. Sets the needed output sample rate.
void set_dac_sample_rate(int rate)
{
    // TODO: REAL? SO OFT?
    // CHECK OB NEU, SONST SEIN LASSEN!!!!
    // i2s_set_sample_rates(0, rate);
}
