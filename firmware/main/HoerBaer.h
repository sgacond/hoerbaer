#ifndef HOERBAER_H_
#define HOERBAER_H_

#include <memory>
#include <vector>
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
    std::vector<std::string> pawFiles[16];
    uint8_t curVol;
    bool shouldBeRunning;
    uint8_t curPaw;
    uint8_t curIdxOnPaw;
    void enablePeripherials();
    void disablePeripherials();
    void muteAmp();
    void unmuteAmp();
    void increaseVolume();
    void decreaseVolume();
    void loadPawsFromStorage();
    void playNextFromPaw(uint8_t idx);
    void playNext();
    void playPrev();
    std::string getCurrentAudioFile();
    void checkUnderVoltageShutdown();
    void shutdown();
};

#endif