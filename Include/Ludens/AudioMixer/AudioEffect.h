#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <cstdint>

namespace LD {

/// @brief Audio effect object base class.
class AudioEffectObj : public AudioObject
{
public:
    AudioEffectObj* next = nullptr;

    virtual void process(float* outFrames, const float* inFrames, uint32_t frameCount) = 0;
};

struct AudioEffect : AudioHandle
{
    void process(float* outFrames, const float* inFrames, uint32_t frameCount);
};

} // namespace LD