#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

#include <memory>
#include <stdio.h>
#include "../storage/Storage.h"
#include "PlaybackBase.h"

class AudioPlayer {
public:
	AudioPlayer(std::shared_ptr<Storage> storage);
	virtual ~AudioPlayer();
    void InitCodec();
    void PlayFile(std::string filename, uint32_t samplesStart = 0);
    void SetVolume(uint8_t vol);
    void Stop();
    uint32_t samplesPlayed;

private:
	std::shared_ptr<Storage> storage;
    std::unique_ptr<PlaybackBase> audioPlayback;
    int sample_rate;
    int bits;
    void setSamplerateBits(int sample_rate, int bits);
};

#endif