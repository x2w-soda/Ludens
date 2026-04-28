#include <Ludens/AudioMixer/AudioEffectInfo.h>
#include <Ludens/AudioMixer/Effect/AudioEffectLowPassFilter.h>
#include <Ludens/DSA/TripleBuffer.h>
#include <Ludens/DSP/BiquadFilterCoeff.h>
#include <Ludens/Memory/Memory.h>
#include <atomic>

namespace LD {

/// @brief Generic LPF implementation.
struct AudioEffectLowPassFilterObj : AudioEffectObj
{
    BiquadFilterCoeff coeff;
    BiquadFilterHistory historyL;
    BiquadFilterHistory historyR;
    TripleBuffer<AudioEffectLowPassFilterState> state;

    virtual void process(float* outFrames, const float* inFrames, uint32_t frameCount) override;
};

void AudioEffectLowPassFilterObj::process(float* outFrames, const float* inFrames, uint32_t frameCount)
{
    for (uint32_t i = 0; i < frameCount; i++)
    {
        outFrames[2 * i + 0] = biquad_filter_process(coeff, historyL, inFrames[2 * i + 0]);
        outFrames[2 * i + 1] = biquad_filter_process(coeff, historyR, inFrames[2 * i + 1]);
    }
}

AudioEffectLowPassFilter AudioEffectLowPassFilter::create()
{
    AudioEffectLowPassFilterState state{};

    auto* obj = heap_new<AudioEffectLowPassFilterObj>(MEMORY_USAGE_AUDIO);
    obj->state.store(state);

    const float resonance = 1.0f;
    obj->coeff.as_low_pass_filter(resonance, state.cutoffFreq, state.sampleRate);
    obj->historyL = {};
    obj->historyR = {};

    return {obj};
}

void AudioEffectLowPassFilter::destroy(AudioEffectLowPassFilter filter)
{
    auto* obj = (AudioEffectLowPassFilterObj*)filter.unwrap();

    heap_delete<AudioEffectLowPassFilterObj>(obj);
}

} // namespace LD