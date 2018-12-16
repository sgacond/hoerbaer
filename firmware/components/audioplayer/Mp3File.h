#ifndef MP3FILE_H_
#define MP3FILE_H_

#include <string>
#include <memory>

#include "mad.h"
#include "../storage/Storage.h"
#include "IAudioFile.h"

class Mp3File : public IAudioFile
{
    public:
    	Mp3File(std::shared_ptr<Storage> storage);
        virtual ~Mp3File();
        virtual AudioFileInfo Load(std::string filename);
        virtual void SeekToSeconds(float sec);
        virtual size_t StreamSamples(void * buffer, size_t bufferSize);
        virtual bool Eof();
    private:
    	std::shared_ptr<Storage> storage;
        FILE* fp;
        // static mad_flow headerCallback(void *data, struct mad_header *header);
        struct mad_stream madStream;
        struct mad_frame madFrame;
        struct mad_synth madSynth;
};

#endif