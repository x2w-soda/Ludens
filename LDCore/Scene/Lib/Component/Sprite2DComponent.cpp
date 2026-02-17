#include <Ludens/Profiler/Profiler.h>

#include "Sprite2DComponent.h"

namespace LD {

bool load_sprite_2d_component_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    RUID layerRUID = scene->renderSystemCache.get_or_create_screen_layer(layerSUID);
    if (!layerRUID)
        return false;

    return load_sprite_2d_component_ruid(scene, sprite, layerRUID, texture2D);
}

bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    sprite->draw = scene->renderSystemCache.create_sprite_2d_draw(base->cuid, layerRUID, texture2D);
    if (!sprite->draw)
        return false;

    sprite->assetID = texture2D;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

bool clone_sprite_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::Sprite2D srcSprite((Sprite2DComponent*)srcData);
    Scene::Sprite2D dstSprite((Sprite2DComponent*)dstData);
    LD_ASSERT(srcSprite && dstSprite);

    RUID layerRUID = srcSprite.get_screen_layer_ruid();
    AssetID texture2D = srcSprite.get_texture_2d_asset();

    if (!load_sprite_2d_component_ruid(scene, (Sprite2DComponent*)dstData, layerRUID, texture2D))
        return false;

    dstSprite.set_pivot(srcSprite.get_pivot());
    dstSprite.set_region(srcSprite.get_region());
    dstSprite.set_z_depth(srcSprite.get_z_depth());

    return true;
}

void unload_sprite_2d_component(SceneObj* scene, ComponentBase** data)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;
    ComponentBase* base = sprite->base;

    if (sprite->draw)
    {
        scene->renderSystemCache.destroy_sprite_2d_draw(sprite->draw);
        sprite->draw = {};
    }

    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

Scene::Sprite2D::Sprite2D(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SPRITE_2D)
    {
        mData = comp.data();
        mSprite = (Sprite2DComponent*)mData;
    }
}

Scene::Sprite2D::Sprite2D(Sprite2DComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mSprite = comp;
    }
}

bool Scene::Sprite2D::load(SUID layerSUID, AssetID textureID)
{
    return load_sprite_2d_component_suid(sScene, mSprite, layerSUID, textureID);
}

bool Scene::Sprite2D::set_texture_2d_asset(AssetID textureID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    Image2D image = sScene->renderSystemCache.get_or_create_image_2d(textureID);

    if (mSprite->draw.set_image(image))
    {
        mSprite->assetID = textureID;
        return true;
    }

    return false;
}

AssetID Scene::Sprite2D::get_texture_2d_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->assetID;
}

uint32_t Scene::Sprite2D::get_z_depth()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_z_depth();
}

void Scene::Sprite2D::set_z_depth(uint32_t zDepth)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mSprite->draw.set_z_depth(zDepth);
}

Vec2 Scene::Sprite2D::get_pivot()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_pivot();
}

void Scene::Sprite2D::set_pivot(const Vec2& pivot)
{
    LD_ASSERT_COMPONENT_LOADED(mData);
}

Rect Scene::Sprite2D::get_region()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_region();
}

void Scene::Sprite2D::set_region(const Rect& rect)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mSprite->draw.set_region(rect);
}

RUID Scene::Sprite2D::get_screen_layer_ruid()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_layer_id();
}

SUID Scene::Sprite2D::get_screen_layer_suid()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    RUID layerRUID = mSprite->draw.get_layer_id();
    LD_ASSERT(layerRUID);

    SUID layerSUID = sScene->renderSystemCache.get_screen_layer_suid(layerRUID);
    LD_ASSERT(layerSUID);

    return layerSUID;
}

} // namespace LD