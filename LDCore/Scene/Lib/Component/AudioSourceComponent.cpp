#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/AudioSourceView.h>

#include "AudioSourceComponent.h"

namespace LD {

static bool load_audio_clip(SceneObj* scene, AudioSourceComponent* source, AssetID clipID)
{
    // NOTE: Buffer not destroyed upon component unload.
    //       Other components may still be using it for playback.
    AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(clipID);
    if (!buffer)
        return false;

    if (source->playback)
        scene->audioSystemCache.set_playback_buffer(source->playback, buffer);

    source->clipID = clipID;
    return true;
}

static bool load_audio_playback(SceneObj* scene, AudioSourceComponent* source, float pan, float volumeLinear)
{
    AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(source->clipID);
    if (!buffer)
        return false;

    source->playback = scene->audioSystemCache.create_playback(buffer, pan, volumeLinear);
    if (!source->playback)
        return false;

    source->pan = pan;
    source->volumeLinear = volumeLinear;
    return true;
}

void init_audio_source_component(ComponentBase** dstData)
{
    AudioSourceComponent* dstAudioSource = (AudioSourceComponent*)dstData;
    dstAudioSource->playback = {};
    dstAudioSource->clipID = (AssetID)0;
    dstAudioSource->pan = 0.5f;
    dstAudioSource->volumeLinear = 1.0f;
}

bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!load_audio_clip(scene, source, clipID))
    {
        err = "failed to prepare audio clip";
        return false;
    }

    if (!load_audio_playback(scene, source, pan, volumeLinear))
    {
        err = "failed to create AudioPlayback";
        return false;
    }

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
        scene->audioSystemCache.stop_playback(source->playback);

    return true;
}

AssetID audio_source_component_get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex)
{
    if (assetSlotIndex != 0)
        return 0;

    auto* source = (AudioSourceComponent*)data;
    return source->clipID;
}

bool audio_source_component_set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID id)
{
    auto* source = (AudioSourceComponent*)data;
    AudioSourceView sourceV(source);

    if (assetSlotIndex != 0)
        return false;

    return sourceV.set_clip_asset(id);
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

    AssetManager AM = AssetManager::get();
    AudioClipAsset clipA(AM.get_asset(clipID).unwrap());
    AudioBuffer buffer = sScene->audioSystemCache.get_or_create_audio_buffer(clipID);

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
    volume = std::clamp(volume, 0.0f, 1.0f);
    mAudioSource->volumeLinear = volume;

    if (mAudioSource->playback)
    {
        AudioPlayback::Accessor accessor = mAudioSource->playback.access();
        accessor.set_volume_linear(volume);
    }

    return true;
}

float AudioSourceView::get_pan()
{
    return mAudioSource->pan;
}

bool AudioSourceView::set_pan(float pan)
{
    pan = std::clamp(pan, 0.0f, 1.0f);
    mAudioSource->pan = pan;

    if (mAudioSource->playback)
    {
        AudioPlayback::Accessor accessor = mAudioSource->playback.access();
        accessor.set_pan(pan);
    }

    return true;
}

} // namespace LD
