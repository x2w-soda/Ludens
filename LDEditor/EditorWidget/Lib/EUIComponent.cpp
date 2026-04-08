#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIComponent.h>

#include "EUI.h"

namespace LD {

void eui_component_transform_2d(EUITransform2DStorage* storage, ComponentView view)
{
    Transform2D transform;
    view.get_transform_2d(transform);
    if (eui_transform_2d_edit(storage, &transform))
        view.set_transform_2d(transform);
}

void eui_component_audio_source(EUIAudioSourceStorage* storage, AudioSourceView view)
{
    LD_ASSERT(storage && view);

    EditorTheme theme = eui_get_theme();

    AssetID clipID = view.get_clip_asset();
    if (eui_asset_slot(clipID, ASSET_TYPE_AUDIO_CLIP))
        eui_ctx_request_asset(view.suid(), clipID, ASSET_TYPE_AUDIO_CLIP);

    float vol = view.get_volume_linear();
    if (eui_slider_edit(&storage->volumeLinear, "Volume", &vol))
        view.set_volume_linear(vol);

    float pan = view.get_pan();
    if (eui_slider_edit(&storage->pan, "Pan", &pan))
        view.set_pan(pan);
}

void eui_component_camera_2d(EUICamera2DStorage* storage, Camera2DView view)
{
    LD_ASSERT(storage && view);

    Transform2D transform = Transform2D::identity();
    (void)view.get_transform_2d(transform);

    if (eui_transform_2d_edit(&storage->transform, &transform))
        view.set_transform_2d(transform);

    Rect viewport = view.get_viewport();
    if (eui_rect_edit(&storage->viewport, "Viewport", &viewport, true))
        view.set_viewport(viewport);
}

void eui_component_sprite_2d(EUISprite2DStorage* storage, Sprite2DView view)
{
    LD_ASSERT(storage && view);

    Transform2D transform = Transform2D::identity();
    (void)view.get_transform_2d(transform);

    if (eui_transform_2d_edit(&storage->transform, &transform))
        view.set_transform_2d(transform);

    uint32_t zDepth = view.get_z_depth();
    if (eui_u32_edit(&storage->zDepth, "Z Depth", &zDepth))
        view.set_z_depth(zDepth);

    Vec2 pivot = view.get_pivot();
    if (eui_vec2_edit(&storage->pivot, "Pivot", &pivot))
        view.set_pivot(pivot);

    Rect region = view.get_region();
    if (eui_rect_edit(&storage->region, "Region", &region, false))
        view.set_region(region);

    AssetID assetID = view.get_texture_2d_asset();
    if (eui_asset_slot(assetID, ASSET_TYPE_TEXTURE_2D))
        eui_ctx_request_asset(view.suid(), assetID, ASSET_TYPE_TEXTURE_2D);
}

void eui_component_screen_ui(EUIScreenUIStorage* storage, ScreenUIView view)
{
    AssetID assetID = view.get_ui_template_asset();
    if (eui_asset_slot(assetID, ASSET_TYPE_UI_TEMPLATE))
        eui_ctx_request_asset(view.suid(), assetID, ASSET_TYPE_UI_TEMPLATE);
}

void EUIComponentStorage::startup(ComponentType newType)
{
    if (type == newType)
        return;

    LD_ASSERT(type == COMPONENT_TYPE_ENUM_COUNT);
    type = newType;

    switch (type)
    {
    case COMPONENT_TYPE_AUDIO_SOURCE:
        new (&as.audioSource) EUIAudioSourceStorage();
        break;
    case COMPONENT_TYPE_TRANSFORM_2D:
        new (&as.transform2D) EUITransform2DStorage();
        break;
    case COMPONENT_TYPE_CAMERA_2D:
        new (&as.camera2D) EUICamera2DStorage();
        break;
    case COMPONENT_TYPE_SPRITE_2D:
        new (&as.sprite2D) EUISprite2DStorage();
        break;
    case COMPONENT_TYPE_SCREEN_UI:
        new (&as.screenUI) EUIScreenUIStorage();
        break;
    default:
        LD_UNREACHABLE;
    }
}

void EUIComponentStorage::cleanup()
{
    if (type == COMPONENT_TYPE_ENUM_COUNT)
        return;

    switch (type)
    {
    case COMPONENT_TYPE_AUDIO_SOURCE:
        (&as.audioSource)->~EUIAudioSourceStorage();
        break;
    case COMPONENT_TYPE_TRANSFORM_2D:
        (&as.transform2D)->~EUITransform2DStorage();
        break;
    case COMPONENT_TYPE_CAMERA_2D:
        (&as.camera2D)->~EUICamera2DStorage();
        break;
    case COMPONENT_TYPE_SPRITE_2D:
        (&as.sprite2D)->~EUISprite2DStorage();
        break;
    case COMPONENT_TYPE_SCREEN_UI:
        (&as.screenUI)->~EUIScreenUIStorage();
        break;
    default:
        LD_UNREACHABLE;
    }

    type = COMPONENT_TYPE_ENUM_COUNT;
}

EUIComponentStorage::~EUIComponentStorage()
{
    cleanup();
}

} // namespace LD