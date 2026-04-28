#pragma once

#define AUDIO_MIXER_SAMPLE_RATE 48000
#define AUDIO_MIXER_CHANNELS 2

namespace LD {

struct AudioPlaybackObj;

enum AudioEffectType
{
    AUDIO_EFFECT_LOW_PASS_FILTER = 0,
    AUDIO_EFFECT_HIGH_PASS_FILTER,
};

struct AudioPlaybackState
{
    float pan;
    float volumeLinear;
};

struct AudioEffectLowPassFilterState
{
    float cutoffFreq;
    float sampleRate;
};

struct AudioEffectHighPassFilterState
{
    float cutoffFreq;
    float sampleRate;
};

} // namespace LD