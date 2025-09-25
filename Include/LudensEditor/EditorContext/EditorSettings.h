#pragma once
#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Format/JSON.h>
#include <Ludens/UI/UITheme.h>

namespace LD {

/// @brief Editor theme determines the color scheme, typography, UI layouts.
struct EditorTheme : Handle<struct EditorThemeObj>
{
    UITheme get_ui_theme();
    Color get_play_button_color() const;
    Color get_stop_button_color() const;
    float get_font_size() const;
    float get_padding() const;
    void get_gizmo_colors(Color& axisX, Color& axisY, Color& axisZ) const;
    void get_gizmo_highlight_color(Color& hl) const;
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