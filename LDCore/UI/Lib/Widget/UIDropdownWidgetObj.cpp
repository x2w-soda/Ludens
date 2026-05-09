#include <Ludens/DSA/HashMap.h>
#include <Ludens/UI/UIContext.h>

#include "UIDropdownWidgetObj.h"

#include "../UIContextObj.h"
#include "../UIWidgetMeta.h"
#include "../UIWidgetObj.h"

namespace LD {

#if 0
struct UIDropdownWindow
{
    UIID dropdownID;
    HashMap<UIID, int> optionIndices;

    static bool on_event(UIWidget widget, const UIEvent& event)
    {
        auto& self = *(UIDropdownWindow*)widget.get_user();

        if (event.type != UI_EVENT_MOUSE_DOWN)
            return false;

        const auto& it = self.optionIndices.find(widget.get_id());
        if (it == self.optionIndices.end())
            return false;

        int optionIndex = it->second;
        UIContext ctx(widget.get_context_obj());

        // UIContext safely resolves ID, UIDropdownWidget safely resolves option.
        return ctx.input_dropdown_option(self.dropdownID, optionIndex);
    }
};

void UIDropdownWidgetObj::build_dropdown_window()
{
    UIDropdownData& data = get_data();
    Rect rect = get_rect();
    Vec2 pos = rect.get_pos_tr();

    UIDropdownWindow* user = heap_new<UIDropdownWindow>(MEMORY_USAGE_UI);
    user->dropdownID = base->id;

    UIWindow window = base->ctx()->overlay.get_window(data.popupLevel, pos);
    window.set_user(user);
    window.remove_children();
    UIPanelWidget panelW = (UIPanelWidget)window.add_child(UI_WIDGET_PANEL, UILayoutInfo(UISize::fit(), UISize::fit(), UI_AXIS_Y), nullptr, user);
    UIPanelData* panelData = (UIPanelData*)panelW.get_data();
    panelData->color = 0x202020FF;

    for (size_t optionI = 0; optionI < data.options.size(); optionI++)
    {
        UITextWidget textW = (UITextWidget)panelW.add_child(UI_WIDGET_TEXT, UIWidget::get_default_layout(UI_WIDGET_TEXT), nullptr, user);
        textW.set_on_event(&UIDropdownWindow::on_event);
        UITextData* textData = (UITextData*)textW.get_data();
        textData->set_value(data.options[optionI]);

        user->optionIndices[textW.get_id()] = optionI;
    }
}
#endif

bool UIDropdownWidgetObj::set_option(int index)
{
    UIDropdownData& data = get_data();

    if (0 <= index && index < (int)data.options.size())
    {
        data.mOptionIndex = index;
        return true;
    }

    return false;
}

void UIDropdownWidgetObj::startup(UIWidgetObj* obj)
{
    UIDropdownWidgetObj& self = obj->U->dropdown;
    new (&self) UIDropdownWidgetObj();
    self.connect(obj);
}

void UIDropdownWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIDropdownWidgetObj& self = obj->U->dropdown;

    (&self)->~UIDropdownWidgetObj();
}

bool UIDropdownWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIDropdownWidgetObj& self = obj->U->dropdown;
    UIDropdownData& data = self.get_data();
    UIContextObj* ctx = obj->ctx();
    Rect rect = self.get_rect();

    if (event.type == UI_EVENT_MOUSE_DOWN)
    {
        if (data.onOpen)
            data.onOpen(UIWidget(obj));

        return true;
    }

    return false;
}

void UIDropdownWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIDropdownWidgetObj& self = obj->U->dropdown;
    UIDropdownData& data = self.get_data();
    Rect rect = self.get_rect();
    UIContextObj* ctx = obj->ctx();
    UITheme theme = ctx->theme;

    renderer.draw_rect(rect, theme.get_surface_color());

    if (0 <= data.mOptionIndex && data.mOptionIndex < (int)data.options.size())
    {
        const String& opt = data.options[data.mOptionIndex];
        UIFont font = ctx->get_font_from_hint(TEXT_SPAN_FONT_REGULAR);
        FontAtlas atlas = font.font_atlas();
        RImage image = font.image();

        Vec2 pos = rect.get_pos();
        View textView((const byte*)opt.data(), opt.size());
        renderer.draw_text(atlas, image, data.fontSize, pos, textView, theme.get_on_surface_color(), rect.w);
    }
}

bool UIDropdownWidget::set_option(int index)
{
    UIDropdownWidgetObj& self = mObj->U->dropdown;

    return self.set_option(index);
}

} // namespace LD