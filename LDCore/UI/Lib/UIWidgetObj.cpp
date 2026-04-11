#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/WindowRegistry/Input.h>
#include <algorithm>

#include "UIObj.h"
#include "UIWidgetMeta.h"

namespace LD {

UIWidgetObj::UIWidgetObj(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, UIWindowObj* window, void* storage, void* user)
    : window(window), type(type), parent(parent), user(user)
{
    layout.info = layoutI;
    layout.rect = {};
    node = UINode(this);

    widget_startup(this, storage);
}

UIWidgetObj::~UIWidgetObj()
{
    widget_cleanup(this);
}

UIContextObj* UIWidgetObj::ctx() const
{
    // crazy pointer chasing
    return window->space->layer->ctx;
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

UINode& UIWidget::node()
{
    return mObj->node;
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

Rect UIWidget::get_rect()
{
    return mObj->layout.rect;
}

Vec2 UIWidget::get_pos()
{
    return mObj->layout.rect.get_pos();
}

Vec2 UIWidget::get_size()
{
    return mObj->layout.rect.get_size();
}

UITheme UIWidget::get_theme()
{
    return mObj->theme;
}

bool UIWidget::get_mouse_pos(Vec2& pos)
{
    UIContextObj* ctx = mObj->ctx();

    const Rect& widgetRect = mObj->layout.rect;

    if (widgetRect.contains(ctx->cursorPos))
    {
        pos = ctx->cursorPos - widgetRect.get_pos();
        return true;
    }

    return false;
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
    layout = mObj->layout.info;
}

void UIWidget::set_layout(const UILayoutInfo& layout)
{
    mObj->layout.info = layout;
}

void UIWidget::set_layout_size(UISize sizeX, UISize sizeY)
{
    mObj->layout.info.sizeX = sizeX;
    mObj->layout.info.sizeY = sizeY;
}

void UIWidget::set_layout_size_x(UISize sizeX)
{
    mObj->layout.info.sizeX = sizeX;
}

void UIWidget::set_layout_size_y(UISize sizeY)
{
    mObj->layout.info.sizeY = sizeY;
}

void UIWidget::set_layout_child_padding(const UIPadding& childPad)
{
    mObj->layout.info.childPadding = childPad;
}

void UIWidget::set_layout_child_gap(float childGap)
{
    mObj->layout.info.childGap = childGap;
}

void UIWidget::set_layout_child_axis(UIAxis axis)
{
    mObj->layout.info.childAxis = axis;
}

void UIWidget::set_layout_child_align_x(UIAlign childAlignX)
{
    mObj->layout.info.childAlignX = childAlignX;
}

void UIWidget::set_layout_child_align_y(UIAlign childAlignY)
{
    mObj->layout.info.childAlignY = childAlignY;
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

UIContextObj* UINode::get_context()
{
    return mObj->ctx();
}

void UINode::get_children(std::vector<UIWidget>& widgets)
{
    widgets.clear();

    for (UIWidgetObj* child = mObj->child; child; child = child->next)
        widgets.push_back(UIWidget(child));
}

void UINode::remove()
{
    UIContextObj* ctx = mObj->ctx();

    ctx->free_widget(mObj);
    mObj = nullptr;
}

UIWidget UINode::add_scroll(const UILayoutInfo& layoutI, UIScrollStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_SCROLL, layoutI, mObj, storage, user);

    return {obj};
}

UIWidget UINode::add_button(const UILayoutInfo& layoutI, UIButtonStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_BUTTON, layoutI, mObj, storage, user);

    return {obj};
};

UIWidget UINode::add_slider(const UILayoutInfo& layoutI, UISliderStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_SLIDER, layoutI, mObj, storage, user);

    return {obj};
}

UIWidget UINode::add_toggle(const UILayoutInfo& layoutI, UIToggleStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TOGGLE, layoutI, mObj, storage, user);

    return {obj};
}

UIWidget UINode::add_text(const UILayoutInfo& layoutI, UITextStorage* storage, void* user)
{
    UILayoutInfo textLayoutI = layoutI;
    if (!(layoutI.sizeX.type == UI_SIZE_FIXED && layoutI.sizeX.extent > 0.0f))
    {
        textLayoutI.sizeX = UISize::wrap();
        textLayoutI.sizeY = UISize::fit();
    }

    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TEXT, textLayoutI, mObj, storage, user);

    return {obj};
}

UIWidget UINode::add_text_edit(const UILayoutInfo& layoutI, UITextEditStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TEXT_EDIT, layoutI, mObj, storage, user);

    return {obj};
}

UIWidget UINode::add_panel(const UILayoutInfo& layoutI, UIPanelStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_PANEL, layoutI, mObj, storage, user);

    return {obj};
}

UIWidget UINode::add_image(const UILayoutInfo& layoutI, UIImageStorage* storage, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_IMAGE, layoutI, mObj, storage, user);

    return {obj};
}

const char* get_ui_widget_type_cstr(UIWidgetType type)
{
    return sWidgetMeta[(int)type].typeName;
}

bool get_ui_widget_type_from_cstr(UIWidgetType& outType, const char* cstr)
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