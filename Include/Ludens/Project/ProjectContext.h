#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Project/Project.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Top-level states of a Game Project.
///        Equalize lifetime for a tuple (Project, AssetRegistry, SUIDRegistry).
class ProjectContext
{
public:
    ProjectContext();
    ProjectContext(const ProjectContext&) = delete;
    ~ProjectContext();

    /// @brief Synchronously deserialize Project from its schema.
    bool load_project_schema(const FS::Path& projectSchemaPath, std::string& err);

    /// @brief Synchronously deserialize AssetRegistry from its schema.
    bool load_asset_schema(const FS::Path& assetSchemaPath, std::string& err);

    /// @brief Synchronously serialize Project to its schema.
    bool save_project(std::string err);

    /// @brief Synchronously serialize AssetRegistry to its schema.
    bool save_asset_registry(std::string err);

    /// @brief Propagate ProjectScreenLayerSettings to Scene.
    void configure_project_screen_layers();

    inline Project project() const { return mProject; }
    inline SUIDRegistry suid_registry() const { return mSUIDRegistry; }
    inline AssetRegistry asset_registry() const { return mAssetRegistry; }
    inline FS::Path asset_schema_abs_path() { return mProject ? mProject.get_asset_schema_absolute_path() : FS::Path(); }
    inline FS::Path project_schema_abs_path() { return mProject ? mProject.get_project_schema_path() : FS::Path(); }
    Vector<FS::Path> scene_schema_abs_paths();
    FS::Path default_scene_schema_abs_path();

private:
    Project mProject = {};             // project handle
    SUIDRegistry mSUIDRegistry = {};   // project serial identities
    AssetRegistry mAssetRegistry = {}; // project assets
};

} // namespace LD