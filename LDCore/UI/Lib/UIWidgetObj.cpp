#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/WindowRegistry/Input.h>
#include <algorithm>

#include "UIObj.h"
#include "UIWidgetMeta.h"

namespace LD {

static_assert(sizeof(UIWidgetUnion) <= 128);

UIWidgetObj::UIWidgetObj(UIWidgetType type, UIContextObj* ctx, UIWidgetLayout* widgetL, UIWidgetUnion* widgetU, UIWidgetObj* parent, UIWindowObj* window, void* data, void* user)
    : type(type), L(widgetL), U(widgetU), window(window), parent(parent), data(data), user(user)
{
    LD_ASSERT(L);

    id = ctx->idRegistry.create();
    theme = ctx->theme;
}

UIWidgetObj::~UIWidgetObj()
{
    window->ctx->idRegistry.destroy(id);
}

void UIWidgetObj::append_child(UIWidgetObj* newChild)
{
    if (!child)
    {
        child = newChild;
        return;
    }

    UIWidgetObj* last = child;
    while (last && last->next)
        last = last->next;
    last->next = newChild;
}

void UIWidgetObj::remove_child(UIWidgetObj* c)
{
    UIWidgetObj** pnext = &child;
    while (*pnext && *pnext != c)
        pnext = &(*pnext)->next;

    if (*pnext)
        *pnext = (*pnext)->next;
}

/// @brief get children count in linear time
int UIWidgetObj::get_children_count()
{
    int count = 0;
    for (UIWidgetObj* c = child; c; c = c->next)
        count++;
    return count;
}

UIWidgetObj* UIWidgetObj::get_child_by_name(const std::string& name)
{
    for (UIWidgetObj* c = child; c; c = c->next)
    {
        if (c->name == name)
            return c;
    }

    return nullptr;
}

Rect UIWidgetObj::get_child_rect_union()
{
    if (!child)
        return {};

    Rect u = child->L->rect;
    for (UIWidgetObj* c = child->next; c; c = c->next)
        u = Rect::get_union(u, c->L->rect);

    return u;
}

UIContextObj* UIWidgetObj::ctx() const
{
    return window->ctx;
}

void UIWidgetObj::draw(ScreenRenderComponent renderer)
{
    if (flags & UI_WIDGET_FLAG_HIDDEN_BIT)
        return;

    if (userCB.onDraw)
    {
        userCB.onDraw(UIWidget(this), renderer);
        return; // overrides default draw behavior
    }

    if (sWidgetMeta[(int)type].onDraw)
        sWidgetMeta[(int)type].onDraw(this, renderer);
}

bool UIWidget::is_hovered()
{
    UIContextObj* ctx = mObj->ctx();
    UIWidgetObj* hovered = ctx->hoverWidgetLeaf;

    while (hovered)
    {
        if (hovered == mObj)
            return true;

        hovered = hovered->parent;
    }

    return false;
}

bool UIWidget::is_focused()
{
    UIContextObj* ctx = mObj->ctx();
    return ctx->focusWidget == mObj;
}

bool UIWidget::is_pressed()
{
    UIContextObj* ctx = mObj->ctx();
    return ctx->pressWidget == mObj;
}

bool UIWidget::is_dragged()
{
    UIContextObj* ctx = mObj->ctx();
    return ctx->dragWidget == mObj;
}

void UIWidget::set_visible(bool isVisible)
{
    if (isVisible)
        mObj->flags &= ~UI_WIDGET_FLAG_HIDDEN_BIT;
    else
        mObj->flags |= UI_WIDGET_FLAG_HIDDEN_BIT;
}

bool UIWidget::is_visible()
{
    return !static_cast<bool>(mObj->flags & UI_WIDGET_FLAG_HIDDEN_BIT);
}

UIWidgetType UIWidget::get_type()
{
    return mObj->type;
}

UIID UIWidget::get_id()
{
    return mObj->id;
}

UIContextObj* UIWidget::get_context_obj()
{
    return mObj->ctx();
}

Rect UIWidget::get_rect()
{
    return mObj->L->rect;
}

Vec2 UIWidget::get_pos()
{
    return mObj->L->rect.get_pos();
}

Vec2 UIWidget::get_size()
{
    return mObj->L->rect.get_size();
}

Rect UIWidget::get_child_rect_union()
{
    return mObj->get_child_rect_union();
}

UITheme UIWidget::get_theme()
{
    return mObj->theme;
}

bool UIWidget::get_mouse_pos(Vec2& pos)
{
    UIContextObj* ctx = mObj->ctx();

    const Rect& widgetRect = mObj->L->rect;

    if (widgetRect.contains(ctx->cursorPos))
    {
        pos = ctx->cursorPos - widgetRect.get_pos();
        return true;
    }

    return false;
}

void* UIWidget::get_data()
{
    return mObj->data;
}

void UIWidget::set_data(void* data)
{
    LD_ASSERT(data);

    mObj->data = data;
}

void* UIWidget::get_user()
{
    return mObj->user;
}

void UIWidget::set_user(void* user)
{
    mObj->user = user;
}

void UIWidget::get_name(std::string& name)
{
    name = mObj->name;
}

void UIWidget::set_name(const std::string& name)
{
    mObj->name = name;
}

UIWidget UIWidget::add_child(UIWidgetType type, const UILayoutInfo& layoutI, void* data, void* user)
{
    UIWidgetAllocInfo allocI{};
    allocI.parent = mObj;
    allocI.data = data;
    allocI.user = user;
    allocI.type = type;
    UIWidgetObj* obj = mObj->ctx()->alloc_widget_obj(allocI);

    *obj->L = {};
    obj->L->info = layoutI;

    return UIWidget(obj);
}

void UIWidget::set_consume_mouse_event(bool consumes)
{
    if (consumes)
        mObj->flags |= UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT;
    else
        mObj->flags &= ~UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT;
}

void UIWidget::set_consume_key_event(bool consumes)
{
    if (consumes)
        mObj->flags |= UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT;
    else
        mObj->flags &= ~UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT;
}

void UIWidget::set_consume_scroll_event(bool consumes)
{
    if (consumes)
        mObj->flags |= UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT;
    else
        mObj->flags &= ~UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT;
}

Color UIWidget::get_state_color(Color color)
{
    if (is_pressed())
        color = Color::darken(color, 0.05f);
    else if (is_hovered())
        color = Color::lift(color, 0.07f);

    return color;
}

UIWidget UIWidget::get_child_by_name(const std::string& childName)
{
    return UIWidget(mObj->get_child_by_name(childName));
}

void UIWidget::get_layout(UILayoutInfo& layout)
{
    layout = mObj->L->info;
}

void UIWidget::set_layout(const UILayoutInfo& layout)
{
    mObj->L->info = layout;
}

void UIWidget::set_layout_size(UISize sizeX, UISize sizeY)
{
    mObj->L->info.sizeX = sizeX;
    mObj->L->info.sizeY = sizeY;
}

void UIWidget::set_layout_size_x(UISize sizeX)
{
    mObj->L->info.sizeX = sizeX;
}

void UIWidget::set_layout_size_y(UISize sizeY)
{
    mObj->L->info.sizeY = sizeY;
}

void UIWidget::set_layout_child_padding(const UIPadding& childPad)
{
    mObj->L->info.childPadding = childPad;
}

void UIWidget::set_layout_child_gap(float childGap)
{
    mObj->L->info.childGap = childGap;
}

void UIWidget::set_layout_child_axis(UIAxis axis)
{
    mObj->L->info.childAxis = axis;
}

void UIWidget::set_layout_child_align_x(UIAlign childAlignX)
{
    mObj->L->info.childAlignX = childAlignX;
}

void UIWidget::set_layout_child_align_y(UIAlign childAlignY)
{
    mObj->L->info.childAlignY = childAlignY;
}

void UIWidget::set_on_update(void (*onUpdate)(UIWidget widget, float delta))
{
    mObj->userCB.onUpdate = onUpdate;
}

void UIWidget::set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer))
{
    mObj->userCB.onDraw = onDraw;
}

void UIWidget::set_on_event(bool (*onEvent)(UIWidget widget, const UIEvent& event))
{
    mObj->userCB.onEvent = onEvent;
}

bool UIWidget::has_on_event()
{
    return mObj->userCB.onEvent != nullptr;
}

void UIWidget::get_children(std::vector<UIWidget>& widgets)
{
    widgets.clear();

    for (UIWidgetObj* child = mObj->child; child; child = child->next)
        widgets.push_back(UIWidget(child));
}

void UIWidget::remove()
{
    UIContextObj* ctx = mObj->ctx();

    ctx->free_widget_obj(mObj);
    mObj = nullptr;
}

void UIWidget::remove_children()
{
    UIContextObj* ctx = mObj->ctx();

    while (mObj->child)
        ctx->free_widget_obj(mObj->child);
}

const char* UIWidget::get_type_cstr(UIWidgetType type)
{
    return sWidgetMeta[(int)type].typeName;
}

UILayoutInfo UIWidget::get_default_layout(UIWidgetType type)
{
    return widget_default_layout(type);
}

bool UIWidget::get_type_from_cstr(UIWidgetType& outType, const char* cstr)
{
    if (!cstr)
        return false;

    outType = UI_WIDGET_TYPE_COUNT;

    for (int i = 0; i < (int)UI_WIDGET_TYPE_COUNT; i++)
    {
        if (!strcmp(cstr, sWidgetMeta[i].typeName))
        {
            outType = (UIWidgetType)i;
            return true;
        }
    }

    return false;
}

} // namespace LD