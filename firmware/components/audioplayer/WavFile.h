#ifndef WAVFILE_H_
#define WAVFILE_H_

#include <string>
#include <memory>
#include "../storage/Storage.h"
#include "IAudioFile.h"

class WavFile : public IAudioFile
{
    public:
    	WavFile(std::shared_ptr<Storage> storage);
        virtual ~WavFile();
        virtual AudioFileInfo Load(std::string filename);
        virtual void SeekToSeconds(float sec);
        virtual size_t StreamSamples(void * buffer, size_t bufferSize);
        virtual bool Eof();
    private:
    	std::shared_ptr<Storage> storage;
        FILE* fp;
};

#endif