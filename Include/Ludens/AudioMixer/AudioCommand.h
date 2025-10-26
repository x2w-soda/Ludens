#pragma once

#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioEffect.h>
#include <Ludens/AudioMixer/AudioEffectInfo.h>
#include <Ludens/AudioMixer/AudioPlayback.h>
#include <Ludens/Header/Handle.h>
#include <atomic>
#include <cstdint>

namespace LD {

enum AudioCommandType
{
    AUDIO_COMMAND_CREATE_BUFFER = 0,
    AUDIO_COMMAND_DESTROY_BUFFER,
    AUDIO_COMMAND_CREATE_PLAYBACK,
    AUDIO_COMMAND_DESTROY_PLAYBACK,
    AUDIO_COMMAND_CREATE_PLAYBACK_EFFECT,
    AUDIO_COMMAND_DESTROY_PLAYBACK_EFFECT,
    AUDIO_COMMAND_START_PLAYBACK,
    AUDIO_COMMAND_PAUSE_PLAYBACK,
    AUDIO_COMMAND_RESUME_PLAYBACK,
    AUDIO_COMMAND_TYPE_ENUM_COUNT
};

struct AudioCommandCreatePlayback
{
    AudioPlayback playback;
    AudioBuffer buffer;
};

struct AudioCommandDestroyPlayback
{
    AudioPlayback playback;
};

struct AudioCommandCreatePlaybackEffect
{
    AudioPlayback playback;
    AudioEffect effect;
    uint32_t effectIdx;
};

struct AudioCommandDestroyPlaybackEffect
{
    AudioPlayback playback;
    AudioEffect effect;
};

struct AudioCommand
{
    AudioCommandType type;
    union
    {
        AudioBuffer createBuffer;
        AudioBuffer destroyBuffer;
        AudioCommandCreatePlayback createPlayback;
        AudioCommandDestroyPlayback destroyPlayback;
        AudioCommandCreatePlaybackEffect createPlaybackEffect;
        AudioCommandDestroyPlaybackEffect destroyPlaybackEffect;
        AudioPlayback startPlayback;
        AudioPlayback pausePlayback;
        AudioPlayback resumePlayback;
    };
};

/// @brief Audio command queue configuration.
struct AudioCommandQueueInfo
{
    size_t capacity; // max number of commands
};

/// @brief Thread safe queue of audio commands. Main thread enqueues new commands
///        while the audio thread dequeues commands for sequential execution.
struct AudioCommandQueue : Handle<struct AudioCommandQueueObj>
{
    /// @brief Create audio command queue.
    static AudioCommandQueue create(const AudioCommandQueueInfo& info);

    /// @brief Destroy audio command queue.
    static void destroy(AudioCommandQueue queue);

    /// @brief Main thread enqueues a new command for execution.
    /// @return True on success, or false if the queue is full.
    bool enqueue(const AudioCommand& cmd);

    /// @brief Audio thread dequeues a command for execution.
    /// @param cmd Output command on success.
    /// @return True on success, or false if the queue is empty.
    bool dequeue(AudioCommand& cmd);
};

} // namespace LD