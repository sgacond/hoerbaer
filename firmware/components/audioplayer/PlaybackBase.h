#ifndef IAudioPlayback_H_
#define IAudioPlayback_H_

#include <memory>
#include <Task.h>

typedef struct AudioFileInfo { 
    uint32_t pcmSamplerate;
    uint32_t pcmBits;
    uint16_t pcmChannels;
    float durationSeconds;
    size_t recommendedBufferSize;
} AudioFileInfo;

class PlaybackBase : public Task
{
    public:
        virtual ~PlaybackBase() {}
        virtual AudioFileInfo Load(std::string filename) = 0;
        virtual void SeekToSeconds(float sec) = 0;
        // virtual size_t StreamSamples(void * buffer, size_t bufferSize) = 0;
        virtual bool Eof() = 0;
        virtual void stop() = 0;
};

#endif