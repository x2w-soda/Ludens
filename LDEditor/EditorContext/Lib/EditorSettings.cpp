#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UITheme.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

/// @brief Editor theme data, embeds an UI theme
struct EditorThemeObj
{
    UITheme uiTheme = UITheme::get_default_theme();
    float fontSize = 16.0f;
    float childPad = 5.0f;
    float childPadL = 10.0f;
    float childGap = 2.0f;
    float childGapL = 8.0f;
    float textRowHeight = 20.0f;
    float textLabelWidth = 120.0f;
    Color tabBGColor = 0x202225FF;
    Color gizmoColorAxisX = Vec4(0.9f, 0.1f, 0.1f, 0.8f);
    Color gizmoColorAxisY = Vec4(0.1f, 0.9f, 0.1f, 0.8f);
    Color gizmoColorAxisZ = Vec4(0.1f, 0.1f, 0.9f, 0.8f);
    Color gizmoHighlightColor = 0xFFA000E0;
    Color playButtonColor = 0x89F336FF;
    Color stopButtonColor = 0xFF6347FF;
    Color errorColor = 0xFF5374FF;
};

UILayoutInfo EditorDocumentTheme::get_scroll_layout()
{
    UILayoutInfo layoutI{};
    layoutI.childPadding = UIPadding(45.0f);
    layoutI.childGap = 16.0f;
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();

    return layoutI;
}

UILayoutInfo EditorDocumentTheme::get_heading_layout(int level, float& outFontSize)
{
    float baseFontSize = mObj->fontSize;
    float topPad = 0.0f;

    switch (level)
    {
    case 1:
        outFontSize = baseFontSize * 2.0f;
        topPad = baseFontSize * 0.5f;
        break;
    case 2:
        outFontSize = baseFontSize * 1.5f;
        topPad = baseFontSize * 0.5f;
        break;
    case 3:
        outFontSize = baseFontSize * 1.25f;
        topPad = baseFontSize * 0.5f;
        break;
    case 4:
        outFontSize = baseFontSize;
        topPad = baseFontSize * 0.25f;
        break;
    case 5:
    case 6:
        outFontSize = baseFontSize;
        topPad = 0.0f;
        break;
    default:
        LD_UNREACHABLE;
    }

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childPadding = UIPadding::top_bottom(topPad, 0.0f);

    return layoutI;
}

UILayoutInfo EditorDocumentTheme::get_paragraph_layout()
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = UIPadding(0.0f);
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();

    return layoutI;
}

UILayoutInfo EditorDocumentTheme::get_code_block_layout()
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = UIPadding::left_right(5.0f, 5.0f);
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();

    return layoutI;
}

EditorDocumentTheme EditorTheme::get_document_theme()
{
    return EditorDocumentTheme(mObj);
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

float EditorTheme::get_child_gap() const
{
    return mObj->childGap;
}

float EditorTheme::get_child_gap_large() const
{
    return mObj->childGapL;
}

float EditorTheme::get_child_pad() const
{
    return mObj->childPad;
}

float EditorTheme::get_child_pad_large() const
{
    return mObj->childPadL;
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
    float gap = childGap ? *childGap : mObj->childGap;
    float pad = childPad ? *childPad : mObj->childPad;

    UILayoutInfo layoutI{};
    layoutI.childGap = gap;
    layoutI.childPadding = UIPadding(pad);
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    return layoutI;
}

UILayoutInfo EditorTheme::make_vbox_layout_fixed(const Vec2& extent) const
{
    UILayoutInfo layoutI{};
    layoutI.childGap = mObj->childGap;
    layoutI.childPadding = UIPadding(mObj->childPad);
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fixed(extent.x);
    layoutI.sizeY = UISize::fixed(extent.y);
    return layoutI;
}

UILayoutInfo EditorTheme::make_hbox_layout(float* childGap, float* childPad) const
{
    float gap = childGap ? *childGap : mObj->childGap;
    float pad = childPad ? *childPad : mObj->childPad;

    UILayoutInfo layoutI{};
    layoutI.childGap = gap;
    layoutI.childPadding = UIPadding(pad);
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
    layoutI.childPadding = UIPadding(mObj->childPad);
    layoutI.childGap = mObj->childGap;
    layoutI.sizeX = UISize::fixed(textLabelWidth);
    layoutI.sizeY = UISize::fixed(textRowHeight);

    return layoutI;
}

UILayoutInfo EditorTheme::make_text_row_layout() const
{
    const float textRowHeight = get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = UIPadding(mObj->childPad);
    layoutI.childGap = mObj->childGap;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(textRowHeight);

    return layoutI;
}

/// @brief Editor settings registry
struct EditorSettingsObj
{
    EditorThemeObj themeObj;
};

EditorSettings EditorSettings::create()
{
    auto* obj = (EditorSettingsObj*)heap_malloc(sizeof(EditorSettingsObj), MEMORY_USAGE_MISC);

    obj->themeObj = {};

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