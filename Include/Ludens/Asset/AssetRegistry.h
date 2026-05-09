#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>

namespace LD {

class URI;

/// @brief Asset entry in the project, validated by AssetRegistry.
struct AssetEntry : Handle<struct AssetEntryObj>
{
    SUID get_id();
    AssetType get_type();
    String get_name();
    String get_path();
    bool set_path(const String& path);
    Vector<String> get_file_path_keys();
    String get_file_path(const String& key);
    void set_file_path(const String& key, const String& filePath);
};

/// @brief Bookkeeping for all assets in a project.
struct AssetRegistry : Handle<struct AssetRegistryObj>
{
    /// @brief Create empty asset registry.
    static AssetRegistry create();

    /// @brief Destroy asset registry.
    static void destroy(AssetRegistry registry);

    /// @brief Try register a new Asset, upon success a stable ID is assigned to the Asset.
    /// @note May fail due to URI path collisions.
    AssetEntry register_asset(SUIDRegistry idReg, AssetType type, const String& path);

    /// @brief Try register an Asset with known ID. Intended for deserialization code paths.
    /// @note May fail due to ID or URI collisions.
    AssetEntry register_asset_with_id(SUIDRegistry idReg, SUID id, AssetType type, const String& path);

    /// @brief Unregister an Asset that is no longer used in the project.
    void unregister_asset(SUIDRegistry idReg, SUID id);

    /// @brief Check if a path is valid for registration.
    bool is_path_valid(const String& path, String& collidingPath);
    bool is_uri_valid(const URI& uri, String& collidingPath);

    /// @brief Check if registry is dirty due to AssetEntry changes
    bool is_dirty();

    /// @brief Clear dirty status.
    void clear_dirty();

    /// @brief Lookup an Asset from ID.
    /// @return Asset entry in the registry or null handle.
    AssetEntry get_entry(SUID id);

    /// @brief Lookup an Asset entry by path.
    ///        If the full URI is ld://asset/script/player the path is "script/player"
    AssetEntry get_entry_by_path(const String& path);

    /// @brief Lookup an Asset entry by name.
    ///        If the full URI is ld://asset/script/player the name is "player"
    AssetEntry get_entry_by_name(const String& name);

    /// @brief Lookup all asset entries for a given type.
    void get_entries_by_type(Vector<AssetEntry>& outEntries, AssetType type);

    /// @brief Get all registered assets in the registry in no particular order.
    void get_all_entries(Vector<AssetEntry>& outEntries);
};

} // namespace LD
