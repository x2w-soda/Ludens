#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Scene/ComponentViews.h>
#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIComponent.h>
#include <LudensEditor/EditorWidget/EUIVectorEdit.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

static void eui_inspect_audio_source_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_transform_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_transform_2d_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_camera_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_camera_2d_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_mesh_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_sprite_2d_component(EUIComponentStorage* storage, ComponentView comp);
static void eui_inspect_screen_ui_component(EUIComponentStorage* storage, ComponentView comp);

static void (*sEUIInspectFnTable[])(EUIComponentStorage* storage, ComponentView comp) = {
    nullptr,
    &eui_inspect_audio_source_component,
    &eui_inspect_transform_component,
    &eui_inspect_transform_2d_component,
    &eui_inspect_camera_component,
    &eui_inspect_camera_2d_component,
    &eui_inspect_mesh_component,
    &eui_inspect_sprite_2d_component,
    &eui_inspect_screen_ui_component,
};

static_assert(sizeof(sEUIInspectFnTable) / sizeof(*sEUIInspectFnTable) == COMPONENT_TYPE_ENUM_COUNT);

void eui_inspect_audio_source_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE);

    eui_component_audio_source(&storage->as.audioSource, comp);
}

void eui_inspect_transform_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM);

    LD_UNREACHABLE;
}

void eui_inspect_transform_2d_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM_2D);

    eui_component_transform_2d(&storage->as.transform2D, comp);
}

void eui_inspect_camera_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA);

    LD_UNREACHABLE;
}

void eui_inspect_camera_2d_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA_2D);

    eui_component_camera_2d(&storage->as.camera2D, comp);
}

static void eui_inspect_mesh_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_MESH);

    LD_UNREACHABLE;
}

void eui_inspect_sprite_2d_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_SPRITE_2D);

    eui_component_sprite_2d(&storage->as.sprite2D, comp);
}

void eui_inspect_screen_ui_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_SCREEN_UI);

    eui_component_screen_ui(&storage->as.screenUI, comp);
}

void eui_inspect_component(EUIComponentStorage* storage, ComponentView comp)
{
    LD_ASSERT(comp);

    ComponentType type = comp.type();

    if (type != storage->type)
    {
        storage->cleanup();
        storage->startup(type);
    }

    if (sEUIInspectFnTable[(int)type])
        sEUIInspectFnTable[(int)type](storage, comp);
}

} // namespace LD