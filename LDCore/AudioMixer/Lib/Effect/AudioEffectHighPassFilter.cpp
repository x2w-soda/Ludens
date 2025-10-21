#include <Ludens/AudioMixer/Effect/AudioEffectHighPassFilter.h>
#include <Ludens/DSP/BiquadFilterCoeff.h>
#include <Ludens/System/Memory.h>

namespace LD {

/// @brief Generic HPF implementation.
struct AudioEffectHighPassFilterObj : AudioEffectObj
{
    BiquadFilterCoeff coeff;
    BiquadFilterHistory historyL;
    BiquadFilterHistory historyR;

    virtual void process(float* outFrames, const float* inFrames, uint32_t frameCount) override;
};

void AudioEffectHighPassFilterObj::process(float* outFrames, const float* inFrames, uint32_t frameCount)
{
    for (uint32_t i = 0; i < frameCount; i++)
    {
        outFrames[2 * i + 0] = biquad_filter_process(coeff, historyL, inFrames[2 * i + 0]);
        outFrames[2 * i + 1] = biquad_filter_process(coeff, historyR, inFrames[2 * i + 1]);
    }
}

AudioEffectHighPassFilter AudioEffectHighPassFilter::create(const AudioEffectHighPassFilterInfo& info)
{
    auto* obj = heap_new<AudioEffectHighPassFilterObj>(MEMORY_USAGE_AUDIO);

    const float resonance = 1.0f;
    obj->coeff.as_high_pass_filter(resonance, info.cutoffFreq, info.sampleRate);
    obj->historyL = {};
    obj->historyR = {};

    return {obj};
}

void AudioEffectHighPassFilter::destroy(AudioEffectHighPassFilter filter)
{
    auto* obj = (AudioEffectHighPassFilterObj*)filter.unwrap();

    heap_delete<AudioEffectHighPassFilterObj>(obj);
}

} // namespace LD