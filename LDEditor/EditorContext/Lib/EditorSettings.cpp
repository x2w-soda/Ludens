#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UITheme.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

/// @brief Editor theme data, embeds an UI theme
struct EditorThemeObj
{
    UIThemeInfo uiTheme;
    float fontSize;
    float padding;
    Color tabBGColor;
    Color gizmoColorAxisX;
    Color gizmoColorAxisY;
    Color gizmoColorAxisZ;
    Color gizmoHighlightColor;
    Color playButtonColor;
    Color stopButtonColor;

    void initialize_default();
};

void EditorThemeObj::initialize_default()
{
    uiTheme = UITheme::get_default_info();
    fontSize = 16.0f;
    padding = 5.0f;
    tabBGColor = 0x141516FF;
    gizmoColorAxisX = Vec4(0.9f, 0.1f, 0.1f, 0.8f);
    gizmoColorAxisY = Vec4(0.1f, 0.9f, 0.1f, 0.8f);
    gizmoColorAxisZ = Vec4(0.1f, 0.1f, 0.9f, 0.8f);
    gizmoHighlightColor = 0xFFA000E0;
    playButtonColor = 0x89F336FF;
    stopButtonColor = 0xFF6347FF;
}

UITheme EditorTheme::get_ui_theme()
{
    return UITheme(&mObj->uiTheme);
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

UILayoutInfo EditorTheme::make_vbox_layout() const
{
    float pad = mObj->padding;

    UILayoutInfo layoutI{};
    layoutI.childGap = 5.0f;
    layoutI.childPadding = {pad, pad, pad, pad};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childAxis = UI_AXIS_Y;
    return layoutI;
}

UILayoutInfo EditorTheme::make_hbox_layout() const
{
    float pad = mObj->padding;

    UILayoutInfo layoutI{};
    layoutI.childGap = 5.0f;
    layoutI.childPadding = {pad, pad, pad, pad};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childAxis = UI_AXIS_X;
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

EditorSettings EditorSettings::create_default()
{
    auto* obj = (EditorSettingsObj*)heap_malloc(sizeof(EditorSettingsObj), MEMORY_USAGE_MISC);

    obj->initialize_default();

    return {obj};
}

EditorSettings EditorSettings::create(JSONDocument doc)
{
    auto* obj = (EditorSettingsObj*)heap_malloc(sizeof(EditorSettingsObj), MEMORY_USAGE_MISC);

    // SPACE: load json fields

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