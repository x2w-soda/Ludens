#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/AudioSourceView.h>
#include <Ludens/Serial/Property.h>

#include "AudioSourceComponent.h"

namespace LD {

static bool audio_source_prop_getter(void* data, uint32_t propIndex, uint32_t arrayIndex, Value64& val)
{
    AudioSourceView view((AudioSourceComponent*)data);

    switch (propIndex)
    {
    case AUDIO_SOURCE_PROP_CLIP_ASSET:
        val.set_u32((uint32_t)view.get_clip_asset());
        break;
    case AUDIO_SOURCE_PROP_PAN:
        val.set_f32(view.get_pan());
        break;
    case AUDIO_SOURCE_PROP_VOLUME_LINEAR:
        val.set_f32(view.get_volume_linear());
        break;
    default:
        return false;
    }

    return true;
}

static bool audio_source_prop_setter(void* data, uint32_t propIndex, uint32_t arrayIndex, const Value64& val)
{
    AudioSourceView view((AudioSourceComponent*)data);

    switch (propIndex)
    {
    case AUDIO_SOURCE_PROP_CLIP_ASSET:
        view.set_clip_asset((AssetID)val.get_u32());
        break;
    case AUDIO_SOURCE_PROP_PAN:
        view.set_pan(val.get_f32());
        break;
    case AUDIO_SOURCE_PROP_VOLUME_LINEAR:
        view.set_volume_linear(val.get_f32());
        break;
    default:
        return false;
    }

    return true;
}

static PropertyMeta sAudioSourcePropMeta[] = {
    {"clip", nullptr, VALUE_TYPE_U32, Value64(0u), PROPERTY_UI_HINT_ASSET},
    {"pan", nullptr, VALUE_TYPE_F32, Value64(0.5f), PROPERTY_UI_HINT_SLIDER},
    {"volume_linear", nullptr, VALUE_TYPE_F32, Value64(1.0f), PROPERTY_UI_HINT_SLIDER},
};

TypeMeta AudioSourceMeta::sTypeMeta = {
    .props = sAudioSourcePropMeta,
    .propCount = sizeof(sAudioSourcePropMeta) / sizeof(*sAudioSourcePropMeta),
    .getLocal = &audio_source_prop_getter,
    .setLocal = &audio_source_prop_setter,
};

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

    source->playback = scene->audioSystemCache.create_playback(buffer);
    if (!source->playback)
        return false;

    source->playbackState.pan = pan;
    source->playbackState.volumeLinear = volumeLinear;
    source->playback.store(source->playbackState);
    return true;
}

void AudioSourceMeta::init(ComponentBase** dstData)
{
    AudioSourceComponent* dstAudioSource = (AudioSourceComponent*)dstData;
    dstAudioSource->playback = {};
    dstAudioSource->playbackState = {};
    dstAudioSource->clipID = (AssetID)0;
}

bool AudioSourceMeta::load(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear, String& err)
{
    LD_PROFILE_SCOPE;

    if (!clipID) // empty state without audio buffer or playback
        return true;

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

bool AudioSourceMeta::load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, String& err)
{
    AssetID clipAsset = {};
    float pan = 0.5f;
    float volumeLinear = 1.0f;

    for (const PropertyValue& prop : props)
    {
        switch (prop.propIndex)
        {
        case AUDIO_SOURCE_PROP_CLIP_ASSET:
            clipAsset = (AssetID)prop.value.get_u32();
            break;
        case AUDIO_SOURCE_PROP_PAN:
            pan = prop.value.get_f32();
            break;
        case AUDIO_SOURCE_PROP_VOLUME_LINEAR:
            volumeLinear = prop.value.get_f32();
            break;
        default:
            break;
        }
    }

    return load(sScene, (AudioSourceComponent*)data, clipAsset, pan, volumeLinear, err);
}

bool AudioSourceMeta::clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err)
{
    LD_PROFILE_SCOPE;

    AudioSourceView srcSource(srcData);
    AudioSourceView dstSource(dstData);
    LD_ASSERT(srcSource && dstSource);

    AssetID clipAID = srcSource.get_clip_asset();
    float pan = srcSource.get_pan();
    float volume = srcSource.get_volume_linear();

    return load(scene, (AudioSourceComponent*)dstSource.data(), clipAID, pan, volume, err);
}

bool AudioSourceMeta::unload(SceneObj* scene, ComponentBase** sourceData, String& err)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.destroy_playback(source->playback);
        source->playback = {};
    }

    return true;
}

bool AudioSourceMeta::cleanup(SceneObj* scene, ComponentBase** sourceData, String& err)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
        scene->audioSystemCache.stop_playback(source->playback);

    return true;
}

AssetID AudioSourceMeta::get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex)
{
    if (assetSlotIndex != 0)
        return 0;

    auto* source = (AudioSourceComponent*)data;
    return source->clipID;
}

bool AudioSourceMeta::set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID id)
{
    auto* source = (AudioSourceComponent*)data;
    AudioSourceView sourceV(source);

    if (assetSlotIndex != 0)
        return false;

    return sourceV.set_clip_asset(id);
}

AssetType AudioSourceMeta::get_asset_type(SceneObj* scene, uint32_t assetSlotIndex)
{
    if (assetSlotIndex != 0)
        return ASSET_TYPE_ENUM_COUNT;

    return ASSET_TYPE_AUDIO_CLIP;
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
    String err;

    return AudioSourceMeta::load(sScene, mAudioSource, clipAsset, pan, volumeLinear, err);
}

void AudioSourceView::play()
{
    sScene->audioSystemCache.start_playback(mAudioSource->playback);
}

void AudioSourceView::pause()
{
    sScene->audioSystemCache.pause_playback(mAudioSource->playback);
}

void AudioSourceView::resume()
{
    sScene->audioSystemCache.resume_playback(mAudioSource->playback);
}

bool AudioSourceView::set_clip_asset(AssetID clipID)
{
    AssetManager AM = AssetManager::get();
    AudioClipAsset clipA(AM.get_asset(clipID).unwrap());
    AudioBuffer buffer = sScene->audioSystemCache.get_or_create_audio_buffer(clipID);

    if (buffer)
    {
        if (!mAudioSource->playback)
            mAudioSource->playback = sScene->audioSystemCache.create_playback(buffer);
        else
            sScene->audioSystemCache.set_playback_buffer(mAudioSource->playback, buffer);

        mAudioSource->clipID = clipID;
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
    return mAudioSource->playbackState.volumeLinear;
}

bool AudioSourceView::set_volume_linear(float volume)
{
    volume = std::clamp(volume, 0.0f, 1.0f);
    mAudioSource->playbackState.volumeLinear = volume;

    if (mAudioSource->playback)
    {
        mAudioSource->playback.store(mAudioSource->playbackState);
    }

    return true;
}

float AudioSourceView::get_pan()
{
    return mAudioSource->playbackState.pan;
}

bool AudioSourceView::set_pan(float pan)
{
    pan = std::clamp(pan, 0.0f, 1.0f);
    mAudioSource->playbackState.pan = pan;

    if (mAudioSource->playback)
    {
        mAudioSource->playback.store(mAudioSource->playbackState);
    }

    return true;
}

} // namespace LD
