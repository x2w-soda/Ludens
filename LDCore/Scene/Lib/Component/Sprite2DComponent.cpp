#include <Ludens/Profiler/Profiler.h>

#include "Sprite2DComponent.h"

namespace LD {

static const Sprite2DComponent sDefaultSprite2D = {
    .transform = {},
    .draw = {},
    .assetID = 0,
};

void init_sprite_2d_component(ComponentBase** dstData)
{
    memcpy(dstData, &sDefaultSprite2D, sizeof(Sprite2DComponent));
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

bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D, std::string& err)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    sprite->draw = scene->renderSystemCache.create_sprite_2d_draw(base->cuid, layerRUID, texture2D);
    if (!sprite->draw)
    {
        err = "RenderSystem failed to create Sprite2DDraw";
        return false;
    }

    sprite->assetID = texture2D;

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
    
    ((Sprite2DComponent*)dstData)->transform = ((Sprite2DComponent*)srcData)->transform;

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

bool Sprite2DView::set_texture_2d_asset(AssetID textureID)
{
    Image2D image = sScene->renderSystemCache.get_or_create_image_2d(textureID);

    if (mSprite->draw.set_image(image))
    {
        mSprite->assetID = textureID;
        return true;
    }

    return false;
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

void Sprite2DView::set_pivot(const Vec2& pivot)
{
    LD_ASSERT(mSprite->draw);

    mSprite->draw.set_pivot(pivot);
}

Rect Sprite2DView::get_region()
{
    LD_ASSERT(mSprite->draw);

    return mSprite->draw.get_region();
}

void Sprite2DView::set_region(const Rect& rect)
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

} // namespace LD