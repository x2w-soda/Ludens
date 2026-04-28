#pragma once

#include <Ludens/AudioMixer/AudioEffect.h>
#include <Ludens/AudioMixer/AudioMixerDef.h>

namespace LD {

struct AudioEffectHighPassFilter : AudioEffect
{
    /// @brief Create HPF instance.
    static AudioEffectHighPassFilter create();

    /// @brief Destroy HPF instance.
    static void destroy(AudioEffectHighPassFilter filter);
};

} // namespace LD