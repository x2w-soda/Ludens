#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/System/FileSystem.h>

#include <string>

namespace LD {

struct AssetImportInfo
{
    AssetType type = ASSET_TYPE_ENUM_COUNT;
    FS::Path dstPath;   /// absolute path to save the imported file
    std::string dstURI; /// requested asset URI
};

} // namespace LD