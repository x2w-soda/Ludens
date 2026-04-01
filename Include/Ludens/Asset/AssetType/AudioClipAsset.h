#pragma once

#include <Ludens/Asset/Asset.h>

namespace LD {

/// @brief Audio clip asset handle. This is typically a static buffer of
///        audio samples after decoding and resampling from WAV, MP3, etc.
struct AudioClipAsset : Asset
{
    /// @brief Get number of frames in this clip.
    uint32_t get_frame_count();

    /// @brief Get number of channels in this clip.
    uint32_t get_channel_count();

    /// @brief Get default sample rate of this clip.
    uint32_t get_sample_rate();

    /// @brief Read frames from offset.
    const float* get_frames(uint32_t frameOffset);
};

} // namespace LD