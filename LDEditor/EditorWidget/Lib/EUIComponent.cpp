#include <LudensEditor/EditorWidget/EUIAssetSlot.h>
#include <LudensEditor/EditorWidget/EUIComponent.h>
#include <LudensEditor/EditorWidget/EUISegmentControl.h>

#include "EUI.h"

namespace LD {

void eui_component_transform_2d(EUITransform2DProp* storage, ComponentView view)
{
    Transform2D transform;
    view.get_transform_2d(transform);
    if (storage->update(&transform))
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
    if (storage->volumeLinear.update("Volume", &vol))
        view.set_volume_linear(vol);

    float pan = view.get_pan();
    if (storage->pan.update("Pan", &pan))
        view.set_pan(pan);
}

void eui_component_camera_2d(EUICamera2DStorage* storage, Camera2DView view)
{
    LD_ASSERT(storage && view);

    Transform2D transform = Transform2D::identity();
    (void)view.get_transform_2d(transform);

    if (storage->transform.update(&transform))
        view.set_transform_2d(transform);

    Rect viewport = view.get_viewport();
    if (storage->viewport.update("Viewport", &viewport, true))
        view.set_viewport(viewport);

    float zoom = view.get_zoom();
    if (storage->zoom.update("Zoom", &zoom))
        view.set_zoom(zoom);

    bool isFixed = view.get_constraint() == CAMERA_2D_CONSTRAINT_FIXED;
    if (storage->constraint.update("Constraint", &isFixed))
        view.set_constraint(isFixed ? CAMERA_2D_CONSTRAINT_FIXED : CAMERA_2D_CONSTRAINT_FREE);

    Vec2 extent = view.get_extent();
    if (storage->extent.update("Extent", &extent))
        view.set_extent(extent);
}

void eui_component_sprite_2d(EUISprite2DStorage* storage, Sprite2DView view)
{
    LD_ASSERT(storage && view);

    Transform2D transform = Transform2D::identity();
    (void)view.get_transform_2d(transform);

    if (storage->transform.update(&transform))
        view.set_transform_2d(transform);

    uint32_t zDepth = view.get_z_depth();
    if (storage->zDepth.update("Z Depth", &zDepth))
        view.set_z_depth(zDepth);

    Vec2 pivot = view.get_pivot();
    if (storage->pivot.update("Pivot", &pivot))
        view.set_pivot(pivot);

    Rect region = view.get_region();
    if (storage->region.update("Region", &region, false))
        view.set_region(region);

    AssetID assetID = view.get_texture_2d_asset();
    if (eui_asset_slot(assetID, ASSET_TYPE_TEXTURE_2D))
        eui_ctx_request_asset(view.suid(), assetID, ASSET_TYPE_TEXTURE_2D);
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
        new (&as.transform2D) EUITransform2DProp();
        break;
    case COMPONENT_TYPE_CAMERA_2D:
        new (&as.camera2D) EUICamera2DStorage();
        break;
    case COMPONENT_TYPE_SPRITE_2D:
        new (&as.sprite2D) EUISprite2DStorage();
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
        (&as.transform2D)->~EUITransform2DProp();
        break;
    case COMPONENT_TYPE_CAMERA_2D:
        (&as.camera2D)->~EUICamera2DStorage();
        break;
    case COMPONENT_TYPE_SPRITE_2D:
        (&as.sprite2D)->~EUISprite2DStorage();
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