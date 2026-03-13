#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/WindowRegistry/Input.h>
#include <algorithm>

#include "UIObj.h"

namespace LD {

// clang-format off
struct
{
    UIWidgetType type;
    const char* typeName;
    size_t objSize;
    void (*cleanup)(UIWidgetObj* obj);
} sWidgetTable[] = {
    { UI_WIDGET_WINDOW,    "UIWindow",   sizeof(UIWindowObj),         nullptr },
    { UI_WIDGET_SCROLL,    "UIScroll",   sizeof(UIScrollWidgetObj),   nullptr },
    { UI_WIDGET_BUTTON,    "UIButton",   sizeof(UIButtonWidgetObj),   &UIButtonWidgetObj::cleanup  },
    { UI_WIDGET_SLIDER,    "UISlider",   sizeof(UISliderWidgetObj),   nullptr },
    { UI_WIDGET_TOGGLE,    "UIToggle",   sizeof(UIToggleWidgetObj),   nullptr },
    { UI_WIDGET_PANEL,     "UIPanel",    sizeof(UIPanelWidgetObj),    nullptr },
    { UI_WIDGET_IMAGE,     "UIImage",    sizeof(UIImageWidgetObj),    nullptr },
    { UI_WIDGET_TEXT,      "UIText",     sizeof(UITextWidgetObj),     &UITextWidgetObj::cleanup },
    { UI_WIDGET_TEXT_EDIT, "UITextEdit", sizeof(UITextEditWidgetObj), &UITextEditWidgetObj::cleanup },
};
// clang-format on

static_assert(sizeof(sWidgetTable) / sizeof(*sWidgetTable) == UI_WIDGET_TYPE_COUNT);
static_assert(IsTrivial<UIScrollWidgetObj>);
static_assert(IsTrivial<UITextWidgetObj>);
static_assert(IsTrivial<UIPanelWidgetObj>);
static_assert(IsTrivial<UIImageWidgetObj>);
static_assert(IsTrivial<UIToggleWidgetObj>);
static_assert(IsTrivial<UISliderWidgetObj>);
static_assert(IsTrivial<UIButtonWidgetObj>);

UIWidgetObj::UIWidgetObj(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, UIWindowObj* window, void* user)
    : window(window), type(type), parent(parent), user(user)
{
    LD_ASSERT(window);

    layout.info = layoutI;
    layout.rect = {};
    node = UINode(this);

    switch (type)
    {
    case UI_WIDGET_TEXT_EDIT:
        new (&as.textEdit) UITextEditWidgetObj();
        break;
    }
}

UIWidgetObj::~UIWidgetObj()
{
    switch (type)
    {
    case UI_WIDGET_TEXT_EDIT:
        (&as.textEdit)->~UITextEditWidgetObj();
        break;
    }
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

    if (cb.onDraw)
    {
        cb.onDraw(UIWidget(this), renderer);
        return;
    }

    switch (type)
    {
    case UI_WIDGET_WINDOW:
        UIWindowObj::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_PANEL:
        UIPanelWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_BUTTON:
        UIButtonWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_SLIDER:
        UISliderWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_TOGGLE:
        UIToggleWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_IMAGE:
        UIImageWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_TEXT:
        UITextWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_TEXT_EDIT:
        UITextEditWidget::on_draw(UIWidget(this), renderer);
        break;
    default:
        LD_UNREACHABLE;
    }
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

void UIWidget::set_layout_size(const UISize& sizeX, const UISize& sizeY)
{
    mObj->layout.info.sizeX = sizeX;
    mObj->layout.info.sizeY = sizeY;
}

void UIWidget::set_layout_child_padding(const UIPadding& padding)
{
    mObj->layout.info.childPadding = padding;
}

void UIWidget::set_layout_child_gap(float gap)
{
    mObj->layout.info.childGap = gap;
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
    mObj->cb.onUpdate = onUpdate;
}

void UIWidget::set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer))
{
    mObj->cb.onDraw = onDraw;
}

void UIWidget::set_on_event(bool (*onEvent)(UIWidget widget, const UIEvent& event))
{
    mObj->cb.onEvent = onEvent;
}

bool UIWidget::has_on_event()
{
    return mObj->cb.onEvent != nullptr;
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

UIScrollWidget UINode::add_scroll(const UILayoutInfo& layoutI, const UIScrollWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_SCROLL, layoutI, mObj, user);
    obj->as.scroll.base = obj;
    obj->as.scroll.bgColor = widgetI.bgColor;
    obj->as.scroll.offsetXDst = 0.0f;
    obj->as.scroll.offsetXSpeed = 0.0f;
    obj->as.scroll.offsetYDst = 0.0f;
    obj->as.scroll.offsetYSpeed = 0.0f;
    obj->cb.onDraw = &UIScrollWidget::on_draw;
    obj->cb.onUpdate = &UIScrollWidget::on_update;
    obj->cb.onEvent = &UIScrollWidgetObj::on_event;
    obj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;

    return {obj};
}

UIButtonWidget UINode::add_button(const UILayoutInfo& layoutI, const UIButtonWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_BUTTON, layoutI, mObj, user);
    obj->cb.onEvent = UIButtonWidgetObj::on_event;
    obj->as.button.base = obj;
    obj->as.button.text = widgetI.text ? heap_strdup(widgetI.text, MEMORY_USAGE_UI) : nullptr;
    obj->as.button.onClick = widgetI.onClick;
    obj->as.button.textColor = widgetI.textColor;
    obj->as.button.transparentBG = widgetI.transparentBG;

    UIButtonWidget handle{obj};
    return handle;
};

UISliderWidget UINode::add_slider(const UILayoutInfo& layoutI, const UISliderWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_SLIDER, layoutI, mObj, user);
    obj->cb.onEvent = &UISliderWidgetObj::on_event;
    obj->as.slider.base = obj;
    obj->as.slider.min = widgetI.min;
    obj->as.slider.max = widgetI.max;
    obj->as.slider.value = widgetI.min;
    obj->as.slider.ratio = 0.0f;

    return {obj};
}

UIToggleWidget UINode::add_toggle(const UILayoutInfo& layoutI, const UIToggleWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TOGGLE, layoutI, mObj, user);
    obj->cb.onEvent = &UIToggleWidgetObj::on_event;
    obj->cb.onUpdate = &UIToggleWidgetObj::on_update;
    obj->as.toggle.base = obj;
    obj->as.toggle.state = widgetI.state;
    obj->as.toggle.user_on_toggle = widgetI.on_toggle;
    obj->as.toggle.anim.reset(1.0f);

    return {obj};
}

UITextWidget UINode::add_text(const UILayoutInfo& layoutI, const UITextWidgetInfo& widgetI, void* user)
{
    UILayoutInfo textLayoutI = layoutI;
    if (!(layoutI.sizeX.type == UI_SIZE_FIXED && layoutI.sizeX.extent > 0.0f))
    {
        textLayoutI.sizeX = UISize::wrap();
        textLayoutI.sizeY = UISize::fit();
    }

    UIContextObj* ctx = mObj->ctx();
    UIWidgetObj* obj = ctx->alloc_widget(UI_WIDGET_TEXT, textLayoutI, mObj, user);
    obj->as.text.fontSize = widgetI.fontSize;
    obj->as.text.value = widgetI.cstr ? heap_strdup(widgetI.cstr, MEMORY_USAGE_UI) : nullptr;
    obj->as.text.fontAtlas = ctx->fontAtlas;
    obj->as.text.fontImage = ctx->fontAtlasImage;
    obj->as.text.bgColor = 0;
    obj->as.text.fgColor = ctx->theme.get_on_surface_color();

    if (widgetI.bgColor)
        obj->as.text.bgColor = *widgetI.bgColor;

    return {obj};
}

UITextEditWidget UINode::add_text_edit(const UILayoutInfo& layoutI, const UITextEditWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TEXT_EDIT, layoutI, mObj, user);
    obj->as.textEdit.fontSize = widgetI.fontSize;
    obj->as.textEdit.buf = TextBuffer<char>::create();
    obj->as.textEdit.domain = widgetI.domain;
    obj->as.textEdit.onChange = widgetI.onChange;
    obj->as.textEdit.onSubmit = widgetI.onSubmit;
    obj->flags |= UI_WIDGET_FLAG_FOCUSABLE_BIT;
    obj->cb.onEvent = &UITextEditWidgetObj::on_event;
    obj->cb.onDraw = &UITextEditWidget::on_draw;

    return {obj};
}

UIPanelWidget UINode::add_panel(const UILayoutInfo& layoutI, const UIPanelWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_PANEL, layoutI, mObj, user);
    obj->as.panel.color = widgetI.color;

    return {obj};
}

void ui_obj_cleanup(UIWidgetObj* widget)
{
    LD_ASSERT(widget);

    if (!sWidgetTable[widget->type].cleanup)
        return;

    sWidgetTable[widget->type].cleanup(widget);
}

const char* get_ui_widget_type_cstr(UIWidgetType type)
{
    return sWidgetTable[(int)type].typeName;
}

bool get_ui_widget_type_from_cstr(UIWidgetType& outType, const char* cstr)
{
    if (!cstr)
        return false;

    outType = UI_WIDGET_TYPE_COUNT;

    for (int i = 0; i < (int)UI_WIDGET_TYPE_COUNT; i++)
    {
        if (!strcmp(cstr, sWidgetTable[i].typeName))
        {
            outType = (UIWidgetType)i;
            return true;
        }
    }

    return false;
}

} // namespace LD