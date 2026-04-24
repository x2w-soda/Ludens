#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/Sprite2DView.h>

#include "Sprite2DComponent.h"

#define INIT_REGION_SIZE 100.0f

namespace LD {

void init_sprite_2d_component(ComponentBase** dstData)
{
    ComponentBase* dstBase = *dstData;
    Sprite2DComponent* dstSprite2D = (Sprite2DComponent*)dstData;
    dstSprite2D->transform = dstBase->transform2D;
    dstSprite2D->assetID = 0;
    dstSprite2D->draw = sScene->renderSystemCache.create_sprite_2d_draw(dstBase->cuid, 0);

    dstSprite2D->draw.set_region(Rect(0.0f, 0.0f, INIT_REGION_SIZE, INIT_REGION_SIZE));
    dstSprite2D->draw.set_pivot(Vec2(INIT_REGION_SIZE / 2.0f));
}

bool load_sprite_2d_component_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D, std::string& err)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    RUID layerRUID = scene->renderSystemCache.get_or_create_screen_layer(layerSUID);
    if (!layerRUID)
    {
        err = "RenderSystem failed to locate screen layer for sprite";
        return false;
    }

    return load_sprite_2d_component_ruid(scene, sprite, layerRUID, texture2D, err);
}

bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID textureID, std::string& err)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sprite->draw); // should have been created in init

    ComponentBase* base = sprite->base;

    if (layerRUID != sprite->draw.get_layer_id())
    {
        sprite->draw = scene->renderSystemCache.migrate_sprite_2d_draw(sprite->draw, layerRUID);
    }

    Image2D image = scene->renderSystemCache.get_or_create_image_2d(textureID);
    sprite->draw.set_image(image);
    sprite->assetID = textureID;

    return true;
}

bool clone_sprite_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    Sprite2DView srcSprite((Sprite2DComponent*)srcData);
    Sprite2DView dstSprite((Sprite2DComponent*)dstData);
    LD_ASSERT(srcSprite && dstSprite);

    RUID layerRUID = srcSprite.get_screen_layer_ruid();
    AssetID texture2D = srcSprite.get_texture_2d_asset();

    if (!load_sprite_2d_component_ruid(scene, (Sprite2DComponent*)dstData, layerRUID, texture2D, err))
        return false;

    dstSprite.set_pivot(srcSprite.get_pivot());
    dstSprite.set_region(srcSprite.get_region());
    dstSprite.set_z_depth(srcSprite.get_z_depth());

    return true;
}

bool unload_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;
    ComponentBase* base = sprite->base;

    if (sprite->draw)
    {
        scene->renderSystemCache.destroy_sprite_2d_draw(sprite->draw);
        sprite->draw = {};
    }

    return true;
}

bool startup_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;
    ComponentBase* base = *data;

    if (!sprite->draw)
    {
        err = "Sprite2DDraw missing";
        return false;
    }

    return true;
}

bool cleanup_sprite_2d_component(SceneObj* scene, ComponentBase** data, std::string& err)
{
    return true;
}

AssetID sprite_2d_component_get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;

    if (assetSlotIndex != 0)
        return AssetID(0);

    return sprite->assetID;
}

bool sprite_2d_component_set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID assetID)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;
    Sprite2DView spriteV(sprite);

    if (assetSlotIndex != 0)
        return false;

    spriteV.set_texture_2d_asset(assetID);
    return true;
}

Sprite2DView::Sprite2DView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SPRITE_2D)
    {
        mData = comp.data();
        mSprite = (Sprite2DComponent*)mData;
    }
}

Sprite2DView::Sprite2DView(Sprite2DComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mSprite = comp;
    }
}

bool Sprite2DView::load(SUID layerSUID, AssetID textureID)
{
    std::string err;

    return load_sprite_2d_component_suid(sScene, mSprite, layerSUID, textureID, err);
}

void Sprite2DView::set_texture_2d_asset(AssetID textureID)
{
    Image2D image = sScene->renderSystemCache.get_or_create_image_2d(textureID);

    mSprite->draw.set_image(image);
    mSprite->assetID = textureID;
}

AssetID Sprite2DView::get_texture_2d_asset()
{
    return mSprite->assetID;
}

uint32_t Sprite2DView::get_z_depth()
{
    LD_ASSERT(mSprite->draw);

    return mSprite->draw.get_z_depth();
}

void Sprite2DView::set_z_depth(uint32_t zDepth)
{
    LD_ASSERT(mSprite->draw);

    mSprite->draw.set_z_depth(zDepth);
}

Vec2 Sprite2DView::get_pivot()
{
    LD_ASSERT(mSprite->draw);

    return mSprite->draw.get_pivot();
}

void Sprite2DView::set_pivot(Vec2 pivot)
{
    LD_ASSERT(mSprite->draw);

    mSprite->draw.set_pivot(pivot);
}

Rect Sprite2DView::get_region()
{
    LD_ASSERT(mSprite->draw);

    return mSprite->draw.get_region();
}

void Sprite2DView::set_region(Rect rect)
{
    LD_ASSERT(mSprite->draw);

    mSprite->draw.set_region(rect);
}

RUID Sprite2DView::get_screen_layer_ruid()
{
    LD_ASSERT(mSprite->draw);

    return mSprite->draw.get_layer_id();
}

SUID Sprite2DView::get_screen_layer_suid()
{
    LD_ASSERT(mSprite->draw);

    RUID layerRUID = mSprite->draw.get_layer_id();
    LD_ASSERT(layerRUID);

    SUID layerSUID = sScene->renderSystemCache.get_screen_layer_suid(layerRUID);
    LD_ASSERT(layerSUID);

    return layerSUID;
}

Rect Sprite2DView::local_rect()
{
    Rect region = mSprite->draw.get_region();
    Vec2 pivot = mSprite->draw.get_pivot();
    return Rect(-pivot.x, -pivot.y, region.w, region.h);
}

} // namespace LD