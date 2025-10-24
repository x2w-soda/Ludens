#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>

namespace LD {

struct AudioEffectObj;

struct AudioPlaybackObj : AudioObject
{
    AudioPlaybackObj* next = nullptr;
    AudioEffectObj* effectList = nullptr;
    PoolAllocator playbackPA;
    AudioBuffer buffer;
    uint32_t frameCursor;
    float volumeLinear;
    float pan;
    bool isPlaying = false;
};

} // namespace LD