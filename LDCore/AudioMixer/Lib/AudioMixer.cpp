#include "AudioPlaybackObj.h"
#include <Ludens/AudioMixer/AudioMixer.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <utility>

#define AUDIO_MIXER_TEMP_FRAME_COUNT 256

namespace LD {

static uint32_t mix_playback(float* mixFrames, uint32_t frameCount, AudioPlayback playback);
static void mix_frames(float* mixFrames, const float* inFrames, uint32_t frameCount);

uint32_t mix_playback(float* mixFrames, uint32_t frameCount, AudioPlayback playback)
{
    LD_PROFILE_SCOPE;

    uint32_t framesLeftToRead = frameCount;
    AudioPlaybackObj* playbackObj = (AudioPlaybackObj*)playback.unwrap();

    float tempBuffer1[AUDIO_MIXER_TEMP_FRAME_COUNT * AUDIO_MIXER_CHANNELS];
    float tempBuffer2[AUDIO_MIXER_TEMP_FRAME_COUNT * AUDIO_MIXER_CHANNELS];
    float* dstMixFrames = mixFrames;
    float* frontBuffer = tempBuffer1;
    float* backBuffer = tempBuffer2;

    while (framesLeftToRead != 0)
    {
        uint32_t framesToRead = std::min<uint32_t>(framesLeftToRead, AUDIO_MIXER_TEMP_FRAME_COUNT);
        uint32_t framesRead = playback.read_frames(frontBuffer, framesToRead);

        if (framesRead == 0)
            break; // playback exhausted

        // process playback-level DSP chain
        for (AudioEffectObj* effectObj = playbackObj->effectList; effectObj; effectObj = effectObj->next)
        {
            effectObj->process(backBuffer, frontBuffer, framesRead);

            std::swap(frontBuffer, backBuffer);
        }

        mix_frames(dstMixFrames, frontBuffer, framesRead);

        dstMixFrames += framesRead * AUDIO_MIXER_CHANNELS;
        framesLeftToRead -= framesRead;
    }

    return framesLeftToRead;
}

void mix_frames(float* mixFrames, const float* inFrames, uint32_t frameCount)
{
    LD_ASSERT(AUDIO_MIXER_CHANNELS == 2);

    for (uint32_t i = 0; i < frameCount; i++)
    {
        mixFrames[2 * i + 0] += inFrames[2 * i + 0];
        mixFrames[2 * i + 1] += inFrames[2 * i + 1];
    }
}

/// @brief Audio mixer implementation.
class AudioMixerObj
{
public:
    AudioMixerObj();
    ~AudioMixerObj();

    void poll_commands();

    void mix(float* outFrames, uint32_t frameCount);

    inline AudioCommandQueue get_command_queue()
    {
        return mCommands;
    }

    static void create_buffer(AudioMixerObj* self, const AudioCommand& cmd);
    static void destroy_buffer(AudioMixerObj* self, const AudioCommand& cmd);
    static void create_playback(AudioMixerObj* self, const AudioCommand& cmd);
    static void destroy_playback(AudioMixerObj* self, const AudioCommand& cmd);
    static void set_playback_buffer(AudioMixerObj* self, const AudioCommand& cmd);
    static void create_playback_effect(AudioMixerObj* self, const AudioCommand& cmd);
    static void destroy_playback_effect(AudioMixerObj* self, const AudioCommand& cmd);
    static void start_playback(AudioMixerObj* self, const AudioCommand& cmd);
    static void pause_playback(AudioMixerObj* self, const AudioCommand& cmd);
    static void resume_playback(AudioMixerObj* self, const AudioCommand& cmd);

private:
    /// @brief The command queue is accessed by both main thread and audio thread.
    AudioCommandQueue mCommands;
    AudioPlaybackObj* mPlaybackList;
};

void AudioMixerObj::create_buffer(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_CREATE_BUFFER);

    AudioBuffer buffer = cmd.createBuffer;
    if (buffer.is_acquired())
        return;

    buffer.acquire();
}

void AudioMixerObj::destroy_buffer(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_DESTROY_BUFFER);

    AudioBuffer buffer = cmd.destroyBuffer;
    if (!buffer.is_acquired())
        return;

    // TODO: check for playbacks that are reading from this buffer.

    buffer.release();
}

void AudioMixerObj::create_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_CREATE_PLAYBACK);

    AudioPlayback playback = cmd.createPlayback.playback;
    AudioBuffer buffer = cmd.createPlayback.buffer;

    if (playback.is_acquired() || !buffer.is_acquired())
        return;

    playback.acquire();
    playback.set_buffer(buffer);

    AudioPlaybackObj* playbackObj = (AudioPlaybackObj*)playback.unwrap();
    playbackObj->next = mixer->mPlaybackList;
    mixer->mPlaybackList = playbackObj;
}

void AudioMixerObj::destroy_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_DESTROY_PLAYBACK);

    AudioPlayback playback = cmd.destroyPlayback.playback;
    if (!playback.is_acquired())
        return;

    playback.release();
    AudioPlaybackObj* toRemove = (AudioPlaybackObj*)playback.unwrap();

    AudioPlaybackObj** pObj = &mixer->mPlaybackList;
    while (*pObj && (*pObj) != toRemove)
        pObj = &(*pObj)->next;

    if (*pObj == toRemove)
        *pObj = toRemove->next;
}

void AudioMixerObj::set_playback_buffer(AudioMixerObj* self, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_SET_PLAYBACK_BUFFER);

    AudioPlayback playback = cmd.setPlaybackBuffer.playback;
    AudioBuffer buffer = cmd.setPlaybackBuffer.buffer;

    if (!playback.is_acquired() || !buffer.is_acquired())
        return;

    AudioPlaybackObj* playbackObj = (AudioPlaybackObj*)playback.unwrap();
    playbackObj->buffer = buffer;
    playbackObj->isPlaying = false;
    playbackObj->frameCursor = 0;
}

void AudioMixerObj::create_playback_effect(AudioMixerObj* self, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_CREATE_PLAYBACK_EFFECT);

    AudioPlayback playback = cmd.createPlaybackEffect.playback;
    AudioPlaybackObj* playbackObj = (AudioPlaybackObj*)playback.unwrap();
    AudioEffect effect = cmd.createPlaybackEffect.effect;
    AudioEffectObj* effectObj = (AudioEffectObj*)effect.unwrap();
    uint32_t idx = cmd.createPlaybackEffect.effectIdx;

    if (effect.is_acquired() || !playback.is_acquired())
        return;

    AudioEffectObj** pObj = &playbackObj->effectList;
    while (idx != 0 && *pObj)
    {
        idx--;
        pObj = &(*pObj)->next;
    }

    if (idx != 0)
        return;

    effect.acquire();

    effectObj->next = *pObj;
    *pObj = effectObj;
}

void AudioMixerObj::destroy_playback_effect(AudioMixerObj* self, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_DESTROY_PLAYBACK_EFFECT);

    AudioPlayback playback = cmd.destroyPlaybackEffect.playback;
    AudioEffect effect = cmd.destroyPlaybackEffect.effect;

    if (!playback.is_acquired() || !effect.is_acquired())
        return;

    effect.release();

    // TODO: remove from linked list
}

void AudioMixerObj::start_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_START_PLAYBACK);

    AudioPlayback playback = cmd.startPlayback;

    if (!playback.is_acquired())
        return;

    playback.start();
}

void AudioMixerObj::pause_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_PAUSE_PLAYBACK);

    AudioPlayback playback = cmd.pausePlayback;

    if (!playback.is_acquired())
        return;

    playback.pause();
}

void AudioMixerObj::resume_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_RESUME_PLAYBACK);

    AudioPlayback playback = cmd.resumePlayback;

    if (!playback.is_acquired())
        return;

    playback.resume();
}

// clang-format off
struct AudioCommandMeta
{
    AudioCommandType type;
    void (*fn)(AudioMixerObj* mixer, const AudioCommand&);
} sCommandTable[] = {
    {AUDIO_COMMAND_CREATE_BUFFER,           &AudioMixerObj::create_buffer},
    {AUDIO_COMMAND_DESTROY_BUFFER,          &AudioMixerObj::destroy_buffer},
    {AUDIO_COMMAND_CREATE_PLAYBACK,         &AudioMixerObj::create_playback},
    {AUDIO_COMMAND_DESTROY_PLAYBACK,        &AudioMixerObj::destroy_playback},
    {AUDIO_COMMAND_SET_PLAYBACK_BUFFER,     &AudioMixerObj::set_playback_buffer},
    {AUDIO_COMMAND_CREATE_PLAYBACK_EFFECT,  &AudioMixerObj::create_playback_effect},
    {AUDIO_COMMAND_DESTROY_PLAYBACK_EFFECT, &AudioMixerObj::destroy_playback_effect},
    {AUDIO_COMMAND_START_PLAYBACK,          &AudioMixerObj::start_playback},
    {AUDIO_COMMAND_PAUSE_PLAYBACK,          &AudioMixerObj::pause_playback},
    {AUDIO_COMMAND_RESUME_PLAYBACK,         &AudioMixerObj::resume_playback},
};
// clang-format on

static_assert(sizeof(sCommandTable) / sizeof(*sCommandTable) == AUDIO_COMMAND_TYPE_ENUM_COUNT);

AudioMixerObj::AudioMixerObj()
{
    AudioCommandQueueInfo queueI{};
    queueI.capacity = 256;
    mCommands = AudioCommandQueue::create(queueI);
    mPlaybackList = nullptr;
}

AudioMixerObj::~AudioMixerObj()
{
    AudioCommandQueue::destroy(mCommands);

    // TODO: mPlaybackList
}

void AudioMixerObj::poll_commands()
{
    AudioCommand cmd;

    while (mCommands.dequeue(cmd))
    {
        sCommandTable[(int)cmd.type].fn(this, cmd);
    }
}

void AudioMixerObj::mix(float* outFrames, uint32_t frameCount)
{
    memset(outFrames, 0, sizeof(float) * frameCount * AUDIO_MIXER_CHANNELS);

    for (AudioPlaybackObj* playbackObj = mPlaybackList; playbackObj; playbackObj = playbackObj->next)
    {
        AudioPlayback playback(playbackObj);

        if (!playback.is_playing())
            continue;

        mix_playback(outFrames, frameCount, playback);
    }
}

//
// Public API
//

AudioMixer AudioMixer::create()
{
    AudioMixerObj* obj = heap_new<AudioMixerObj>(MEMORY_USAGE_AUDIO);

    return AudioMixer(obj);
}

void AudioMixer::destroy(AudioMixer mixer)
{
    AudioMixerObj* obj = mixer.unwrap();

    heap_delete<AudioMixerObj>(obj);
}

AudioCommandQueue AudioMixer::get_command_queue()
{
    return mObj->get_command_queue();
}

void AudioMixer::poll_commands()
{
    LD_PROFILE_SCOPE;

    mObj->poll_commands();
}

void AudioMixer::mix(float* outFrames, uint32_t frameCount)
{
    LD_PROFILE_SCOPE;

    mObj->mix(outFrames, frameCount);
}

} // namespace LD