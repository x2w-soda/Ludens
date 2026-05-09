#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/String.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Schema for listing all Assets involved in a project.
///        This schema serializes to and from AssetRegistry.
struct AssetSchema
{
    /// @brief Load asset registry from TOML schema file on disk.
    static bool load_registry_from_file(AssetRegistry registry, SUIDRegistry idRegistry, const FS::Path& tomlPath, String& err);

    /// @brief Try saving asset registry as TOML string.
    static bool save_registry_to_string(AssetRegistry registry, String& saveTOML, String& err);

    /// @brief Try saving asset registry as TOML schema file on disk.
    static bool save_registry(AssetRegistry registry, const FS::Path& savePath, String& err);

    static String create_empty();
};

} // namespace LD