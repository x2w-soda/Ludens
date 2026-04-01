#include <Ludens/Header/Assert.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfoStorage.h>

namespace LD {

void AssetImportInfoStorage::startup(AssetType newType)
{
    if (type == newType)
        return;

    LD_ASSERT(type == ASSET_TYPE_ENUM_COUNT);
    type = newType;

    switch (type)
    {
    case ASSET_TYPE_BLOB:
        new (&as.blob) BlobAssetImportInfo();
        break;
    case ASSET_TYPE_FONT:
        new (&as.font) FontAssetImportInfo();
        break;
    case ASSET_TYPE_MESH:
        new (&as.mesh) MeshAssetImportInfo();
        break;
    case ASSET_TYPE_AUDIO_CLIP:
        new (&as.audioClip) AudioClipAssetImportInfo();
        break;
    case ASSET_TYPE_TEXTURE_2D:
        new (&as.texture2D) Texture2DAssetImportInfo();
        break;
    case ASSET_TYPE_TEXTURE_CUBE:
        new (&as.textureCube) TextureCubeAssetImportInfo();
        break;
    case ASSET_TYPE_LUA_SCRIPT:
        new (&as.luaScript) LuaScriptAssetImportInfo();
        break;
    default:
        LD_DEBUG_BREAK;
        break;
    }
}

void AssetImportInfoStorage::cleanup()
{
    if (type == ASSET_TYPE_ENUM_COUNT)
        return;

    switch (type)
    {
    case ASSET_TYPE_BLOB:
        (&as.blob)->~BlobAssetImportInfo();
        break;
    case ASSET_TYPE_FONT:
        (&as.font)->~FontAssetImportInfo();
        break;
    case ASSET_TYPE_MESH:
        (&as.mesh)->~MeshAssetImportInfo();
        break;
    case ASSET_TYPE_AUDIO_CLIP:
        (&as.audioClip)->~AudioClipAssetImportInfo();
        break;
    case ASSET_TYPE_TEXTURE_2D:
        (&as.texture2D)->~Texture2DAssetImportInfo();
        break;
    case ASSET_TYPE_TEXTURE_CUBE:
        (&as.textureCube)->~TextureCubeAssetImportInfo();
        break;
    case ASSET_TYPE_LUA_SCRIPT:
        (&as.luaScript)->~LuaScriptAssetImportInfo();
        break;
    default:
        LD_DEBUG_BREAK;
        break;
    }

    type = ASSET_TYPE_ENUM_COUNT;
}

} // namespace LD
