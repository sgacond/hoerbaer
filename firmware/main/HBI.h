#ifndef HBI_H_
#define HBI_H_

#include <memory>
#include <stdio.h>
#include <Task.h>
#include <PWM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "HBIShift.h"

#define CMD_NOOP    0x00
#define CMD_VOL_UP  0x11
#define CMD_VOL_DN  0x12
#define CMD_PLAY    0x01
#define CMD_STOP    0x02
#define CMD_REV     0x03
#define CMD_FWD     0x04
#define CMD_PAW_1   0xA0
#define CMD_PAW_2   0xA1
#define CMD_PAW_3   0xA2
#define CMD_PAW_4   0xA3
#define CMD_PAW_5   0xA4
#define CMD_PAW_6   0xA5
#define CMD_PAW_7   0xA6
#define CMD_PAW_8   0xA7
#define CMD_PAW_9   0xA8
#define CMD_PAW_10  0xA9
#define CMD_PAW_11  0xAA
#define CMD_PAW_12  0xAB
#define CMD_PAW_13  0xAC
#define CMD_PAW_14  0xAD
#define CMD_PAW_15  0xAE
#define CMD_PAW_16  0xAF

class HBI : public Task {
public:
	HBI();
	virtual ~HBI();
	void run(void* data);
    void setEyes(uint8_t leftPercentage, uint8_t rightPercentage);
    uint8_t getCommandFromQueue();
private:
    std::unique_ptr<HBIShift> shiftTask;
    std::unique_ptr<PWM> leftEyePWM;
    std::unique_ptr<PWM> rightEyePWM;
    QueueHandle_t shiftToHBIQUeue;
    QueueHandle_t commandQueue;
};

#endif