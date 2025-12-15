#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>
#include <cstdint>

namespace LD {

/// @brief TextureCube asset handle.
struct TextureCubeAsset : Asset
{
    /// @brief Get bitmap for cubemap.
    Bitmap get_bitmap() const;
};

struct TextureCubeAssetImportInfo
{
    RSamplerInfo samplerHint; /// desired texture sampler mode
    FS::Path sourcePaths[6];  /// path to load the source format
    FS::Path savePath;        /// path to save the imported format
};

/// @brief Creates TextureCubeAsset from source files,
///        and saves the imported asset to disk.
class TextureCubeAssetImportJob
{
public:
    TextureCubeAsset asset;          /// subject asset handle
    TextureCubeAssetImportInfo info; /// import configuration

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD