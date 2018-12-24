#include <iostream>
#include <stdexcept>
#include <memory>
#include <climits>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "driver/i2c.h"

#include "../../Configuration.h"
#include "PlaybackBase.h"
#include "WavPlayback.h"
#include "Mp3Playback.h"
#include "AudioPlayer.h"

static const char* LOG_TAG = "AUDIO";

AudioPlayer::AudioPlayer(std::shared_ptr<Storage> storage) {
    ESP_LOGI(LOG_TAG, "AudioPlayer constructed.");
    this->storage = storage;
    this->sample_rate = 44100;
    this->bits = 16;
    this->samplesPlayed = 0;
}

AudioPlayer::~AudioPlayer() {
    i2c_driver_delete(I2C_NUM_0);
    ESP_LOGI(LOG_TAG, "AudioPlayer destructed.");
}

void AudioPlayer::InitCodec() {

    esp_err_t ret;

    // I2C VOLUME CONTROL CHANNEL
    ESP_LOGI(LOG_TAG, "Inititialize I2C");
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = PIN_I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = PIN_I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;

    ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2c param configuration.");

    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2c driver.");

    //for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes or 8-bytes each sample)
    //depend on bits_per_sample
    //using 6 buffers, we need 60-samples per buffer
    //if 2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
    //if 2-channels, 24/32-bit each channel, total buffer is 360*8 = 2880 bytes
    ESP_LOGI(LOG_TAG, "Initialize I2S, %d, %dbit", sample_rate, bits);
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

    ESP_LOGI(LOG_TAG, "Inititialize I2S pins");
    i2s_pin_config_t pin_config = {};
    pin_config.bck_io_num              = PIN_I2S_BCK;
    pin_config.ws_io_num               = PIN_I2S_LRCK;
    pin_config.data_out_num            = PIN_I2S_OUT;
    pin_config.data_in_num             = -1;

    ret = i2s_set_pin(I2S_NUM, &pin_config);
    if (ret != ESP_OK) throw std::runtime_error("Failed to initialize the i2s pin configuration.");

    // MCLK OUTPUT ON PIN 1 (DIRECT FROM I2S_0)
    // https://esp32.com/viewtopic.php?f=5&t=1585&start=10
    ESP_LOGI(LOG_TAG, "Inititialize MCK output");
    REG_WRITE(PIN_CTRL, 0b111111110000);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    vTaskDelay(100/portTICK_RATE_MS);
}

void AudioPlayer::PlayFile(std::string filename, uint32_t samplesStart) {
    
    if(this->audioPlayback) {
        ESP_LOGW(LOG_TAG, "Stopping current playing file.");
        this->Stop();
    }

    if (0 == filename.compare(filename.length() - 3, 3, "WAV")) {
        ESP_LOGI(LOG_TAG, "Reading %s", filename.c_str());
        this->audioPlayback = std::make_unique<WavPlayback>(this->storage, I2S_NUM, &this->samplesPlayed);
    } 
    else if (0 == filename.compare(filename.length() - 3, 3, "MP3")) {
        ESP_LOGI(LOG_TAG, "Reading %s", filename.c_str());
        this->audioPlayback = std::make_unique<Mp3Playback>(this->storage, I2S_NUM, &this->samplesPlayed);
    }
    else
        throw std::runtime_error("Unable to read" + filename + ". Only WAV or MP3 files are supported.");

    auto audioPbInfo = this->audioPlayback->Load(filename);

    ESP_LOGI(LOG_TAG, "Audio file loaded: %d / %dbit / %dch / %fs / %zd bytes buffer", 
        audioPbInfo.pcmSamplerate, audioPbInfo.pcmBits, audioPbInfo.pcmChannels, 
        audioPbInfo.durationSeconds, audioPbInfo.recommendedBufferSize);

    this->setSamplerateBits(audioPbInfo.pcmSamplerate, audioPbInfo.pcmBits);
    this->samplesPlayed = 0;

    if(samplesStart > 0)
        this->audioPlayback->SeekToSamples(samplesStart);

    this->audioPlayback->start();
}

void AudioPlayer::Stop() {
    if(!this->audioPlayback) {
        ESP_LOGW(LOG_TAG, "Unable to stop - nothing playing.");
        return;
    }
    this->audioPlayback->stop();
    this->audioPlayback.reset();
}

void AudioPlayer::SetVolume(uint8_t vol) {
    uint8_t addr = CODEC_I2C_ADDR;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, 0x1);
    i2c_master_write_byte(cmd, vol, 0x1);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 300 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);
    vTaskDelay(100/portTICK_RATE_MS);

    if (ret != ESP_OK) 
        ESP_LOGW(LOG_TAG, "Failed to write i2c volume.");
    else
        ESP_LOGI(LOG_TAG, "AUDIO: Writen Volume: %d/%d", vol, CODEC_MAX_VOL);
}

void AudioPlayer::setSamplerateBits(int sample_rate, int bits) {

    if(this->sample_rate == sample_rate && this->bits == bits)
        return;

    this->sample_rate = sample_rate;
    this->bits = bits;

    esp_err_t ret = i2s_set_clk(I2S_NUM, sample_rate, (i2s_bits_per_sample_t)bits, (i2s_channel_t)2);
    if (ret != ESP_OK) throw std::runtime_error("Failed to adjust samplerate / bits.");
    ESP_LOGI(LOG_TAG, "Set samplerate to %d / %dbits", sample_rate, bits);

}
