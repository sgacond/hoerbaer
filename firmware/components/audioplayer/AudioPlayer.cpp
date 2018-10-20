#include <iostream>
#include <stdexcept>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "driver/i2c.h"

#include "../../Configuration.h"
#include "IAudioFile.h"
#include "WavFile.h"
#include "AudioPlayer.h"

AudioPlayer::AudioPlayer(std::shared_ptr<Storage> storage) {
    std::cout << "AudioPlayer constructed." << std::endl;
    this->storage = storage;
    this->sample_rate = 44100;
    this->bits = 16;
}

AudioPlayer::~AudioPlayer() {
    std::cout << "AudioPlayer destructed." << std::endl;
}

void AudioPlayer::InitCodec() {

    esp_err_t ret;

    //for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes or 8-bytes each sample)
    //depend on bits_per_sample
    //using 6 buffers, we need 60-samples per buffer
    //if 2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
    //if 2-channels, 24/32-bit each channel, total buffer is 360*8 = 2880 bytes
    std::cout << "INIT I2S, " << sample_rate << ", " << bits << "bit" << std::endl;
    i2s_config_t i2s_config = {};
    i2s_config.mode                    = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.sample_rate             = this->sample_rate;
    i2s_config.bits_per_sample         = (i2s_bits_per_sample_t) this->bits;
    i2s_config.channel_format          = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.communication_format    = (i2s_comm_format_t) (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config.dma_buf_count           = 6;
    i2s_config.dma_buf_len             = 60;
    i2s_config.use_apll                = true;
    i2s_config.intr_alloc_flags        = ESP_INTR_FLAG_LEVEL1;
    
    ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2s interface.");

    std::cout << "INIT I2S PINS" << std::endl;
    i2s_pin_config_t pin_config = {};
    pin_config.bck_io_num              = PIN_I2S_BCK;
    pin_config.ws_io_num               = PIN_I2S_LRCK;
    pin_config.data_out_num            = PIN_I2S_OUT;
    pin_config.data_in_num             = -1;

    ret = i2s_set_pin(I2S_NUM, &pin_config);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2s pin configuration.");

    // MCLK OUTPUT ON PIN 1 (DIRECT FROM I2S_0)
    // https://esp32.com/viewtopic.php?f=5&t=1585&start=10
    std::cout << "INIT MCK OUTPUT" << std::endl;
    REG_WRITE(PIN_CTRL, 0b111111110000);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    // I2C VOLUME CONTROL CHANNEL
    std::cout << "INIT I2C" << std::endl;
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = PIN_I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = PIN_I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    
    ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2c param configuration.");

    ret = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2c driver.");
}

void AudioPlayer::PlayFile(std::string filename) {
    
    std::unique_ptr<IAudioFile> audioFile;

    if (0 == filename.compare ( filename.length() - 3, 3, "WAV")) {
        std::cout << "READ " << filename << std::endl;
        audioFile = std::make_unique<WavFile>(this->storage);
    } 
    else if (0 == filename.compare ( filename.length() - 3, 3, "MP3")) {
        std::cout << "READ " << filename << std::endl;
    }
    else
        throw std::runtime_error("Unable to read" + filename + ". Only WAV or MP3 files are supported.");

    auto audioFileInfo = audioFile->Load(filename);

    std::cout << "Audio file loaded: " << audioFileInfo.pcmSamplerate 
                              << " / " << audioFileInfo.pcmBits << "bit" 
                              << " / " << audioFileInfo.durationSeconds << "s" << std::endl;

    this->setSamplerateBits(audioFileInfo.pcmSamplerate, audioFileInfo.pcmBits);

    int bufferSize = 1440;
    auto buffer = (unsigned short *) malloc(bufferSize);

    while(!audioFile->Eof()) {

        audioFile->StreamSamples(buffer, bufferSize);

    }

    free(buffer);

    // int offset = 36;
    // while(offset < header->overall_size)
    // {
    //     printf("SEEK TO OFFSET: %d \n", offset);
    //     fseek(f, offset, SEEK_SET);

    //     struct data_chunk_header *chunk_header = malloc(sizeof(*chunk_header));
    //     read_data_chunk_header(f, chunk_header);
    //     offset += sizeof(*chunk_header);
    //     fseek(f, offset, SEEK_SET);

    //     if(strncmp(chunk_header->data_chunk_header, "data", 4) == 0)
    //     {
    //         printf("play chunk, %.*s, length %d \n", 4, chunk_header->data_chunk_header, chunk_header->data_size);

    //         int buffer_size = 1440;
    //         unsigned short *buffer = malloc(buffer_size);
    //         size_t i2s_bytes_written = 0;

    //         while(offset < chunk_header->data_size)
    //         {
    //             fread(buffer, buffer_size, 1, f);
    //             i2s_write(I2S_NUM, buffer, buffer_size, &i2s_bytes_written, 100);

    //             // delay time: 44100 samples per seconds, 1440 / 2 / 2 = 360 samples written, ca 8ms
    //             vTaskDelay(8/portTICK_RATE_MS);

    //             offset += buffer_size;
    //             fseek(f, offset, SEEK_SET);
    //         }

    //         free(buffer);
    //     }
    //     else 
    //     {
    //         printf("skip chunk, %.*s, length %d \n", 4, chunk_header->data_chunk_header, chunk_header->data_size);
    //         offset += chunk_header->data_size;
    //     }
    // }
}

void AudioPlayer::setSamplerateBits(int sample_rate, int bits) {

    if(this->sample_rate == sample_rate && this->bits == bits)
        return;

    this->sample_rate = sample_rate;
    this->bits = bits;

    esp_err_t ret = i2s_set_clk(I2S_NUM, sample_rate, (i2s_bits_per_sample_t)bits, (i2s_channel_t)2);
    if (ret != ESP_OK) throw std::runtime_error("Failed to adjust samplerate / bits.");

}
