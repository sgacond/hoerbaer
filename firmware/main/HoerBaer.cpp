#include <iostream>
#include <memory>

#include "esp_log.h"
#include <freertos/FreeRTOS.h>   // Include the base FreeRTOS definitions.
#include <freertos/task.h>
#include <GPIO.h>
#include "esp_heap_caps.h"

#include "../Configuration.h"

#include "HoerBaer.h"

static const char* LOG_TAG = "MAIN";

using namespace std;
using namespace ESP32CPP;

HoerBaer::HoerBaer() {
    this->storage = make_shared<Storage>(); // TODO: WAS WAR DAS PROBLEM MIT UNIQUE?
    this->hbi = make_shared<HBI>();
    this->audioPlayer = make_shared<AudioPlayer>(this->storage); // move() nicht mehr?
    this->curVol = 16;
    ESP_LOGI(LOG_TAG, "Baer initialized: initial volume: %d", this->curVol);
}

HoerBaer::~HoerBaer() {
}

void HoerBaer::run() {
    try {
        ESP_LOGI(LOG_TAG, "Baer started.");

        GPIO::setOutput(PIN_PERIPH_MAIN);
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

        while(1) {
            auto cmd = hbi->getCommandFromQueue();
            switch(cmd) {
                case CMD_VOL_UP: this->increaseVolume(); break;
                case CMD_VOL_DN: this->decreaseVolume(); break;
                case CMD_PLAY: this->audioPlayer->PlayFile("1.WAV"); break;
                case CMD_STOP: this->audioPlayer->Stop(); break;
            }
        }
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
