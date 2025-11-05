#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIWidget.h>
#include <LudensEditor/EditorWidget/UIDraw.h>

namespace LD {

void eui_draw_text_with_bg(UIWidget widget, ScreenRenderComponent renderer, void* user)
{
    LD_ASSERT(widget.get_type() == UI_WIDGET_TEXT);

    Color bgColor = widget.get_theme().get_field_color();
    renderer.draw_rect(widget.get_rect(), bgColor);

    UITextWidget::on_draw(widget, renderer);
}

} // namespace LD