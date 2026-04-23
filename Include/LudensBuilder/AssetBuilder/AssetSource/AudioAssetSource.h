#pragma once

#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

namespace LD {

struct AudioClipAssetImportInfo : AssetImportInfo
{
    FS::Path srcFile; /// path to load the source audio file

    AudioClipAssetImportInfo()
        : AssetImportInfo(ASSET_TYPE_AUDIO_CLIP) {}
};

void audio_clip_asset_import(void*);
bool audio_clip_asset_import_info_set_src_files(AssetImportInfo* info, size_t count, const FS::Path* srcFilePaths);

} // namespace LD