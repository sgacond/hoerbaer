#include <iostream>
#include <cstring>
#include "esp_log.h"

#include "../../Configuration.h"
#include "WavPlayback.h"

#define WAV_DEFAULT_BUFFER_SIZE 1440
static const char* LOG_TAG = "WAV";

struct wave_header {
	char riff[4];						// RIFF string
	uint32_t overall_size; 				// overall size of file in bytes
	char wave[4];						// WAVE string
};

struct chunk_header {
    char type[4];						// type
	uint32_t size;     		    		// overall size of file in bytes
};

struct fmt_chunk {
    char fmt_chunk_marker[4];			// fmt string with trailing null char
	uint32_t length_of_fmt;				// length of the format data
	uint16_t format_type;	    		// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint16_t channels;					// no.of channels
	uint32_t sample_rate;				// sampling rate (blocks per second)
	uint32_t byterate;					// SampleRate * NumChannels * BitsPerSample/8
	uint16_t block_align;				// NumChannels * BitsPerSample/8
	uint16_t bits_per_sample;			// bits per sample, 8- 8bits, 16- 16 bits etc
};

struct data_chunk_header {
    char data_chunk_header [4]; 		// DATA string or FLLR string
	unsigned int data_size;				// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

WavPlayback::WavPlayback(std::shared_ptr<Storage> storage, i2s_port_t i2sPort, uint32_t * samplesPlayed) {
    this->storage = storage;
    this->fp = 0;
    this->fileSize = 0;
    this->curFileOffset = 0;
    this->curChunkRemaining = 0;
    this->i2sPort = i2sPort;
    this->samplesPlayed = samplesPlayed;
    this->samplesToStart = 0;
    this->sampleBytesPerSecond = 0;
    this->oneSampleBytes = 0;

    this->setStackSize(2048); // NOT YET CALIBRATED TO LIMIT - can be decreased i think
    this->setPriority(TSK_PRIO_WAVPLAY);
    this->setName("WAVPlay");
}

WavPlayback::~WavPlayback() {
    if(this->fp)
        this->storage->Close(this->fp);
    ESP_LOGD(LOG_TAG, "Playback closed. File closed.");
}

AudioFileInfo WavPlayback::Load(std::string filename) {
    
    // OPEN File
    this->fp = this->storage->OpenRead(filename);

    // Parse RIFF Header
    struct wave_header riffHeader = {};
    size_t nRead = sizeof(riffHeader);
    fread(&riffHeader, nRead, 1, this->fp);
    this->curFileOffset += nRead;

    ESP_LOGI(LOG_TAG, "Overall size: %dKB", riffHeader.overall_size/1024);
    this->fileSize = riffHeader.overall_size;

    // Skip until fmt-Header
    struct chunk_header nextChunkHeader = {};
    nRead = sizeof(nextChunkHeader);
    fread(&nextChunkHeader, nRead, 1, this->fp);
    this->curFileOffset += nRead;

    while(strncmp(nextChunkHeader.type, "fmt", 3) != 0) {
        ESP_LOGI(LOG_TAG, "skip header chunk: %s length: %d", nextChunkHeader.type, nextChunkHeader.size);
        fseek(this->fp, nextChunkHeader.size, SEEK_CUR);
        this->curFileOffset += nextChunkHeader.size;
        fread(&nextChunkHeader, nRead, 1, this->fp);
        this->curFileOffset += nRead;
    } 

    fseek(this->fp, -nRead, SEEK_CUR);
    this->curFileOffset -= nRead;

    // Read fmt header
    struct fmt_chunk formatHeader = {};
    nRead = sizeof(formatHeader);
    fread(&formatHeader, nRead, 1, this->fp);
    this->curFileOffset += nRead;

    ESP_LOGI(LOG_TAG, "Format marker: %s", formatHeader.fmt_chunk_marker);
    ESP_LOGI(LOG_TAG, "Format type: %d", formatHeader.format_type);
    ESP_LOGI(LOG_TAG, "Channels: %d", formatHeader.channels);
    ESP_LOGI(LOG_TAG, "Sample rate: %u", formatHeader.sample_rate);
    ESP_LOGI(LOG_TAG, "Bits per sample: %d", formatHeader.bits_per_sample);

    this->oneSampleBytes = (formatHeader.channels * formatHeader.bits_per_sample) / 8;
    ESP_LOGI(LOG_TAG, "Size of each sample: %d bytes.", this->oneSampleBytes);
    this->sampleBytesPerSecond = this->oneSampleBytes * formatHeader.sample_rate;

    float durationSeconds = (float) riffHeader.overall_size / formatHeader.byterate;
    ESP_LOGI(LOG_TAG, "Approx.Duration in seconds: %f", durationSeconds);

    return (AudioFileInfo) { formatHeader.sample_rate, formatHeader.bits_per_sample, formatHeader.channels, durationSeconds, WAV_DEFAULT_BUFFER_SIZE };
}

void WavPlayback::SeekToSamples(uint32_t samples) {
    this->samplesToStart = samples;
}

void WavPlayback::run(void* data) {

    if(!this->fp) throw std::runtime_error("Operation failed. Load file first.");
   
    // auto buffer = (unsigned short *) heap_caps_malloc(WAV_DEFAULT_BUFFER_SIZE, MALLOC_CAP_DMA);
    auto buffer = (unsigned short *) malloc(WAV_DEFAULT_BUFFER_SIZE);
    size_t i2sBytesWritten = 0;
    struct data_chunk_header ch;
    
    auto sampleBytesToSkip = this->samplesToStart * this->oneSampleBytes;

    ESP_LOGI(LOG_TAG, "Play from samples %u (sampleBytesPerSecond: %u) -> %u bytes to skip.", 
        this->samplesToStart, this->sampleBytesPerSecond, sampleBytesToSkip);

    while(!this->Eof()) {

        while(this->curChunkRemaining <= 0 && !this->Eof()) {
            fread(&ch, sizeof(ch), 1, this->fp);
            this->curFileOffset += sizeof(ch);

            ESP_LOGI(LOG_TAG, "CHUNK Data Marker: %s", ch.data_chunk_header);
            ESP_LOGI(LOG_TAG, "CHUNK Size of data chunk: %u", ch.data_size);
            this->curChunkRemaining = ch.data_size;

            if(strncmp(ch.data_chunk_header, "data", 4) != 0) {
                ESP_LOGI(LOG_TAG, "WAV CHUNK skipped.");
                fseek(this->fp, ch.data_size, SEEK_CUR);
                this->curFileOffset += ch.data_size;
                this->curChunkRemaining = 0;
            }
        }

        // seek
        if(sampleBytesToSkip > this->curChunkRemaining) { // whole chunk
            fseek(this->fp, this->curChunkRemaining, SEEK_CUR);
            this->curFileOffset += this->curChunkRemaining;
            sampleBytesToSkip -= this->curChunkRemaining;
            *(this->samplesPlayed) += this->curChunkRemaining / this->oneSampleBytes;
            this->curChunkRemaining = 0;
            ESP_LOGI(LOG_TAG, "SEEK, skipped whole chunk: bytes remaining to seek: %d.\n", sampleBytesToSkip);
            continue;
        }
        else if(sampleBytesToSkip > 0) { // part of the chunck
            fseek(this->fp, sampleBytesToSkip, SEEK_CUR);
            this->curFileOffset += sampleBytesToSkip;
            this->curChunkRemaining -= sampleBytesToSkip;
            *(this->samplesPlayed) += sampleBytesToSkip / this->oneSampleBytes;
            ESP_LOGI(LOG_TAG, "SEEK, less then one chunk, skipped %d bytes.\n", sampleBytesToSkip);
            sampleBytesToSkip = 0;
        }

        int read = WAV_DEFAULT_BUFFER_SIZE;

        if(read > this->curChunkRemaining)
            read = this->curChunkRemaining;

        *(this->samplesPlayed) += read / this->oneSampleBytes;
        this->curChunkRemaining -= read;
        this->curFileOffset += read;

        // read buffer and stream audio
        fread(buffer, read, 1, this->fp);
        i2s_write(this->i2sPort, buffer, read, &i2sBytesWritten, 100);
        this->delay(i2sBytesWritten * 1000UL / this->sampleBytesPerSecond / portTICK_RATE_MS);

    }

    ESP_LOGI(LOG_TAG, "STOPPED, cleaning up.");
    std::memset(buffer, 0, WAV_DEFAULT_BUFFER_SIZE);
    i2s_write(this->i2sPort, buffer, WAV_DEFAULT_BUFFER_SIZE, &i2sBytesWritten, 100);

    heap_caps_free(buffer);
    Task::stop();
}

void WavPlayback::stop() {
    if(this->fp && !this->Eof()) {
        fseek(this->fp, this->fileSize, SEEK_SET);
        this->curFileOffset = this->fileSize;
        vTaskDelay(100/portTICK_RATE_MS); // give the playback task the chance to stop
    }
}

bool WavPlayback::Eof() {
    if(!this->fp) throw std::runtime_error("Operation failed. Load file first.");
    return this->curFileOffset >= this->fileSize;
}
