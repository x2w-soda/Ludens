#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
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

    RUID get_or_create_mesh(AUID meshAUID);

    RUID get_mesh(AUID meshAUID);

    RImage get_or_create_image(AUID textureAUID);

    /// @brief Create mesh draw call for component.
    RUID create_mesh_draw_call(RUID meshID, CUID compID);

    /// @brief Get draw call associated with component.
    RUID get_component_ruid(CUID compID);

    /// @brief Get component associated with draw call.
    CUID get_ruid_component(RUID drawI);

private:
    RenderServer mServer{};
    AssetManager mAssetManager{};
    std::unordered_map<RUID, CUID> mRuidToCuid;    /// map draw call to corresponding component
    std::unordered_map<CUID, RUID> mCuidToRuid;    /// map component to corresponding draw call
    std::unordered_map<AUID, RUID> mAuidToRuid;    /// map asset to GPU resource
    std::unordered_map<AUID, RImage> mAuidToImage; /// map asset to GPU image resource
};

} // namespace LD