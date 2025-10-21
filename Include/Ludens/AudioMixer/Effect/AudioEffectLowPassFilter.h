#include <Ludens/AudioMixer/AudioEffect.h>

namespace LD {

struct AudioEffectLowPassFilterInfo
{
    float cutoffFreq;
    float sampleRate;
};

struct AudioEffectLowPassFilter : AudioEffect
{
    static AudioEffectLowPassFilter create(const AudioEffectLowPassFilterInfo& info);
    static void destroy(AudioEffectLowPassFilter filter);
};

} // namespace LD