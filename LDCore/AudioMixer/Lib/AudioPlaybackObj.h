#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioCommand.h>
#include <Ludens/Memory/Allocator.h>
#include <atomic>
#include <cstdint>

namespace LD {

struct AudioEffectObj;

struct AudioPlaybackObj : AudioObject
{
    AudioPlaybackObj* next = nullptr;
    AudioEffectObj* effectList = nullptr;
    PoolAllocator playbackPA;
    AudioBuffer buffer;
    AudioCommandQueue commandQueue;
    uint32_t frameCursor;
    std::atomic<float> volumeLinear;
    std::atomic<float> pan;
    bool isPlaying = false;
};

} // namespace LD