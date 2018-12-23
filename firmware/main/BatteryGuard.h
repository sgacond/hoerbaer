#ifndef BATTERYGUARD_H_
#define BATTERYGUARD_H_

#include <string>
#include <driver/adc.h>
#include "esp_adc_cal.h"

class BatteryGuard
{
    public:
    	BatteryGuard();
        virtual ~BatteryGuard();
        uint32_t GetVoltage();
        bool ShutDownVoltageExceeded();
    private:
        esp_adc_cal_characteristics_t* adcCharacteristics;
};

#endif