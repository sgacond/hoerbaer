#ifndef Mp3Playback_H_
#define Mp3Playback_H_

#include <string>
#include <memory>
#include "mad.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "../storage/Storage.h"
#include "PlaybackBase.h"

class Mp3Playback : public PlaybackBase
{
    public:
    	Mp3Playback(std::shared_ptr<Storage> storage, i2s_port_t i2sPort);
        virtual ~Mp3Playback();
        virtual AudioFileInfo Load(std::string filename);
        virtual void SeekToSeconds(float sec);
        virtual bool Eof();
    	void run(void* data);
        void stop() override;
    private:
    	std::shared_ptr<Storage> storage;
        FILE* fp;
        // static mad_flow headerCallback(void *data, struct mad_header *header);
        struct mad_stream madStream;
        struct mad_frame madFrame;
        struct mad_synth madSynth;
        i2s_port_t i2sPort;
};

#endif