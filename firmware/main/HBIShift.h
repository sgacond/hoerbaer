#ifndef HBISHIFT_H_
#define HBISHIFT_H_

#include <memory>
#include <stdio.h>
#include <Task.h>
#include <PWM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define MASK_PAW_INS        0x1F1F1F1F
#define MASK_ENCODER_A      0x00000040
#define MASK_ENCODER_B      0x00000080
#define MASK_ENCODER_INS    (MASK_ENCODER_A | MASK_ENCODER_B)

class HBIShift : public Task {
public:
	HBIShift(QueueHandle_t shiftToHBIQUeue, uint16_t* powLedState);
	virtual ~HBIShift();
	void run(void* data);
private:
    uint16_t* powLedState;
    QueueHandle_t shiftToHBIQUeue;
};

#endif