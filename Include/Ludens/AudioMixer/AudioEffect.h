#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <cstdint>

namespace LD {

struct AudioEffectInfo;

/// @brief Audio effect object base class.
class AudioEffectObj : public AudioObject
{
public:
    AudioEffectObj* next = nullptr;

    /// @brief Polymorphic frame processing.
    virtual void process(float* outFrames, const float* inFrames, uint32_t frameCount) = 0;
};

struct AudioEffect : AudioHandle
{
    /// @brief Audio thread processes the input stream with effect.
    void process(float* outFrames, const float* inFrames, uint32_t frameCount);
};

} // namespace LD