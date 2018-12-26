#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/spi_master.h"

#define MAIN_NTHLOOP_BATCHK 25 //256 // every nth loop - battery check

#define PIN_PERIPH_MAIN     GPIO_NUM_2

#define ADC_VBAT            ADC1_CHANNEL_6 // PIN 34
#define ADC_VBAT_UNIT       ADC_UNIT_1
#define ADC_VBAT_ATTEN      ADC_ATTEN_DB_11
#define ADC_VBAT_WIDTH      ADC_WIDTH_BIT_12
#define ADC_VBAT_MULTISMPL  64
#define ADC_VBAT_VDIV       2 // applied volatage divider

#define SPI_HOST_SDCARD     VSPI_HOST
#define PIN_SDCARD_MISO     GPIO_NUM_19
#define PIN_SDCARD_MOSI     GPIO_NUM_23
#define PIN_SDCARD_CLK      GPIO_NUM_18
#define PIN_SDCARD_CS       GPIO_NUM_5

#define TSK_PRIO_WAVPLAY    configMAX_PRIORITIES - 2
#define I2S_NUM             I2S_NUM_0
#define PIN_I2S_BCK         GPIO_NUM_26
#define PIN_I2S_LRCK        GPIO_NUM_25
#define PIN_I2S_OUT         GPIO_NUM_27

#define PIN_I2C_SDA         GPIO_NUM_21
#define PIN_I2C_SCL         GPIO_NUM_22
#define AMP_I2C_ADDR        0x4B
#define AMP_I2C_RETRIES     5
#define AMP_I2C_CLOCK       10000 // was 100kHz - but with long wires and this payload 10k is enough. Signal looks way better on the scope.
#define AMP_MAX_VOL         63
#define PIN_AMP_MUTE        GPIO_NUM_15

#define TSK_PRIO_HBI        configMAX_PRIORITIES - 4
#define TSK_PRIO_HBISHIFT   configMAX_PRIORITIES - 1
#define TIME_HBISHIFT_OP_MS 20
#define QUEUE_LEN_HBISHIFT  10
#define QUEUE_LEN_HBICMD    10
#define CMD_QUEUE_BLOCK_MS  200
#define HBI_NOSE_SHUT_TICKS 120
#define SPI_HOST_HBI        HSPI_HOST
#define PIN_HBI_MISO        GPIO_NUM_12
#define PIN_HBI_MOSI        GPIO_NUM_13
#define PIN_HBI_CLK         GPIO_NUM_14
#define PIN_HBI_LIN         GPIO_NUM_16
#define PIN_HBI_LOUT        GPIO_NUM_17
#define PIN_HBI_PWM         GPIO_NUM_4
#define PIN_HBI_EYEL        GPIO_NUM_32
#define PIN_HBI_EYER        GPIO_NUM_33
#define PIN_HBI_NOSE_CLK    GPIO_NUM_35

#endif