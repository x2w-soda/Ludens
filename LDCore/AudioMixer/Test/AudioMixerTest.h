#pragma once

#include <Ludens/AudioBackend/MiniAudio.h>
#include <Ludens/AudioMixer/AudioCommand.h>
#include <Ludens/AudioMixer/AudioEffectInfo.h>
#include <Ludens/AudioMixer/AudioMixer.h>
#include <Ludens/Memory/Allocator.h>
#include <cstdint>
#include <vector>

namespace LD {

struct AudioMixerTest
{
    /// @brief In-place test environment startup.
    void startup();

    /// @brief In-place test environment cleanup.
    void cleanup();

    AudioMixer mixer;
    MiniAudio miniAudio;
    PoolAllocator playbackPA;
};

} // namespace LD