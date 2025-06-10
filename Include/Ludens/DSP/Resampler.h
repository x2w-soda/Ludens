#pragma once

#include <Ludens/DSP/DSP.h>
#include <Ludens/Header/Handle.h>
#include <cstdint>

namespace LD {

struct ResamplerInfo
{
    int channels;         /// number of channels for input and output
    double dstSampleRate; /// output sample rate
};

struct ResamplerProcessInfo
{
    float srcSampleRate;
    SampleFormat srcFormat;
    const void* srcSamples;
    uint32_t srcFrameCount;
    SampleFormat dstFormat;
    void* dstSamples;
    uint32_t dstFrameCount;
};

struct Resampler : Handle<struct ResamplerObj>
{
    static Resampler create(const ResamplerInfo& info);
    static void destroy(Resampler resampler);

    uint32_t get_dst_sample_count(uint32_t srcSampleCount, float srcSampleRate) const;

    uint32_t process(const ResamplerProcessInfo& info);
};

} // namespace LD