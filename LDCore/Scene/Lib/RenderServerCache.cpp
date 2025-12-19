#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>

#include "RenderServerCache.h"

namespace LD {

void RenderServerCache::startup(RenderServer server, AssetManager assetManager)
{
    mServer = server;
    mAssetManager = assetManager;
    mRuidToCuid.clear();
    mCuidToRuid.clear();
    mAuidToRuid.clear();
    mAuidToImage.clear();
}

void RenderServerCache::cleanup()
{
    if (!mServer)
        return;

    RDevice device = mServer.get_device();
    device.wait_idle();

    for (auto ite : mAuidToImage)
        device.destroy_image(ite.second);

    mAssetManager = {};
    mServer = {};
}

RUID RenderServerCache::get_or_create_mesh(AUID meshAUID)
{
    MeshAsset meshA = (MeshAsset)mAssetManager.get_asset(meshAUID, ASSET_TYPE_MESH);
    LD_ASSERT(meshA);

    if (!mAuidToRuid.contains(meshAUID))
        mAuidToRuid[meshAUID] = mServer.create_mesh(*meshA.data());

    return mAuidToRuid[meshAUID];
}

RUID RenderServerCache::get_mesh(AUID meshAUID)
{
    if (!mAuidToRuid.contains(meshAUID))
        return 0;

    RUID meshID = mAuidToRuid[meshAUID];
    if (!mServer.mesh_exists(meshID))
        return 0;

    return meshID;
}

RImage RenderServerCache::get_or_create_image(AUID textureAUID)
{
    Texture2DAsset textureA = (Texture2DAsset)mAssetManager.get_asset(textureAUID, ASSET_TYPE_TEXTURE_2D);
    LD_ASSERT(textureA);

    auto ite = mAuidToImage.find(textureAUID);
    if (!mAuidToImage.contains(textureAUID))
    {
        RDevice device = mServer.get_device();
        Bitmap bitmap = textureA.get_bitmap();
        LD_ASSERT(bitmap.format() == BITMAP_FORMAT_RGBA8U);

        RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_TRANSFER_DST_BIT | RIMAGE_USAGE_SAMPLED_BIT,
                                                      RFORMAT_RGBA8, bitmap.width(), bitmap.height(), textureA.get_sampler_hint());

        // create and upload to VRAM
        RImage image = mAuidToImage[textureAUID] = device.create_image(imageI);
        RStager stager(device, RQUEUE_TYPE_GRAPHICS);
        stager.add_image_data(image, bitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
        stager.submit(device.get_graphics_queue());
    }

    return mAuidToImage[textureAUID];
}

RUID RenderServerCache::create_mesh_draw_call(RUID meshID, CUID compID)
{
    if (!mServer.mesh_exists(meshID))
        return 0;

    auto ite = mCuidToRuid.find(compID);
    if (ite != mCuidToRuid.end())
    {
        RUID oldDrawCall = ite->second;
        mServer.destroy_mesh_draw_call(oldDrawCall);
        mRuidToCuid.erase(oldDrawCall);
        mCuidToRuid.erase(compID);
    }

    RUID drawCall = mServer.create_mesh_draw_call(meshID);
    mRuidToCuid[drawCall] = compID;
    mCuidToRuid[compID] = drawCall;

    return drawCall;
}

RUID RenderServerCache::get_component_ruid(CUID compID)
{
    auto ite = mCuidToRuid.find(compID);

    if (ite == mCuidToRuid.end())
        return 0;

    return ite->second;
}

CUID RenderServerCache::get_ruid_component(RUID drawID)
{
    auto ite = mRuidToCuid.find(drawID);

    if (ite == mRuidToCuid.end())
        return 0;

    return ite->second;
}

} // namespace LD