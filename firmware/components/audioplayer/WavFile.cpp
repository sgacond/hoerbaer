#include <iostream>
#include <cstring>

#include "WavFile.h"

struct wave_header {
	char riff[4];						// RIFF string
	uint32_t overall_size; 				// overall size of file in bytes
	char wave[4];						// WAVE string
};

struct chunk_header {
    char type[4];						// type
	uint32_t size;     		    		// overall size of file in bytes
};

struct fmt_chunk {
    char fmt_chunk_marker[4];			// fmt string with trailing null char
	uint32_t length_of_fmt;				// length of the format data
	uint16_t format_type;	    		// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint16_t channels;					// no.of channels
	uint32_t sample_rate;				// sampling rate (blocks per second)
	uint32_t byterate;					// SampleRate * NumChannels * BitsPerSample/8
	uint16_t block_align;				// NumChannels * BitsPerSample/8
	uint16_t bits_per_sample;			// bits per sample, 8- 8bits, 16- 16 bits etc
};

struct data_chunk_header {
    char data_chunk_header [4]; 		// DATA string or FLLR string
	unsigned int data_size;				// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

WavFile::WavFile(std::shared_ptr<Storage> storage) {
    this->storage = storage;
    this->fp = 0;
    this->fileSize = 0;
    this->curFileOffset = 0;
    this->curChunkRemaining = 0;
}

WavFile::~WavFile() {
    if(this->fp)
        this->storage->Close(this->fp);
}

AudioFileInfo WavFile::Load(std::string filename) {
    
    // OPEN File
    this->fp = this->storage->OpenRead(filename);

    // Parse RIFF Header
    struct wave_header riffHeader = {};
    size_t nRead = sizeof(riffHeader);
    fread(&riffHeader, nRead, 1, this->fp);
    this->curFileOffset += nRead;

    std::cout << "WAV: Overall size: " << (riffHeader.overall_size/1024) << "KB" << std::endl;
    this->fileSize = riffHeader.overall_size;

    // Skip until fmt-Header
    struct chunk_header nextChunkHeader = {};
    nRead = sizeof(nextChunkHeader);
    fread(&nextChunkHeader, nRead, 1, this->fp);
    this->curFileOffset += nRead;

    while(strncmp(nextChunkHeader.type, "fmt", 3) != 0) {
        std::cout << "WAV: skip header chunk: " << nextChunkHeader.type << " length: " << nextChunkHeader.size << std::endl;
        fseek(this->fp, nextChunkHeader.size, SEEK_CUR);
        this->curFileOffset += nextChunkHeader.size;
        fread(&nextChunkHeader, nRead, 1, this->fp);
        this->curFileOffset += nRead;
    } 

    fseek(this->fp, -nRead, SEEK_CUR);
    this->curFileOffset -= nRead;

    // Read fmt header
    struct fmt_chunk formatHeader = {};
    nRead = sizeof(formatHeader);
    fread(&formatHeader, nRead, 1, this->fp);
    this->curFileOffset += nRead;

    std::cout << "WAV: Format marker: " << formatHeader.fmt_chunk_marker << std::endl;
    std::cout << "WAV: Format type: " << formatHeader.format_type << std::endl;
    std::cout << "WAV: Channels: " << formatHeader.channels << std::endl;
    std::cout << "WAV: Sample rate: " << formatHeader.sample_rate << std::endl;
    std::cout << "WAV: Bits per sample: " << formatHeader.bits_per_sample << std::endl;

    long size_of_each_sample = (formatHeader.channels * formatHeader.bits_per_sample) / 8;
    std::cout << "WAV: Size of each sample: " << size_of_each_sample << " bytes." << std::endl;

    float durationSeconds = (float) riffHeader.overall_size / formatHeader.byterate;
    std::cout << "WAV: Approx.Duration in seconds: " << durationSeconds << std::endl;

    return (AudioFileInfo) { formatHeader.sample_rate, formatHeader.bits_per_sample, formatHeader.channels, durationSeconds };

}

void WavFile::SeekToSeconds(float sec) {
    if(!this->fp) throw std::runtime_error("Operation failed. Load file first.");

    throw "NOT IMPLEMENTED YET";
}

size_t WavFile::StreamSamples(void * buffer, size_t bufferSize) {
    if(!this->fp) throw std::runtime_error("Operation failed. Load file first.");

    while(this->curChunkRemaining <= 0 && !this->Eof()) {

        struct data_chunk_header ch;
        size_t nRead = sizeof(ch);
        fread(&ch, nRead, 1, this->fp);
        this->curFileOffset += nRead;

        std::cout << "WAV CHUNK Data Marker: " << ch.data_chunk_header << std::endl;
        std::cout << "WAV CHUNK Size of data chunk: " << ch.data_size << std::endl;
        this->curChunkRemaining = ch.data_size;

        if(strncmp(ch.data_chunk_header, "data", 4) != 0) {
            std::cout << "WAV CHUNK skipped." << std::endl;
            fseek(this->fp, ch.data_size, SEEK_CUR);
            this->curFileOffset += ch.data_size;
            this->curChunkRemaining = 0;
        }
    }

    int read = bufferSize;

    if(read > this->curChunkRemaining)
        read = this->curChunkRemaining;

    fread(buffer, read, 1, this->fp);

    this->curChunkRemaining -= read;
    this->curFileOffset += read;

    return read;
}

bool WavFile::Eof() {
    if(!this->fp) throw std::runtime_error("Operation failed. Load file first.");
    return this->curFileOffset == this->fileSize;
}
