#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>

#include "RenderServerCache.h"
#include "SceneObj.h"

namespace LD {

void RenderServerCache::startup(RenderServer server, AssetManager assetManager)
{
    mServer = server;
    mAssetManager = assetManager;
    mDrawToCuid.clear();
    mCuidToDraw.clear();
    mMeshData.clear();
    mImage2D.clear();
}

void RenderServerCache::cleanup()
{
    if (!mServer)
        return;

    for (const auto& it : mImage2D)
        mServer.destroy_image_2d(it.second);
    mImage2D.clear();

    for (const auto& it : mMeshData)
        mServer.destroy_mesh_data(it.second);
    mMeshData.clear();

    mAssetManager = {};
    mServer = {};
}

RUID RenderServerCache::get_component_draw_id(CUID compID)
{
    auto ite = mCuidToDraw.find(compID);

    if (ite == mCuidToDraw.end())
        return 0;

    return ite->second;
}

CUID RenderServerCache::get_draw_id_component(RUID drawID)
{
    auto ite = mDrawToCuid.find(drawID);

    if (ite == mDrawToCuid.end())
        return 0;

    return ite->second;
}

MeshData RenderServerCache::get_or_create_mesh_data(AUID meshAUID)
{
    MeshAsset meshA = (MeshAsset)mAssetManager.get_asset(meshAUID, ASSET_TYPE_MESH);
    LD_ASSERT(meshA);

    if (!mMeshData.contains(meshAUID))
        mMeshData[meshAUID] = mServer.create_mesh_data(*meshA.data());

    LD_ASSERT(mMeshData[meshAUID]);
    return mMeshData[meshAUID];
}

MeshDraw RenderServerCache::create_mesh_draw(CUID compID, AUID meshAUID)
{
    auto it = mCuidToDraw.find(compID);
    if (it != mCuidToDraw.end())
    {
        RUID oldDrawID = it->second;
        mDrawToCuid.erase(oldDrawID);
    }

    MeshDraw draw;
    if (meshAUID)
    {
        MeshData data = get_or_create_mesh_data(meshAUID);
        draw = mServer.create_mesh_draw(data);
    }
    else
        draw = mServer.create_mesh_draw();
    LD_ASSERT(draw);

    RUID drawID = draw.get_id();
    mDrawToCuid[drawID] = compID;
    mCuidToDraw[compID] = drawID;

    return draw;
}

Image2D RenderServerCache::get_or_create_image_2d(AUID textureAUID)
{
    Texture2DAsset textureA = (Texture2DAsset)mAssetManager.get_asset(textureAUID, ASSET_TYPE_TEXTURE_2D);
    LD_ASSERT(textureA);

    if (!mImage2D.contains(textureAUID))
        mImage2D[textureAUID] = mServer.create_image_2d(textureA.get_bitmap());

    LD_ASSERT(mImage2D[textureAUID]);
    return mImage2D[textureAUID];
}

Sprite2DDraw RenderServerCache::create_sprite_draw(CUID compID, RUID layerID, AUID textureAUID)
{
    auto it = mCuidToDraw.find(compID);
    if (it != mCuidToDraw.end())
    {
        RUID oldDrawID = it->second;
        mDrawToCuid.erase(oldDrawID);
    }

    Sprite2DDraw draw{};
    if (textureAUID)
    {
        Image2D image2D = get_or_create_image_2d(textureAUID);
        draw = mServer.create_sprite_2d_draw(image2D, layerID, {}, 0);
    }
    else
        draw = mServer.create_sprite_2d_draw({}, layerID, {}, 0);
    LD_ASSERT(draw);

    RUID drawID = draw.unwrap()->id;
    mDrawToCuid[drawID] = compID;
    mCuidToDraw[compID] = drawID;

    return draw;
}

} // namespace LD