#include <Ludens/DSA/IDCounter.h>
#include <Ludens/DSA/String.h>
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
    String windowName = DEFAULT_STARTUP_WINDOW_NAME;
    SUID defaultSceneID = (SUID)0;
};

struct ProjectRenderingSettingsObj
{
    Vec4 clearColor = ProjectRenderingSettings::get_default_clear_color();
};

/// @brief Project-wide source of truth for screen layers.
struct ProjectScreenLayerSettingsObj
{
    struct ProjectScreenLayerObj
    {
        SUID id;
        String name;
    };

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

String ProjectStartupSettings::get_window_name()
{
    return mObj->startup.windowName;
}

void ProjectStartupSettings::set_window_name(View name)
{
    mObj->startup.windowName = name;
}

SUID ProjectStartupSettings::get_default_scene_id()
{
    return mObj->startup.defaultSceneID;
}

void ProjectStartupSettings::set_default_scene_id(SUID id)
{
    mObj->startup.defaultSceneID = id;
}

Vec4 ProjectRenderingSettings::get_clear_color()
{
    return mObj->rendering.clearColor;
}

void ProjectRenderingSettings::set_clear_color(const Vec4& color)
{
    mObj->rendering.clearColor = color;
}

Vec4 ProjectRenderingSettings::get_default_clear_color()
{
    return DEFAULT_RENDERING_CLEAR_COLOR;
}

//
// Screen Layer Settings
//

SUID ProjectScreenLayerSettings::create_layer(SUIDRegistry idReg, View name)
{
    SUID id = idReg.get_suid(SERIAL_TYPE_SCREEN_LAYER);

    mObj->screenLayer.order.emplace_back(id, String(name));

    return id;
}

bool ProjectScreenLayerSettings::create_layer(SUIDRegistry idReg, SUID id, View name)
{
    if (id.type() != SERIAL_TYPE_SCREEN_LAYER || !idReg.try_get_suid(id))
        return false;

    mObj->screenLayer.order.emplace_back(id, String(name));

    return true;
}

void ProjectScreenLayerSettings::destroy_layer(SUIDRegistry idReg, SUID id)
{
    std::erase_if(mObj->screenLayer.order, [&](const auto& obj) { return obj.id == id; });
    idReg.free_suid(id);
}

void ProjectScreenLayerSettings::rename_layer(SUID id, View name)
{
    for (auto& obj : mObj->screenLayer.order)
    {
        if (obj.id == id)
        {
            obj.name = name;
            return;
        }
    }
}

void ProjectScreenLayerSettings::rotate_layer(SUID id, int newIndex)
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

ProjectStartupSettings ProjectSettings::startup_settings()
{
    return ProjectStartupSettings(mObj);
}

ProjectRenderingSettings ProjectSettings::rendering_settings()
{
    return ProjectRenderingSettings(mObj);
}

ProjectScreenLayerSettings ProjectSettings::screen_layer_settings()
{
    return ProjectScreenLayerSettings(mObj);
}

} // namespace LD