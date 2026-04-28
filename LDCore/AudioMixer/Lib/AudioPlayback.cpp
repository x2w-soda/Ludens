#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioPlayback.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Memory/Memory.h>
#include <algorithm>

#include "AudioPlaybackObj.h"

namespace LD {

size_t AudioPlayback::byte_size()
{
    return sizeof(AudioPlaybackObj);
}

AudioPlayback AudioPlayback::create(PoolAllocator pa)
{
    AudioPlaybackState state{};
    state.pan = 0.5f;
    state.volumeLinear = 1.0f;

    auto* obj = (AudioPlaybackObj*)pa.allocate();
    new (obj) AudioPlaybackObj();
    obj->playbackPA = pa;
    obj->state.store(state);
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

    obj->~AudioPlaybackObj();
    obj->playbackPA.free(obj);
}

void AudioPlayback::store(AudioPlaybackState state)
{
    auto* obj = (AudioPlaybackObj*)mObj;

    obj->state.store(state);
}

AudioPlaybackState AudioPlayback::load()
{
    auto* obj = (AudioPlaybackObj*)mObj;

    return obj->state.load();
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

void AudioPlayback::stop()
{
    auto* obj = (AudioPlaybackObj*)mObj;

    obj->frameCursor = 0;
    obj->isPlaying = false;
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
    AudioPlaybackState state = obj->state.load();
    float panR = state.pan;
    float volume = state.volumeLinear;
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
