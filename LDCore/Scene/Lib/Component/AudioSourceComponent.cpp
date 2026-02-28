#include <Ludens/Profiler/Profiler.h>

#include "AudioSourceComponent.h"

namespace LD {

static const AudioSourceComponent sDefaultAudioSource = {
    .playback = {},
    .clipID = 0,
    .pan = 0.5f,
    .volumeLinear = 1.0f,
};

void init_audio_source_component(ComponentBase** dstData)
{
    memcpy(dstData, &sDefaultAudioSource, sizeof(AudioSourceComponent));
}

bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, std::string& err)
{
    LD_PROFILE_SCOPE;

    // NOTE: Buffer not destroyed upon component unload.
    //       Other components may still be using it for playback.
    AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(clipID);
    if (!buffer)
    {
        err = "failed to create AudioBuffer";
        return false;
    }

    source->pan = pan;
    source->volumeLinear = volumeLinear;
    source->playback = scene->audioSystemCache.create_playback(buffer, pan, volumeLinear);
    if (!source->playback)
    {
        err = "failed to create AudioPlayback";
        return false;
    }

    source->clipID = clipID;
    return true;
}

bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    AudioSourceView srcSource(srcData);
    AudioSourceView dstSource(dstData);
    LD_ASSERT(srcSource && dstSource);

    AssetID clipAID = srcSource.get_clip_asset();
    float pan = srcSource.get_pan();
    float volume = srcSource.get_volume_linear();

    return load_audio_source_component(scene, (AudioSourceComponent*)dstSource.data(), clipAID, pan, volume, err);
}

bool unload_audio_source_component(SceneObj* scene, ComponentBase** sourceData, std::string& err)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.destroy_playback(source->playback);
        source->playback = {};
    }

    return true;
}

bool cleanup_audio_source_component(SceneObj* scene, ComponentBase** sourceData, std::string& err)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.stop_playback(source->playback);
        source->playback = {};
    }

    return true;
}

AudioSourceView::AudioSourceView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = comp.data();
        mAudioSource = (AudioSourceComponent*)mData;
    }
}

AudioSourceView::AudioSourceView(AudioSourceComponent* comp)
{
    if (comp && comp->base && comp->base->type == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = (ComponentBase**)comp;
        mAudioSource = comp;
    }
}

bool AudioSourceView::load(AssetID clipAsset, float pan, float volumeLinear)
{
    std::string err;

    return load_audio_source_component(sScene, mAudioSource, clipAsset, pan, volumeLinear, err);
}

void AudioSourceView::play()
{
    LD_ASSERT(mAudioSource->playback);

    sScene->audioSystemCache.start_playback(mAudioSource->playback);
}

void AudioSourceView::pause()
{
    LD_ASSERT(mAudioSource->playback);

    sScene->audioSystemCache.pause_playback(mAudioSource->playback);
}

void AudioSourceView::resume()
{
    LD_ASSERT(mAudioSource->playback);

    sScene->audioSystemCache.resume_playback(mAudioSource->playback);
}

bool AudioSourceView::set_clip_asset(AssetID clipID)
{
    LD_ASSERT(mAudioSource->playback);

    AudioClipAsset clipA(sScene->assetManager.get_asset(clipID).unwrap());
    AudioBuffer buffer = sScene->audioSystemCache.get_or_create_audio_buffer(clipA);

    if (buffer)
    {
        mAudioSource->clipID = clipID;
        sScene->audioSystemCache.set_playback_buffer(mAudioSource->playback, buffer);
        return true;
    }

    return false;
}

AssetID AudioSourceView::get_clip_asset()
{
    return mAudioSource->clipID;
}

float AudioSourceView::get_volume_linear()
{
    return mAudioSource->volumeLinear;
}

bool AudioSourceView::set_volume_linear(float volume)
{
    LD_ASSERT(mAudioSource->playback);

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    volume = std::clamp(volume, 0.0f, 1.0f);
    mAudioSource->volumeLinear = volume;
    accessor.set_volume_linear(volume);

    return true;
}

float AudioSourceView::get_pan()
{
    return mAudioSource->pan;
}

bool AudioSourceView::set_pan(float pan)
{
    LD_ASSERT(mAudioSource->playback);

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    pan = std::clamp(pan, 0.0f, 1.0f);
    mAudioSource->pan = pan;
    accessor.set_pan(pan);

    return true;
}

} // namespace LD
