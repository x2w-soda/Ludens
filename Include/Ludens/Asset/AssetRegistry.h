#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <string>

namespace LD {

/// @brief Asset entry in the project, validated by AssetRegistry.
struct AssetEntry : Handle<struct AssetEntryObj>
{
    SUID get_id();
    AssetType get_type();
    std::string get_name();
    std::string get_uri();
    Vector<std::string> get_path_keys();
    std::string get_path(const std::string& key);
    inline std::string get_main_path() { return get_path("main"); }
    void set_path(const std::string& key, const std::string& uri);
};

/// @brief Bookkeeping for all assets in a project.
struct AssetRegistry : Handle<struct AssetRegistryObj>
{
    /// @brief Create empty asset registry.
    static AssetRegistry create();

    /// @brief Destroy asset registry.
    static void destroy(AssetRegistry registry);

    /// @brief Register a new Asset, an ID is assigned to uniquely identify the Asset throughout the project.
    AssetEntry register_asset(AssetType type, const std::string& uri);

    /// @brief Used when the asset ID is also known, such as loading an Asset from the project.
    AssetEntry register_asset_with_id(SUID id, AssetType type, const std::string& uri);

    /// @brief Unregister an Asset that is no longer used in the project.
    void unregister_asset(SUID id);

    /// @brief Lookup an Asset from ID.
    /// @return Asset entry in the registry or null handle.
    AssetEntry get_entry(SUID id);

    /// @brief Lookup an Asset from URI.
    AssetEntry get_entry_by_uri(const std::string& uri);

    /// @brief Lookup all asset entries for a given type.
    void get_entries_by_type(Vector<AssetEntry>& outEntries, AssetType type);

    /// @brief Get all registered assets in the registry in no particular order.
    void get_all_entries(Vector<AssetEntry>& outEntries);
};

} // namespace LD