#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/BlobAsset.h>
#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>
#include <LudensBuilder/AssetBuilder/AssetType/AssetBuilders.h>
#include <LudensBuilder/AssetUtil/AssetUtil.h>

#define LD_ASSET_EXT ".lda"

namespace LD {

static Log sLog("LDBuilder");

struct AssetUtilObj
{
    AssetImporter importer = {};
};

AssetUtil AssetUtil::create()
{
    auto* obj = heap_new<AssetUtilObj>(MEMORY_USAGE_ASSET);

    obj->importer = AssetImporter::create();

    // We still need AssetManager to allocate Asset memory as load destination
    // because AssetImporter always performs "import + load" simultaneously.
    if (!AssetManager::get())
        AssetManager::create({});

    return AssetUtil(obj);
}

void AssetUtil::destroy(AssetUtil util)
{
    auto* obj = util.unwrap();

    AssetImporter::destroy(obj->importer);

    heap_delete<AssetUtilObj>(obj);
}

bool AssetUtil::import_blob(const FS::Path& sourcePath)
{
    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    auto* importI = (BlobAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_BLOB);
    importI->srcPath = sourcePath;
    importI->dstPath = savePath;
    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_blob: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_texture_2d(const FS::Path& sourcePath)
{
    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    auto* importI = (Texture2DAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_TEXTURE_2D);
    importI->srcPath = sourcePath;
    importI->dstPath = savePath;
    importI->samplerHint.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    importI->samplerHint.filter = RFILTER_LINEAR;
    importI->samplerHint.mipmapFilter = RFILTER_LINEAR;
    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_texture_2d: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_texture_cube(const FS::Path& sourcePath)
{
    if (!FS::is_directory(sourcePath))
    {
        sLog.warn("import_texture_cube: directory not found {}", sourcePath.string());
        return false;
    }

    FS::Path savePath = sourcePath / FS::Path("cubemap");
    savePath.replace_extension(LD_ASSET_EXT);

    const char* faceFileNames[6] = {
        "px.png",
        "nx.png",
        "py.png",
        "ny.png",
        "pz.png",
        "nz.png",
    };

    auto* importI = (TextureCubeAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_TEXTURE_CUBE);
    importI->samplerHint.filter = RFILTER_LINEAR;
    importI->samplerHint.mipmapFilter = RFILTER_LINEAR;
    importI->samplerHint.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    importI->dstPath = savePath;

    bool allFacesFound = true;

    for (int i = 0; i < 6; i++)
    {
        FS::Path faceFilePath = importI->srcPaths[i] = sourcePath / FS::Path(faceFileNames[i]);

        if (!FS::exists(faceFilePath))
        {
            sLog.warn("import_texture_cube: face image not found {}", faceFilePath.string());
            allFacesFound = false;
        }
    }

    if (!allFacesFound)
        return false;

    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_texture_cube: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_font(const FS::Path& sourcePath)
{
    std::string ext = sourcePath.extension().string();
    if (ext != ".ttf")
    {
        sLog.warn("import_font: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    auto* importI = (FontAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_FONT);
    importI->srcPath = sourcePath;
    importI->dstPath = savePath;
    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_font: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_mesh(const FS::Path& sourcePath)
{
    std::string ext = sourcePath.extension().string();
    if (ext != ".gltf")
    {
        sLog.warn("import_mesh: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    auto* importI = (MeshAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_MESH);
    importI->srcPath = sourcePath;
    importI->dstPath = savePath;
    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_mesh: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_audio_clip(const FS::Path& sourcePath)
{
    std::string ext = sourcePath.extension().string();
    if (ext != ".wav" && ext != ".mp3")
    {
        sLog.warn("import_audio_clip: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    auto* importI = (AudioClipAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_AUDIO_CLIP);
    importI->srcPath = sourcePath;
    importI->dstPath = savePath;
    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_audio_clip: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_lua_script(const FS::Path& sourcePath, LuaScriptDomain domain)
{
    std::string ext = sourcePath.extension().string();
    if (ext != ".lua")
    {
        sLog.warn("import_lua_script: expected .lua file, found {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    auto* importI = (LuaScriptAssetImportInfo*)mObj->importer.allocate_import_info(ASSET_TYPE_LUA_SCRIPT);
    importI->srcPath = sourcePath;
    importI->dstPath = savePath;
    importI->domain = domain;
    AssetImportResult result = mObj->importer.import_asset_synchronous(importI);

    if (result.status)
        sLog.info("import_lua_script: saved to {}", savePath.string());

    return true;
}

} // namespace LD