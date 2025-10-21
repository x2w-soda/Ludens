#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioPlayback.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Memory.h>
#include <vector>

namespace LD {

struct AudioPlaybackObj : AudioObject
{
    PoolAllocator playbackPA;
    AudioBuffer buffer;
    uint32_t frameCursor;
    bool isPlaying;
};

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

    // TODO: Playback level pitch and panning

    if (framesRead > 0)
    {
        const float* bufferFrames = obj->buffer.view_frame(obj->frameCursor);

        for (uint32_t i = 0; i < framesRead; i++)
        {
            outFrames[2 * i + 0] = bufferFrames[2 * i + 0];
            outFrames[2 * i + 1] = bufferFrames[2 * i + 1];
        }

        obj->frameCursor += framesRead;
    }
    else
        obj->isPlaying = false;

    return framesRead;
}

} // namespace LD