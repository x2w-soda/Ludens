#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

static void eui_inspect_audio_source_component(InspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_transform_component(InspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_camera_component(InspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_mesh_component(InspectorWindowObj& self, ComponentType type, void* comp);
static void eui_inspect_sprite_2d_component(InspectorWindowObj& self, ComponentType type, void* comp);

static void (*sEUIInspectFnTable[COMPONENT_TYPE_ENUM_COUNT])(InspectorWindowObj& self, ComponentType type, void* comp) = {
    nullptr,
    &eui_inspect_audio_source_component,
    &eui_inspect_transform_component,
    &eui_inspect_camera_component,
    &eui_inspect_mesh_component,
    &eui_inspect_sprite_2d_component,
};

static_assert(sizeof(sEUIInspectFnTable) / sizeof(*sEUIInspectFnTable) == COMPONENT_TYPE_ENUM_COUNT);

void eui_inspect_audio_source_component(InspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_AUDIO_SOURCE);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    const float rowHeight = editorTheme.get_text_row_height();
    const float propNameWidth = editorTheme.get_text_label_width();

    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;
    AudioClipAsset clipA(AM.get_asset(sourceC->clipAUID).unwrap());
    LD_ASSERT(clipA);

    if (eui_asset_slot(editorTheme, ASSET_TYPE_AUDIO_CLIP, sourceC->clipAUID, clipA.get_name()))
        self.request_new_asset(ASSET_TYPE_AUDIO_CLIP, sourceC->clipAUID);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 6.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    {
        ui_top_layout(layoutI);
        ui_push_text("Volume");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        ui_push_slider(0.0f, 1.0f, &sourceC->volumeLinear);
        ui_pop();
    }
    ui_pop();
    ui_push_panel();
    {
        ui_top_layout(layoutI);
        ui_push_text("Pan");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        ui_push_slider(0.0f, 1.0f, &sourceC->pan);
        ui_pop();
    }
    ui_pop();

    // sync with audio server
    if (sourceC->playback)
    {
        AudioPlayback::Accessor accessor = sourceC->playback.access();

        accessor.set_pan(sourceC->pan);
        accessor.set_volume_linear(sourceC->volumeLinear);
    }
}

void eui_inspect_transform_component(InspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_TRANSFORM);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    TransformComponent* transformC = (TransformComponent*)comp;
    eui_transform_edit(editorTheme, &transformC->transform);
}

void eui_inspect_camera_component(InspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_CAMERA);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    CameraComponent* cameraC = (CameraComponent*)comp;

    eui_transform_edit(editorTheme, &cameraC->transform);
    // TODO:
}

static void eui_inspect_mesh_component(InspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_MESH);

    EditorTheme editorTheme = self.ctx.get_theme();
    AssetManager AM = self.ctx.get_asset_manager();

    MeshComponent* meshC = (MeshComponent*)comp;
    eui_transform_edit(editorTheme, &meshC->transform);

    MeshAsset asset = (MeshAsset)AM.get_asset(meshC->auid, ASSET_TYPE_MESH);
    LD_ASSERT(asset);

    if (eui_asset_slot(editorTheme, ASSET_TYPE_MESH, meshC->auid, asset.get_name()))
        self.request_new_asset(ASSET_TYPE_MESH, meshC->auid);
}

void eui_inspect_sprite_2d_component(InspectorWindowObj& self, ComponentType type, void* comp)
{
    LD_ASSERT(type == COMPONENT_TYPE_SPRITE_2D);

    EditorTheme editorTheme = self.ctx.get_theme();
    AssetManager AM = self.ctx.get_asset_manager();

    auto* spriteC = (Sprite2DComponent*)comp;
    eui_transform_2d_edit(editorTheme, &spriteC->transform);

    Texture2DAsset asset = (Texture2DAsset)AM.get_asset(spriteC->auid, ASSET_TYPE_TEXTURE_2D);
    LD_ASSERT(asset);

    if (eui_asset_slot(editorTheme, ASSET_TYPE_TEXTURE_2D, spriteC->auid, asset.get_name()))
        self.request_new_asset(ASSET_TYPE_TEXTURE_2D, spriteC->auid);
}

void eui_inspect_component(InspectorWindowObj& self, ComponentType type, void* comp)
{
    if (sEUIInspectFnTable[(int)type])
        sEUIInspectFnTable[(int)type](self, type, comp);
}

} // namespace LD