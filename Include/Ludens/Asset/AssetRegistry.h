#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Handle.h>
#include <string>
#include <vector>

namespace LD {

struct AssetEntry
{
    std::string uri;
    std::string name;
    AssetType type;
    AUID id;
};

/// @brief Bookkeeping for all assets in a project.
struct AssetRegistry : Handle<struct AssetRegistryObj>
{
    /// @brief Create empty asset registry.
    static AssetRegistry create();

    /// @brief Destroy asset registry.
    static void destroy(AssetRegistry registry);

    /// @brief Register a new Asset, an ID is assigned to uniquely identify the Asset throughout the project.
    AUID register_asset(AssetType type, const std::string& uri, const std::string& name);

    /// @brief Used when the asset ID is also known, such as loading an Asset from the project.
    bool register_asset_with_id(const AssetEntry& entry);

    /// @brief Unregister an Asset that is no longer used in the project.
    void unregister_asset(AUID auid);

    /// @brief Set the underyling asset ID counter value.
    void set_auid_counter(uint32_t auidCounter);

    /// @brief Get current asset ID counter value.
    uint32_t get_auid_counter();

    /// @brief Lookup an Asset from ID.
    /// @return Asset entry in the registry or nullptr.
    const AssetEntry* find_asset(AUID auid);

    /// @brief Lookup all asset entries for a given type.
    void find_assets_by_type(AssetType type, std::vector<const AssetEntry*>& entries);
};

} // namespace LD