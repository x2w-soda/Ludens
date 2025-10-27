#pragma once

#include <Ludens/DSP/DSP.h>
#include <Ludens/Header/Handle.h>
#include <cstdint>

namespace LD {

/// @brief Common interface of audio data handles.
struct AudioData : Handle<struct AudioDataObj>
{
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