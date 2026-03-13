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
    static bool load_registry_from_file(AssetRegistry registry, const FS::Path& tomlPath, std::string& err);

    /// @brief Try saving asset registry as TOML string.
    static bool save_registry_to_string(AssetRegistry registry, std::string& saveTOML, std::string& err);

    /// @brief Try saving asset registry as TOML schema file on disk.
    static bool save_registry(AssetRegistry registry, const FS::Path& savePath, std::string& err);
};

} // namespace LD