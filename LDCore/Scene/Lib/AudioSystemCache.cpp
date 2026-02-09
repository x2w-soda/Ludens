#include "AudioSystemCache.h"

namespace LD {

void AudioSystemCache::startup(AudioSystem server, AssetManager manager)
{
    mSystem = server;
    mAssetManager = manager;
    mClipToBuffer.clear();
}

void AudioSystemCache::cleanup()
{
    if (!mSystem)
        return;

    for (auto ite : mClipToBuffer)
    {
        AudioBuffer buffer = ite.second;
        mSystem.destroy_buffer(buffer);
    }
    mClipToBuffer.clear();

    mSystem = {};
}

AudioBuffer AudioSystemCache::get_or_create_audio_buffer(AssetID clipID)
{
    AudioClipAsset clipA = (AudioClipAsset)mAssetManager.get_asset(clipID, ASSET_TYPE_AUDIO_CLIP);

    if (!clipA)
        return {};

    if (mClipToBuffer.contains(clipID))
        return mClipToBuffer[clipID];

    AudioBufferInfo bufferI{};
    bufferI.channels = clipA.get_channel_count();
    bufferI.format = SAMPLE_FORMAT_F32;
    bufferI.frameCount = clipA.get_frame_count();
    bufferI.sampleRate = clipA.get_sample_rate();
    bufferI.samples = clipA.get_frames(0);
    AudioBuffer buffer = mSystem.create_buffer(bufferI);

    if (buffer)
        mClipToBuffer[clipID] = buffer;

    return buffer;
}

} // namespace LD