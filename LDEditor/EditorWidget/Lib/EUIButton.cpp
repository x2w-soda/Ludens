#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/EUIButton.h>

#include "EUI.h"

namespace LD {

bool EUIButton::update(EditorIcon icon, const char* label, const UILayoutInfo* info)
{
    EditorContext ctx = eui_get_context();
    EditorTheme theme = ctx.get_theme();
    float height = theme.get_text_row_height();
    float pad = theme.get_child_pad();

    ui_push_button(&mButton);
    bool isClicked = ui_button_is_pressed();

    UILayoutInfo layoutI(UISize::fit(), UISize::fit(), UI_AXIS_X);
    layoutI.childGap = theme.get_child_gap();
    layoutI.childPadding = UIPadding::left_right(pad);
    if (info)
        layoutI = *info;
    ui_top_layout(layoutI);
    {
        // optional icon
        if (icon != EDITOR_ICON_ENUM_LAST)
        {
            mIcon.rect = EditorIconAtlas::get_icon_rect(icon);
            mIcon.image = ctx.get_editor_icon_atlas();
            ui_push_image(&mIcon, height, height);
            ui_pop();
        }

        ui_push_text(&mText, label);
        ui_pop();
    }
    ui_pop();

    return isClicked;
}

} // namespace LD