#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/System/FileSystem.h>
#include <string>

namespace LD {

/// @brief Schema for listing all Assets under the current framework version.
struct AssetSchema
{
    /// @brief Load asset registry from TOML schema file on disk.
    static void load_registry_from_file(AssetRegistry registry, const FS::Path& tomlPath);

    /// @brief Try saving scene as TOML schema file on disk.
    static bool save_registry(AssetRegistry registry, const FS::Path& savePath, std::string& err);
};

} // namespace LD