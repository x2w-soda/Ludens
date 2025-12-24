#include <Ludens/AudioBackend/MiniAudio.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioMixer.h>
#include <Ludens/AudioMixer/AudioPlayback.h>
#include <Ludens/AudioMixer/Effect/AudioEffectHighPassFilter.h>
#include <Ludens/AudioMixer/Effect/AudioEffectLowPassFilter.h>
#include <Ludens/Event/Event.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Format/MP3.h>
#include <Ludens/Media/Format/WAV.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/Window/Input.h>
#include <Ludens/Window/Window.h>

// Using audio samples from the LFS repository.
#include <LDUtil/LudensLFS/Include/LudensLFS.h>

namespace LD {

static void mixer_data_callback(MiniAudioDevice device, void* outFrames, const void* inFrames, uint32_t frameCount)
{
    AudioMixer mixer((AudioMixerObj*)device.get_user_data());

    mixer.poll_commands();

    mixer.mix((float*)outFrames, frameCount);
}

class AudioMixerSandbox
{
public:
    AudioMixerSandbox()
    {
        PoolAllocatorInfo paI{};
        paI.blockSize = AudioPlayback::byte_size();
        paI.isMultiPage = true;
        paI.pageSize = 128;
        paI.usage = MEMORY_USAGE_AUDIO;
        mPlaybackPA = PoolAllocator::create(paI);

        mMixer = AudioMixer::create();

        MiniAudioInfo maI{};
        maI.dataCallback = &mixer_data_callback;
        maI.userData = mMixer.unwrap();
        mMA = MiniAudio::create(maI);

        mAmbienceAB = AudioBuffer::create_from_wav(sLudensLFS.audio.forestAmbiencePath);
        LD_ASSERT(mAmbienceAB);
        mClickAB = AudioBuffer::create_from_wav(sLudensLFS.audio.uiClick1Path);
        LD_ASSERT(mClickAB);

        AudioPlaybackInfo playbackI{};
        playbackI.playbackPA = mPlaybackPA;
        playbackI.pan = 0.5f;
        playbackI.volumeLinear = 0.6f;
        mPlayback1 = AudioPlayback::create(playbackI);
        playbackI.pan = 0.5f;
        playbackI.volumeLinear = 1.0f;
        mPlayback2 = AudioPlayback::create(playbackI);

        mCommands = mMixer.get_command_queue();
        {
            AudioCommand cmd{};
            cmd.type = AUDIO_COMMAND_CREATE_BUFFER;
            cmd.createBuffer = mAmbienceAB;
            mCommands.enqueue(cmd);

            cmd.type = AUDIO_COMMAND_CREATE_BUFFER;
            cmd.createBuffer = mClickAB;
            mCommands.enqueue(cmd);

            cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK;
            cmd.createPlayback.buffer = mAmbienceAB;
            cmd.createPlayback.playback = mPlayback1;
            mCommands.enqueue(cmd);

            cmd.type = AUDIO_COMMAND_CREATE_PLAYBACK;
            cmd.createPlayback.buffer = mClickAB;
            cmd.createPlayback.playback = mPlayback2;
            mCommands.enqueue(cmd);

            cmd.type = AUDIO_COMMAND_START_PLAYBACK;
            cmd.startPlayback = mPlayback1;
            mCommands.enqueue(cmd);
        }

        WindowInfo windowI{};
        windowI.user = this;
        windowI.width = 600;
        windowI.height = 600;
        windowI.name = "AudioMixerSandbox";
        windowI.onEvent = nullptr;
        Window::create(windowI);
    }

    ~AudioMixerSandbox()
    {
        AudioCommand cmd;
        cmd.type = AUDIO_COMMAND_DESTROY_PLAYBACK;
        cmd.destroyPlayback.playback = mPlayback1;
        mCommands.enqueue(cmd);
        cmd.type = AUDIO_COMMAND_DESTROY_PLAYBACK;
        cmd.destroyPlayback.playback = mPlayback2;
        mCommands.enqueue(cmd);
        cmd.type = AUDIO_COMMAND_DESTROY_BUFFER;
        cmd.destroyBuffer = mAmbienceAB;
        mCommands.enqueue(cmd);
        cmd.type = AUDIO_COMMAND_DESTROY_BUFFER;
        cmd.destroyBuffer = mClickAB;
        mCommands.enqueue(cmd);
        {
            while (mPlayback1.is_acquired() || mPlayback2.is_acquired())
                continue;
            AudioPlayback::destroy(mPlayback1);
            AudioPlayback::destroy(mPlayback2);
            while (mAmbienceAB.is_acquired() || mClickAB.is_acquired())
                continue;
            AudioBuffer::destroy(mAmbienceAB);
            AudioBuffer::destroy(mClickAB);
        }

        Window::destroy(Window::get());
        MiniAudio::destroy(mMA);
        AudioMixer::destroy(mMixer);
        PoolAllocator::destroy(mPlaybackPA);
    }

    void run()
    {
        Window window = Window::get();

        while (window.is_open())
        {
            window.poll_events();

            AudioCommand cmd;

            if (Input::get_key_down(KEY_CODE_SPACE))
            {
                static bool sToggle = true;

                if (sToggle)
                {
                    cmd.type = AUDIO_COMMAND_PAUSE_PLAYBACK;
                    cmd.pausePlayback = mPlayback1;
                }
                else
                {
                    cmd.type = AUDIO_COMMAND_RESUME_PLAYBACK;
                    cmd.resumePlayback = mPlayback1;
                }
                sToggle = !sToggle;

                mCommands.enqueue(cmd);
            }
            if (Input::get_key_down(KEY_CODE_O))
            {
                cmd.type = AUDIO_COMMAND_START_PLAYBACK;
                cmd.startPlayback = mPlayback2;
                mCommands.enqueue(cmd);
            }
        }
    }

private:
    MiniAudio mMA;
    AudioMixer mMixer;
    AudioCommandQueue mCommands;
    AudioBuffer mAmbienceAB;
    AudioBuffer mClickAB;
    AudioPlayback mPlayback1;
    AudioPlayback mPlayback2;
    PoolAllocator mPlaybackPA;
};

} // namespace LD

int main(int argc, char** argv)
{
    {
        LD::AudioMixerSandbox sandbox;
        sandbox.run();
    }
}