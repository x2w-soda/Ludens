#include "AudioServerCache.h"

namespace LD {

void AudioServerCache::startup(AudioServer server, AssetManager manager)
{
    mServer = server;
    mAssetManager = manager;
    mClipToBuffer.clear();
}

void AudioServerCache::cleanup()
{
    if (!mServer)
        return;

    for (auto ite : mClipToBuffer)
    {
        AudioBuffer buffer = ite.second;
        mServer.destroy_buffer(buffer);
    }
    mClipToBuffer.clear();

    mServer = {};
}

AudioBuffer AudioServerCache::get_or_create_audio_buffer(AUID clipAUID)
{
    AudioClipAsset clipA = (AudioClipAsset)mAssetManager.get_asset(clipAUID, ASSET_TYPE_AUDIO_CLIP);

    if (!clipA)
        return {};

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