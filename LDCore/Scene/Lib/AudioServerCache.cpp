#include "AudioServerCache.h"

namespace LD {

void AudioServerCache::startup(AudioServer server)
{
    mServer = server;
    mClipToBuffer.clear();
}

void AudioServerCache::cleanup()
{
    for (auto ite : mClipToBuffer)
    {
        AudioBuffer buffer = ite.second;
        mServer.destroy_buffer(buffer);
    }
    mClipToBuffer.clear();

    mServer = {};
}

AudioBuffer AudioServerCache::get_or_create_audio_buffer(AudioClipAsset clipA)
{
    if (!clipA)
        return {};

    AUID clipAUID = clipA.get_auid();

    if (mClipToBuffer.contains(clipAUID))
        return mClipToBuffer[clipAUID];

    AudioBufferInfo bufferI{};
    bufferI.channels = clipA.get_channel_count();
    bufferI.format = SAMPLE_FORMAT_F32;
    bufferI.frameCount = clipA.get_frame_count();
    bufferI.sampleRate = clipA.get_sample_rate();
    bufferI.samples = clipA.get_frames(0);
    AudioBuffer buffer = mServer.create_buffer(bufferI);

    if (buffer)
        mClipToBuffer[clipAUID] = buffer;

    return buffer;
}

} // namespace LD