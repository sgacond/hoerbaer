#ifndef WavPlayback_H_
#define WavPlayback_H_

#include <string>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "../storage/Storage.h"
#include "PlaybackBase.h"

class WavPlayback : public PlaybackBase
{
    public:
    	WavPlayback(std::shared_ptr<Storage> storage, i2s_port_t i2sPort);
        virtual ~WavPlayback();
        virtual AudioFileInfo Load(std::string filename);
        virtual void SeekToSeconds(float sec);
        virtual bool Eof();
    	void run(void* data);
        void stop() override;
    private:
    	std::shared_ptr<Storage> storage;
        FILE* fp;
        size_t fileSize;
        size_t curFileOffset;
        size_t curChunkRemaining;
        i2s_port_t i2sPort;
};

#endif