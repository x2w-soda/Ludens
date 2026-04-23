#pragma once

#include <Ludens/Scene/ComponentViews.h>
#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIProp.h>
#include <LudensEditor/EditorWidget/EUITransformEdit.h>

namespace LD {

void eui_component_transform_2d(EUITransform2DProp* storage, ComponentView view);

struct EUIAudioSourceStorage
{
    EUISliderProp volumeLinear;
    EUISliderProp pan;
};

void eui_component_audio_source(EUIAudioSourceStorage* storage, AudioSourceView view);

struct EUICamera2DStorage
{
    EUITransform2DProp transform;
    EUIF32Prop zoom;
    EUIRectProp viewport;
    EUIVec2Prop extent;
    EUIToggleProp constraint;
};

void eui_component_camera_2d(EUICamera2DStorage* storage, Camera2DView view);

struct EUISprite2DStorage
{
    EUITransform2DProp transform;
    EUIU32Prop zDepth;
    EUIVec2Prop pivot;
    EUIRectProp region;
};

void eui_component_sprite_2d(EUISprite2DStorage* storage, Sprite2DView view);

/// @brief Tagged union of all storage types.
struct EUIComponentStorage
{
    ComponentType type = COMPONENT_TYPE_ENUM_COUNT;
    union Storage
    {
        EUIAudioSourceStorage audioSource;
        EUITransform2DProp transform2D;
        EUICamera2DStorage camera2D;
        EUISprite2DStorage sprite2D;

        Storage() {}
        ~Storage() {}
    } as;

    void startup(ComponentType newType);
    void cleanup();

    ~EUIComponentStorage();
    EUIComponentStorage() {}
    EUIComponentStorage(const EUIComponentStorage&) = delete;

    EUIComponentStorage& operator=(const EUIComponentStorage&) = delete;
};

} // namespace LD