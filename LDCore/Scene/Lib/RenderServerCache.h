#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/RenderServer/RenderServer.h>

namespace LD {

/// @brief Cache of render server resources. This class connects Scene, AssetManager, and RenderServer.
class RenderServerCache
{
public:
    /// @brief In-place startup, connect to audio server.
    void startup(RenderServer server, AssetManager assetManager);

    /// @brief In-place cleanup, destroys all resources from render server.
    void cleanup();

    /// @brief Get draw id associated with component.
    RUID get_component_draw_id(CUID compID);

    /// @brief Get component associated with draw id.
    CUID get_draw_id_component(RUID drawID);

    /// @brief Destroys all draw ids across all types. Data ids are not affected.
    void destroy_all_draw_id();

    class IMesh
    {
    public:
        IMesh(RenderServerCache* cache, MeshComponent* comp, CUID cuid)
            : mCache(cache), mComp(comp), mCUID(cuid) {}

        void set_mesh_asset(AUID meshAUID);

    private:
        RenderServerCache* mCache;
        MeshComponent* mComp;
        CUID mCUID;
    };

    inline IMesh mesh(MeshComponent* comp, CUID cuid)
    {
        return IMesh(this, comp, cuid);
    }

    class ISprite2D
    {
    public:
        ISprite2D(RenderServerCache* cache, Sprite2DComponent* comp, CUID cuid)
            : mCache(cache), mComp(comp), mCUID(cuid) {}

        void set_texture_2d_asset(AUID textureAUID);

    private:
        RenderServerCache* mCache;
        Sprite2DComponent* mComp;
        CUID mCUID;
    };

    inline ISprite2D sprite_2d(Sprite2DComponent* comp, CUID cuid)
    {
        return ISprite2D(this, comp, cuid);
    }

private:
    MeshDataID get_or_create_mesh_data(AUID meshAUID);
    MeshDrawID invalidate_mesh_draw_id(MeshDataID dataID, CUID compID);
    Sprite2DDataID get_or_create_sprite_data(AUID textureAUID);
    Sprite2DDrawID invalidate_sprite_2d_draw_id(Sprite2DDataID dataID, CUID compID);

private:
    RenderServer mServer{};
    AssetManager mAssetManager{};
    HashMap<RUID, CUID> mDrawToCuid; /// map RenderServer draw ID to component
    HashMap<CUID, RUID> mCuidToDraw; /// map component to RenderServer draw ID
    HashMap<AUID, RUID> mAuidToData; /// map asset to RenderServer data ID
};

} // namespace LD