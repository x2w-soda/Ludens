#pragma once
#include <Ludens/AudioMixer/AudioEffect.h>
#include <Ludens/AudioMixer/AudioMixerDef.h>

namespace LD {


struct AudioEffectLowPassFilter : AudioEffect
{
    /// @brief Create LPF instance.
    static AudioEffectLowPassFilter create();

    /// @brief Destroy LPF instance.
    static void destroy(AudioEffectLowPassFilter filter);
};

} // namespace LD