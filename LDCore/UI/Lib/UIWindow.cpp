#include "UIObj.h"
#include <Ludens/UI/UIWindow.h>
#include <Ludens/Window/Input.h>
#include <vector>

namespace LD {

UIWindowObj::UIWindowObj()
    : UIWidgetObj{}
{
}

UIWindowObj::~UIWindowObj()
{
    while (!widgets.empty())
        ctx()->free_widget(widgets.front());
}

Hash64 UIWindowObj::get_hash() const
{
    uint64_t hash = space->get_hash();
    hash_combine(hash, id);

    return hash;
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

    bool useScissor = (widget->flags & UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT);
    if (useScissor)
        renderer.push_scissor(widget->layout.rect);

    widget->draw(renderer);

    for (UIWidgetObj* w = widget->child; w; w = w->next)
    {
        draw_widget_subtree(w, renderer);
    }

    if (useScissor)
        renderer.pop_scissor();
}

void UIWindowObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    auto* obj = (UIWindowObj*)widget.unwrap();
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, obj->color);
}

void UIWindowObj::on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    auto* obj = (UIWindowObj*)widget.unwrap();
    UIWindow window(obj);
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

//
// Public API
//

UIWindow::UIWindow(UIWindowObj* obj)
{
    mObj = (UIWidgetObj*)obj;
}

void UIWindow::layout()
{
    ui_layout(mObj);
}

void UIWindow::render(ScreenRenderComponent& renderer)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    if (obj->flags & UI_WIDGET_FLAG_HIDDEN_BIT)
        return;

    bool useColorMask = obj->colorMask.has_value();
    if (useColorMask)
        renderer.push_color_mask(obj->colorMask.value());

    UIWindowObj::draw_widget_subtree(obj, renderer);

    if (useColorMask)
        renderer.pop_color_mask();
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

void UIWindow::set_color(Color bg)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    obj->color = bg;
}

void UIWindow::set_color_mask(Color mask)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    obj->colorMask = mask;
}

void UIWindow::get_widgets(Vector<UIWidget>& widgets)
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

Hash64 UIWindow::get_hash()
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    return obj->get_hash();
}

void UIWindow::set_on_resize(void (*onResize)(UIWindow window, const Vec2& size))
{
    UIWindowObj* obj = (UIWindowObj*)mObj;

    obj->onResize = onResize;
}

} // namespace LD