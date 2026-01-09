#include <Ludens/AudioMixer/AudioEffectInfo.h>
#include <Ludens/AudioMixer/Effect/AudioEffectHighPassFilter.h>
#include <Ludens/DSP/BiquadFilterCoeff.h>
#include <Ludens/Memory/Memory.h>
#include <atomic>

namespace LD {

/// @brief Generic HPF implementation.
struct AudioEffectHighPassFilterObj : AudioEffectObj
{
    BiquadFilterCoeff coeff;
    BiquadFilterHistory historyL;
    BiquadFilterHistory historyR;
    std::atomic<float> cutoffFreq;
    std::atomic<float> sampleRate;

    virtual void process(float* outFrames, const float* inFrames, uint32_t frameCount) override;

    virtual void read(AudioEffectInfo& info) override;
};

void AudioEffectHighPassFilterObj::process(float* outFrames, const float* inFrames, uint32_t frameCount)
{
    for (uint32_t i = 0; i < frameCount; i++)
    {
        outFrames[2 * i + 0] = biquad_filter_process(coeff, historyL, inFrames[2 * i + 0]);
        outFrames[2 * i + 1] = biquad_filter_process(coeff, historyR, inFrames[2 * i + 1]);
    }
}

void AudioEffectHighPassFilterObj::read(AudioEffectInfo& info)
{
    info.type = AUDIO_EFFECT_HIGH_PASS_FILTER;
    info.highPassFilter.cutoffFreq = cutoffFreq.load();
    info.highPassFilter.sampleRate = sampleRate.load();
}

AudioEffectHighPassFilter AudioEffectHighPassFilter::create(const AudioEffectHighPassFilterInfo& info)
{
    auto* obj = heap_new<AudioEffectHighPassFilterObj>(MEMORY_USAGE_AUDIO);
    obj->cutoffFreq = info.cutoffFreq;
    obj->sampleRate = info.sampleRate;

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