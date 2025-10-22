#pragma once
#include <Ludens/AudioMixer/AudioEffect.h>

namespace LD {

struct AudioEffectLowPassFilterInfo
{
    float cutoffFreq;
    float sampleRate;
};

struct AudioEffectLowPassFilter : AudioEffect
{
    /// @brief Create LPF instance.
    static AudioEffectLowPassFilter create(const AudioEffectLowPassFilterInfo& info);

    /// @brief Destroy LPF instance.
    static void destroy(AudioEffectLowPassFilter filter);
};

} // namespace LD