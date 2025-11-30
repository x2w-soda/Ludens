#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>

#include "EInspectorWindowObj.h"
#include "InspectComponent.h"

namespace LD {

static void eui_inspect_audio_source_component(EInspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_transform_component(EInspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_camera_component(EInspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_mesh_component(EInspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_sprite_2d_component(EInspectorWindowObj& self, ComponentType type, void* comp);

static void (*sEUIInspectFnTable[COMPONENT_TYPE_ENUM_COUNT])(EInspectorWindowObj& self, ComponentType type, void* comp) = {
    nullptr,
    &eui_inspect_audio_source_component,
    &eui_inspect_transform_component,
    &eui_inspect_camera_component,
    &eui_inspect_mesh_component,
    &eui_inspect_sprite_2d_component,
};

static_assert(sizeof(sEUIInspectFnTable) / sizeof(*sEUIInspectFnTable) == COMPONENT_TYPE_ENUM_COUNT);

void eui_inspect_audio_source_component(EInspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_AUDIO_SOURCE);

    EditorTheme editorTheme = self.editorCtx.get_settings().get_theme();
    AssetManager AM = self.editorCtx.get_asset_manager();

    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;
    AudioClipAsset clipA(AM.get_asset(sourceC->clipAUID).unwrap());
    LD_ASSERT(clipA);

    if (eui_asset_slot(editorTheme, ASSET_TYPE_AUDIO_CLIP, sourceC->clipAUID, clipA.get_name()) && self.selectAssetFn)
    {
        self.isSelectingNewAsset = true;
        self.selectAssetFn(ASSET_TYPE_AUDIO_CLIP, sourceC->clipAUID, self.user);
    }

    // TODO:
}

void eui_inspect_transform_component(EInspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_TRANSFORM);

    EditorTheme editorTheme = self.editorCtx.get_settings().get_theme();

    TransformComponent* transformC = (TransformComponent*)comp;
    eui_transform_edit(editorTheme, &transformC->transform);
}

void eui_inspect_camera_component(EInspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_CAMERA);

    EditorTheme editorTheme = self.editorCtx.get_settings().get_theme();

    CameraComponent* cameraC = (CameraComponent*)comp;

    eui_transform_edit(editorTheme, &cameraC->transform);
    // TODO:
}

static void eui_inspect_mesh_component(EInspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_MESH);

    EditorTheme editorTheme = self.editorCtx.get_settings().get_theme();
    AssetManager AM = self.editorCtx.get_asset_manager();

    MeshComponent* meshC = (MeshComponent*)comp;
    eui_transform_edit(editorTheme, &meshC->transform);

    MeshAsset asset(AM.get_asset(meshC->auid).unwrap());
    LD_ASSERT(asset);
    if (eui_asset_slot(editorTheme, ASSET_TYPE_MESH, meshC->auid, asset.get_name()) && self.selectAssetFn)
    {
        self.isSelectingNewAsset = true;
        self.selectAssetFn(ASSET_TYPE_MESH, meshC->auid, self.user);
    }
}

void eui_inspect_sprite_2d_component(EInspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_SPRITE_2D);
    // TODO:
}

void eui_inspect_component(EInspectorWindowObj& self, ComponentType type, void* comp)
{
    if (sEUIInspectFnTable[(int)type])
        sEUIInspectFnTable[(int)type](self, type, comp);
}

} // namespace LD