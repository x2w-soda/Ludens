#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <string>

namespace LD {

struct AssetEntry
{
    std::string uri;
    std::string name;
    AssetType type;
    SUID id;
};

/// @brief Bookkeeping for all assets in a project.
struct AssetRegistry : Handle<struct AssetRegistryObj>
{
    /// @brief Create empty asset registry.
    static AssetRegistry create();

    /// @brief Destroy asset registry.
    static void destroy(AssetRegistry registry);

    /// @brief Register a new Asset, an ID is assigned to uniquely identify the Asset throughout the project.
    SUID register_asset(AssetType type, const std::string& uri, const std::string& name);

    /// @brief Used when the asset ID is also known, such as loading an Asset from the project.
    bool register_asset_with_id(const AssetEntry& entry);

    /// @brief Unregister an Asset that is no longer used in the project.
    void unregister_asset(SUID id);

    /// @brief Lookup an Asset from ID.
    /// @return Asset entry in the registry or nullptr.
    const AssetEntry* find_asset(SUID id);

    /// @brief Lookup all asset entries for a given type.
    void find_assets_by_type(AssetType type, Vector<const AssetEntry*>& entries);
};

} // namespace LD