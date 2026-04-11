#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/MouseValue.h>
#include <LudensEditor/EditorWidget/EUIText.h>

#include "EUI.h"

namespace LD {

bool eui_text(EUITextStorage* storage, const char* label, float height, Rect* outRect)
{
    bool isPressed = false;
    EditorTheme theme = eui_get_theme();
    MouseValue mouseVal;
    Vec2 mousePos;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(height);
    layoutI.childPadding = UIPadding::left_right(4.0f, 4.0f);
    UIPanelStorage* panel = ui_push_panel(&storage->panel);
    panel->color = 0;
    panel->radius = storage->radius;
    if (ui_top_is_hovered())
        storage->panel.color = theme.get_ui_theme().get_surface_color_lifted();

    if (outRect)
        ui_top_get_rect(*outRect);

    ui_top_layout(layoutI);
    {
        layoutI.sizeX = UISize::wrap();
        layoutI.sizeY = UISize::fixed(height);
        ui_push_text(&storage->text, label);
        ui_top_layout(layoutI);

        if (ui_top_mouse_down(mouseVal, mousePos))
            isPressed = true;

        ui_pop();
    }
    ui_pop();

    return isPressed;
}

} // namespace LD