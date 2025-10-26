#include <Ludens/AudioMixer/AudioEffectInfo.h>
#include <Ludens/AudioMixer/Effect/AudioEffectLowPassFilter.h>
#include <Ludens/DSP/BiquadFilterCoeff.h>
#include <Ludens/System/Memory.h>
#include <atomic>

namespace LD {

/// @brief Generic LPF implementation.
struct AudioEffectLowPassFilterObj : AudioEffectObj
{
    BiquadFilterCoeff coeff;
    BiquadFilterHistory historyL;
    BiquadFilterHistory historyR;
    std::atomic<float> cutoffFreq;
    std::atomic<float> sampleRate;

    virtual void process(float* outFrames, const float* inFrames, uint32_t frameCount) override;

    virtual void read(AudioEffectInfo& info) override;
};

void AudioEffectLowPassFilterObj::process(float* outFrames, const float* inFrames, uint32_t frameCount)
{
    for (uint32_t i = 0; i < frameCount; i++)
    {
        outFrames[2 * i + 0] = biquad_filter_process(coeff, historyL, inFrames[2 * i + 0]);
        outFrames[2 * i + 1] = biquad_filter_process(coeff, historyR, inFrames[2 * i + 1]);
    }
}

void AudioEffectLowPassFilterObj::read(AudioEffectInfo& info)
{
    info.type = AUDIO_EFFECT_LOW_PASS_FILTER;
    info.lowPassFilter.cutoffFreq = cutoffFreq.load();
    info.lowPassFilter.sampleRate = sampleRate.load();
}

AudioEffectLowPassFilter AudioEffectLowPassFilter::create(const AudioEffectLowPassFilterInfo& info)
{
    auto* obj = heap_new<AudioEffectLowPassFilterObj>(MEMORY_USAGE_AUDIO);
    obj->cutoffFreq = info.cutoffFreq;
    obj->sampleRate = info.sampleRate;

    const float resonance = 1.0f;
    obj->coeff.as_low_pass_filter(resonance, info.cutoffFreq, info.sampleRate);
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