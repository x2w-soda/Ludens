#include <Ludens/DSP/Resampler.h>
#include <Ludens/System/Memory.h>
#include <samplerate.h> // hide
#include <vector>

namespace LD {

struct ResamplerObj
{
    SRC_STATE* state;
    float dstSampleRate;
    int channels;
};

Resampler Resampler::create(const ResamplerInfo& info)
{
    ResamplerObj* obj = (ResamplerObj*)heap_malloc(sizeof(ResamplerObj), MEMORY_USAGE_MISC);

    int error;
    obj->state = src_new(SRC_SINC_BEST_QUALITY, info.channels, &error);

    if (error)
    {
        heap_free(obj);
        return {};
    }

    obj->dstSampleRate = info.dstSampleRate;
    obj->channels = info.channels;

    return {obj};
}

void Resampler::destroy(Resampler resampler)
{
    ResamplerObj* obj = resampler;

    src_delete(obj->state);

    heap_free(obj);
}

uint32_t Resampler::get_dst_sample_count(uint32_t srcSampleCount, float srcSampleRate) const
{
    const float sampleRatio = mObj->dstSampleRate / srcSampleRate;

    return static_cast<uint32_t>(srcSampleCount * sampleRatio + 1);
}

uint32_t Resampler::process(const ResamplerProcessInfo& info)
{
    const float sampleRatio = mObj->dstSampleRate / info.srcSampleRate;
    uint32_t srcSampleCount = info.srcFrameCount * mObj->channels;
    uint32_t dstSampleCount = info.dstFrameCount * mObj->channels;
    std::vector<float> floatInput(srcSampleCount);

    bool success = sample_format_conversion(info.srcFormat, info.srcSamples, SAMPLE_FORMAT_F32, floatInput.data(), floatInput.size());
    if (!success)
        return 0;

    SRC_DATA data{};
    data.data_in = floatInput.data();
    data.input_frames = (long)info.srcFrameCount;
    data.data_out = info.dstSamples;
    data.output_frames = (long)info.dstFrameCount;
    data.src_ratio = sampleRatio;
    data.end_of_input = 1;

    int error = src_process(mObj->state, &data);
    if (error)
        return 0;

    return (uint32_t)data.output_frames_gen * mObj->channels;
}

} // namespace LD