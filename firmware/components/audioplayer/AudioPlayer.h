#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

#include <memory>
#include <stdio.h>
#include "../storage/Storage.h"

class AudioPlayer {
public:
	AudioPlayer(std::shared_ptr<Storage> storage);
	virtual ~AudioPlayer();
    void InitCodec();
    void PlayFile(std::string filename);
    void SetVolume(uint8_t vol);

private:
	std::shared_ptr<Storage> storage;
    int sample_rate;
    int bits;
    void setSamplerateBits(int sample_rate, int bits);
};

#endif