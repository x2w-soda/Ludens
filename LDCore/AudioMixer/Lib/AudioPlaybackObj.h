#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioCommand.h>
#include <Ludens/DSA/TripleBuffer.h>
#include <Ludens/Memory/Allocator.h>

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
    TripleBuffer<AudioPlaybackState> state;
    uint32_t frameCursor;
    bool isPlaying = false;
};

} // namespace LD