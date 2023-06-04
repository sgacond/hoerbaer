#include "esp_log.h"
#include <driver/adc.h>

#include "../Configuration.h"
#include "BatteryGuard.h"

/*
    MEASURED ADC VALUES - goot enough to cut at 3.2v
    MULTIMETER(V)  	ADC(mV)
    2.6	            2630 2614 2670
    2.8	            2880 2888 2918
    3.0	            3046 3068 3022
    3.2	            3252 3312 3258
    3.4	            3486 3480 3470
    3.6	            3668 3656 3678
    3.9	            3980 3998 3964
    4.2	            4256 4214 4272
*/

#define DEFAULT_VREF     1100
#define SHUTDOWN_VOLTAGE 3200

static const char* LOG_TAG = "BAT";

BatteryGuard::BatteryGuard() {
    adc1_config_width(ADC_VBAT_WIDTH);
    adc1_config_channel_atten(ADC_VBAT, ADC_VBAT_ATTEN);

    this->adcCharacteristics = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t adcCalValType = esp_adc_cal_characterize(ADC_VBAT_UNIT, ADC_VBAT_ATTEN, ADC_VBAT_WIDTH, DEFAULT_VREF, this->adcCharacteristics);

    if (adcCalValType == ESP_ADC_CAL_VAL_EFUSE_TP)
        ESP_LOGI(LOG_TAG, "ADC characterized using Two Point Value.");
    else if (adcCalValType == ESP_ADC_CAL_VAL_EFUSE_VREF)
        ESP_LOGI(LOG_TAG, "ADC characterized using eFuse Vref.");
    else
        ESP_LOGI(LOG_TAG, "ADC characterized using Default Vref.");
}

BatteryGuard::~BatteryGuard() {
}

uint32_t BatteryGuard::GetVoltage() {
   
    uint32_t adcRaw = 0;

    // use multisampling according docs

    for(int i=0; i<ADC_VBAT_MULTISMPL; i++)
        adcRaw += adc1_get_raw(ADC_VBAT);

    adcRaw /= ADC_VBAT_MULTISMPL;

    uint32_t voltage = esp_adc_cal_raw_to_voltage(adcRaw, this->adcCharacteristics) * ADC_VBAT_VDIV;
    ESP_LOGI(LOG_TAG, "ADC Readout raw: %lu, Voltage: %lumV", adcRaw, voltage);

    return voltage;
}

bool BatteryGuard::ShutDownVoltageExceeded() {
    auto voltage = this->GetVoltage();
    return (voltage < SHUTDOWN_VOLTAGE);
}
