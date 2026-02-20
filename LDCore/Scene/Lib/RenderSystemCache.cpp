#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>

#include "RenderSystemCache.h"
#include "SceneObj.h"

namespace LD {

void RenderSystemCache::create(RenderSystem system, AssetManager assetManager)
{
    LD_PROFILE_SCOPE;

    mSystem = system;
    mAssetManager = assetManager;
    mDrawToCuid.clear();
    mCuidToDraw.clear();
    mMeshData.clear();
    mImage2D.clear();
    mSuidToScreenLayer.clear();
    mScreenLayerToSuid.clear();
}

void RenderSystemCache::destroy()
{
    LD_PROFILE_SCOPE;

    if (!mSystem)
        return;

    for (const auto& it : mImage2D)
        mSystem.destroy_image_2d(it.second);
    mImage2D.clear();

    for (const auto& it : mMeshData)
        mSystem.destroy_mesh_data(it.second);
    mMeshData.clear();

    for (const auto& it : mSuidToScreenLayer)
        mSystem.destroy_screen_layer(it.second);
    mSuidToScreenLayer.clear();
    mScreenLayerToSuid.clear();

    mAssetManager = {};
    mSystem = {};
}

RUID RenderSystemCache::get_component_draw_id(CUID compID)
{
    auto ite = mCuidToDraw.find(compID);

    if (ite == mCuidToDraw.end())
        return 0;

    return ite->second;
}

CUID RenderSystemCache::get_draw_id_component(RUID drawID)
{
    auto ite = mDrawToCuid.find(drawID);

    if (ite == mDrawToCuid.end())
        return 0;

    return ite->second;
}

CUID RenderSystemCache::get_2d_component_by_position(const Vec2& worldPos, RenderSystemMat4Callback mat4CB, void* user)
{
    LD_PROFILE_SCOPE;

    RUID ruid = mSystem.get_screen_layer_item(worldPos, mat4CB, user);
    if (!ruid)
        return 0;

    LD_ASSERT(mDrawToCuid.contains(ruid));

    return mDrawToCuid[ruid];
}

RUID RenderSystemCache::get_or_create_screen_layer(SUID layerSUID)
{
    if (mSuidToScreenLayer.contains(layerSUID))
        return mSuidToScreenLayer[layerSUID];

    RUID layerRUID = mSystem.create_screen_layer("layer"); // TODO:
    if (!layerRUID)
        return 0;

    mScreenLayerToSuid[layerRUID] = layerSUID;
    mSuidToScreenLayer[layerSUID] = layerRUID;

    return layerRUID;
}

SUID RenderSystemCache::get_screen_layer_suid(RUID layerRUID)
{
    if (!mScreenLayerToSuid.contains(layerRUID))
        return (SUID)0;

    return mScreenLayerToSuid[layerRUID];
}

MeshData RenderSystemCache::get_or_create_mesh_data(AssetID meshAUID)
{
    if (mMeshData.contains(meshAUID))
        return mMeshData[meshAUID];

    MeshAsset meshA = (MeshAsset)mAssetManager.get_asset(meshAUID, ASSET_TYPE_MESH);
    if (!meshA)
        return {};

    return mMeshData[meshAUID] = mSystem.create_mesh_data(*meshA.data());
}

MeshDraw RenderSystemCache::create_mesh_draw(CUID compID, AssetID meshAUID)
{
    MeshDraw draw{};

    if (meshAUID)
    {
        MeshData data = get_or_create_mesh_data(meshAUID);
        draw = mSystem.create_mesh_draw(data);
    }
    else
        draw = mSystem.create_mesh_draw();

    if (!draw)
        return {};

    link_id(compID, draw.get_id());
    return draw;
}

void RenderSystemCache::destroy_mesh_draw(MeshDraw draw)
{
    if (!draw || !mDrawToCuid.contains(draw.get_id()))
        return;

    CUID meshCUID = mDrawToCuid[draw.get_id()];
    mDrawToCuid.erase(draw.get_id());
    mCuidToDraw.erase(meshCUID);

    mSystem.destroy_mesh_draw(draw);
}

Image2D RenderSystemCache::get_or_create_image_2d(AssetID textureID)
{
    Texture2DAsset textureA = (Texture2DAsset)mAssetManager.get_asset(textureID, ASSET_TYPE_TEXTURE_2D);
    LD_ASSERT(textureA);

    if (!mImage2D.contains(textureID))
        mImage2D[textureID] = mSystem.create_image_2d(textureA.get_bitmap());

    LD_ASSERT(mImage2D[textureID]);
    return mImage2D[textureID];
}

Sprite2DDraw RenderSystemCache::create_sprite_2d_draw(CUID compID, RUID layerID, AssetID textureID)
{
    Sprite2DDraw draw{};

    if (textureID)
    {
        Image2D image2D = get_or_create_image_2d(textureID);
        draw = mSystem.create_sprite_2d_draw(image2D, layerID);
    }
    else
        draw = mSystem.create_sprite_2d_draw({}, layerID);

    if (!draw)
        return {};

    link_id(compID, draw.get_id());
    return draw;
}

void RenderSystemCache::destroy_sprite_2d_draw(Sprite2DDraw draw)
{
    if (!draw || !mDrawToCuid.contains(draw.get_id()))
        return;

    CUID compCUID = mDrawToCuid[draw.get_id()];
    mDrawToCuid.erase(draw.get_id());
    mCuidToDraw.erase(compCUID);

    mSystem.destroy_sprite_2d_draw(draw);
}

void RenderSystemCache::link_id(CUID compID, RUID drawID)
{
    // invalidate old drawID associated with component
    auto it = mCuidToDraw.find(compID);
    if (it != mCuidToDraw.end())
    {
        RUID oldDrawID = it->second;
        mDrawToCuid.erase(oldDrawID);
    }

    mDrawToCuid[drawID] = compID;
    mCuidToDraw[compID] = drawID;
}

} // namespace LD