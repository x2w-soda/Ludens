#pragma once

#include <Ludens/Scene/ComponentViews.h>
#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIPrimitiveEdit.h>
#include <LudensEditor/EditorWidget/EUITransformEdit.h>

namespace LD {

void eui_component_transform_2d(EUITransform2DStorage* storage, ComponentView view);

struct EUIAudioSourceStorage
{
    EUISliderStorage volumeLinear;
    EUISliderStorage pan;
};

void eui_component_audio_source(EUIAudioSourceStorage* storage, AudioSourceView view);

struct EUICamera2DStorage
{
    EUITransform2DStorage transform;
    EUIRectStorage viewport;
};

void eui_component_camera_2d(EUICamera2DStorage* storage, Camera2DView view);

struct EUISprite2DStorage
{
    EUITransform2DStorage transform;
    EUIU32Storage zDepth;
    EUIVec2Storage pivot;
    EUIRectStorage region;
};

void eui_component_sprite_2d(EUISprite2DStorage* storage, Sprite2DView view);

struct EUIScreenUIStorage
{
};

void eui_component_screen_ui(EUIScreenUIStorage* storage, ScreenUIView view);

/// @brief Tagged union of all storage types.
struct EUIComponentStorage
{
    ComponentType type = COMPONENT_TYPE_ENUM_COUNT;
    union Storage
    {
        EUIAudioSourceStorage audioSource;
        EUITransform2DStorage transform2D;
        EUICamera2DStorage camera2D;
        EUISprite2DStorage sprite2D;
        EUIScreenUIStorage screenUI;

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