#include <iostream>
#include <cstring>

#include "mad.h"
#include "Mp3File.h"

Mp3File::Mp3File(std::shared_ptr<Storage> storage) {
    this->storage = storage;
    this->fp = 0;
}

Mp3File::~Mp3File() {
    if(this->fp)
        this->storage->Close(this->fp);
}

// static mad_flow headerCallback(void *data, const struct mad_header *header)
// {
//     std::cout << "MP3 seconds " << header->duration.seconds << std::endl;

//     return MAD_FLOW_CONTINUE;
// }

AudioFileInfo Mp3File::Load(std::string filename) {

    // OPEN File
    this->fp = this->storage->OpenRead(filename);

    size_t bufferSize = 1440, read = 0;
    auto buffer = (unsigned short *) malloc(bufferSize);
    fread(buffer, read, 1, this->fp);

    // struct mad_decoder decoder;
    // mad_decoder_init(&decoder, &buffer,
    //         0 /* input */, 
    //         &headerCallback,
    //         0 /* filter */, 
    //         0 /* output */,
	// 	    0 /* error */, 
    //         0 /* message */);

    mad_stream_init(&this->madStream);
    mad_frame_init(&this->madFrame);
    mad_synth_init(&this->madSynth);

    return (AudioFileInfo) { 44100, 16, 2, 0, 1440 };
}

void Mp3File::SeekToSeconds(float sec) {
}

size_t Mp3File::StreamSamples(void * buffer, size_t bufferSize) {
    return 0;
}

bool Mp3File::Eof() {
    return 0;
}

