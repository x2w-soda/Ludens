#pragma once

#include <Ludens/AudioMixer/AudioCommand.h>
#include <Ludens/Header/Handle.h>

#define AUDIO_MIXER_SAMPLE_RATE 48000
#define AUDIO_MIXER_CHANNELS 2

namespace LD {

struct AudioMixer : Handle<struct AudioMixerObj>
{
    /// @brief Create audio mixer.
    static AudioMixer create();

    /// @brief Destroy audio mixer.
    static void destroy(AudioMixer mixer);

    /// @brief Called on the main thread to retrieve command queue.
    AudioCommandQueue get_command_queue();

    /// @brief Called on the audio thread to digest all commands sequentially in queue.
    void poll_commands();

    /// @brief Called on the audio thread to mix all playback instances.
    /// @param outFrames Mixer output destination.
    /// @param frameCount Number of output frames requested
    void mix(float* outFrames, uint32_t frameCount);
};

} // namespace LD