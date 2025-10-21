#pragma once

#include <Ludens/Header/Handle.h>
#include <cstdint>

namespace LD {

/// @brief MiniAudio device handle.
struct MiniAudioDevice : Handle<struct MiniAudioDeviceObj>
{
    /// @brief Get user data specified in MiniAudioInfo.
    void* get_user_data();

    /// @brief Get device sample rate.
    uint32_t get_sample_rate();
};

/// @brief Audio thread callback that requests new frames for playback.
typedef void (*MiniAudioDataCallback)(MiniAudioDevice device, void* outFrames, const void* inFrames, uint32_t frameCount);

/// @brief MiniAudio configuration.
struct MiniAudioInfo
{
    MiniAudioDataCallback dataCallback;
    void* userData;
};

/// @brief MiniAudio context handle.
struct MiniAudio : Handle<struct MiniAudioObj>
{
    /// @brief Create miniaudio context.
    static MiniAudio create(const MiniAudioInfo& info);

    /// @brief Destroy miniaudio context
    static void destroy(MiniAudio ma);
};

} // namespace LD