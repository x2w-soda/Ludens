#pragma once

#include <LudensBuilder/AssetBuilder/AssetType/AssetBuilders.h>

namespace LD {

/// @brief Tagged union of all import infos
struct AssetImportInfoStorage
{
    AssetType type = ASSET_TYPE_ENUM_COUNT; /// required tag for down casting

    union Storage
    {
        BlobAssetImportInfo blob;
        FontAssetImportInfo font;
        MeshAssetImportInfo mesh;
        AudioClipAssetImportInfo audioClip;
        Texture2DAssetImportInfo texture2D;
        TextureCubeAssetImportInfo textureCube;
        LuaScriptAssetImportInfo luaScript;

        Storage() {}
        ~Storage() {}
    } as;

    void startup(AssetType newType);
    void cleanup();
};

} // namespace LD