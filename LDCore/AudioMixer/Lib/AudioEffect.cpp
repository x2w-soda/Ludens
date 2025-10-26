#include <Ludens/AudioMixer/AudioEffect.h>

namespace LD {

void AudioEffect::process(float* outFrames, const float* inFrames, uint32_t frameCount)
{
    ((AudioEffectObj*)mObj)->process(outFrames, inFrames, frameCount);
}

void AudioEffect::read(AudioEffectInfo& info)
{
    ((AudioEffectObj*)mObj)->read(info);
}

} // namespace LD