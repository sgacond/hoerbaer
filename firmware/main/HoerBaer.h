#ifndef HOERBAER_H_
#define HOERBAER_H_

#include <memory>
#include <stdio.h>
#include <Task.h>
#include <PWM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../storage/Storage.h"
#include "../audioplayer/AudioPlayer.h"
#include "HBI.h"
#include "BatteryGuard.h"

class HoerBaer {
public:
	HoerBaer();
    HoerBaer(const HoerBaer& other) = delete;
	virtual ~HoerBaer();
	void run();
private:
    std::shared_ptr<Storage> storage;
    std::unique_ptr<HBI> hbi;
    std::unique_ptr<AudioPlayer> audioPlayer;
    std::unique_ptr<BatteryGuard> batteryGuard;
    uint8_t curVol;
    bool shouldBeRunning;
    void enablePeripherials();
    void disablePeripherials();
    void increaseVolume();
    void decreaseVolume();
    void checkUnderVoltageShutdown();
    void shutdown();
};

#endif