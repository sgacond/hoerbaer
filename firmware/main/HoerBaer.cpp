#include <iostream>
#include <memory>

#include "esp_log.h"
#include <freertos/FreeRTOS.h>   // Include the base FreeRTOS definitions.
#include <freertos/task.h>
#include <GPIO.h>
#include <FreeRTOS.h>
#include "esp_heap_caps.h"

#include "../Configuration.h"

#include "HoerBaer.h"

static const char* LOG_TAG = "MAIN";

using namespace std;
using namespace ESP32CPP;

HoerBaer::HoerBaer() {
    this->storage = make_shared<Storage>();
    this->hbi = make_unique<HBI>();
    this->audioPlayer = make_unique<AudioPlayer>(this->storage); // move() nicht mehr?
    this->batteryGuard = make_unique<BatteryGuard>();
    this->curVol = 16;
    this->shouldBeRunning = true;

    ESP_LOGI(LOG_TAG, "Baer initialized: initial volume: %d", this->curVol);

    GPIO::setOutput(PIN_PERIPH_MAIN);
}

HoerBaer::~HoerBaer() {
    ESP_LOGI(LOG_TAG, "Baer shut down.");
}

void HoerBaer::run() {
    try {
        ESP_LOGI(LOG_TAG, "Baer starting up.");

        this->checkUnderVoltageShutdown();

        this->enablePeripherials();
        ESP_LOGI(LOG_TAG, "Peripherials enabled.");

        this->storage->Init();
        ESP_LOGI(LOG_TAG, "Storage initialized.");

        this->hbi->start();
        this->hbi->setEyes(30, 30);
        ESP_LOGI(LOG_TAG, "HBI tasks started.");

        this->audioPlayer->InitCodec();
        this->audioPlayer->SetVolume(this->curVol);
        ESP_LOGI(LOG_TAG, "Audio initialized. Volume: %d", this->curVol);

        uint32_t samplesPaused = 0;
        std::string filePlaying = "";

        uint32_t loopCount = 0;
        while(this->shouldBeRunning) {
            loopCount++;

            // check battery?           
            if(loopCount % MAIN_NTHLOOP_BATCHK == 0)
                this->checkUnderVoltageShutdown();

            auto cmd = hbi->getCommandFromQueue();
            switch(cmd) {
                case CMD_VOL_UP: this->increaseVolume(); break;
                case CMD_VOL_DN: this->decreaseVolume(); break;
                case CMD_PLAY: 
                    if(filePlaying.empty()) {
                        ESP_LOGW(LOG_TAG, "Unable to play, no file loaded.");
                        break;
                    }
                    this->audioPlayer->PlayFile(filePlaying, samplesPaused); 
                    break;
                case CMD_PAUSE:
                    samplesPaused = this->audioPlayer->samplesPlayed;
                    this->audioPlayer->Stop();
                    break;
                case CMD_PAW_1: 
                    filePlaying = "1.WAV";
                    this->audioPlayer->PlayFile(filePlaying, 0); 
                    break;
                case CMD_PAW_2: 
                    filePlaying = "2.WAV";
                    this->audioPlayer->PlayFile(filePlaying, 0); 
                    break;
                case CMD_PAW_3: 
                    filePlaying = "3.WAV";
                    this->audioPlayer->PlayFile(filePlaying, 0); 
                    break;
                case CMD_SHUTDN: this->shutdown(); break;
            }

            printf("MP: %u / %f s\n", this->audioPlayer->samplesPlayed, this->audioPlayer->samplesPlayed / (float)44100.0);
        }
        
        this->audioPlayer->Stop();
        this->disablePeripherials();

        ESP_LOGI(LOG_TAG, "Waiting until nose released...");
        while(hbi->nosePressed())
            FreeRTOS::sleep(200);
    }
    catch (std::runtime_error &e) {
        ESP_LOGE(LOG_TAG, "Caught a runtime_error exception: %s", e.what());
        this->disablePeripherials();
    }
    catch (std::exception &e) {
        ESP_LOGE(LOG_TAG, "Caught an exception of an unexpected type: %s", e.what());
        this->disablePeripherials();
    } 
    catch (...) {
        ESP_LOGE(LOG_TAG, "Caught an unknown exception.");
        this->disablePeripherials();
    }
}

void HoerBaer::enablePeripherials() {
    GPIO::write(PIN_PERIPH_MAIN, true);
}

void HoerBaer::disablePeripherials() {
    GPIO::write(PIN_PERIPH_MAIN, false);
}

void HoerBaer::increaseVolume() {
    if(this->curVol == CODEC_MAX_VOL)
        return;
    this->curVol++;
    ESP_LOGI(LOG_TAG, "Increase volume to %d", this->curVol);
    this->audioPlayer->SetVolume(this->curVol);
}

void HoerBaer::decreaseVolume() {
    if(this->curVol == 0)
        return;
    this->curVol--;
    ESP_LOGI(LOG_TAG, "decrease volume to %d", this->curVol);
    this->audioPlayer->SetVolume(this->curVol);
}

void HoerBaer::checkUnderVoltageShutdown() {
    if(!this->batteryGuard->ShutDownVoltageExceeded())
        return;
    ESP_LOGW(LOG_TAG, "Shutdown voltage limit exceeded! Shutting down.");
}

void HoerBaer::shutdown() {
    this->shouldBeRunning = false;
}