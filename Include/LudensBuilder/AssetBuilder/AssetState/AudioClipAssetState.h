#pragma once

#include <LudensBuilder/AssetBuilder/AssetSource.h>

namespace LD {

struct AudioClipAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to load the source audio file
};

void audio_clip_asset_import(void*);

} // namespace LD