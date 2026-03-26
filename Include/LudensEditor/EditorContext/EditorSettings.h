#pragma once
#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UITheme.h>

namespace LD {

struct EditorDocumentTheme : Handle<struct EditorThemeObj>
{
    UILayoutInfo get_scroll_layout();
    UILayoutInfo get_heading_layout(int level, float& outFontSize);
    UILayoutInfo get_paragraph_layout();
    UILayoutInfo get_code_block_layout();
};

/// @brief Editor theme determines the color scheme, typography, UI layouts.
struct EditorTheme : Handle<struct EditorThemeObj>
{
    EditorDocumentTheme get_document_theme();
    UITheme get_ui_theme();

    Color get_play_button_color() const;
    Color get_stop_button_color() const;
    float get_font_size() const;
    float get_padding() const;
    float get_text_row_height() const;
    float get_text_label_width() const;
    void get_tab_background_color(Color& bg) const;
    void get_gizmo_colors(Color& axisX, Color& axisY, Color& axisZ) const;
    void get_gizmo_highlight_color(Color& hl) const;
    void get_error_color(Color& err) const;

    UILayoutInfo make_vbox_layout(float* childGap = nullptr, float* childPad = nullptr) const;
    UILayoutInfo make_vbox_layout_fixed(const Vec2& extent) const;
    UILayoutInfo make_hbox_layout(float* childGap = nullptr, float* childPad = nullptr) const;
    UILayoutInfo make_text_label_layout() const;
};

/// @brief Global editor settings that apply to all projects
struct EditorSettings : Handle<struct EditorSettingsObj>
{
    static EditorSettings create();
    static void destroy(EditorSettings settings);

    /// @brief Get current editor theme
    EditorTheme get_theme();
};

} // namespace LD