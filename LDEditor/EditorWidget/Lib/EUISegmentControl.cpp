#include <LudensEditor/EditorWidget/EUISegmentControl.h>

#include "EUI.h"

namespace LD {

int eui_segment_control(EUISegmentControlStorage& storage, const char* label, const char* options[3], float optionWidth)
{
    EditorTheme theme = eui_get_theme();

    int response = -1;

    push_prop_hbox();
    {
        ui_push_text(nullptr, label);
        ui_top_layout(theme.make_text_label_layout());
        ui_pop();

        UILayoutInfo layoutI = theme.make_hbox_layout();

        for (int i = 0; i < 3; i++)
        {
            ui_push_button(&storage.buttons[i], options[i]);
            if (optionWidth > 0.0f)
                ui_top_layout_size_x(UISize::fixed(optionWidth));
            if (ui_button_is_pressed())
                response = i;
            ui_pop();
        }
    }
    pop_prop_hbox();

    return response;
}

} // namespace LD