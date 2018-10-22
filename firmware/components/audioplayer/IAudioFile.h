#ifndef IAUDIOFILE_H_
#define IAUDIOFILE_H_

typedef struct AudioFileInfo { 
    uint32_t pcmSamplerate;
    uint32_t pcmBits;
    float durationSeconds;
} AudioFileInfo;

class IAudioFile
{
    public:
        virtual ~IAudioFile() {}
        virtual AudioFileInfo Load(std::string filename) = 0;
        virtual void SeekToSeconds(float sec) = 0;
        virtual size_t StreamSamples(void * buffer, size_t bufferSize) = 0;
        virtual bool Eof() = 0;

};

#endif