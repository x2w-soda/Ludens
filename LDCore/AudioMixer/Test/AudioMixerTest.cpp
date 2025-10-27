#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

#include "AudioMixerTest.h"

namespace LD {

/// @brief Does not play any sound, only processes commands on audio thread.
static void mixer_test_data_callback(MiniAudioDevice device, void* outFrames, const void* inFrames, uint32_t frameCount)
{
    memset(outFrames, 0, sizeof(float) * 2 * frameCount);

    AudioMixer mixer((AudioMixerObj*)device.get_user_data());
    mixer.poll_commands();
}

void AudioMixerTest::startup()
{
    PoolAllocatorInfo paI{};
    paI.blockSize = AudioPlayback::byte_size();
    paI.isMultiPage = true;
    paI.pageSize = 128;
    paI.usage = MEMORY_USAGE_AUDIO;
    playbackPA = PoolAllocator::create(paI);

    mixer = AudioMixer::create();

    MiniAudioInfo maI{};
    maI.dataCallback = &mixer_test_data_callback;
    maI.userData = mixer.unwrap();
    miniAudio = MiniAudio::create(maI);
}

void AudioMixerTest::cleanup()
{
    MiniAudio::destroy(miniAudio);
    miniAudio = {};

    AudioMixer::destroy(mixer);
    mixer = {};

    PoolAllocator::destroy(playbackPA);
    playbackPA = {};
}

} // namespace LD