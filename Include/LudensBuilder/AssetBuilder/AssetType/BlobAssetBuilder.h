#pragma once

#include <Ludens/Header/View.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct BlobAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath;  /// path to load the blob file, used if srcData is empty.
    View srcView = {}; /// if not empty, the blob data in memory.
};

void blob_asset_copy_import_info(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo);
void blob_asset_import(void*);

} // namespace LD