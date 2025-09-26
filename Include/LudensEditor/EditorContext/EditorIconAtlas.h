#pragma once

#include <Ludens/Header/Math/Rect.h>

namespace LD {

/// @brief Early definition of icons for the Editor.
///        These names correspond to Google Material Icons.
enum class EditorIcon
{
    Folder,
    Description,
    Close,
    Remove,
    Transform,
    Refresh,
    LinearScale,
    PlayArrow,
    Code,
    ENUM_COUNT,
};

/// @brief Early definition of an icon texture atlas for the Editor.
///        Currently we are using an atlas of fixed-sized icons, but
///        we may switch to using SVGs for individual icons as well.
struct EditorIconAtlas
{
    /// @brief Get icon area in the atlas.
    static Rect get_icon_rect(EditorIcon icon);

    /// @brief Get icon UV area in the atlas.
    static Rect get_icon_rect_uv(EditorIcon icon);
};

} // namespace LD