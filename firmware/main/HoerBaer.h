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

class HoerBaer {
public:
	HoerBaer();
	virtual ~HoerBaer();
	void run();
private:
    std::shared_ptr<Storage> storage;
    std::shared_ptr<HBI> hbi;
    std::shared_ptr<AudioPlayer> audioPlayer;
    uint8_t curVol;

    void enablePeripherials();
    void disablePeripherials();
    void increaseVolume();
    void decreaseVolume();
    static void playFile(void* p);
};

#endif