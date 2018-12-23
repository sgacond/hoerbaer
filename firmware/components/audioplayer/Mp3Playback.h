#ifndef Mp3Playback_H_
#define Mp3Playback_H_

#include <string>
#include <memory>
#include "mad.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "../storage/Storage.h"
#include "PlaybackBase.h"

/* ESP32 is Little Endian, I2S is Big Endian.
 *
 * Samples produced by a decoder will probably be LE,
 * and I2S recordings BE.
 */
typedef enum { PCM_INTERLEAVED, PCM_LEFT_RIGHT } pcm_buffer_layout_t;
typedef enum { PCM_BIG_ENDIAN, PCM_LITTLE_ENDIAN } pcm_endianness_t;

struct pcm_format_t
{
    uint32_t sample_rate;
    i2s_bits_per_sample_t bit_depth;
    uint8_t num_channels;
    pcm_buffer_layout_t buffer_format;
    pcm_endianness_t endianness; // currently unused
};

class Mp3Playback : public PlaybackBase
{
    public:
    	Mp3Playback(std::shared_ptr<Storage> storage, i2s_port_t i2sPort, uint32_t * bytesPlayed);
        virtual ~Mp3Playback();
        virtual AudioFileInfo Load(std::string filename);
        void SeekToSamples(uint32_t samples);
        virtual bool Eof();
    	void run(void* data);
        void stop() override;
    private:
    	std::shared_ptr<Storage> storage;
        FILE* fp;
        i2s_port_t i2sPort;
        size_t fileSize;
        size_t curFileOffset;
        struct pcm_format_t madBufferFormat;
        uint32_t* bytesPlayed;
};

#endif