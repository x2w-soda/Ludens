#include <Ludens/DSA/URI.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectDef.h>
#include <Ludens/Scene/SceneDef.h>
#include <Ludens/Serial/SUIDTable.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct ProjectObj
{
    std::string name;                                                     /// project name, user defined
    FS::Path projectSchemaAbsPath;                                        /// absolute path to project schema file
    FS::Path assetSchemaRelPath;                                          /// relative path to asset schema file
    FS::Path storageDirRelPath = LD_PROJECT_DEFAULT_STORAGE_DIR_REL_PATH; /// relative path to project storage dir
    ProjectSettings settings;                                             /// project-wide settings
    SUIDTable sceneTable;                                                 /// registered scenes in the project

    SUID register_scene(SUIDRegistry idReg, SUID sceneID, const std::string& uriPath, std::string& err);
    void unregister_scene(SUIDRegistry idReg, SUID sceneID);
};

SUID ProjectObj::register_scene(SUIDRegistry idReg, SUID sceneID, const std::string& uriPath, std::string& err)
{
#if 0
    for (const ProjectSceneEntry& existingEntry : mObj->scenes)
    {
        if (existingEntry.id == entry.id)
        {
            err = std::format("Scene SUID 0x{} already registered in project", entry.id.to_string());
            return false;
        }
    }

    if (entry.uriPath.empty())
    {
        err = "empty scene schema path";
        return false;
    }

    if (entry.uriPath.empty())
    {
        err = "empty scene name";
        return false;
    }
#else
    if (sceneID) // try register with known ID
    {
        if (idReg.contains(sceneID) || sceneTable.contains(sceneID))
            return false;

        (void)idReg.try_get_suid(sceneID);
        return sceneTable.register_id(sceneID, uriPath);
    }

    // try register with new ID
    sceneID = idReg.get_suid(SERIAL_TYPE_SCENE);
    if (sceneTable.register_id(sceneID, uriPath))
        return true;

    idReg.free_suid(sceneID);
    return false;
#endif
}

void ProjectObj::unregister_scene(SUIDRegistry idReg, SUID sceneID)
{
    sceneTable.unregister_id(sceneID);
    idReg.free_suid(sceneID);
}

Project Project::create()
{
    ProjectObj* obj = heap_new<ProjectObj>(MEMORY_USAGE_MISC);

    obj->settings = ProjectSettings::create();

    return Project(obj);
}

void Project::destroy(Project project)
{
    ProjectObj* obj = project.unwrap();

    ProjectSettings::destroy(obj->settings);

    heap_delete<ProjectObj>(obj);
}

void Project::get_version(int& major, int& minor, int& patch)
{
    major = LD_VERSION_MAJOR;
    minor = LD_VERSION_MINOR;
    patch = LD_VERSION_PATCH;
}

void Project::set_name(const std::string& name)
{
    mObj->name = name;
}

std::string Project::get_name()
{
    return mObj->name;
}

FS::Path Project::get_root_dir_abs_path()
{
    return mObj->projectSchemaAbsPath.parent_path();
}

FS::Path Project::get_storage_dir_rel_path()
{
    return mObj->storageDirRelPath;
}

void Project::set_storage_dir_rel_path(const FS::Path& relPath)
{
    mObj->storageDirRelPath = relPath;
}

FS::Path Project::get_storage_dir_abs_path()
{
    return FS::absolute(get_root_dir_abs_path() / mObj->storageDirRelPath);
}

void Project::set_project_schema_abs_path(const FS::Path& projectSchemaPath)
{
    mObj->projectSchemaAbsPath = FS::absolute(projectSchemaPath);
}

FS::Path Project::get_project_schema_abs_path()
{
    return mObj->projectSchemaAbsPath;
}

void Project::set_asset_schema_rel_path(const FS::Path& relPath)
{
    mObj->assetSchemaRelPath = relPath;
}

FS::Path Project::get_asset_schema_rel_path()
{
    return mObj->assetSchemaRelPath;
}

FS::Path Project::get_asset_schema_abs_path()
{
    return FS::absolute(get_root_dir_abs_path() / mObj->assetSchemaRelPath);
}

SUID Project::register_scene(SUIDRegistry idReg, const std::string& uriPath, std::string& err)
{
    return mObj->register_scene(idReg, SUID(0), uriPath, err);
}

SUID Project::register_scene_with_id(SUIDRegistry idReg, SUID id, const std::string& uriPath, std::string& err)
{
    return mObj->register_scene(idReg, id, uriPath, err);
}

void Project::unregister_scene(SUIDRegistry idReg, SUID id)
{
    mObj->unregister_scene(idReg, id);
}

bool Project::has_scene(SUID sceneID)
{
    return mObj->sceneTable.contains(sceneID);
}

bool Project::get_scene_uri_path(SUID sceneID, std::string& outPath)
{
    return mObj->sceneTable.get_path(sceneID, outPath);
}

bool Project::get_default_scene_uri_path(std::string& outPath)
{
    SUID defaultSceneID = settings().startup_settings().get_default_scene_id();

    return get_scene_uri_path(defaultSceneID, outPath);
}

bool Project::set_scene_uri_path(SUID sceneID, const std::string& path)
{
    return mObj->sceneTable.set_path(sceneID, path);
}

bool Project::get_scene_schema_rel_path(SUID sceneID, FS::Path& outPath)
{
    if (!mObj->sceneTable.contains(sceneID))
        return false;

    outPath = mObj->storageDirRelPath / sceneID.to_string() / LD_SCENE_DEFAULT_SCHEMA_FILE_NAME;
    return true;
}

bool Project::get_scene_schema_abs_path(SUID sceneID, FS::Path& outPath)
{
    FS::Path relPath;
    if (!get_scene_schema_rel_path(sceneID, relPath))
        return false;

    outPath = FS::absolute(get_root_dir_abs_path() / relPath);
    return true;
}

void Project::get_scene_schema_abs_paths(Vector<FS::Path>& scenePaths)
{
    Vector<SUIDEntry> entries = mObj->sceneTable.get_entries();
    scenePaths.resize(entries.size());

    for (size_t i = 0; i < scenePaths.size(); i++)
    {
        const SUIDEntry& entry = entries[i];

        bool success = get_scene_schema_abs_path(entry.id, scenePaths[i]);
        LD_ASSERT(success);
    }
}

bool Project::get_default_scene_schema_abs_path(FS::Path& outPath)
{
    SUID defaultSceneID = settings().startup_settings().get_default_scene_id();

    return get_scene_schema_abs_path(defaultSceneID, outPath);
}

void Project::get_scenes(Vector<SUIDEntry>& outEntries)
{
    outEntries = mObj->sceneTable.get_entries();
}

ProjectSettings Project::settings()
{
    return mObj->settings;
}

} // namespace LD