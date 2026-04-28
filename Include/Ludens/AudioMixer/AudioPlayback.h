#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioMixerDef.h>
#include <Ludens/Memory/Allocator.h>
#include <cstdint>

namespace LD {

struct AudioPlayback : AudioHandle
{
    /// @brief Get AudioPlaybackObj byte size.
    static size_t byte_size();

    /// @brief Main thread creates audio playback instance.
    static AudioPlayback create(PoolAllocator playbackPA);

    /// @brief Main thread destroys audio playback instance.
    static void destroy(AudioPlayback playback);

    /// @brief Main thread stores latest state atomically.
    void store(AudioPlaybackState state);

    /// @brief Audio thread loads latest state atomically.
    AudioPlaybackState load();

    /// @brief Audio thread sets audio buffer as source, resets frame cursor to 0 and pauses.
    void set_buffer(AudioBuffer buffer);

    /// @brief Audio thread checks if playback instance is playing frames.
    bool is_playing();

    /// @brief Audio thread starts audio playback from the first frame.
    void start();

    /// @brief Audio thread stops audio playback and resets frame cursor.
    void stop();

    /// @brief Audio thread pauses audio playback.
    void pause();

    /// @brief Audio thread resumes audio playback.
    void resume();

    /// @brief Audio thread reads frames to output buffer and advances frame cursor.
    /// @return Number of frames read.
    uint32_t read_frames(float* outFrames, uint32_t frameCount);
};

} // namespace LD