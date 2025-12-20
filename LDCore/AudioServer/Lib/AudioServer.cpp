#include <Ludens/AudioBackend/MiniAudio.h>
#include <Ludens/AudioMixer/AudioMixer.h>
#include <Ludens/AudioServer/AudioServer.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/System/Memory.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <unordered_set>

namespace LD {

struct AudioThreadData
{
    AudioMixer mixer;
    AudioCommandQueue commandQueue;
};

/// @brief Audio server implementation.
class AudioServerObj
{
public:
    AudioServerObj();
    ~AudioServerObj();

    void poll_deferred_destruction();

    AudioBuffer create_buffer(const AudioBufferInfo& bufferI);
    void destroy_buffer(AudioBuffer buffer);
    AudioPlayback create_playback(AudioBuffer buffer, float pan, float volumeLinear);
    void destroy_playback(AudioPlayback playback);
    void start_playback(AudioPlayback playback);
    void stop_playback(AudioPlayback playback);
    void pause_playback(AudioPlayback playback);
    void resume_playback(AudioPlayback playback);
    void set_playback_buffer(AudioPlayback playback, AudioBuffer buffer);

private:
    /// @brief Data callback invoked on the audio thread.
    static void data_callback(MiniAudioDevice device, void* outFrames, const void* inFrames, uint32_t frameCount);

private:
    MiniAudio mMA;
    AudioThreadData mAudioThread;
    PoolAllocator mPlaybackPA; // heap memory allocation happens on main thread
    std::unordered_set<void*> mDeferredBufferDestruction;
};

AudioServerObj::AudioServerObj()
{
    PoolAllocatorInfo paI{};
    paI.blockSize = AudioPlayback::byte_size();
    paI.isMultiPage = true;
    paI.pageSize = paI.blockSize * 32;
    paI.usage = MEMORY_USAGE_AUDIO;
    mPlaybackPA = PoolAllocator::create(paI);

    // The main thread pushes commands into a lock-free
    // queue, while the AudioMixer crunches commands and
    // mixes playbacks on the audio thread.
    mAudioThread.mixer = AudioMixer::create();
    mAudioThread.commandQueue = mAudioThread.mixer.get_command_queue();

    MiniAudioInfo maI{};
    maI.dataCallback = &AudioServerObj::data_callback;
    maI.userData = &mAudioThread;
    mMA = MiniAudio::create(maI);
}

AudioServerObj::~AudioServerObj()
{
    // NOTE: Technically this deadlocks if user does not call destroy_buffer
    //       on all handles returned by create_buffer. We could dummy-proof
    //       this by keeping track of all created handles... Currently we
    //       assume the user of AudioServer to be responsible.
    while (!mDeferredBufferDestruction.empty())
    {
        poll_deferred_destruction();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // kill audio thread
    MiniAudio::destroy(mMA);
    AudioMixer::destroy(mAudioThread.mixer);
    PoolAllocator::destroy(mPlaybackPA);
}

void AudioServerObj::poll_deferred_destruction()
{
    std::vector<void*> toErase;

    for (void* ptr : mDeferredBufferDestruction)
    {
        AudioBuffer buffer((AudioObject*)ptr);

        if (buffer.is_acquired())
            continue;

        // safe to free memory on main thread
        AudioBuffer::destroy(buffer);
        toErase.push_back(ptr);
    }

    for (void* ptr : toErase)
        mDeferredBufferDestruction.erase(ptr);
}

AudioBuffer AudioServerObj::create_buffer(const AudioBufferInfo& bufferI)
{
    AudioBuffer buffer = AudioBuffer::create(bufferI);
    if (!buffer)
        return {};

    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_CREATE_BUFFER;
    cmd.createBuffer = buffer;
    mAudioThread.commandQueue.enqueue(cmd);

    return buffer;
}

void AudioServerObj::destroy_buffer(AudioBuffer buffer)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_DESTROY_BUFFER;
    cmd.destroyBuffer = buffer;
    mAudioThread.commandQueue.enqueue(cmd);

    // NOTE: We can only free heap memory after audio thread releases the resource.
    //       Defer destruction until next update.
    mDeferredBufferDestruction.insert(buffer.unwrap());
}

AudioPlayback AudioServerObj::create_playback(AudioBuffer buffer, float pan, float volumeLinear)
{
    AudioPlaybackInfo playbackI{};
    playbackI.playbackPA = mPlaybackPA;
    playbackI.pan = pan;
    playbackI.volumeLinear = volumeLinear;
    AudioPlayback playback = AudioPlayback::create(playbackI);

    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK;
    cmd.createPlayback.buffer = buffer;
    cmd.createPlayback.playback = playback;
    mAudioThread.commandQueue.enqueue(cmd);

    return playback;
}

void AudioServerObj::destroy_playback(AudioPlayback playback)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_DESTROY_PLAYBACK;
    cmd.destroyPlayback.playback = playback;
    mAudioThread.commandQueue.enqueue(cmd);
}

void AudioServerObj::start_playback(AudioPlayback playback)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_START_PLAYBACK;
    cmd.startPlayback = playback;
    mAudioThread.commandQueue.enqueue(cmd);
}

void AudioServerObj::stop_playback(AudioPlayback playback)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_STOP_PLAYBACK;
    cmd.stopPlayback = playback;
    mAudioThread.commandQueue.enqueue(cmd);
}

void AudioServerObj::pause_playback(AudioPlayback playback)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_PAUSE_PLAYBACK;
    cmd.pausePlayback = playback;
    mAudioThread.commandQueue.enqueue(cmd);
}

void AudioServerObj::resume_playback(AudioPlayback playback)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_RESUME_PLAYBACK;
    cmd.resumePlayback = playback;
    mAudioThread.commandQueue.enqueue(cmd);
}

void AudioServerObj::set_playback_buffer(AudioPlayback playback, AudioBuffer buffer)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_SET_PLAYBACK_BUFFER;
    cmd.setPlaybackBuffer.playback = playback;
    cmd.setPlaybackBuffer.buffer = buffer;
    mAudioThread.commandQueue.enqueue(cmd);
}

void AudioServerObj::data_callback(MiniAudioDevice device, void* outFrames, const void* inFrames, uint32_t frameCount)
{
    LD_PROFILE_SCOPE;

    AudioThreadData& thread = *(AudioThreadData*)device.get_user_data();

    // NOTE: We are already in the MiniAudio data callback. This should probably be moved out of
    //       the data callback once we decide to design our own audio thread loop. In practice,
    //       observe profiler results to see if this eats up too much time.
    thread.mixer.poll_commands();

    // Have the mixer grind out frames.
    thread.mixer.mix((float*)outFrames, frameCount);
}

//
// Public API
//

AudioServer AudioServer::create()
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<AudioServerObj>(MEMORY_USAGE_AUDIO);

    return AudioServer(obj);
}

void AudioServer::destroy(AudioServer server)
{
    LD_PROFILE_SCOPE;

    auto* obj = server.unwrap();

    heap_delete<AudioServerObj>(obj);
}

void AudioServer::update()
{
    mObj->poll_deferred_destruction();
}

AudioBuffer AudioServer::create_buffer(const AudioBufferInfo& info)
{
    if (!info.samples)
        return {};

    return mObj->create_buffer(info);
}

void AudioServer::destroy_buffer(AudioBuffer buffer)
{
    if (!buffer)
        return;

    mObj->destroy_buffer(buffer);
}

AudioPlayback AudioServer::create_playback(AudioBuffer buffer, float pan, float volumeLinear)
{
    if (!buffer)
        return {};

    return mObj->create_playback(buffer, pan, volumeLinear);
}

void AudioServer::destroy_playback(AudioPlayback playback)
{
    if (!playback)
        return;

    mObj->destroy_playback(playback);
}

void AudioServer::start_playback(AudioPlayback playback)
{
    if (!playback)
        return;

    mObj->start_playback(playback);
}

void AudioServer::stop_playback(AudioPlayback playback)
{
    if (!playback)
        return;

    mObj->stop_playback(playback);
}

void AudioServer::pause_playback(AudioPlayback playback)
{
    if (!playback)
        return;

    mObj->pause_playback(playback);
}

void AudioServer::resume_playback(AudioPlayback playback)
{
    if (!playback)
        return;

    mObj->resume_playback(playback);
}

void AudioServer::set_playback_buffer(AudioPlayback playback, AudioBuffer buffer)
{
    if (!playback || !buffer)
        return;

    mObj->set_playback_buffer(playback, buffer);
}

} // namespace LD