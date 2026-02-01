#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/RenderServer/RenderServer.h>

#include <unordered_map>

namespace LD {

/// @brief Cache of render server resources.
class RenderServerCache
{
public:
    /// @brief In-place startup, connect to audio server.
    void startup(RenderServer server, AssetManager assetManager);

    /// @brief In-place cleanup, destroys all resources from render server.
    void cleanup();

    MeshDataID get_or_create_mesh(AUID meshAUID);

    MeshDataID get_mesh(AUID meshAUID);

    RImage get_or_create_image(AUID textureAUID);

    /// @brief Create mesh draw id for component.
    MeshDrawID create_mesh_draw_id(MeshDataID dataID, CUID compID);

    /// @brief Get draw id associated with component.
    MeshDrawID get_component_draw_id(CUID compID);

    /// @brief Get component associated with draw call.
    CUID get_ruid_component(RUID drawID);

private:
    RenderServer mServer{};
    AssetManager mAssetManager{};
    HashMap<RUID, CUID> mRuidToCuid;    /// map draw call to corresponding component
    HashMap<CUID, RUID> mCuidToRuid;    /// map component to corresponding draw call
    HashMap<AUID, RUID> mAuidToRuid;    /// map asset to GPU resource
    HashMap<AUID, RImage> mAuidToImage; /// map asset to GPU image resource
};

} // namespace LD