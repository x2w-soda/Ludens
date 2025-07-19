#pragma once

#include <Ludens/DSP/DSP.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Types.h>
#include <cstdint>
#include <filesystem>

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

    /// @brief get samples
    const void* get_data(uint64_t& byteSize) const;

    /// @brief get channel count
    uint32_t get_channels() const;

    /// @brief get sample format
    SampleFormat get_sample_format() const;

    /// @brief get number of samples
    uint32_t get_sample_count() const;

    /// @brief get sample rate hz
    uint32_t get_sample_rate() const;

    /// @brief get bit depth per sample
    uint32_t get_bits_per_sample() const;

    static bool save_to_disk(const std::filesystem::path& path, const WAVHeader& header, const void* data, uint64_t dataSize);
};

} // namespace LD