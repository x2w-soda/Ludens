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

    /// @brief Polymorphic state readback. This reads the complete state of the audio effect,
    ///        safer than reading individual atomic fields from main thread.
    virtual void read(AudioEffectInfo& info) = 0;
};

struct AudioEffect : AudioHandle
{
    void process(float* outFrames, const float* inFrames, uint32_t frameCount);
};

} // namespace LD