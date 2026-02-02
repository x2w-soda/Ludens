#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>

#include "RenderServerCache.h"

namespace LD {

void RenderServerCache::startup(RenderServer server, AssetManager assetManager)
{
    mServer = server;
    mAssetManager = assetManager;
    mDrawToCuid.clear();
    mCuidToDraw.clear();
    mAuidToData.clear();
}

void RenderServerCache::cleanup()
{
    if (!mServer)
        return;

    mAssetManager = {};
    mServer = {};
}

void RenderServerCache::IMesh::set_mesh_asset(AUID meshAUID)
{
    MeshDataID dataID = mCache->get_or_create_mesh_data(meshAUID);
    LD_ASSERT(dataID != 0);

    mComp->auid = meshAUID;

    mCache->invalidate_mesh_draw_id(dataID, mCUID);
}

void RenderServerCache::ISprite2D::set_texture_2d_asset(AUID textureAUID)
{
    Sprite2DDataID dataID = mCache->get_or_create_sprite_data(textureAUID);
    LD_ASSERT(dataID != 0);

    mComp->auid = textureAUID;

    mCache->invalidate_sprite_2d_draw_id(dataID, mCUID);
}

MeshDataID RenderServerCache::get_or_create_mesh_data(AUID meshAUID)
{
    RenderServer::IMesh mesh = mServer.mesh();
    MeshAsset meshA = (MeshAsset)mAssetManager.get_asset(meshAUID, ASSET_TYPE_MESH);
    LD_ASSERT(meshA);

    if (!mAuidToData.contains(meshAUID))
        mAuidToData[meshAUID] = mesh.create_data_id(*meshA.data());

    LD_ASSERT(mesh.exists(mAuidToData[meshAUID]));
    return mAuidToData[meshAUID];
}

MeshDrawID RenderServerCache::invalidate_mesh_draw_id(MeshDataID dataID, CUID compID)
{
    RenderServer::IMesh mesh = mServer.mesh();

    if (!mesh.exists(dataID))
        return 0;

    auto ite = mCuidToDraw.find(compID);
    if (ite != mCuidToDraw.end())
    {
        MeshDrawID oldDrawID = ite->second;
        mesh.destroy_draw_id(oldDrawID);
        mDrawToCuid.erase(oldDrawID);
    }

    MeshDrawID drawID = mesh.create_draw_id(dataID);
    mDrawToCuid[drawID] = compID;
    mCuidToDraw[compID] = drawID;

    return drawID;
}

Sprite2DDataID RenderServerCache::get_or_create_sprite_data(AUID textureAUID)
{
    RenderServer::ISprite2D sprite2D = mServer.sprite_2d();
    Texture2DAsset textureA = (Texture2DAsset)mAssetManager.get_asset(textureAUID, ASSET_TYPE_TEXTURE_2D);
    LD_ASSERT(textureA);

    if (!mAuidToData.contains(textureAUID))
        mAuidToData[textureAUID] = sprite2D.create_data_id(textureA.get_bitmap());

    LD_ASSERT(sprite2D.exists(mAuidToData[textureAUID]));
    return mAuidToData[textureAUID];
}

Sprite2DDrawID RenderServerCache::invalidate_sprite_2d_draw_id(Sprite2DDataID dataID, CUID compID)
{
    RenderServer::ISprite2D sprite2D = mServer.sprite_2d();

    auto ite = mCuidToDraw.find(compID);
    if (ite != mCuidToDraw.end())
    {
        Sprite2DDrawID oldDrawID = ite->second;
        sprite2D.destroy_draw_id(oldDrawID);
        mDrawToCuid.erase(oldDrawID);
    }

    Sprite2DDrawID drawID = sprite2D.create_draw_id(dataID);
    mDrawToCuid[drawID] = compID;
    mCuidToDraw[compID] = drawID;

    return drawID;
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

void RenderServerCache::destroy_all_draw_id()
{
    LD_PROFILE_SCOPE;

    mServer.mesh().destroy_all_draw_id();
    mServer.sprite_2d().destroy_all_draw_id();
}

} // namespace LD