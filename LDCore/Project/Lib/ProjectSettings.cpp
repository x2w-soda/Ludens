#include "ProjectSettingsDefault.h"
#include <Ludens/Project/ProjectSettings.h>
#include <Ludens/System/Memory.h>

namespace LD {

struct ProjectStartupSettingsObj
{
    uint32_t windowWidth = DEFAULT_STARTUP_WINDOW_WIDTH;
    uint32_t windowHeight = DEFAULT_STARTUP_WINDOW_HEIGHT;
    std::string windowName = DEFAULT_STARTUP_WINDOW_NAME;
    std::string defaultScenePath = DEFAULT_STARTUP_DEFAULT_SCENE_PATH;
};

/// @brief Project settings implementation.
struct ProjectSettingsObj
{
    ProjectStartupSettingsObj startup;
};

uint32_t ProjectStartupSettings::get_window_width()
{
    return mObj->startup.windowWidth;
}

void ProjectStartupSettings::set_window_width(uint32_t width)
{
    mObj->startup.windowWidth = width;
}

uint32_t ProjectStartupSettings::get_window_height()
{
    return mObj->startup.windowHeight;
}

void ProjectStartupSettings::set_window_height(uint32_t height)
{
    mObj->startup.windowHeight = height;
}

std::string ProjectStartupSettings::get_window_name()
{
    return mObj->startup.windowName;
}

void ProjectStartupSettings::set_window_name(const std::string& name)
{
    mObj->startup.windowName = name;
}

std::string ProjectStartupSettings::get_default_scene_path()
{
    return mObj->startup.defaultScenePath;
}

void ProjectStartupSettings::set_default_scene_path(const std::string& scenePath)
{
    mObj->startup.defaultScenePath = scenePath;
}

ProjectSettings ProjectSettings::create()
{
    ProjectSettingsObj* obj = heap_new<ProjectSettingsObj>(MEMORY_USAGE_MISC);

    return ProjectSettings(obj);
}

void ProjectSettings::destroy(ProjectSettings settings)
{
    ProjectSettingsObj* obj = settings.unwrap();

    heap_delete<ProjectSettingsObj>(obj);
}

ProjectStartupSettings ProjectSettings::get_startup_settings()
{
    return ProjectStartupSettings(mObj);
}

} // namespace LD