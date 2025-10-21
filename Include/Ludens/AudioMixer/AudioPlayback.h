#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>

namespace LD {

struct AudioPlaybackInfo
{
    PoolAllocator playbackPA;
};

struct AudioPlayback : AudioHandle
{
    /// @brief Get AudioPlaybackObj byte size.
    static size_t byte_size();

    /// @brief Main thread creates audio playback instance.
    static AudioPlayback create(const AudioPlaybackInfo& info);

    /// @brief Main thread destroys audio playback instance.
    static void destroy(AudioPlayback playback);

    /// @brief Set audio buffer as source, resets frame cursor to 0 and pauses.
    void set_buffer(AudioBuffer buffer);

    /// @brief Check if playback instance is playing frames.
    bool is_playing();

    /// @brief Start audio playback from the first frame.
    void start();

    /// @brief Pause audio playback.
    void pause();

    /// @brief Resume audio playback.
    void resume();

    uint32_t read_frames(float* outFrames, uint32_t frameCount);
};

} // namespace LD