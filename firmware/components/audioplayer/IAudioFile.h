#ifndef IAUDIOFILE_H_
#define IAUDIOFILE_H_

typedef struct AudioFileInfo { 
    int pcmSamplerate;
    int pcmBits;
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