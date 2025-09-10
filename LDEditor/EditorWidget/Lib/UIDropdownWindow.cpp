#include <LudensEditor/EditorWidget/UIDropdownWindow.h>
#include <vector>

namespace LD {

/// @brief Dropdown window implementation.
struct UIDropdownWindowObj
{
    struct Option
    {
        UITextWidget textW;
        int index;
    };

    UIWindow window;
    EditorTheme theme;
    UIDropdownWindowCallback callback;
    void* user;
    std::vector<Option> options;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_option_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
};

void UIDropdownWindowObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIDropdownWindowObj& self = *(UIDropdownWindowObj*)widget.get_user();
    UITheme uiTheme = self.theme.get_ui_theme();

    Rect rect = widget.get_rect();
    Color color = uiTheme.get_background_color();
    renderer.draw_rect(rect, color);
}

void UIDropdownWindowObj::on_option_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    UIDropdownWindowObj& self = *(UIDropdownWindowObj*)widget.get_user();

    if (!self.callback || event != UI_MOUSE_DOWN)
        return;

    for (Option& opt : self.options)
    {
        if (opt.textW.unwrap() == widget.unwrap())
        {
            bool shouldHide = self.callback(opt.index, widget.get_rect(), self.user);

            if (shouldHide)
                self.window.hide();

            return;
        }
    }
}

UIDropdownWindow UIDropdownWindow::create(const UIDropdownWindowInfo& info)
{
    auto* obj = heap_new<UIDropdownWindowObj>(MEMORY_USAGE_UI);
    obj->theme = info.theme;
    obj->user = info.user;
    obj->callback = info.callback;

    UIContext ctx = info.context;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childPadding = { 5, 5, 5, 5 };
    UIWindowInfo windowI{};
    windowI.defaultMouseControls = false;
    windowI.drawWithScissor = false;
    windowI.name = "dropdown";
    obj->window = ctx.add_window(layoutI, windowI, obj);
    obj->window.set_on_draw(&UIDropdownWindowObj::on_draw);

    return UIDropdownWindow(obj);
}

void UIDropdownWindow::destroy(UIDropdownWindow dropdown)
{
    UIDropdownWindowObj* obj = dropdown.unwrap();

    heap_delete<UIDropdownWindowObj>(obj);
}

void UIDropdownWindow::add_option(const char* text, int optionIndex)
{
    UINode node = mObj->window.node();

    float fontSize;
    mObj->theme.get_font_size(fontSize);

    UITextWidgetInfo textWI{};
    textWI.cstr = text;
    textWI.fontSize = fontSize;
    textWI.hoverHL = true;
    UITextWidget textW = node.add_text({}, textWI, mObj);
    textW.set_on_mouse(&UIDropdownWindowObj::on_option_mouse);

    mObj->options.push_back({textW, optionIndex});
}

UIWindow UIDropdownWindow::get_native()
{
    return mObj->window;
}

void UIDropdownWindow::set_callback(UIDropdownWindowCallback cb)
{
    mObj->callback = cb;
}

} // namespace LD