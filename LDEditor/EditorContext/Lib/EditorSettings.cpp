#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UITheme.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

/// @brief Editor theme data, embeds an UI theme
struct EditorThemeObj
{
    UITheme uiTheme;
    float fontSize;
    float padding;
    float gap;
    float textRowHeight;
    float textLabelWidth;
    Color tabBGColor;
    Color gizmoColorAxisX;
    Color gizmoColorAxisY;
    Color gizmoColorAxisZ;
    Color gizmoHighlightColor;
    Color playButtonColor;
    Color stopButtonColor;
    Color errorColor;

    void initialize_default();
};

void EditorThemeObj::initialize_default()
{
    uiTheme = UITheme::get_default_theme();
    fontSize = 16.0f;
    padding = 5.0f;
    gap = 2.0f;
    textRowHeight = 20.0f;
    textLabelWidth = 120.0f;
    tabBGColor = 0x202225FF;
    gizmoColorAxisX = Vec4(0.9f, 0.1f, 0.1f, 0.8f);
    gizmoColorAxisY = Vec4(0.1f, 0.9f, 0.1f, 0.8f);
    gizmoColorAxisZ = Vec4(0.1f, 0.1f, 0.9f, 0.8f);
    gizmoHighlightColor = 0xFFA000E0;
    playButtonColor = 0x89F336FF;
    stopButtonColor = 0xFF6347FF;
    errorColor = 0xFF5374FF;
}

UITheme EditorTheme::get_ui_theme()
{
    return mObj->uiTheme;
}

Color EditorTheme::get_play_button_color() const
{
    return mObj->playButtonColor;
}

Color EditorTheme::get_stop_button_color() const
{
    return mObj->stopButtonColor;
}

float EditorTheme::get_font_size() const
{
    return mObj->fontSize;
}

float EditorTheme::get_padding() const
{
    return mObj->padding;
}

float EditorTheme::get_text_row_height() const
{
    return mObj->textRowHeight;
}

float EditorTheme::get_text_label_width() const
{
    return mObj->textLabelWidth;
}

void EditorTheme::get_tab_background_color(Color& bg) const
{
    bg = mObj->tabBGColor;
}

void EditorTheme::get_gizmo_colors(Color& axisX, Color& axisY, Color& axisZ) const
{
    axisX = mObj->gizmoColorAxisX;
    axisY = mObj->gizmoColorAxisY;
    axisZ = mObj->gizmoColorAxisZ;
}

void EditorTheme::get_gizmo_highlight_color(Color& hl) const
{
    hl = mObj->gizmoHighlightColor;
}

void EditorTheme::get_error_color(Color& err) const
{
    err = mObj->errorColor;
}

UILayoutInfo EditorTheme::make_vbox_layout(float* childGap, float* childPad) const
{
    float gap = childGap ? *childGap : mObj->gap;
    float pad = childPad ? *childPad : mObj->padding;

    UILayoutInfo layoutI{};
    layoutI.childGap = gap;
    layoutI.childPadding = {pad, pad, pad, pad};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    return layoutI;
}

UILayoutInfo EditorTheme::make_hbox_layout(float* childGap, float* childPad) const
{
    float gap = childGap ? *childGap : mObj->gap;
    float pad = childPad ? *childPad : mObj->padding;

    UILayoutInfo layoutI{};
    layoutI.childGap = gap;
    layoutI.childPadding = {pad, pad, pad, pad};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    return layoutI;
}

UILayoutInfo EditorTheme::make_text_label_layout() const
{
    const float textLabelWidth = get_text_label_width();
    const float textRowHeight = get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = 5.0f;
    layoutI.sizeX = UISize::fixed(textLabelWidth);
    layoutI.sizeY = UISize::fixed(textRowHeight);

    return layoutI;
}

/// @brief Editor settings registry
struct EditorSettingsObj
{
    EditorThemeObj themeObj;

    void initialize_default();
};

void EditorSettingsObj::initialize_default()
{
    themeObj.initialize_default();
}

EditorSettings EditorSettings::create()
{
    auto* obj = (EditorSettingsObj*)heap_malloc(sizeof(EditorSettingsObj), MEMORY_USAGE_MISC);

    obj->initialize_default();

    return {obj};
}

void EditorSettings::destroy(EditorSettings settings)
{
    EditorSettingsObj* obj = settings;

    heap_free(obj);
}

EditorTheme EditorSettings::get_theme()
{
    return EditorTheme(&mObj->themeObj);
}

} // namespace LD