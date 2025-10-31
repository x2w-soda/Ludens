#pragma once

#include <Ludens/DSP/DSP.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <cstdint>

namespace LD {

/// @brief Common interface of audio data handles.
struct AudioData : Handle<struct AudioDataObj>
{
    /// @brief Create audio data from file. Decoding is based on it's
    ///        file path extension ".wav", ".mp3", etc.
    /// @note  The returned handle should not be downcasted to MP3Data, WAVData, etc.
    ///        And should be destroyed using AudioData::destroy.
    static AudioData create_from_path(const FS::Path& path);

    /// @brief Create audio data from f32 samples in RAM.
    /// @param channels Number of channels.
    /// @param sampleRate Sample rate.
    /// @param frameCount Frame count of src buffer.
    /// @param src Sample source buffer.
    /// @param srcSize Buffer byte size, sanity check against channels and frameCount.
    /// @return Valid handle on success.
    static AudioData create_from_samples(uint32_t channels, uint32_t sampleRate, uint32_t frameCount, const void* src, size_t srcSize);

    /// @brief Destroy audio data.
    static void destroy(AudioData data);

    /// @brief Get samples.
    const void* get_samples() const;

    /// @brief Get sample format.
    SampleFormat get_sample_format() const;

    /// @brief Get sample rate hz.
    uint32_t get_sample_rate() const;

    /// @brief Get channel count.
    uint32_t get_channels() const;

    /// @brief Get number of frames.
    uint32_t get_frame_count() const;
};

} // namespace LD