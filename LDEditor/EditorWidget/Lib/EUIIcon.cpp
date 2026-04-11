#include <Ludens/Header/MouseValue.h>
#include <LudensEditor/EditorWidget/EUIIcon.h>

#include "EUI.h"

namespace LD {

bool eui_icon(EUIIconStorage& storage, EditorIcon iconType, float iconSize)
{
    EditorContext ctx = eui_get_context();

    UIImageStorage* image = ui_push_image(&storage.image, iconSize, iconSize);
    image->image = ctx.get_editor_icon_atlas();
    image->rect = EditorIconAtlas::get_icon_rect(iconType);
    ui_top_user(&storage);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto& storage = *(EUIIconStorage*)user;
        if (storage.bgColor != 0)
            renderer.draw_rect_rounded(widget.get_rect(), storage.bgColor, 0.35f);
        UIImageWidget::on_draw(widget, renderer);
    });

    bool isPressed = false;
    MouseValue mouseVal;
    Vec2 mousePos;
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
        isPressed = true;
    ui_pop();

    return isPressed;
}

} // namespace LD