#pragma once

#include <Ludens/DSP/DSP.h>
#include <cstdint>

namespace LD {

enum AudioDataFormat
{
    AUDIO_DATA_FORMAT_WAV,
    AUDIO_DATA_FORMAT_MP3,
};

struct AudioDataObj
{
    void* samples;
    SampleFormat sampleFormat;
    uint32_t sampleRate;
    uint32_t channels;
    uint32_t frameCount;
};

AudioDataObj* create_audio_data(const void* data, size_t dataSize, AudioDataFormat dataFormat);
void destroy_audio_data(AudioDataObj* data);

} // namespace LD