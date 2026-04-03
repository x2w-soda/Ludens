#include <Ludens/Header/MouseValue.h>
#include <LudensEditor/EditorWidget/EUIRow.h>

#include "EUI.h"

namespace LD {

bool eui_row_label_text_edit(const char* label, UITextEditStorage* edit, std::string& outText)
{
    EditorTheme theme = eui_get_theme();
    UILayoutInfo layoutI = theme.make_text_row_layout();

    ui_push_text(nullptr, label);
    ui_top_layout(layoutI);
    ui_pop();

    ui_push_text_edit(edit);
    bool hasChanged = ui_text_edit_changed(outText);
    ui_top_layout(layoutI);
    ui_pop();

    return hasChanged;
}

bool eui_row_label(int rowIndex, const char* label, bool isHighlighted)
{
    EditorTheme theme = eui_get_theme();
    const float textRowHeight = theme.get_text_row_height();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = theme.get_child_gap();
    layoutI.childPadding.left = 10.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(textRowHeight);

    UIPanelStorage* panelS = ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    Color panelColor = theme.get_ui_theme().get_surface_color();
    if (isHighlighted)
        panelColor = theme.get_ui_theme().get_selection_color();

    if (rowIndex % 2)
        panelColor = Color::lift(panelColor, 0.02f);

    if (ui_top_is_hovered())
        panelColor = Color::lift(panelColor, 0.06f);

    panelS->color = panelColor;

    bool isSelected = false;

    Vec2 mousePos;
    MouseValue mouseVal;
    if (ui_top_mouse_down(mouseVal, mousePos))
        isSelected = true;

    ui_push_text(nullptr, label);
    if (ui_top_mouse_down(mouseVal, mousePos))
        isSelected = true;
    ui_pop();

    ui_pop();

    return isSelected;
}

int eui_row_btn_btn(const char* btnLeft, const char* btnRight)
{
    int hasPressed = 0;

    EditorTheme theme = eui_get_theme();
    const float textRowHeight = theme.get_text_row_height();
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(textRowHeight);
    layoutI.childAlignX = UI_ALIGN_END;
    layoutI.childGap = theme.get_child_gap_large();
    layoutI.childPadding = UIPadding(theme.get_child_pad());

    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    ui_push_button(nullptr, btnLeft);
    if (ui_button_is_pressed())
        hasPressed = 1;
    ui_pop();

    ui_push_button(nullptr, btnRight);
    if (ui_button_is_pressed())
        hasPressed = 2;
    ui_pop();

    ui_pop();
    return hasPressed;
}

void eui_push_row_scroll(UIScrollStorage* scroll)
{
    EditorTheme theme = eui_get_theme();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.childGap = theme.get_child_gap();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    scroll = ui_push_scroll(scroll);
    scroll->bgColor = theme.get_ui_theme().get_surface_color();
    ui_top_layout(layoutI);
}

void eui_pop_row_scroll()
{
    ui_pop();
}

} // namespace LD
