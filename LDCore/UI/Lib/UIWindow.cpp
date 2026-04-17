#include <Ludens/DSA/Vector.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIWindow.h>

#include "UIObj.h"
#include "UIWidgetMeta.h"

namespace LD {

UIWindowObj::UIWindowObj(const UILayoutInfo& layoutI, UIContextObj* ctx)
    : UIWidgetObj(UI_WIDGET_WINDOW, ctx, &layout, nullptr, nullptr, this, nullptr, nullptr), ctx(ctx)
{
    layout.info = layoutI;
    theme = ctx->theme;
    id = ctx->idRegistry.create();
}

UIWindowObj::~UIWindowObj()
{
    ctx->idRegistry.destroy(id);

    while (!widgets.empty())
        ctx->free_widget_obj(widgets.front());
}

Hash64 UIWindowObj::get_hash() const
{
    uint64_t hash = space->get_hash();
    hash_combine(hash, id);

    return hash;
}

void UIWindowObj::update_widgets(float delta)
{
    for (UIWidgetObj* widget : widgets)
        widget_on_update(widget, delta);
}

void UIWindowObj::draw_widget_subtree(UIWidgetObj* widget, ScreenRenderComponent renderer)
{
    if (!widget || (widget->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
        return;

    bool useScissor = (widget->flags & UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT);
    if (useScissor)
        renderer.push_scissor(widget->L->rect);

    widget->draw(renderer);

    for (UIWidgetObj* w = widget->child; w; w = w->next)
    {
        draw_widget_subtree(w, renderer);
    }

    if (useScissor)
        renderer.pop_scissor();
}

void UIWindowObj::on_draw(UIWidgetObj* widget, ScreenRenderComponent renderer)
{
    auto* obj = (UIWindowObj*)widget;

    renderer.draw_rect(obj->L->rect, obj->color);
}

bool UIWindowObj::on_event(UIWidgetObj* widgetObj, const UIEvent& event)
{
    UIWindowObj* obj = (UIWindowObj*)widgetObj;

    if (!obj->defaultMouseControls || event.type != UI_EVENT_MOUSE_DRAG)
        return false;

    UIWindow window(obj);
    Rect rect = obj->L->rect;

    const Vec2& dragPos = event.drag.position;

    if (event.drag.begin)
    {
        obj->dragResize = event.drag.button == MOUSE_BUTTON_RIGHT; // right button to resize, left button to reposition
        obj->dragOffset = dragPos - rect.get_pos();                 // fixed drag offset
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

    return true;
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
    LD_PROFILE_SCOPE;

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
    obj->L->rect.x = pos.x;
    obj->L->rect.y = pos.y;
}

void UIWindow::set_size(const Vec2& size)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    (void)obj;

    mObj->L->info.sizeX = UISize::fixed(size.x);
    mObj->L->info.sizeY = UISize::fixed(size.y);
}

void UIWindow::set_rect(const Rect& rect)
{
    UIWindowObj* obj = (UIWindowObj*)mObj;
    (void)obj;

    obj->L->rect.x = rect.x;
    obj->L->rect.y = rect.y;
    mObj->L->info.sizeX = UISize::fixed(rect.w);
    mObj->L->info.sizeY = UISize::fixed(rect.h);
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
    return mObj->L->rect;
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