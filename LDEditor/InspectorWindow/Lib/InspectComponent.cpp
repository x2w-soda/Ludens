#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Scene/ComponentViews.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>
#include <LudensEditor/EditorWidget/UIVectorEditWidget.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

static void eui_inspect_audio_source_component(InspectorWindowObj& self, ComponentView comp);
static void eui_inspect_transform_component(InspectorWindowObj& self, ComponentView comp);
static void eui_inspect_transform_2d_component(InspectorWindowObj& self, ComponentView comp);
static void eui_inspect_camera_component(InspectorWindowObj& self, ComponentView comp);
static void eui_inspect_camera_2d_component(InspectorWindowObj& self, ComponentView comp);
static void eui_inspect_mesh_component(InspectorWindowObj& self, ComponentView comp);
static void eui_inspect_sprite_2d_component(InspectorWindowObj& self, ComponentView comp);

static void (*sEUIInspectFnTable[])(InspectorWindowObj& self, ComponentView comp) = {
    nullptr,
    &eui_inspect_audio_source_component,
    &eui_inspect_transform_component,
    &eui_inspect_transform_2d_component,
    &eui_inspect_camera_component,
    &eui_inspect_camera_2d_component,
    &eui_inspect_mesh_component,
    &eui_inspect_sprite_2d_component,
    nullptr,
};

static_assert(sizeof(sEUIInspectFnTable) / sizeof(*sEUIInspectFnTable) == COMPONENT_TYPE_ENUM_COUNT);

void eui_inspect_audio_source_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE);

    EditorTheme theme = self.ctx.get_settings().get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    const float rowHeight = theme.get_text_row_height();
    const float propNameWidth = theme.get_text_label_width();
    AudioSourceView source(comp);
    LD_ASSERT(source);
    AssetID clipID = source.get_clip_asset();
    UISliderStorage* sliderS;

    AudioClipAsset clipA(AM.get_asset(clipID).unwrap());
    const char* name = clipA ? clipA.get_name() : nullptr;

    if (eui_asset_slot(theme, ASSET_TYPE_AUDIO_CLIP, clipID, name))
        self.request_new_asset(ASSET_TYPE_AUDIO_CLIP, clipID);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_padding();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel(nullptr);
    {
        ui_top_layout(layoutI);
        ui_push_text(nullptr, "Volume");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        float volume = source.get_volume_linear();
        sliderS = ui_push_slider(nullptr, &volume);
        sliderS->min = 0.0f;
        sliderS->max = 0.0f;

        source.set_volume_linear(volume);
        ui_pop();
    }
    ui_pop();
    ui_push_panel(nullptr);
    {
        ui_top_layout(layoutI);
        ui_push_text(nullptr, "Pan");
        ui_top_layout_size(UISize::fixed(propNameWidth), UISize::fixed(rowHeight));
        ui_pop();
        float pan = source.get_pan();
        sliderS = ui_push_slider(nullptr, &pan);
        sliderS->min = 0.0f;
        sliderS->max = 0.0f;

        source.set_pan(pan);
        ui_pop();
    }
    ui_pop();
}

void eui_inspect_transform_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    TransformEx transform;
    comp.get_transform(transform);
    eui_transform_edit(editorTheme, &transform);
    // comp.set_transform(transform);
}

void eui_inspect_transform_2d_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_TRANSFORM_2D);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    Transform2D transform;
    comp.get_transform_2d(transform);
    eui_transform_2d_edit(editorTheme, &transform);
}

void eui_inspect_camera_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    TransformEx transform;
    bool ok = comp.get_transform(transform);
    LD_ASSERT(ok);
    eui_transform_edit(editorTheme, &transform);
    // comp.set_transform(transform);

    // TODO:
}

void eui_inspect_camera_2d_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_CAMERA_2D);

    EditorTheme editorTheme = self.ctx.get_settings().get_theme();

    Transform2D transform;
    comp.get_transform_2d(transform);
    eui_transform_2d_edit(editorTheme, &transform);
}

static void eui_inspect_mesh_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_MESH);

    EditorTheme editorTheme = self.ctx.get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    MeshView mesh(comp);
    LD_ASSERT(mesh);

    TransformEx transform{};
    bool ok = mesh.get_transform(transform);
    LD_ASSERT(ok);
    eui_transform_edit(editorTheme, &transform);
    // mesh.set_transform(transform);

    AssetID assetID = mesh.get_mesh_asset();
    MeshAsset asset = (MeshAsset)AM.get_asset(assetID, ASSET_TYPE_MESH);
    const char* name = asset ? asset.get_name() : nullptr;

    if (eui_asset_slot(editorTheme, ASSET_TYPE_MESH, assetID, name))
        self.request_new_asset(ASSET_TYPE_MESH, assetID);
}

void eui_inspect_sprite_2d_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp && comp.type() == COMPONENT_TYPE_SPRITE_2D);

    EditorTheme edTheme = self.ctx.get_theme();
    AssetManager AM = self.ctx.get_asset_manager();
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    Transform2D transform{};
    bool ok = sprite.get_transform_2d(transform);
    LD_ASSERT(ok);
    eui_transform_2d_edit(edTheme, &transform);
    sprite.set_transform_2d(transform);

    uint32_t zDepth = sprite.get_z_depth();
    eui_u32_edit(edTheme, "Z Depth", &zDepth);
    sprite.set_z_depth(zDepth);

    Vec2 pivot = sprite.get_pivot();
    eui_vec2_edit(edTheme, "Pivot", &pivot);
    sprite.set_pivot(pivot);

    AssetID assetID = sprite.get_texture_2d_asset();
    Texture2DAsset asset = (Texture2DAsset)AM.get_asset(assetID, ASSET_TYPE_TEXTURE_2D);
    const char* name = asset ? asset.get_name() : nullptr;

    if (eui_asset_slot(edTheme, ASSET_TYPE_TEXTURE_2D, assetID, name))
        self.request_new_asset(ASSET_TYPE_TEXTURE_2D, assetID);
}

void eui_inspect_component(InspectorWindowObj& self, ComponentView comp)
{
    LD_ASSERT(comp);

    ComponentType type = comp.type();

    if (sEUIInspectFnTable[(int)type])
        sEUIInspectFnTable[(int)type](self, comp);
}

} // namespace LD