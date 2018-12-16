#ifndef HBI_H_
#define HBI_H_

#include <memory>
#include <stdio.h>
#include <Task.h>
#include <PWM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "HBIShift.h"

class HBI : public Task {
public:
	HBI();
	virtual ~HBI();
	void run(void* data);
    void setEyes(uint8_t leftPercentage, uint8_t rightPercentage);
private:
    std::unique_ptr<HBIShift> shiftTask;
    std::unique_ptr<PWM> leftEyePWM;
    std::unique_ptr<PWM> rightEyePWM;
    QueueHandle_t queue;
};

#endif