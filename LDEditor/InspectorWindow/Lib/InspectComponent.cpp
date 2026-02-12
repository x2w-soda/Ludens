#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

static void eui_inspect_audio_source_component(InspectorWindowObj& self, Scene::Component comp);
static void eui_inspect_transform_component(InspectorWindowObj& self, Scene::Component comp);
static void eui_inspect_camera_component(InspectorWindowObj& self, Scene::Component comp);
static void eui_inspect_mesh_component(InspectorWindowObj& self, Scene::Component comp);
static void eui_inspect_sprite_2d_component(InspectorWindowObj& self, Scene::Component comp);

static void (*sEUIInspectFnTable[COMPONENT_TYPE_ENUM_COUNT])(InspectorWindowObj& self, Scene::Component comp) = {
    nullptr,
    &eui_inspect_audio_source_component,
    &eui_inspect_transform_component,
    &eui_inspect_camera_component,
    &eui_inspect_mesh_component,
    &eui_inspect_sprite_2d_component,
};

static_assert(sizeof(sEUIInspectFnTable) / sizeof(*sEUIInspectFnTable) == COMPONENT_TYPE_ENUM_COUNT);

void eui_inspect_audio_source_component(InspectorWindowObj& self, Scene::Component comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE);

    EditorTheme theme = self.ctx.get_settings().get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    const float rowHeight = theme.get_text_row_height();
    const float propNameWidth = theme.get_text_label_width();
    Scene::AudioSource source(comp);
    LD_ASSERT(source);
    AssetID clipID = source.get_clip_asset();

    AudioClipAsset clipA(AM.get_asset(clipID).unwrap());
    LD_ASSERT(clipA);

    if (eui_asset_slot(theme, ASSET_TYPE_AUDIO_CLIP, clipID, clipA.get_name()))
        self.request_new_asset(ASSET_TYPE_AUDIO_CLIP, clipID);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_padding();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    {
        ui_top_layout(layoutI);
        ui_push_text("Volume");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        float volume = source.get_volume_linear();
        ui_push_slider(0.0f, 1.0f, &volume);
        source.set_volume_linear(volume);
        ui_pop();
    }
    ui_pop();
    ui_push_panel();
    {
        ui_top_layout(layoutI);
        ui_push_text("Pan");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        float pan = source.get_pan();
        ui_push_slider(0.0f, 1.0f, &pan);
        source.set_pan(pan);
        ui_pop();
    }
    ui_pop();
}

void eui_inspect_transform_component(InspectorWindowObj& self, Scene::Component comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    TransformEx transform;
    comp.get_transform(transform);
    eui_transform_edit(editorTheme, &transform);
    // comp.set_transform(transform);
}

void eui_inspect_camera_component(InspectorWindowObj& self, Scene::Component comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    TransformEx transform;
    bool ok = comp.get_transform(transform);
    LD_ASSERT(ok);
    eui_transform_edit(editorTheme, &transform);
    //comp.set_transform(transform);

    // TODO:
}

static void eui_inspect_mesh_component(InspectorWindowObj& self, Scene::Component comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_MESH);

    EditorTheme editorTheme = self.ctx.get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    Scene::Mesh mesh(comp);
    LD_ASSERT(mesh);

    TransformEx transform{};
    bool ok = mesh.get_transform(transform);
    LD_ASSERT(ok);
    eui_transform_edit(editorTheme, &transform);
    // mesh.set_transform(transform);

    AssetID assetID = mesh.get_mesh_asset();
    MeshAsset asset = (MeshAsset)AM.get_asset(assetID, ASSET_TYPE_MESH);
    LD_ASSERT(asset);

    if (eui_asset_slot(editorTheme, ASSET_TYPE_MESH, assetID, asset.get_name()))
        self.request_new_asset(ASSET_TYPE_MESH, assetID);
}

void eui_inspect_sprite_2d_component(InspectorWindowObj& self, Scene::Component comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_SPRITE_2D);

    EditorTheme editorTheme = self.ctx.get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    Scene::Sprite2D sprite(comp);
    LD_ASSERT(sprite);

    Transform2D transform{};
    bool ok = sprite.get_transform_2d(transform);
    LD_ASSERT(ok);
    eui_transform_2d_edit(editorTheme, &transform);
    // sprite.set_transform_2d(transform);

    AssetID assetID = sprite.get_texture_2d_asset();
    Texture2DAsset asset = (Texture2DAsset)AM.get_asset(assetID, ASSET_TYPE_TEXTURE_2D);
    LD_ASSERT(asset);

    if (eui_asset_slot(editorTheme, ASSET_TYPE_TEXTURE_2D, assetID, asset.get_name()))
        self.request_new_asset(ASSET_TYPE_TEXTURE_2D, assetID);
}

void eui_inspect_component(InspectorWindowObj& self, Scene::Component comp)
{
    LD_ASSERT(comp);

    ComponentType type = comp.type();

    if (sEUIInspectFnTable[(int)type])
        sEUIInspectFnTable[(int)type](self, comp);
}

} // namespace LD