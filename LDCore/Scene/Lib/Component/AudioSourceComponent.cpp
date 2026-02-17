#include <Ludens/Profiler/Profiler.h>

#include "AudioSourceComponent.h"

namespace LD {

bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear)
{
    LD_PROFILE_SCOPE;

    // NOTE: Buffer not destroyed upon component unload.
    //       Other components may still be using it for playback.
    AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(clipID);
    if (!buffer)
        return false;

    source->pan = pan;
    source->volumeLinear = volumeLinear;
    source->playback = scene->audioSystemCache.create_playback(buffer, pan, volumeLinear);
    if (!source->playback)
        return false;

    source->clipID = clipID;
    source->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::AudioSource srcSource(srcData);
    Scene::AudioSource dstSource(dstData);
    LD_ASSERT(srcSource && dstSource);

    AssetID clipAID = srcSource.get_clip_asset();
    float pan = srcSource.get_pan();
    float volume = srcSource.get_volume_linear();

    return load_audio_source_component(scene, (AudioSourceComponent*)dstSource.data(), clipAID, pan, volume);
}

void unload_audio_source_component(SceneObj* scene, ComponentBase** sourceData)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.destroy_playback(source->playback);
        source->playback = {};
    }

    // NOTE: audio buffer still exists in audio system cache
    source->base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

void cleanup_audio_source_component(SceneObj* scene, ComponentBase** sourceData)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.stop_playback(source->playback);
        source->playback = {};
    }
}

Scene::AudioSource::AudioSource(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = comp.data();
        mAudioSource = (AudioSourceComponent*)mData;
    }
}

Scene::AudioSource::AudioSource(AudioSourceComponent* comp)
{
    if (comp && comp->base && comp->base->type == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = (ComponentBase**)comp;
        mAudioSource = comp;
    }
}

bool Scene::AudioSource::load(AssetID clipAsset, float pan, float volumeLinear)
{
    return load_audio_source_component(sScene, mAudioSource, clipAsset, pan, volumeLinear);
}

void Scene::AudioSource::play()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    sScene->audioSystemCache.start_playback(mAudioSource->playback);
}

void Scene::AudioSource::pause()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    sScene->audioSystemCache.pause_playback(mAudioSource->playback);
}

void Scene::AudioSource::resume()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    sScene->audioSystemCache.resume_playback(mAudioSource->playback);
}

bool Scene::AudioSource::set_clip_asset(AssetID clipID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    AudioClipAsset clipA(sScene->assetManager.get_asset(clipID).unwrap());
    AudioBuffer buffer = sScene->audioSystemCache.get_or_create_audio_buffer(clipA);

    if (!buffer)
    {
        mAudioSource->clipID = clipID;
        sScene->audioSystemCache.set_playback_buffer(mAudioSource->playback, buffer);
    }

    return false;
}

AssetID Scene::AudioSource::get_clip_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mAudioSource->clipID;
}

float Scene::AudioSource::get_volume_linear()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mAudioSource->volumeLinear;
}

bool Scene::AudioSource::set_volume_linear(float volume)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    volume = std::clamp(volume, 0.0f, 1.0f);
    mAudioSource->volumeLinear = volume;
    accessor.set_volume_linear(volume);

    return true;
}

float Scene::AudioSource::get_pan()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mAudioSource->pan;
}

bool Scene::AudioSource::set_pan(float pan)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    pan = std::clamp(pan, 0.0f, 1.0f);
    mAudioSource->pan = pan;
    accessor.set_pan(pan);

    return true;
}

} // namespace LD