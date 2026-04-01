#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct AssetImportInfoStorage;

struct AssetImportInfo
{
    FS::Path dstPath;   /// absolute path to save the imported file
    std::string dstURI; /// requested asset URI
};

} // namespace LD