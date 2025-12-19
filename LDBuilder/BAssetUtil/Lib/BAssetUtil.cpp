#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/BlobAsset.h>
#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/Log/Log.h>
#include <Ludens/System/Memory.h>
#include <LudensBuilder/BAssetUtil/BAssetUtil.h>

#define LD_ASSET_EXT ".lda"

namespace LD {

static Log sLog("LDBuilder");

AssetUtil AssetUtil::create()
{
    // SPACE: implement AssetUtilObj when we need to retain some state
    return {};
}

void AssetUtil::destroy(AssetUtil util)
{
}

bool AssetUtil::import_blob(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_BLOB), MEMORY_USAGE_ASSET);
    BlobAsset asset((AssetObj*)memory);

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    BlobAssetImportJob importJob{};
    importJob.asset = asset;
    importJob.info.sourcePath = sourcePath;
    importJob.info.savePath = savePath;
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_blob: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_texture_2d(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_TEXTURE_2D), MEMORY_USAGE_ASSET);
    Texture2DAsset asset((AssetObj*)memory);

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    Texture2DAssetImportJob importJob;
    importJob.asset = asset;
    importJob.info.sourcePath = sourcePath;
    importJob.info.savePath = savePath;
    importJob.info.samplerHint.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    importJob.info.samplerHint.filter = RFILTER_LINEAR;
    importJob.info.samplerHint.mipmapFilter = RFILTER_LINEAR;
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_texture_2d: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_texture_cube(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_TEXTURE_CUBE), MEMORY_USAGE_ASSET);
    TextureCubeAsset asset((AssetObj*)memory);

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

    TextureCubeAssetImportJob importJob{};
    importJob.asset = asset;
    importJob.info.samplerHint.filter = RFILTER_LINEAR;
    importJob.info.samplerHint.mipmapFilter = RFILTER_LINEAR;
    importJob.info.samplerHint.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    importJob.info.savePath = savePath;

    bool allFacesFound = true;

    for (int i = 0; i < 6; i++)
    {
        FS::Path faceFilePath = importJob.info.sourcePaths[i] = sourcePath / FS::Path(faceFileNames[i]);

        if (!FS::exists(faceFilePath))
        {
            sLog.warn("import_texture_cube: face image not found {}", faceFilePath.string());
            allFacesFound = false;
        }
    }

    if (!allFacesFound)
        return false;

    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_texture_cube: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_font(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_FONT), MEMORY_USAGE_ASSET);
    FontAsset asset((AssetObj*)memory);

    std::string ext = sourcePath.extension().string();
    if (ext != ".ttf")
    {
        sLog.warn("import_font: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    FontAssetImportJob importJob{};
    importJob.asset = asset;
    importJob.info.sourcePath = sourcePath;
    importJob.info.savePath = savePath;
    importJob.info.fontSize = 36.0; // TODO: either parameterize or use some config
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_font: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_mesh(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_MESH), MEMORY_USAGE_ASSET);
    MeshAsset asset((AssetObj*)memory);

    std::string ext = sourcePath.extension().string();
    if (ext != ".gltf")
    {
        sLog.warn("import_mesh: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    MeshAssetImportJob importJob;
    importJob.asset = asset;
    importJob.info.sourcePath = sourcePath;
    importJob.info.savePath = savePath;
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_mesh: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_audio_clip(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_AUDIO_CLIP), MEMORY_USAGE_ASSET);
    AudioClipAsset asset((AssetObj*)memory);

    std::string ext = sourcePath.extension().string();
    if (ext != ".wav" && ext != ".mp3")
    {
        sLog.warn("import_audio_clip: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    AudioClipAssetImportJob importJob;
    importJob.asset = asset;
    importJob.sourcePath = sourcePath;
    importJob.savePath = savePath;
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_audio_clip: saved to {}", savePath.string());

    return true;
}

bool AssetUtil::import_lua_script(const FS::Path& sourcePath, LuaScriptDomain domain)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_LUA_SCRIPT), MEMORY_USAGE_ASSET);
    LuaScriptAsset asset((AssetObj*)memory);

    std::string ext = sourcePath.extension().string();
    if (ext != ".lua")
    {
        sLog.warn("import_lua_script: expected .lua file, found {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    LuaScriptAssetImportJob importJob;
    importJob.asset = asset;
    importJob.info.sourcePath = sourcePath;
    importJob.info.savePath = savePath;
    importJob.info.domain = domain;
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_lua_script: saved to {}", savePath.string());

    return true;
}

} // namespace LD