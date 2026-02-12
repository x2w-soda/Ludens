#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/RenderSystem/RenderSystem.h>

namespace LD {

/// @brief Cache of render system resources. This class connects Scene, AssetManager, and RenderSystem.
class RenderSystemCache
{
public:
    /// @brief In-place cretaion, connect to render system.
    void create(RenderSystem system, AssetManager assetManager);

    /// @brief In-place destruction, destroys all resources from render system.
    void destroy();

    /// @brief Get draw id associated with component.
    RUID get_component_draw_id(CUID compID);

    /// @brief Get component associated with draw id.
    CUID get_draw_id_component(RUID drawID);

    RUID get_or_create_screen_layer(SUID screenLayerSUID);
    SUID get_screen_layer_suid(RUID screenLayerRUID);
    MeshData get_or_create_mesh_data(AssetID meshAUID);
    MeshDraw create_mesh_draw(CUID compID, AssetID meshAUID = 0);
    void destroy_mesh_draw(MeshDraw draw);
    Image2D get_or_create_image_2d(AssetID textureAUID);
    Sprite2DDraw create_sprite_2d_draw(CUID compID, RUID layerID, AssetID textureAUID = 0);
    void destroy_sprite_2d_draw(Sprite2DDraw draw);

private:
    void link_id(CUID compID, RUID drawID);

private:
    RenderSystem mSystem{};
    AssetManager mAssetManager{};
    HashMap<RUID, CUID> mDrawToCuid;        /// map RenderSystem draw ID to component
    HashMap<CUID, RUID> mCuidToDraw;        /// map component to RenderSystem draw ID
    HashMap<SUID, RUID> mSuidToScreenLayer; /// map screen layer SUID to RUID
    HashMap<RUID, SUID> mScreenLayerToSuid; /// map screen layer RUID to SUID
    HashMap<AssetID, MeshData> mMeshData;
    HashMap<AssetID, Image2D> mImage2D;
};

} // namespace LD