#pragma once
#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Format/JSON.h>

namespace LD {

/// @brief Editor theme determines the color scheme, typography, UI layouts.
struct EditorTheme : Handle<struct EditorThemeObj>
{
    void get_font_size(float& fontSize);
    void get_background_color(Color& bg);
    void get_gizmo_colors(Color& axisX, Color& axisY, Color& axisZ);
    void get_gizmo_highlight_color(Color& hl);
};

/// @brief Global editor settings that apply to all projects
struct EditorSettings : Handle<struct EditorSettingsObj>
{
    static EditorSettings create_default();
    static EditorSettings create(JSONDocument doc);
    static void destroy(EditorSettings settings);

    /// @brief Get current editor theme
    EditorTheme get_theme();
};

} // namespace LD