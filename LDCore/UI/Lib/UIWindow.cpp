#include "UIObj.h"
#include <Ludens/Application/Input.h>
#include <Ludens/UI/UIWindow.h>
#include <vector>

namespace LD {

UIWindowObj::UIWindowObj()
    : UIWidgetObj{}
{
}

UIWindowObj::~UIWindowObj()
{
    while (!widgets.empty())
        ctx->free_widget(widgets.front());
}

void UIWindowObj::update(float delta)
{
    for (UIWidgetObj* widget : widgets)
    {
        if (widget->cb.onUpdate)
            widget->cb.onUpdate({widget}, delta);
    }
}

void UIWindowObj::draw_widget_subtree(UIWidgetObj* widget, ScreenRenderComponent renderer)
{
    if (!widget || (widget->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
        return;

    widget->draw(renderer);

    for (UIWidgetObj* w = widget->child; w; w = w->next)
    {
        draw_widget_subtree(w, renderer);
    }
}

void UIWindowObj::on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    UIWindow window = (UIWindow)widget;
    UIWindowObj* obj = (UIWindowObj*)window.unwrap();
    Rect rect = widget.get_rect();

    if (begin)
    {
        obj->dragResize = btn == MOUSE_BUTTON_RIGHT; // right button to resize, left button to reposition
        obj->dragOffset = dragPos - rect.get_pos();  // fixed drag offset
        obj->dragBeginPos = dragPos;
        obj->dragBeginSize = rect.get_size();
    }

    if (obj->dragResize)
    {
        Vec2 delta = dragPos - obj->dragBeginPos;
        window.set_size(obj->dragBeginSize + delta);
    }
    else
        window.set_pos(dragPos - obj->dragOffset);
}

void UIWindow::raise()
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    obj->ctx->raise_window(obj);
}

void UIWindow::draw(ScreenRenderComponent renderer)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    if (obj->flags & UI_WIDGET_FLAG_HIDDEN_BIT)
        return;

    bool useScissor = obj->drawWithScissor;
    if (useScissor)
        renderer.push_scissor(obj->layout.rect);

    bool useColorMask = obj->colorMask.has_value();
    if (useColorMask)
        renderer.push_color_mask(obj->colorMask.value());

    UIWindowObj::draw_widget_subtree(obj, renderer);

    if (useColorMask)
        renderer.pop_color_mask();

    if (useScissor)
        renderer.pop_scissor();
}

void UIWindow::layout()
{
    ui_layout(mObj);
}

void UIWindow::set_pos(const Vec2& pos)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    obj->layout.rect.x = pos.x;
    obj->layout.rect.y = pos.y;
}

void UIWindow::set_size(const Vec2& size)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    mObj->layout.info.sizeX = UISize::fixed(size.x);
    mObj->layout.info.sizeY = UISize::fixed(size.y);
}

void UIWindow::set_rect(const Rect& rect)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    obj->layout.rect.x = rect.x;
    obj->layout.rect.y = rect.y;
    mObj->layout.info.sizeX = UISize::fixed(rect.w);
    mObj->layout.info.sizeY = UISize::fixed(rect.h);
}

void UIWindow::set_color_mask(Color mask)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    obj->colorMask = mask;
}

void UIWindow::get_widgets(std::vector<UIWidget>& widgets)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    widgets.resize(obj->widgets.size());

    for (size_t i = 0; i < widgets.size(); i++)
        widgets[i] = {obj->widgets[i]};
}

Rect UIWindow::get_rect() const
{
    return mObj->layout.rect;
}

std::string UIWindow::get_name() const
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    return obj->name;
}

} // namespace LD