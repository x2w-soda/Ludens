#include <Ludens/AudioMixer/AudioMixer.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <unordered_map>

#define AUDIO_MIXER_TEMP_FRAME_COUNT 256

namespace LD {

static uint32_t mix_playback(float* mixFrames, uint32_t frameCount, AudioPlayback playback);
static void mix_frames(float* mixFrames, const float* inFrames, uint32_t frameCount);

uint32_t mix_playback(float* mixFrames, uint32_t frameCount, AudioPlayback playback)
{
    uint32_t framesLeftToRead = frameCount;

    float tempFrames[AUDIO_MIXER_TEMP_FRAME_COUNT * AUDIO_MIXER_CHANNELS];
    float* dstMixFrames = mixFrames;

    while (framesLeftToRead != 0)
    {
        uint32_t framesToRead = std::min<uint32_t>(framesLeftToRead, AUDIO_MIXER_TEMP_FRAME_COUNT);
        uint32_t framesRead = playback.read_frames(tempFrames, framesToRead);

        if (framesRead == 0)
            break; // playback exhausted

        mix_frames(dstMixFrames, tempFrames, framesRead);

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
    static void start_playback(AudioMixerObj* self, const AudioCommand& cmd);
    static void pause_playback(AudioMixerObj* self, const AudioCommand& cmd);
    static void resume_playback(AudioMixerObj* self, const AudioCommand& cmd);

private:
    /// @brief The command queue is accessed by both main thread and audio thread.
    AudioCommandQueue mCommands;
    std::unordered_map<Hash32, AudioBuffer> mBuffers;
    std::unordered_map<Hash32, AudioPlayback> mPlaybacks;
};

void AudioMixerObj::create_buffer(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_CREATE_BUFFER);

    if (mixer->mBuffers.contains(cmd.createBuffer.bufferName))
        return;

    AudioBuffer buffer = cmd.createBuffer.buffer;
    buffer.acquire();

    mixer->mBuffers[cmd.createBuffer.bufferName] = buffer;
}

void AudioMixerObj::destroy_buffer(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_DESTROY_BUFFER);

    auto ite = mixer->mBuffers.find(cmd.destroyBuffer.bufferName);
    if (ite == mixer->mBuffers.end())
        return;

    AudioBuffer buffer = ite->second;
    buffer.release();

    mixer->mBuffers.erase(ite);
}

void AudioMixerObj::create_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_CREATE_PLAYBACK);

    if (mixer->mPlaybacks.contains(cmd.createPlayback.playbackName))
        return;

    if (!mixer->mBuffers.contains(cmd.createPlayback.bufferName))
        return;

    AudioPlayback playback = cmd.createPlayback.playback;
    playback.acquire();
    playback.set_buffer(mixer->mBuffers[cmd.createPlayback.bufferName]);

    mixer->mPlaybacks[cmd.createPlayback.playbackName] = playback;
}

void AudioMixerObj::destroy_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_DESTROY_PLAYBACK);

    auto ite = mixer->mPlaybacks.find(cmd.destroyPlayback.playbackName);
    if (ite == mixer->mPlaybacks.end())
        return;

    AudioPlayback playback = ite->second;
    playback.release();

    mixer->mPlaybacks.erase(ite);
}

void AudioMixerObj::start_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_START_PLAYBACK);

    auto ite = mixer->mPlaybacks.find(cmd.startPlayback);
    if (ite == mixer->mPlaybacks.end())
        return;

    AudioPlayback playback = ite->second;
    playback.start();
}

void AudioMixerObj::pause_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_PAUSE_PLAYBACK);

    auto ite = mixer->mPlaybacks.find(cmd.pausePlayback);
    if (ite == mixer->mPlaybacks.end())
        return;

    AudioPlayback playback = ite->second;
    playback.pause();
}

void AudioMixerObj::resume_playback(AudioMixerObj* mixer, const AudioCommand& cmd)
{
    LD_ASSERT(cmd.type == AUDIO_COMMAND_RESUME_PLAYBACK);

    auto ite = mixer->mPlaybacks.find(cmd.resumePlayback);
    if (ite == mixer->mPlaybacks.end())
        return;

    AudioPlayback playback = ite->second;
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
}

AudioMixerObj::~AudioMixerObj()
{
    AudioCommandQueue::destroy(mCommands);
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

    for (auto ite : mPlaybacks)
    {
        AudioPlayback playback = ite.second;

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