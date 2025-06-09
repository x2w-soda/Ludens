#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Types.h>
#include <cstdint>

namespace LD {

/// @brief wav file header
struct WAVHeader
{
    byte fileTypeBlocID[4]; /// riff chunk magic, identifier "riff"
    uint32_t fileSize;      /// riff file size
    byte fileFormatID[4];   /// riff format, identifier "WAVE"
    byte fileBlocID[4];     /// identifier "fmt_"
    uint32_t blocSize;
    uint16_t audioFormat;  /// PCM integer or IEEE-754 float
    uint16_t channelCount; /// number of channels
    uint32_t sampleRate;   /// sample frequency in hz
    uint32_t byteRate;     /// bytes to read per second
    uint16_t blockAlign;
    uint16_t bitsPerSample; /// bit depth per sample
    byte dataBlockID[4];    /// identifier "data"
    uint32_t dataSize;      /// sample data size in bytes
};

struct WAVData : Handle<struct WAVDataObj>
{
    /// @brief parse WAV data and create handle
    /// @param data bytes to WAV data, including WAV header
    /// @param size byte size of input data
    static WAVData create(const void* data, size_t size);

    /// @brief destroy WAV data
    /// @param data wav data handle
    static void destroy(WAVData data);

    /// @brief get WAV header
    /// @param header output wav file header
    void get_header(WAVHeader& header) const;

    /// @brief get sample rate hz
    uint32_t get_sample_rate() const;
};

} // namespace LD