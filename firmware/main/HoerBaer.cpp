#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

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
    this->curPaw = 0;
    this->curIdxOnPaw = 0xFF; // little cheat: +1 = 0

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
        FreeRTOS::sleep(200);

        this->audioPlayer->InitCodec();
        this->audioPlayer->SetVolume(this->curVol);
        ESP_LOGI(LOG_TAG, "Audio initialized. Volume: %d", this->curVol);

        this->hbi->start();
        this->hbi->setEyes(30, 30);
        ESP_LOGI(LOG_TAG, "HBI tasks started.");

        this->storage->Init();
        ESP_LOGI(LOG_TAG, "Storage initialized.");

        this->loadPawsFromStorage();
        ESP_LOGI(LOG_TAG, "Loaded paws files %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u.",
            this->pawFiles[0].size(), this->pawFiles[1].size(), this->pawFiles[2].size(), this->pawFiles[3].size(), 
            this->pawFiles[4].size(), this->pawFiles[5].size(), this->pawFiles[6].size(), this->pawFiles[7].size(), 
            this->pawFiles[8].size(), this->pawFiles[9].size(), this->pawFiles[10].size(), this->pawFiles[11].size(), 
            this->pawFiles[12].size(), this->pawFiles[13].size(), this->pawFiles[14].size(), this->pawFiles[15].size());

        uint32_t samplesPaused = 0;
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
                case CMD_PLAY: {
                    auto fileToPlay = this->getCurrentAudioFile();
                    if(fileToPlay.empty()) {
                        ESP_LOGW(LOG_TAG, "Unable to play, no file loaded.");
                        break;
                    }
                    this->audioPlayer->PlayFile(fileToPlay, samplesPaused); 
                    this->hbi->setPawPlaying(this->curPaw);
                    break;
                }
                case CMD_PAUSE: {
                    samplesPaused = this->audioPlayer->samplesPlayed;
                    this->audioPlayer->Stop();
                    break;
                }
                case CMD_REV: this->playPrev(); break;
                case CMD_FWD: this->playNext(); break;
                case CMD_PAW_1: this->playNextFromPaw(0); break;
                case CMD_PAW_2: this->playNextFromPaw(1); break;
                case CMD_PAW_3: this->playNextFromPaw(2); break;
                case CMD_PAW_4: this->playNextFromPaw(3); break;
                case CMD_PAW_5: this->playNextFromPaw(4); break;
                case CMD_PAW_6: this->playNextFromPaw(5); break;
                case CMD_PAW_7: this->playNextFromPaw(6); break;
                case CMD_PAW_8: this->playNextFromPaw(7); break;
                case CMD_PAW_9: this->playNextFromPaw(8); break;
                case CMD_PAW_10: this->playNextFromPaw(9); break;
                case CMD_PAW_11: this->playNextFromPaw(10); break;
                case CMD_PAW_12: this->playNextFromPaw(11); break;
                case CMD_PAW_13: this->playNextFromPaw(12); break;
                case CMD_PAW_14: this->playNextFromPaw(13); break;
                case CMD_PAW_15: this->playNextFromPaw(14); break;
                case CMD_PAW_16: this->playNextFromPaw(15); break;
                case CMD_SHUTDN: this->shutdown(); break;
            }

            if(this->audioPlayer->Eof()) {
                ESP_LOGI(LOG_TAG, "Audio EOF, play next song.");
                this->playNext();
            }
            // printf("MP: %u / %f s\n", this->audioPlayer->samplesPlayed, this->audioPlayer->samplesPlayed / (float)44100.0);
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

void HoerBaer::playNextFromPaw(uint8_t idx) {
    if(this->curPaw == idx) {
        this->curIdxOnPaw++;
        if(this->pawFiles[this->curPaw].size() <= this->curIdxOnPaw)
            this->curIdxOnPaw = 0;
    }
    else {
        this->curPaw = idx;
        this->curIdxOnPaw = 0;
        while(this->pawFiles[this->curPaw].empty()) {
            if(this->curPaw == 15)
                this->curPaw = 0;
            else
                this->curPaw++;
        }
    }
    auto file = this->getCurrentAudioFile();
    ESP_LOGI(LOG_TAG, "Playing next from paw %d, idx to play: %d, file: %s", this->curPaw, this->curIdxOnPaw, file.c_str());
    this->audioPlayer->PlayFile(file);
    this->hbi->setPawPlaying(this->curPaw);
}

void HoerBaer::playNext() {
    this->curIdxOnPaw++;
    if(this->pawFiles[this->curPaw].size() <= this->curIdxOnPaw) {
        this->curIdxOnPaw = 0;
        do {
            this->curPaw++;
            if(this->curPaw > 15)
                this->curPaw = 0;
            ESP_LOGI(LOG_TAG, "Skipping to paw %d", this->curPaw);
        } while(this->pawFiles[this->curPaw].empty());
    }
    auto file = this->getCurrentAudioFile();
    ESP_LOGI(LOG_TAG, "Playing next, paw %d, idx to play: %d, file: %s", this->curPaw, this->curIdxOnPaw, file.c_str());
    this->audioPlayer->PlayFile(file);
    this->hbi->setPawPlaying(this->curPaw);
}

void HoerBaer::playPrev() {
    if(this->curIdxOnPaw > 0)
        this->curIdxOnPaw--;
    else {
        do {
            if(this->curPaw == 0)
                this->curPaw = 15;
            else
                this->curPaw--;
        } while(this->pawFiles[this->curPaw].empty());
        this->curIdxOnPaw = this->pawFiles[this->curPaw].size() - 1;
    }
    auto file = this->getCurrentAudioFile();
    ESP_LOGI(LOG_TAG, "Playing prev, paw %d, idx to play: %d, file: %s", this->curPaw, this->curIdxOnPaw, file.c_str());
    this->audioPlayer->PlayFile(file);
    this->hbi->setPawPlaying(this->curPaw);
}

void HoerBaer::loadPawsFromStorage() {
    this->pawFiles[0] = this->storage->ListDirectoryFiles("PAW1");
    this->pawFiles[1] = this->storage->ListDirectoryFiles("PAW2");
    this->pawFiles[2] = this->storage->ListDirectoryFiles("PAW3");
    this->pawFiles[3] = this->storage->ListDirectoryFiles("PAW4");
    this->pawFiles[4] = this->storage->ListDirectoryFiles("PAW5");
    this->pawFiles[5] = this->storage->ListDirectoryFiles("PAW6");
    this->pawFiles[6] = this->storage->ListDirectoryFiles("PAW7");
    this->pawFiles[7] = this->storage->ListDirectoryFiles("PAW8");
    this->pawFiles[8] = this->storage->ListDirectoryFiles("PAW9");
    this->pawFiles[9] = this->storage->ListDirectoryFiles("PAW10");
    this->pawFiles[10] = this->storage->ListDirectoryFiles("PAW11");
    this->pawFiles[11] = this->storage->ListDirectoryFiles("PAW12");
    this->pawFiles[12] = this->storage->ListDirectoryFiles("PAW13");
    this->pawFiles[13] = this->storage->ListDirectoryFiles("PAW14");
    this->pawFiles[14] = this->storage->ListDirectoryFiles("PAW15");
    this->pawFiles[15] = this->storage->ListDirectoryFiles("PAW16");
}

std::string HoerBaer::getCurrentAudioFile() {
    auto vec = &this->pawFiles[this->curPaw];    
    if(vec->size() < (this->curIdxOnPaw+1)) {
        ESP_LOGW(LOG_TAG, "Unable to get audio file. Invalid idx on paw.");
        return "";
    }
    auto filename = vec->at(this->curIdxOnPaw);
    char filepath[20];
    snprintf(filepath, 20, "PAW%u/%s", this->curPaw+1, filename.c_str());
    return string(filepath);
}

void HoerBaer::checkUnderVoltageShutdown() {
    if(!this->batteryGuard->ShutDownVoltageExceeded())
        return;
    ESP_LOGW(LOG_TAG, "Shutdown voltage limit exceeded! Shutting down.");
}

void HoerBaer::shutdown() {
    this->shouldBeRunning = false;
}