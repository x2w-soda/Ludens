#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Font.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Font asset handle.
struct FontAsset : AssetHandle
{
    /// @brief Get font handle.
    Font get_font();

    /// @brief Get font atlas handle.
    FontAtlas get_font_atlas();
};

struct FontAssetImportInfo
{
    const void* sourceData = nullptr; /// if not null, the address of some font data in memory.
    size_t sourceDataSize;            /// byte size of source data
    FS::Path sourcePath;              /// path to load the source format, used if source data is null.
    FS::Path savePath;                /// path to save the imported asset
    float fontSize;                   /// desired font size for generating font atlas
};

class FontAssetImportJob
{
public:
    FontAsset asset;          /// subject asset handle
    FontAssetImportInfo info; /// Font import configuration

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD