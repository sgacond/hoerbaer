#include <iostream>
#include <cstring>

#include "mad.h"
#include "Mp3Playback.h"

Mp3Playback::Mp3Playback(std::shared_ptr<Storage> storage, i2s_port_t i2sPort) {
    this->storage = storage;
    this->fp = 0;
    this->i2sPort = i2sPort;
}

Mp3Playback::~Mp3Playback() {
    if(this->fp)
        this->storage->Close(this->fp);
}

// static mad_flow headerCallback(void *data, const struct mad_header *header)
// {
//     std::cout << "MP3 seconds " << header->duration.seconds << std::endl;

//     return MAD_FLOW_CONTINUE;
// }

AudioFileInfo Mp3Playback::Load(std::string filename) {

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

void Mp3Playback::stop() {
}

void Mp3Playback::SeekToSeconds(float sec) {
}

bool Mp3Playback::Eof() {
    return 0;
}

void Mp3Playback::run(void* data) {

}
