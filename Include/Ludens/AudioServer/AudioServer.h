#pragma once

#include <Ludens/AudioMixer/AudioPlayback.h>
#include <Ludens/Header/Handle.h>

namespace LD {

/// @brief Audio server handle. This is the top-level audio abstraction
///        accessed on the main thread.
struct AudioServer : Handle<struct AudioServerObj>
{
    /// @brief Create the audio server.
    /// @note This creates the audio thread.
    static AudioServer create();

    /// @brief Destroy the audio server.
    /// @note This destroys the audio thread and audio resources on the heap.
    static void destroy(AudioServer server);

    /// @brief Called by main thread to free heap resources,
    ///        audio thread does not manage heap allocations.
    void update();

    /// @brief Create audio buffer from samples.
    AudioBuffer create_buffer(const AudioBufferInfo& info);

    /// @brief Destroy audio buffer.
    void destroy_buffer(AudioBuffer);

    /// @brief Create playback instance sampling from buffer.
    AudioPlayback create_playback(AudioBuffer buffer);

    /// @brief Destroy playback instance.
    void destroy_playback(AudioPlayback playback);

    /// @brief Start a playback. This sets the frame cursor to 0 and plays from beginning.
    void start_playback(AudioPlayback playback);

    /// @brief Pause a playback.
    void pause_playback(AudioPlayback playback);

    /// @brief Resume a playback.
    void resume_playback(AudioPlayback playback);

    /// @brief Set the buffer of playback.
    void set_playback_buffer(AudioPlayback playback, AudioBuffer buffer);
};

} // namespace LD