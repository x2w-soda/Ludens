#pragma once

#include <Ludens/Header/Math/Rect.h>

namespace LD {

enum EditorIcon
{
    EDITOR_ICON_ADD_FOLDER,
    EDITOR_ICON_ANIMATED_SPRITE2D_COMPONENT,
    EDITOR_ICON_ARROW_UP,
    EDITOR_ICON_AUDIO_SOURCE_COMPONENT,
    EDITOR_ICON_CAMERA_COMPONENT,
    EDITOR_ICON_CLOSE,
    EDITOR_ICON_CONSOLE_WINDOW,
    EDITOR_ICON_FILE,
    EDITOR_ICON_FOLDER,
    EDITOR_ICON_INSPECTOR_WINDOW,
    EDITOR_ICON_MESH_COMPONENT,
    EDITOR_ICON_OFFSET,
    EDITOR_ICON_OUTLINER_WINDOW,
    EDITOR_ICON_PAUSE,
    EDITOR_ICON_PLAY,
    EDITOR_ICON_REFRESH,
    EDITOR_ICON_ROTATE,
    EDITOR_ICON_SCALE,
    EDITOR_ICON_SCREEN_UI_COMPONENT,
    EDITOR_ICON_SCRIPT,
    EDITOR_ICON_SKELETAL_MESH_COMPONENT,
    EDITOR_ICON_SPRITE2D_COMPONENT,
    EDITOR_ICON_TILEMAP_COMPONENT,
    EDITOR_ICON_TRANSFORM_COMPONENT,
    EDITOR_ICON_VIEWPORT_WINDOW,
    EDITOR_ICON_ENUM_LAST,
};

enum ComponentType;

/// @brief Early definition of an icon texture atlas for the Editor.
///        Currently we are using an atlas of fixed-sized icons, but
///        we may switch to using SVGs for individual icons as well.
struct EditorIconAtlas
{
    static EditorIcon get_component_icon(const ComponentType& type);

    static Rect get_component_icon_rect(const ComponentType& type);

    /// @brief Get icon area in the atlas.
    static Rect get_icon_rect(EditorIcon icon);

    /// @brief Get icon UV area in the atlas.
    static Rect get_icon_rect_uv(EditorIcon icon);
};

} // namespace LD