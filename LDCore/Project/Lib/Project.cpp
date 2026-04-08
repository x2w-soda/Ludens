#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Project/Project.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct ProjectObj
{
    std::string name;                 /// project name, user defined
    Vector<ProjectSceneEntry> scenes; /// registered scenes in the project
    FS::Path assetSchemaPath;         /// relative path to asset schema file
    FS::Path projectSchemaPath;       /// absolute path to project schema file
    ProjectSettings settings;         /// project-wide settings
};

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

FS::Path Project::get_root_path()
{
    return mObj->projectSchemaPath.parent_path();
}

void Project::set_project_schema_path(const FS::Path& projectSchemaPath)
{
    mObj->projectSchemaPath = projectSchemaPath.lexically_normal();
}

FS::Path Project::get_project_schema_path()
{
    return mObj->projectSchemaPath;
}

void Project::set_asset_schema_path(const FS::Path& assetSchemaPath)
{
    mObj->assetSchemaPath = assetSchemaPath.lexically_normal();
}

FS::Path Project::get_asset_schema_path()
{
    return mObj->assetSchemaPath;
}

bool Project::add_scene(const ProjectSceneEntry& entry, std::string& err)
{
    for (const ProjectSceneEntry& existingEntry : mObj->scenes)
    {
        if (existingEntry.id == entry.id)
        {
            err = std::format("Scene SUID {} already registered in project", entry.id);
            return false;
        }
    }

    if (entry.path.empty())
    {
        err = "empty scene schema path";
        return false;
    }

    if (entry.name.empty())
    {
        err = "empty scene name";
        return false;
    }

    mObj->scenes.push_back(entry);
    return true;
}

bool Project::get_scene(SUID sceneID, ProjectSceneEntry& outEntry)
{
    if (!sceneID || sceneID.type() != SERIAL_TYPE_SCENE)
        return false;

    for (const ProjectSceneEntry& entry : mObj->scenes)
    {
        if (entry.id == sceneID)
        {
            outEntry = entry;
            return true;
        }
    }

    return false;
}

void Project::get_scenes(Vector<ProjectSceneEntry>& outEntries)
{
    outEntries = mObj->scenes;
}

void Project::get_scene_schema_paths(Vector<FS::Path>& scenePaths)
{
    scenePaths.resize(mObj->scenes.size());

    for (size_t i = 0; i < mObj->scenes.size(); i++)
    {
        scenePaths[i] = mObj->scenes[i].path;
    }
}

void Project::set_scene_schema_path(SUID sceneID, const FS::Path& scenePath)
{
    if (!sceneID || sceneID.type() != SERIAL_TYPE_SCENE)
        return;

    for (ProjectSceneEntry& entry : mObj->scenes)
    {
        if (entry.id == sceneID)
        {
            entry.path = scenePath;
            return;
        }
    }
}

ProjectSettings Project::settings()
{
    return mObj->settings;
}

} // namespace LD