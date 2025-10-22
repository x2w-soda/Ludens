#pragma once

#include <Ludens/AudioMixer/Effect/AudioEffectHighPassFilter.h>
#include <Ludens/AudioMixer/Effect/AudioEffectLowPassFilter.h>

namespace LD {

enum AudioEffectType
{
    AUDIO_EFFECT_LOW_PASS_FILTER = 0,
    AUDIO_EFFECT_HIGH_PASS_FILTER,
};

/// @brief Tagged union to encode all parameters across all types of filters.
struct AudioEffectInfo
{
    AudioEffectType type;
    union
    {
        AudioEffectLowPassFilterInfo lowPassFilter;
        AudioEffectHighPassFilterInfo highPassFilter;
    };
};

} // namespace LD