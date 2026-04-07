#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Project/Project.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Top-level states of a Game Project.
///        Equalize lifetime for a tuple (Project, AssetRegistry, SUIDRegistry).
///        Does not leverage singleton services such as AssetManager or Scene.
class ProjectContext
{
public:
    ProjectContext();
    ProjectContext(const ProjectContext&) = delete;
    ~ProjectContext();

    /// @brief Synchronously deserialize Project from its schema.
    bool load_project(const FS::Path& projectSchemaPath, std::string& err);

    /// @brief Synchronously deserialize AssetRegistry from its schema.
    bool load_asset_registry(const FS::Path& assetSchemaPath, std::string& err);

    /// @brief Synchronously serialize Project to its schema.
    bool save_project(std::string err);

    /// @brief Synchronously serialize AssetRegistry to its schema.
    bool save_asset_registry(std::string err);

    inline Project project() const { return mProject; }
    inline SUIDRegistry suid_registry() const { return mSUIDRegistry; }
    inline AssetRegistry asset_registry() const { return mAssetRegistry; }
    inline FS::Path asset_schema_abs_path() { return mProject ? mProject.get_asset_schema_absolute_path() : FS::Path(); }
    inline FS::Path project_schema_abs_path() { return mProject ? mProject.get_project_schema_path() : FS::Path(); }
    Vector<FS::Path> scene_schema_abs_paths();

private:
    Project mProject = {};             // project handle
    SUIDRegistry mSUIDRegistry = {};   // project serial identities
    AssetRegistry mAssetRegistry = {}; // project assets
};

} // namespace LD