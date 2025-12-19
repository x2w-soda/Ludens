#include "RenderServerCache.h"

namespace LD {

void RenderServerCache::startup(RenderServer server)
{
    mServer = server;
    mRuidToCuid.clear();
    mCuidToRuid.clear();
    mAuidToRuid.clear();
}

void RenderServerCache::cleanup()
{
    // TODO: technically render server destroys all resources at the end of its lifetime.

    mServer = {};
}

RUID RenderServerCache::get_or_create_mesh(MeshAsset meshA)
{
    AUID meshAUID = meshA.get_auid();

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