#include <iostream>
#include <memory>
#include <GPIO.h>
#include <PWM.h>
#include "esp_log.h"

#include "../../Configuration.h"
#include "HBI.h"

static const char* LOG_TAG = "HBI";

using namespace std;
using namespace ESP32CPP;

HBI::HBI() {
    // init shift register stuff...
    this->setStackSize(2048); // NOT YET CALIBRATED TO LIMIT - can be decreased i think
    this->setPriority(TSK_PRIO_HBI);
    this->setName("HBI");

    this->queue = xQueueCreate(QUEUE_LEN_HBISHIFT, sizeof(uint32_t));
    this->shiftTask = make_unique<HBIShift>(this->queue);
    this->leftEyePWM = make_unique<PWM>(PIN_HBI_EYEL, 100, LEDC_TIMER_10_BIT, LEDC_TIMER_0, LEDC_CHANNEL_1);
    this->rightEyePWM = make_unique<PWM>(PIN_HBI_EYER, 100, LEDC_TIMER_10_BIT, LEDC_TIMER_0, LEDC_CHANNEL_2);
}

HBI::~HBI() {
}

void HBI::run(void *pvParameters) {

    GPIO::setOutput(PIN_HBI_PWM);
    GPIO::write(PIN_HBI_PWM, false); // PWM LOW -> LEDS ON.
    this->shiftTask->start();
    
    uint32_t valReceived;
    while(1) {
        if (xQueueReceive(this->queue, &valReceived, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(LOG_TAG, "EVENT: 0x%08x", valReceived);
        }
    }
}

void HBI::setEyes(uint8_t leftPercentage, uint8_t rightPercentage) {
    this->leftEyePWM->setDutyPercentage(leftPercentage);
    this->rightEyePWM->setDutyPercentage(rightPercentage);
}