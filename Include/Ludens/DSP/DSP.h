#pragma once

#include <cstdlib>

namespace LD {

/// the sample format of a signal
enum SampleFormat
{
    SAMPLE_FORMAT_UNKNOWN = 0, /// unknown, unsupported format
    SAMPLE_FORMAT_F32,         /// 32-bit float in the range [-1.0f, 1.0f]
    SAMPLE_FORMAT_S16,         /// 16-bit signed integer in the range [-32768, 32767]
    SAMPLE_FORMAT_S24,         /// 24-bit signed integer in the range [-8388608, 8388607]
    SAMPLE_FORMAT_S32,         /// 32-bit signed integer in the range [-2147483648, 2147483647]
    SAMPLE_FORMAT_U8,          /// 8-bit unsigned integer in the range [0, 255]
};

/// @brief perform sample format conversion
/// @param srcFormat input sample format
/// @param srcSamples input samples
/// @param dstFormat output sample format
/// @param dstSamples output samples
/// @param sampleCount number of samples in srcSamples and dstSamples array
/// @return true on success
bool sample_format_conversion(SampleFormat srcFormat, const void* srcSamples, SampleFormat dstFormat, void* dstSamples, size_t sampleCount);

} // namespace LD