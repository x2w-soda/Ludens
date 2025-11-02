#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>

namespace LD {

struct AudioPlaybackObj;

struct AudioPlaybackInfo
{
    PoolAllocator playbackPA; // Used to create and destroy playback instance.
    float volumeLinear;       // Normalized linear volume in [0, 1]
    float pan;                // Left to right panning in [0, 1]
};

struct AudioPlayback : AudioHandle
{
    /// @brief Get AudioPlaybackObj byte size.
    static size_t byte_size();

    /// @brief Main thread creates audio playback instance.
    static AudioPlayback create(const AudioPlaybackInfo& info);

    /// @brief Main thread destroys audio playback instance.
    static void destroy(AudioPlayback playback);

    /// @brief Thread safe API after the playback is acquired by audio thread.
    class Accessor
    {
    public:
        Accessor(AudioPlaybackObj*);

        /// @brief Reads current playback state.
        /// @param info Output playback state except the pool allocator field
        /// @warning Each field is atomically read, but the full tuple of fields may not be atomic.
        void read(AudioPlaybackInfo& info);

        /// @brief Request volume change before next mix.
        void set_volume_linear(float volume);

        /// @brief Request pan change before next mix.
        void set_pan(float pan);

    private:
        AudioPlaybackObj* mObj;
    };

    /// @brief Create accessor from main thread.
    Accessor access();

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