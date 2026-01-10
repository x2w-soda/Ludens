#include <Extra/doctest/doctest.h>
#include <Ludens/AudioBackend/MiniAudio.h>
#include <Ludens/AudioMixer/AudioMixer.h>
#include <LudensUtil/LudensLFS.h>
#include <array>

#include "AudioMixerTest.h"

using namespace LD;

TEST_CASE("AudioMixer read playback" * doctest::skip(!LudensLFS::get_directory_path()))
{
    AudioMixerTest test;
    test.startup();

    AudioCommandQueue cmdQ = test.mixer.get_command_queue();

    AudioBuffer buffer1 = AudioBuffer::create_from_wav(sLudensLFS.audio.uiClick1Path);
    CHECK(buffer1);

    AudioPlaybackInfo playbackI{};
    playbackI.playbackPA = test.playbackPA;
    playbackI.volumeLinear = 1.0f;
    playbackI.pan = 0.0f;
    AudioPlayback playback1 = AudioPlayback::create(playbackI);
    CHECK(playback1);

    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_CREATE_BUFFER;
    cmd.createBuffer = buffer1;
    cmdQ.enqueue(cmd);

    cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK;
    cmd.createPlayback.buffer = buffer1;
    cmd.createPlayback.playback = playback1;
    cmdQ.enqueue(cmd);

    AudioPlayback::Accessor accessor = playback1.access();
    accessor.read(playbackI);
    CHECK(!playbackI.playbackPA);
    CHECK(playbackI.pan == 0.0f);
    CHECK(playbackI.volumeLinear == 1.0f);

    test.cleanup();
}

TEST_CASE("AudioMixer read filter effects" * doctest::skip(!LudensLFS::get_directory_path()))
{
    if (!sLudensLFS.isFound)
        doctest::skip();

    AudioMixerTest test;
    test.startup();

    AudioCommandQueue cmdQ = test.mixer.get_command_queue();

    AudioBuffer buffer1 = AudioBuffer::create_from_wav(sLudensLFS.audio.uiClick1Path);
    CHECK(buffer1);

    AudioPlaybackInfo playbackI{};
    playbackI.playbackPA = test.playbackPA;
    playbackI.pan = 1.0f;
    playbackI.volumeLinear = 1.0f;
    AudioPlayback playback1 = AudioPlayback::create(playbackI);
    CHECK(playback1);

    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_CREATE_BUFFER;
    cmd.createBuffer = buffer1;
    cmdQ.enqueue(cmd);

    cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK;
    cmd.createPlayback.buffer = buffer1;
    cmd.createPlayback.playback = playback1;
    cmdQ.enqueue(cmd);

    AudioEffectLowPassFilterInfo lpfI{};
    lpfI.cutoffFreq = 1234;
    lpfI.sampleRate = AUDIO_MIXER_SAMPLE_RATE;
    AudioEffectLowPassFilter lpf = AudioEffectLowPassFilter::create(lpfI);
    cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK_EFFECT;
    cmd.createPlaybackEffect.effect = lpf;
    cmd.createPlaybackEffect.effectIdx = 0;
    cmd.createPlaybackEffect.playback = playback1;
    cmdQ.enqueue(cmd);

    AudioEffectHighPassFilterInfo hpfI{};
    hpfI.cutoffFreq = 5678;
    hpfI.sampleRate = AUDIO_MIXER_SAMPLE_RATE;
    AudioEffectHighPassFilter hpf = AudioEffectHighPassFilter::create(hpfI);
    cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK_EFFECT;
    cmd.createPlaybackEffect.effect = hpf;
    cmd.createPlaybackEffect.effectIdx = 1;
    cmd.createPlaybackEffect.playback = playback1;
    cmdQ.enqueue(cmd);

    AudioEffectInfo effectI;
    lpf.read(effectI);
    CHECK(effectI.type == AUDIO_EFFECT_LOW_PASS_FILTER);
    CHECK(effectI.lowPassFilter.cutoffFreq == 1234);
    CHECK(effectI.lowPassFilter.sampleRate == AUDIO_MIXER_SAMPLE_RATE);

    hpf.read(effectI);
    CHECK(effectI.type == AUDIO_EFFECT_HIGH_PASS_FILTER);
    CHECK(effectI.highPassFilter.cutoffFreq == 5678);
    CHECK(effectI.highPassFilter.sampleRate == AUDIO_MIXER_SAMPLE_RATE);

    test.cleanup();
}