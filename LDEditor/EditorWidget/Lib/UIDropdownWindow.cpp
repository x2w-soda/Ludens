#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIAnimation.h>
#include <LudensEditor/EditorWidget/UIDropdownWindow.h>
#include <vector>

#define ANIM_DURATION 0.14f

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
    UIOpacityAnimation opacityA;

    void show();
    void hide();

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_update(UIWidget widget, float delta);
    static void on_option_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
};

void UIDropdownWindowObj::show()
{
    opacityA.showing(ANIM_DURATION);

    window.show();
    window.block_input();
}

void UIDropdownWindowObj::hide()
{
    opacityA.hiding(ANIM_DURATION);

    window.block_input();
}

void UIDropdownWindowObj::on_update(UIWidget widget, float delta)
{
    UIDropdownWindowObj& self = *(UIDropdownWindowObj*)widget.get_user();

    bool isHiding = self.opacityA.is_hiding();
    bool animEnded = self.opacityA.update(delta);
    Color mask = self.opacityA.get_color_mask();

    if (animEnded)
    {
        self.window.unblock_input();

        if (isHiding)
            self.window.hide();
    }

    // dropdown window will be rendered with animated opacity.
    self.window.set_color_mask(mask);
}

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
    LD_ASSERT(info.layer);

    auto* obj = heap_new<UIDropdownWindowObj>(MEMORY_USAGE_UI);
    obj->theme = info.theme;
    obj->user = info.user;
    obj->callback = info.callback;

    UIContext ctx = info.context;
    float pad = obj->theme.get_padding();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childPadding = {pad, pad, pad, pad};
    UIWindowInfo windowI{};
    windowI.defaultMouseControls = false;
    windowI.drawWithScissor = false;
    windowI.name = "dropdown";
    windowI.layer = info.layer;
    windowI.hidden = true;
    obj->window = ctx.add_window(layoutI, windowI, obj);
    obj->window.set_on_draw(&UIDropdownWindowObj::on_draw);
    obj->window.set_on_update(&UIDropdownWindowObj::on_update);

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
    float fontSize = mObj->theme.get_font_size();

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

void UIDropdownWindow::set_pos(const Vec2& pos)
{
    mObj->window.set_pos(pos);
}

void UIDropdownWindow::show()
{
    mObj->show();
}

void UIDropdownWindow::hide()
{
    mObj->hide();
}

void UIDropdownWindow::draw(ScreenRenderComponent renderer)
{
    mObj->window.draw(renderer);
}

} // namespace LD