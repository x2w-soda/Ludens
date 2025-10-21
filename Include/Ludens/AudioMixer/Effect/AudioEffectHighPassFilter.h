#include <Ludens/AudioMixer/AudioEffect.h>

namespace LD {

struct AudioEffectHighPassFilterInfo
{
    float cutoffFreq;
    float sampleRate;
};

struct AudioEffectHighPassFilter : AudioEffect
{
    /// @brief Create HPF instance.
    static AudioEffectHighPassFilter create(const AudioEffectHighPassFilterInfo& info);

    /// @brief Destroy HPF instance.
    static void destroy(AudioEffectHighPassFilter filter);
};

} // namespace LD