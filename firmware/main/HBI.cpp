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
    this->powLedState = 0;

    this->commandQueue = xQueueCreate(QUEUE_LEN_HBICMD, sizeof(uint8_t));
    this->shiftToHBIQUeue = xQueueCreate(QUEUE_LEN_HBISHIFT, sizeof(uint32_t));
    this->shiftTask = make_unique<HBIShift>(this->shiftToHBIQUeue, &(this->powLedState));
    this->leftEyePWM = make_unique<PWM>(PIN_HBI_EYEL, 100, LEDC_TIMER_10_BIT, LEDC_TIMER_0, LEDC_CHANNEL_1);
    this->rightEyePWM = make_unique<PWM>(PIN_HBI_EYER, 100, LEDC_TIMER_10_BIT, LEDC_TIMER_0, LEDC_CHANNEL_2);

    GPIO::setInput(PIN_HBI_NOSE_CLK);
    GPIO::setOutput(PIN_HBI_PWM);
    GPIO::write(PIN_HBI_PWM, true); // PWM HIGH -> LEDS OFF.
}

HBI::~HBI() {
    this->stop();
    this->shiftTask.reset();
    ESP_LOGI(LOG_TAG, "HBI stopped.");
}

void HBI::run(void *pvParameters) {

    this->shiftTask->start();
    this->delay(100);
    GPIO::write(PIN_HBI_PWM, false); // PWM LOW -> LEDS ON.

    uint32_t valReceived;
    uint8_t cmdOut;
    bool nosePressed = false;
    TickType_t noseDownOn = 0, timePressed = 0;
    while(1) {
        cmdOut = 0x00;
        // check if something new from shift registers arrived.
        if (xQueueReceive(this->shiftToHBIQUeue, &valReceived, 100 / portTICK_PERIOD_MS) == pdPASS) {
            if((valReceived & 0xff000000) == 0xff000000) { // Encoder
                if(valReceived == 0xff404000 || valReceived == 0xff4080c0)
                    cmdOut = CMD_VOL_DN;
                else if(valReceived == 0xff40c080 || valReceived == 0xff400040)
                    cmdOut = CMD_VOL_UP;
                else // Unused combinations: left 0xff800080 0xff80c040 right 0xff808000 0xff8040c0
                    continue;
            }
            else { // PAW
                switch(valReceived) {
                    case 0x01000000: cmdOut = CMD_PLAY; break;
                    case 0x00000001: cmdOut = CMD_PAUSE; break;
                    case 0x00010000: cmdOut = CMD_REV; break;
                    case 0x00000100: cmdOut = CMD_FWD; break;
                    case 0x02000000: cmdOut = CMD_PAW_1; break;
                    case 0x04000000: cmdOut = CMD_PAW_2; break;
                    case 0x08000000: cmdOut = CMD_PAW_3; break;
                    case 0x10000000: cmdOut = CMD_PAW_4; break;
                    case 0x00000002: cmdOut = CMD_PAW_5; break;
                    case 0x00000004: cmdOut = CMD_PAW_6; break;
                    case 0x00000008: cmdOut = CMD_PAW_7; break;
                    case 0x00000010: cmdOut = CMD_PAW_8; break;
                    case 0x00020000: cmdOut = CMD_PAW_9; break;
                    case 0x00040000: cmdOut = CMD_PAW_10; break;
                    case 0x00080000: cmdOut = CMD_PAW_11; break;
                    case 0x00100000: cmdOut = CMD_PAW_12; break;
                    case 0x00000200: cmdOut = CMD_PAW_13; break;
                    case 0x00000400: cmdOut = CMD_PAW_14; break;
                    case 0x00000800: cmdOut = CMD_PAW_15; break;
                    case 0x00001000: cmdOut = CMD_PAW_16; break;
                }
            }
        }

        // check if nose click changed
        nosePressed = this->nosePressed();

        if(nosePressed && noseDownOn == 0)
            noseDownOn = xTaskGetTickCount();

        if(noseDownOn > 0) {
            timePressed = xTaskGetTickCount() - noseDownOn;
            if(timePressed > HBI_NOSE_SHUT_TICKS) {
                cmdOut = CMD_SHUTDN;
                noseDownOn = 0;
                timePressed = 0;
            }
            else if(!nosePressed) {
                ESP_LOGI(LOG_TAG, "Nose up under threshold %d / %d", timePressed, HBI_NOSE_SHUT_TICKS);
                noseDownOn = 0;
                timePressed = 0;
            }
        }

        // if command ready - transmit
        if(cmdOut > 0x00) {
            ESP_LOGD(LOG_TAG, "CMD: 0x%02x", cmdOut);
            if(xQueueSend(this->commandQueue, &cmdOut, 5 / portTICK_PERIOD_MS) != pdPASS)
                ESP_LOGW(LOG_TAG, "Event queue full!");
        }
    }
}

void HBI::setEyes(uint8_t leftPercentage, uint8_t rightPercentage) {
    this->leftEyePWM->setDutyPercentage(leftPercentage);
    this->rightEyePWM->setDutyPercentage(rightPercentage);
}

bool HBI::nosePressed() {
    return GPIO::read(PIN_HBI_NOSE_CLK);
}

uint8_t HBI::getCommandFromQueue() {
    uint8_t cmdOut;
    if (xQueueReceive(this->commandQueue, &cmdOut, CMD_QUEUE_BLOCK_MS / portTICK_PERIOD_MS) == pdPASS)
        return cmdOut;
    return CMD_NOOP;
}

void HBI::setPawPlaying(uint8_t paw) {    
    // shift registers and paw indices are not exactly coharent ;).
    if(paw >= 4 && paw < 8)
        paw += 12;
    if(paw >= 8)
        paw -=4;
    this->powLedState = (((uint16_t) 1) << paw);
}

void HBI::setNoPawPlaying() {
    this->powLedState = 0;
}