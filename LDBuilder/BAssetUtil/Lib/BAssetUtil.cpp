#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
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

bool AssetUtil::import_texture_2d(const FS::Path& sourcePath)
{
    void* memory = heap_malloc(get_asset_byte_size(ASSET_TYPE_TEXTURE_2D), MEMORY_USAGE_ASSET);
    Texture2DAsset asset((AssetObj*)memory);

    std::string ext = sourcePath.extension().string();
    if (ext != ".png")
    {
        sLog.warn("import_texture_2d: unsupported file type {}", ext);
        return false;
    }

    FS::Path savePath(sourcePath);
    savePath.replace_extension(LD_ASSET_EXT);

    Texture2DAssetImportJob importJob;
    importJob.asset = asset;
    importJob.info.sourcePath = sourcePath;
    importJob.info.savePath = savePath;
    importJob.info.compression = TEXTURE_COMPRESSION_LZ4;
    importJob.info.samplerHint.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    importJob.info.samplerHint.filter = RFILTER_LINEAR;
    importJob.info.samplerHint.mipmapFilter = RFILTER_LINEAR;
    importJob.submit();

    JobSystem::get().wait_all();

    heap_free(memory);
    sLog.info("import_texture_2d: saved to {}", savePath.string());

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

} // namespace LD