#include <Ludens/DSP/DSP.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace LD {

// clang-format off
struct
{
    SampleFormat format;
    size_t byteSize;
    const char* cstr;
} sFormatTable[] = {
    {SAMPLE_FORMAT_UNKNOWN, 0, "SAMPLE_FORMAT_UNKNOWN"},
    {SAMPLE_FORMAT_F32,     4, "SAMPLE_FORMAT_F32"},
    {SAMPLE_FORMAT_S16,     2, "SAMPLE_FORMAT_S16"},
    {SAMPLE_FORMAT_S24,     3, "SAMPLE_FORMAT_S24"},
    {SAMPLE_FORMAT_S32,     4, "SAMPLE_FORMAT_S32"},
    {SAMPLE_FORMAT_U8,      1, "SAMPLE_FORMAT_U8"},
};
// clang-format on

bool sample_format_conversion(SampleFormat srcFormat, const void* srcSamples, SampleFormat dstFormat, void* dstSamples, size_t sampleCount)
{
    // TODO: SIMD?
    std::vector<float> v;
    float* tmp;

    if (dstFormat == SAMPLE_FORMAT_F32)
    {
        tmp = (float*)dstSamples;
    }
    else
    {
        v.resize(sampleCount);
        tmp = v.data();
    }

    switch (srcFormat)
    {
    case SAMPLE_FORMAT_F32:
        memcpy(tmp, srcSamples, sizeof(float) * sampleCount);
        break;
    case SAMPLE_FORMAT_S16:
        for (size_t i = 0; i < sampleCount; i++)
            tmp[i] = ((int16_t*)srcSamples)[i] / 32768.0f;
        break;
    case SAMPLE_FORMAT_S24:
        for (size_t i = 0; i < sampleCount; i++)
        {
            uint8_t* base = (uint8_t*)srcSamples + 3 * i;
            int32_t s24 = (int32_t(base[0]) << 8) |
                          (int32_t(base[1]) << 16) |
                          (int32_t(base[2]) << 24);
            s24 >>= 8; // preserves msb sign
            tmp[i] = s24 / 8388608.0f;
        }
        break;
    case SAMPLE_FORMAT_S32:
        for (size_t i = 0; i < sampleCount; i++)
            tmp[i] = ((int32_t*)srcSamples)[i] / 2147483648.0f;
        break;
    default:
        return false; // unsupported
    }

    switch (dstFormat)
    {
    case SAMPLE_FORMAT_F32:
        break;
    case SAMPLE_FORMAT_S16:
        for (size_t i = 0; i < sampleCount; i++)
            ((int16_t*)dstSamples)[i] = (int16_t)(tmp[i] * 32767.0f);
        break;
    case SAMPLE_FORMAT_S32:
        for (size_t i = 0; i < sampleCount; i++)
            ((int32_t*)dstSamples)[i] = (int32_t)(tmp[i] * 2147483647.0f);
        break;
    default:
        return false; // unsupported
    }

    return true;
}

size_t sample_format_byte_size(SampleFormat format, size_t count)
{
    return sFormatTable[(int)format].byteSize * count;
}

const char* sample_format_cstr(SampleFormat format)
{
    return sFormatTable[(int)format].cstr;
}

} // namespace LD
