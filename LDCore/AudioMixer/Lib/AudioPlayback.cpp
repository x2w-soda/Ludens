#include "AudioPlaybackObj.h"
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioPlayback.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Memory.h>
#include <vector>

namespace LD {

size_t AudioPlayback::byte_size()
{
    return sizeof(AudioPlaybackObj);
}

AudioPlayback AudioPlayback::create(const AudioPlaybackInfo& info)
{
    PoolAllocator pa = info.playbackPA;
    auto* obj = (AudioPlaybackObj*)pa.allocate();
    new (obj) AudioPlaybackObj();
    obj->playbackPA = pa;
    obj->volumeLinear = std::clamp(info.volumeLinear, 0.0f, 1.0f);
    obj->pan = std::clamp(info.pan, 0.0f, 1.0f);
    obj->next = nullptr;
    obj->buffer = {};
    obj->frameCursor = 0;
    obj->isPlaying = false;

    return AudioPlayback(obj);
}

void AudioPlayback::destroy(AudioPlayback playback)
{
    LD_ASSERT(!playback.is_acquired());
    auto* obj = (AudioPlaybackObj*)playback.unwrap();

    obj->playbackPA.free(obj);
}

AudioPlayback::Accessor::Accessor(AudioPlaybackObj* obj)
    : mObj(obj)
{
}

void AudioPlayback::Accessor::read(AudioPlaybackInfo& info)
{
    info.playbackPA = {};
    info.pan = mObj->pan.load();
    info.volumeLinear = mObj->volumeLinear.load();
}

void AudioPlayback::Accessor::set_volume_linear(float volume)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_SET_PLAYBACK_VOLUME_LINEAR;
    cmd.setPlaybackVolumeLinear.playback = AudioPlayback(mObj);
    cmd.setPlaybackVolumeLinear.volumeLinear = volume;
    mObj->commandQueue.enqueue(cmd);
}

void AudioPlayback::Accessor::set_pan(float pan)
{
    AudioCommand cmd;
    cmd.type = AUDIO_COMMAND_SET_PLAYBACK_PAN;
    cmd.setPlaybackPan.playback = AudioPlayback(mObj);
    cmd.setPlaybackPan.pan = pan;
    mObj->commandQueue.enqueue(cmd);
}

AudioPlayback::Accessor AudioPlayback::access()
{
    return Accessor((AudioPlaybackObj*)mObj);
}

void AudioPlayback::set_buffer(AudioBuffer buffer)
{
    auto* obj = (AudioPlaybackObj*)mObj;

    obj->buffer = buffer;
    obj->frameCursor = 0;
    obj->isPlaying = false;
}

bool AudioPlayback::is_playing()
{
    auto* obj = (AudioPlaybackObj*)mObj;

    return obj->isPlaying;
}

void AudioPlayback::start()
{
    auto* obj = (AudioPlaybackObj*)mObj;

    obj->frameCursor = 0;
    obj->isPlaying = true;
}

void AudioPlayback::pause()
{
    auto* obj = (AudioPlaybackObj*)mObj;

    obj->isPlaying = false;
}

void AudioPlayback::resume()
{
    auto* obj = (AudioPlaybackObj*)mObj;

    obj->isPlaying = true;
}

uint32_t AudioPlayback::read_frames(float* outFrames, uint32_t frameCount)
{
    auto* obj = (AudioPlaybackObj*)mObj;

    if (!obj->isPlaying || !obj->buffer)
        return 0;

    uint32_t bufferFrameCount = obj->buffer.frame_count();
    LD_ASSERT(bufferFrameCount >= obj->frameCursor);

    uint32_t framesRead = std::min<uint32_t>(frameCount, bufferFrameCount - obj->frameCursor);

    // using sine approximation y = 0.5 x (3 - x * x) as pan law.
    float panR = obj->pan.load();
    float volume = obj->volumeLinear.load();
    float panL = 1.0f - panR;
    float gainL = volume * 0.5f * panL * (3.0f - panL * panL);
    float gainR = volume * 0.5f * panR * (3.0f - panR * panR);

    if (framesRead > 0)
    {
        const float* bufferFrames = obj->buffer.view_frame(obj->frameCursor);

        for (uint32_t i = 0; i < framesRead; i++)
        {
            outFrames[2 * i + 0] = gainL * bufferFrames[2 * i + 0];
            outFrames[2 * i + 1] = gainR * bufferFrames[2 * i + 1];
        }

        obj->frameCursor += framesRead;
    }
    else
        obj->isPlaying = false;

    return framesRead;
}

} // namespace LD