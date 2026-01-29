#include <Ludens/DSA/IDCounter.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Project/ProjectSettings.h>

#include <algorithm>

#include "ProjectSettingsDefault.h"

namespace LD {

/// @brief Project-wide source of truth for startup configuration.
struct ProjectStartupSettingsObj
{
    uint32_t windowWidth = DEFAULT_STARTUP_WINDOW_WIDTH;
    uint32_t windowHeight = DEFAULT_STARTUP_WINDOW_HEIGHT;
    std::string windowName = DEFAULT_STARTUP_WINDOW_NAME;
    std::string defaultScenePath = DEFAULT_STARTUP_DEFAULT_SCENE_PATH;
};

struct ProjectRenderingSettingsObj
{
    Vec4 clearColor = DEFAULT_RENDERING_CLEAR_COLOR;
};

/// @brief Project-wide source of truth for screen layers.
struct ProjectScreenLayerSettingsObj
{
    struct ProjectScreenLayerObj
    {
        ProjectScreenLayerID id;
        std::string name;
    };

    IDCounter<ProjectScreenLayerID> idCounter;
    Vector<ProjectScreenLayerObj> order;
};

/// @brief Project settings implementation.
struct ProjectSettingsObj
{
    ProjectStartupSettingsObj startup;
    ProjectRenderingSettingsObj rendering;
    ProjectScreenLayerSettingsObj screenLayer;
};

//
// Startup Settings
//

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

Vec4 ProjectRenderingSettings::get_clear_color()
{
    return mObj->rendering.clearColor;
}

void ProjectRenderingSettings::set_clear_color(const Vec4& color)
{
    mObj->rendering.clearColor = color;
}

//
// Screen Layer Settings
//

ProjectScreenLayerID ProjectScreenLayerSettings::create_layer(const char* name)
{
    ProjectScreenLayerID id = mObj->screenLayer.idCounter.get_id();
    LD_ASSERT(id != 0);

    mObj->screenLayer.order.push_back({.id = id, .name = name});

    return id;
}

void ProjectScreenLayerSettings::destroy_layer(ProjectScreenLayerID id)
{
    std::erase_if(mObj->screenLayer.order, [&](const auto& obj) { return obj.id == id; });
}

void ProjectScreenLayerSettings::rename_layer(ProjectScreenLayerID id, const char* name)
{
    for (auto& obj : mObj->screenLayer.order)
    {
        if (obj.id == id)
        {
            obj.name = std::string(name);
            return;
        }
    }
}

void ProjectScreenLayerSettings::rotate_layer(ProjectScreenLayerID id, int newIndex)
{
    int layerCount = (int)mObj->screenLayer.order.size();
    int oldIndex;

    for (oldIndex = 0; oldIndex < layerCount; oldIndex++)
    {
        if (mObj->screenLayer.order[oldIndex].id == id)
            break;
    }

    if (oldIndex >= layerCount || oldIndex == newIndex)
        return;

    const auto it = mObj->screenLayer.order.begin();

    if (newIndex < oldIndex)
        std::rotate(it + newIndex, it + oldIndex, it + oldIndex + 1);
    else
        std::rotate(it + oldIndex, it + oldIndex + 1, it + newIndex + 1);
}

Vector<ProjectScreenLayer> ProjectScreenLayerSettings::get_layers()
{
    Vector<ProjectScreenLayer> layers(mObj->screenLayer.order.size());

    for (size_t i = 0; i < layers.size(); i++)
    {
        layers[i].id = mObj->screenLayer.order[i].id;
        layers[i].name = mObj->screenLayer.order[i].name;
    }

    return layers;
}

//
// Public API
//

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

ProjectRenderingSettings ProjectSettings::get_rendering_settings()
{
    return ProjectRenderingSettings(mObj);
}

ProjectScreenLayerSettings ProjectSettings::get_screen_layer_settings()
{
    return ProjectScreenLayerSettings(mObj);
}

} // namespace LD