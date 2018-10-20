#include <iostream>
#include "WavFile.h"

struct wave_header {
	char riff[4];						// RIFF string
	int overall_size;     				// overall size of file in bytes
	char wave[4];						// WAVE string
	char fmt_chunk_marker[4];			// fmt string with trailing null char
	int length_of_fmt;					// length of the format data
	short format_type;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	short channels;						// no.of channels
	int sample_rate;					// sampling rate (blocks per second)
	int byterate;						// SampleRate * NumChannels * BitsPerSample/8
	short block_align;					// NumChannels * BitsPerSample/8
	short bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
};

struct data_chunk_header {
    char data_chunk_header [4];		// DATA string or FLLR string
	int data_size;						// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

WavFile::WavFile(std::shared_ptr<Storage> storage) {
    this->storage = storage;
}

WavFile::~WavFile() {
    if(this->fp)
        this->storage->Close(this->fp);
}

AudioFileInfo WavFile::Load(std::string filename) {
    this->fp = this->storage->OpenRead(filename);

    struct wave_header header;
    fread(&header, sizeof(header), 1, this->fp);

    std::cout << "WAV: Overall size: " << (header.overall_size/1024) << "KB" << std::endl;
    std::cout << "WAV: Format type: " << header.format_type << std::endl;
    std::cout << "WAV: Channels: " << header.channels << std::endl;
    std::cout << "WAV: Sample rate: " << header.sample_rate << std::endl;
    std::cout << "WAV: Bits per sample: " << header.bits_per_sample << std::endl;

    long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
    std::cout << "WAV: Size of each sample: " << size_of_each_sample << " bytes." << std::endl;

    float durationSeconds = (float) header.overall_size / header.byterate;
    std::cout << "WAV: Approx.Duration in seconds: " << durationSeconds << std::endl;

    return (AudioFileInfo) { header.sample_rate, header.bits_per_sample, durationSeconds };
}

void WavFile::SeekToSeconds(float sec) {
    throw "NOT IMPLEMENTED YET";
}

size_t WavFile::StreamSamples(void * buffer, size_t bufferSize) {

    // TODO: intern mitführen welcher wieviel bytes schon gelesen im ganzen file, gelesen im aktuellen, etc.
    //       offset mitfahren.

    return 0;
}

bool WavFile::Eof() {
    return true;
}