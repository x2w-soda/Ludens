#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Project/Project.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct ProjectObj
{
    std::string name;                 /// project name, user defined
    std::vector<FS::Path> scenePaths; /// relative paths to project schemas
    FS::Path assetsPath;              /// relative path to project assets schema
    FS::Path rootPath;                /// project root path
    ProjectSettings settings;         /// project-wide settings
};

Project Project::create(const FS::Path& rootPath)
{
    ProjectObj* obj = heap_new<ProjectObj>(MEMORY_USAGE_MISC);
    obj->rootPath = rootPath;
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

std::string Project::get_name() const
{
    return mObj->name;
}

FS::Path Project::get_root_path() const
{
    return mObj->rootPath;
}

void Project::set_assets_path(const std::filesystem::path& assetsPath)
{
    mObj->assetsPath = assetsPath;

    // SPACE: error handling if Asset schema is not found.
}

std::filesystem::path Project::get_assets_path() const
{
    return (mObj->rootPath / mObj->assetsPath).lexically_normal();
}

void Project::add_scene_path(const std::filesystem::path& scenePath)
{
    mObj->scenePaths.push_back(scenePath);

    // SPACE: error handling if Scene schema is not found.
}

void Project::get_scene_paths(std::vector<std::filesystem::path>& scenePaths) const
{
    scenePaths.resize(mObj->scenePaths.size());

    for (size_t i = 0; i < mObj->scenePaths.size(); i++)
    {
        scenePaths[i] = (mObj->rootPath / mObj->scenePaths[i]).lexically_normal();
    }
}

ProjectSettings Project::get_settings()
{
    return mObj->settings;
}

} // namespace LD