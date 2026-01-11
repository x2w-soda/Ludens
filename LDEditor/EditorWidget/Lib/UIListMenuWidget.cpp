#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/UIListMenuWidget.h>

namespace LD {

int eui_list_menu(EditorTheme theme, int optionCount, const char** options)
{
    int index = -1;
    ui_push_panel();
    ui_top_layout(theme.make_vbox_layout());

    MouseButton btn;

    for (int i = 0; i < optionCount; i++)
    {
        ui_push_text(options[i]);
        if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
            index = i;
        ui_pop();
    }

    ui_pop();
    return index;
}

} // namespace LD