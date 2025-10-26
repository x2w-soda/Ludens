#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>
#include <atomic>

namespace LD {

struct AudioEffectObj;

struct AudioPlaybackObj : AudioObject
{
    AudioPlaybackObj* next = nullptr;
    AudioEffectObj* effectList = nullptr;
    PoolAllocator playbackPA;
    AudioBuffer buffer;
    uint32_t frameCursor;
    std::atomic<float> volumeLinear;
    std::atomic<float> pan;
    bool isPlaying = false;
};

} // namespace LD