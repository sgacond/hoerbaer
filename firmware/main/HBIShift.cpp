#include <iostream>
#include <memory>
#include <GPIO.h>
#include <SPI.h>
#include <PWM.h>
#include "esp_log.h"

#include "../../Configuration.h"
#include "HBIShift.h"

static const char* LOG_TAG = "HBI_SHIFT";

using namespace std;
using namespace ESP32CPP;

HBIShift::HBIShift(QueueHandle_t shiftToHBIQUeue, uint16_t* powLedState) {
    // init shift register stuff...
    this->setStackSize(2048); // NOT YET CALIBRATED TO LIMIT - can be decreased i think
    this->setPriority(TSK_PRIO_HBISHIFT);
    this->setName("HBI SHIFT");
    this->shiftToHBIQUeue = shiftToHBIQUeue;
    this->powLedState = powLedState;
}

HBIShift::~HBIShift() {
    this->stop();
    ESP_LOGI(LOG_TAG, "Shift Task stopped.");
}

void HBIShift::run(void *pvParameters) {

    auto spi = std::make_unique<SPI>();
    spi->setHost(SPI_HOST_HBI);
    spi->init(
        PIN_HBI_MOSI, // MOSI
        PIN_HBI_MISO, // MISO
        PIN_HBI_CLK, // CLK
        SPI::PIN_NOT_SET // CS
    );

    GPIO::setOutput(PIN_HBI_LIN);
    GPIO::setOutput(PIN_HBI_LOUT);

    size_t bufLen = 4;
    int lastIns = 0, ins = 0, diffIns = 0;
    TickType_t timeStart;

    while(1) {

        timeStart = esp_timer_get_time();

        // falling pulse for in registers
        GPIO::write(PIN_HBI_LIN, false);     
        vTaskDelay(2 / portTICK_PERIOD_MS);
        GPIO::write(PIN_HBI_LIN, true);

        auto powLed = *(this->powLedState);
	    uint8_t data[bufLen];
        data[0] = 0;
        data[1] = 0;
        data[2] = ((powLed & 0x00FF) ^ 0x33);
        data[3] = (((powLed & 0xFF00) >> 8) ^ 0x33);
        spi->transfer(data, bufLen);

        // rising pulse for out registers
        GPIO::write(PIN_HBI_LOUT, true);     
        vTaskDelay(2 / portTICK_PERIOD_MS);
        GPIO::write(PIN_HBI_LOUT, false);

        // get button state
        ins = data[0] 
            | (data[1] << 8)
            | (data[2] << 16)
            | (data[3] << 24);
            
        if(ins != lastIns) {
            diffIns = ins ^ lastIns;

            if((diffIns & MASK_PAW_INS) > 0 && ((ins & MASK_PAW_INS) < (lastIns & MASK_PAW_INS))) {
                // PAW DOWN - send this
                if(xQueueSend(this->shiftToHBIQUeue, &diffIns, 5 / portTICK_PERIOD_MS) != pdPASS)
                    ESP_LOGW(LOG_TAG, "Event queue full!");
            }
            else if(diffIns & MASK_ENCODER_INS) {
                // ENCODER MOVE - send diff / last / cur in one value (begin with 0xFF to mark)
                diffIns = (0xFF << 24) | ((diffIns & MASK_ENCODER_INS) << 16) | ((lastIns & MASK_ENCODER_INS) << 8) | ((ins & MASK_ENCODER_INS));
                if(xQueueSend(this->shiftToHBIQUeue, &diffIns, 5 / portTICK_PERIOD_MS) != pdPASS)
                    ESP_LOGW(LOG_TAG, "Event queue full!");
            }

            lastIns = ins;
        }

        vTaskDelayUntil( &timeStart, TIME_HBISHIFT_OP_MS / portTICK_PERIOD_MS );
    }
}
