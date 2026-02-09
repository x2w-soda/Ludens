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
    /// @brief In-place startup, connect to render system.
    void startup(RenderSystem system, AssetManager assetManager);

    /// @brief In-place cleanup, destroys all resources from render system.
    void cleanup();

    /// @brief Get draw id associated with component.
    RUID get_component_draw_id(CUID compID);

    /// @brief Get component associated with draw id.
    CUID get_draw_id_component(RUID drawID);

    inline RUID create_screen_layer(const std::string& name)
    {
        return mSystem.create_screen_layer(name);
    }

    inline void destroy_screen_layer(RUID layerID)
    {
        mSystem.destroy_screen_layer(layerID);
    }

    MeshData get_or_create_mesh_data(AssetID meshAUID);
    MeshDraw create_mesh_draw(CUID compID, AssetID meshAUID = 0);
    Image2D get_or_create_image_2d(AssetID textureAUID);
    Sprite2DDraw create_sprite_draw(CUID compID, RUID layerID, AssetID textureAUID = 0);

private:
    RenderSystem mSystem{};
    AssetManager mAssetManager{};
    HashMap<RUID, CUID> mDrawToCuid; /// map RenderSystem draw ID to component
    HashMap<CUID, RUID> mCuidToDraw; /// map component to RenderSystem draw ID
    HashMap<AssetID, MeshData> mMeshData;
    HashMap<AssetID, Image2D> mImage2D;
};

} // namespace LD