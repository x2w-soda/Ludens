#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/UIListMenuWidget.h>

namespace LD {

int eui_list_menu(EditorTheme theme, int optionCount, const char** options)
{
    int index = -1;
    ui_push_panel();
    ui_top_layout(theme.make_vbox_layout(2.0f));

    MouseButton btn;

    UILayoutInfo optionPanelLayout{};
    optionPanelLayout.sizeX = UISize::fit();
    optionPanelLayout.sizeY = UISize::fixed(theme.get_text_row_height());

    for (int i = 0; i < optionCount; i++)
    {
        ui_push_panel();

        ui_top_layout(optionPanelLayout);

        Color color = theme.get_ui_theme().get_background_color();
        if (ui_top_is_hovered())
            color = Color::lift(color, 0.2f);

        ui_panel_color(color);

        ui_push_text(options[i]);
        if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
            index = i;
        ui_pop();

        ui_pop();
    }

    ui_pop();
    return index;
}

} // namespace LD