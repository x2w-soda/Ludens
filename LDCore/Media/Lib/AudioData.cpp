#include "AudioDataObj.h"
#include <Ludens/Header/Types.h>
#include <Ludens/Media/AudioData.h>
#include <Ludens/System/Memory.h>

// using the standalone decoder API.
#include <miniaudio.h>

#define RESAMPLE_FORMAT SAMPLE_FORMAT_F32
#define RESAMPLE_CHANNELS 2
#define RESAMPLE_RATE 48000

namespace LD {

static_assert(RESAMPLE_FORMAT == SAMPLE_FORMAT_F32); // ma_format_f32

AudioDataObj* create_audio_data(const void* data, size_t dataSize, AudioDataFormat dataFormat)
{
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, RESAMPLE_CHANNELS, RESAMPLE_RATE);

    switch (dataFormat)
    {
    case AUDIO_DATA_FORMAT_WAV:
        config.encodingFormat = ma_encoding_format_wav;
        break;
    case AUDIO_DATA_FORMAT_MP3:
        config.encodingFormat = ma_encoding_format_mp3;
        break;
    default:
        return {};
    }

    ma_decoder decoder;
    ma_result result = ma_decoder_init_memory(data, dataSize, &config, &decoder);

    if (result != MA_SUCCESS)
        return {};

    uint64_t frameCount;
    result = ma_decoder_get_available_frames(&decoder, &frameCount);

    if (result != MA_SUCCESS)
        return {};

    auto* obj = (AudioDataObj*)heap_malloc(sizeof(AudioDataObj) + sizeof(float) * frameCount * RESAMPLE_CHANNELS, MEMORY_USAGE_MEDIA);
    obj->frameCount = (uint32_t)frameCount;
    obj->sampleFormat = RESAMPLE_FORMAT;
    obj->channels = RESAMPLE_CHANNELS;
    obj->sampleRate = RESAMPLE_RATE;
    obj->samples = (float*)(obj + 1);

    uint64_t framesRead;
    result = ma_decoder_read_pcm_frames(&decoder, obj->samples, frameCount, &framesRead);
    if (result != MA_SUCCESS)
    {
        heap_free(obj);
        return {};
    }

    return obj;
}

void destroy_audio_data(AudioDataObj* data)
{
    heap_free(data);
}

static_assert(IsTrivial<AudioDataObj>);

//
// Public API
//

const void* AudioData::get_samples() const
{
    return mObj->samples;
}

SampleFormat AudioData::get_sample_format() const
{
    return mObj->sampleFormat;
}

uint32_t AudioData::get_sample_rate() const
{
    return mObj->sampleRate;
}

uint32_t AudioData::get_channels() const
{
    return mObj->channels;
}

uint32_t AudioData::get_frame_count() const
{
    return mObj->frameCount;
}

} // namespace LD