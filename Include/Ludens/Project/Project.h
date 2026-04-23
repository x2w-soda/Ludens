#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Project/ProjectSettings.h>
#include <Ludens/Serial/SUID.h>
#include <Ludens/Serial/SUIDTable.h>
#include <Ludens/System/FileSystem.h>

#include <string>

namespace LD {

/// @brief Ludens project handle.
struct Project : Handle<struct ProjectObj>
{
    /// @brief Create empty project.
    static Project create();

    /// @brief Destroy project.
    static void destroy(Project project);

    /// @brief Get project version.
    /// @param major Major semantic version
    /// @param minor Minor semantic version
    /// @param patch Patch semantiv version
    void get_version(int& major, int& minor, int& patch);

    void set_name(const std::string& projectName);
    std::string get_name();

    /// @brief Project root directory is the directory containing Project schema.
    FS::Path get_root_dir_abs_path();
    FS::Path get_storage_dir_rel_path();
    void set_storage_dir_rel_path(const FS::Path& relPath);
    FS::Path get_storage_dir_abs_path();

    void set_project_schema_abs_path(const FS::Path& projectSchemaAbsPath);
    FS::Path get_project_schema_abs_path();

    void set_asset_schema_rel_path(const FS::Path& relPath);
    FS::Path get_asset_schema_rel_path();
    FS::Path get_asset_schema_abs_path();

    SUID register_scene(SUIDRegistry idReg, const std::string& uriPath, std::string& err);
    SUID register_scene_with_id(SUIDRegistry idReg, SUID id, const std::string& uriPath, std::string& err);
    void unregister_scene(SUIDRegistry idReg, SUID id);
    inline SUID get_default_scene_id() { return settings().startup_settings().get_default_scene_id(); }

    bool has_scene(SUID sceneID);
    bool get_scene_uri_path(SUID sceneID, std::string& outPath);
    bool get_default_scene_uri_path(std::string& outPath);
    bool set_scene_uri_path(SUID sceneID, const std::string& path);
    bool get_scene_schema_rel_path(SUID sceneID, FS::Path& outPath);
    bool get_scene_schema_abs_path(SUID sceneID, FS::Path& outPath);
    void get_scene_schema_abs_paths(Vector<FS::Path>& scenePaths);
    bool get_default_scene_schema_abs_path(FS::Path& outPath);

    /// @brief Get all scene entries in project.
    void get_scenes(Vector<SUIDEntry>& outEntries);

    /// @brief Get interface to project settings.
    ProjectSettings settings();
};

} // namespace LD