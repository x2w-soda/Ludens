#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

enum TextureCompression
{
    /// @brief LZ4 compressed bitmap
    TEXTURE_COMPRESSION_LZ4 = 0,
};

/// @brief Texture2D asset handle.
struct Texture2DAsset : Asset
{
    /// @brief Unload asset from RAM.
    void unload();

    /// @brief Get bitmap in RAM.
    Bitmap get_bitmap();

    /// @brief Get desired texture sampling mode.
    RSamplerInfo get_sampler_hint() const;
};

struct Texture2DAssetImportInfo
{
    RSamplerInfo samplerHint; /// desired texture sampler mode
    FS::Path sourcePath;      /// path to load the source format
    FS::Path savePath;        /// path to save the imported format
};

/// @brief Creates Texture2DAsset from source file,
///        and saves the imported asset to disk.
class Texture2DAssetImportJob
{
public:
    Texture2DAsset asset;          /// subject asset handle
    Texture2DAssetImportInfo info; /// import configuration

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD