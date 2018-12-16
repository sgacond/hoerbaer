#include <iostream>
#include <stdexcept>
#include <memory>
#include <climits>

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "driver/i2c.h"

#include "../../Configuration.h"
#include "IAudioFile.h"
#include "WavFile.h"
#include "Mp3File.h"
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

    if (0 == filename.compare(filename.length() - 3, 3, "WAV")) {
        std::cout << "READ " << filename << std::endl;
        audioFile = std::make_unique<WavFile>(this->storage);
    } 
    else if (0 == filename.compare(filename.length() - 3, 3, "MP3")) {
        std::cout << "READ " << filename << std::endl;
        audioFile = std::make_unique<Mp3File>(this->storage);
    }
    else
        throw std::runtime_error("Unable to read" + filename + ". Only WAV or MP3 files are supported.");

    auto audioFileInfo = audioFile->Load(filename);

    std::cout << "Audio file loaded: " << audioFileInfo.pcmSamplerate 
                              << " / " << audioFileInfo.pcmBits << "bit" 
                              << " / " << audioFileInfo.pcmChannels << "ch"
                              << " / " << audioFileInfo.durationSeconds << "s"
                              << " / " << audioFileInfo.recommendedBufferSize << "bytes buffer" << std::endl;

    this->setSamplerateBits(audioFileInfo.pcmSamplerate, audioFileInfo.pcmBits);

    size_t bufferSize = audioFileInfo.recommendedBufferSize, samplesRead = 0, i2s_bytes_written = 0;
    auto buffer = (unsigned short *) malloc(bufferSize);

    while(!audioFile->Eof()) {
        samplesRead = audioFile->StreamSamples(buffer, bufferSize);
        i2s_write(I2S_NUM, buffer, samplesRead, &i2s_bytes_written, 100);
        // delay time: braucht's das? glaub nicht.
        // vTaskDelay(8/portTICK_RATE_MS);
    }

    free(buffer);
}

void AudioPlayer::SetVolume(float vol) {
    uint8_t addr = CODEC_I2C_ADDR;

    uint8_t volScaled = (uint8_t)((vol * CODEC_MAX_VOL) + 0.5f);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, 0x1);
    i2c_master_write_byte(cmd, volScaled, 0x1);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) throw std::runtime_error("Failed to write i2c volume.");
    std::cout << "AUDIO: Writen Volume: " << static_cast<int>(volScaled) << "/" << CODEC_MAX_VOL << std::endl;
}

void AudioPlayer::setSamplerateBits(int sample_rate, int bits) {

    if(this->sample_rate == sample_rate && this->bits == bits)
        return;

    this->sample_rate = sample_rate;
    this->bits = bits;

    esp_err_t ret = i2s_set_clk(I2S_NUM, sample_rate, (i2s_bits_per_sample_t)bits, (i2s_channel_t)2);
    if (ret != ESP_OK) throw std::runtime_error("Failed to adjust samplerate / bits.");

}
