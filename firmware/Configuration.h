#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/i2s.h"

#define PIN_SDCARD_MISO     GPIO_NUM_19
#define PIN_SDCARD_MOSI     GPIO_NUM_23
#define PIN_SDCARD_CLK      GPIO_NUM_18
#define PIN_SDCARD_CS       GPIO_NUM_5

#define I2S_NUM             I2S_NUM_0
#define PIN_I2S_BCK         GPIO_NUM_26
#define PIN_I2S_LRCK        GPIO_NUM_25
#define PIN_I2S_OUT         GPIO_NUM_27

#define PIN_I2C_SDA         GPIO_NUM_21
#define PIN_I2C_SCL         GPIO_NUM_22
#define CODEC_I2C_ADDR      0x4B
#define CODEC_MAX_VOL       63

#endif