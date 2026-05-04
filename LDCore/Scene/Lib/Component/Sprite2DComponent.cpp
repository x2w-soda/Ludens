#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/Sprite2DView.h>
#include <Ludens/Serial/Property.h>

#include "Sprite2DComponent.h"

#define INIT_REGION_SIZE 100.0f

namespace LD {

// clang-format off
static PropertyMeta sSprite2DPropMeta[] = {
    {"transform", nullptr, VALUE_TYPE_TRANSFORM_2D, Value64(Transform2D::identity())},
    {"texture",   nullptr, VALUE_TYPE_U32,          Value64((uint32_t)32), PROPERTY_UI_HINT_ASSET },
    {"z_depth",   nullptr, VALUE_TYPE_U32,          Value64((uint32_t)0)},
    {"pivot",     nullptr, VALUE_TYPE_VEC2,         Value64(Vec2(INIT_REGION_SIZE / 2.0f))},
    {"region",    nullptr, VALUE_TYPE_RECT,         Value64(Rect(0.0f, 0.0f, INIT_REGION_SIZE, INIT_REGION_SIZE))},
};
// clang-format on

bool sprite_2d_prop_getter(void* data, uint32_t propIndex, uint32_t, Value64& val)
{
    Sprite2DView view((Sprite2DComponent*)data);
    Transform2D transform2D;

    switch (propIndex)
    {
    case SPRITE_2D_PROP_TRANSFORM:
        (void)view.get_transform_2d(transform2D);
        val.set_transform_2d(transform2D);
        break;
    case SPRITE_2D_PROP_TEXTURE_2D_ASSET:
        val.set_u32((uint32_t)view.get_texture_2d_asset());
        break;
    case SPRITE_2D_PROP_Z_DEPTH:
        val.set_u32(view.get_z_depth());
        break;
    case SPRITE_2D_PROP_PIVOT:
        val.set_vec2(view.get_pivot());
        break;
    case SPRITE_2D_PROP_REGION:
        val.set_rect(view.get_region());
        break;
    case SPRITE_2D_PROP_SCREEN_LAYER:
        // TODO:
        break;
    default:
        return false;
    }

    return true;
}

bool sprite_2d_prop_setter(void* data, uint32_t propIndex, uint32_t, const Value64& val)
{
    Sprite2DView view((Sprite2DComponent*)data);

    switch (propIndex)
    {
    case SPRITE_2D_PROP_TRANSFORM:
        view.set_transform_2d(val.get_transform_2d());
        break;
    case SPRITE_2D_PROP_TEXTURE_2D_ASSET:
        view.set_texture_2d_asset((AssetID)val.get_u32());
        break;
    case SPRITE_2D_PROP_Z_DEPTH:
        view.set_z_depth(val.get_u32());
        break;
    case SPRITE_2D_PROP_PIVOT:
        view.set_pivot(val.get_vec2());
        break;
    case SPRITE_2D_PROP_REGION:
        view.set_region(val.get_rect());
        break;
    case SPRITE_2D_PROP_SCREEN_LAYER:
        // TODO:
        break;
    default:
        return false;
    }

    return true;
}

TypeMeta Sprite2DMeta::sTypeMeta = {
    .name = "Sprite2DComponent",
    .props = sSprite2DPropMeta,
    .propCount = sizeof(sSprite2DPropMeta) / sizeof(*sSprite2DPropMeta),
    .getLocal = &sprite_2d_prop_getter,
    .setLocal = &sprite_2d_prop_setter,
};

void Sprite2DMeta::init(ComponentBase** dstData)
{
    ComponentBase* dstBase = *dstData;
    Sprite2DComponent* dstSprite2D = (Sprite2DComponent*)dstData;
    dstSprite2D->transform = dstBase->transform2D;
    dstSprite2D->assetID = 0;
    dstSprite2D->draw = sScene->renderSystemCache.create_sprite_2d_draw(dstBase->cuid, 0);

    dstSprite2D->draw.set_region(Rect(0.0f, 0.0f, INIT_REGION_SIZE, INIT_REGION_SIZE));
    dstSprite2D->draw.set_pivot(Vec2(INIT_REGION_SIZE / 2.0f));
}

bool Sprite2DMeta::load_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D, std::string& err)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    RUID layerRUID = scene->renderSystemCache.get_or_create_screen_layer(layerSUID);
    if (!layerRUID)
    {
        err = "RenderSystem failed to locate screen layer for sprite";
        return false;
    }

    return load_ruid(scene, sprite, layerRUID, texture2D, err);
}

bool Sprite2DMeta::load_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID textureID, std::string& err)
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

bool Sprite2DMeta::clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    Sprite2DView srcSprite((Sprite2DComponent*)srcData);
    Sprite2DView dstSprite((Sprite2DComponent*)dstData);
    LD_ASSERT(srcSprite && dstSprite);

    RUID layerRUID = srcSprite.get_screen_layer_ruid();
    AssetID texture2D = srcSprite.get_texture_2d_asset();

    if (!load_ruid(scene, (Sprite2DComponent*)dstData, layerRUID, texture2D, err))
        return false;

    dstSprite.set_pivot(srcSprite.get_pivot());
    dstSprite.set_region(srcSprite.get_region());
    dstSprite.set_z_depth(srcSprite.get_z_depth());

    return true;
}

bool Sprite2DMeta::unload(SceneObj* scene, ComponentBase** data, std::string& err)
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

bool Sprite2DMeta::startup(SceneObj* scene, ComponentBase** data, std::string& err)
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

bool Sprite2DMeta::cleanup(SceneObj* scene, ComponentBase** data, std::string& err)
{
    return true;
}

AssetID Sprite2DMeta::get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;

    if (assetSlotIndex != 0)
        return AssetID(0);

    return sprite->assetID;
}

bool Sprite2DMeta::set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID assetID)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;
    Sprite2DView spriteV(sprite);

    if (assetSlotIndex != 0)
        return false;

    spriteV.set_texture_2d_asset(assetID);
    return true;
}

AssetType Sprite2DMeta::get_asset_type(SceneObj* scene, uint32_t assetSlotIndex)
{
    if (assetSlotIndex != 0)
        return ASSET_TYPE_ENUM_COUNT;

    return ASSET_TYPE_TEXTURE_2D;
}

bool Sprite2DMeta::load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, std::string& err)
{
    Transform2D transform2D = Transform2D::identity();
    SUID layerSUID = {};
    AssetID textureID = {};
    uint32_t zDepth = sSprite2DPropMeta[SPRITE_2D_PROP_Z_DEPTH].valueDefault.get_u32();
    Vec2 pivot = sSprite2DPropMeta[SPRITE_2D_PROP_PIVOT].valueDefault.get_vec2();
    Rect region = sSprite2DPropMeta[SPRITE_2D_PROP_REGION].valueDefault.get_rect();

    for (const PropertyValue& prop : props)
    {
        switch (prop.propIndex)
        {
        case SPRITE_2D_PROP_TRANSFORM:
            transform2D = prop.value.get_transform_2d();
            break;
        case SPRITE_2D_PROP_TEXTURE_2D_ASSET:
            textureID = (AssetID)prop.value.get_u32();
            break;
        case SPRITE_2D_PROP_Z_DEPTH:
            zDepth = prop.value.get_u32();
            break;
        case SPRITE_2D_PROP_PIVOT:
            pivot = prop.value.get_vec2();
            break;
        case SPRITE_2D_PROP_REGION:
            region = prop.value.get_rect();
            break;
        case SPRITE_2D_PROP_SCREEN_LAYER:
            break; // TODO:
        default:
            break;
        }
    }

    auto* spriteC = (Sprite2DComponent*)data;

    if (!load_suid(sScene, spriteC, layerSUID, textureID, err))
        return false;

    Sprite2DView spriteV(spriteC);
    spriteV.set_transform_2d(transform2D);
    spriteV.set_region(region);
    spriteV.set_pivot(pivot);
    spriteV.set_z_depth(zDepth);

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

    return Sprite2DMeta::load_suid(sScene, mSprite, layerSUID, textureID, err);
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