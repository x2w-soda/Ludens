#include <Ludens/UI/Widget/UIButtonWidget.h>
#include <Ludens/UI/Widget/UIImageWidget.h>
#include <Ludens/UI/Widget/UIPanelWidget.h>
#include <Ludens/UI/Widget/UISliderWidget.h>
#include <Ludens/UI/Widget/UITextEditWidget.h>
#include <Ludens/UI/Widget/UITextWidget.h>

#include "UIObj.h"
#include "UIWidgetMeta.h"

namespace LD {

// clang-format off
UIWidgetMeta sWidgetMeta[] = {
    { UI_WIDGET_WINDOW,      "UIWindow",     sizeof(UIWindowObj),          nullptr,                              nullptr,                         nullptr,                         &UIWindowObj::on_event,           nullptr,                          &UIWindowObj::on_draw,          nullptr, nullptr},
    { UI_WIDGET_SCROLL,      "UIScroll",     sizeof(UIScrollWidgetObj),    &UIScrollWidgetObj::default_layout,   &UIScrollWidgetObj::startup,     &UIScrollWidgetObj::cleanup,     &UIScrollWidgetObj::on_event,     &UIScrollWidgetObj::on_update,    &UIScrollWidgetObj::on_draw,    nullptr, nullptr},
    { UI_WIDGET_SCROLL_BAR,  "UIScrollBar",  sizeof(UIScrollBarWidgetObj), nullptr,                              &UIScrollBarWidgetObj::startup,  &UIScrollBarWidgetObj::cleanup,  &UIScrollBarWidgetObj::on_event,  &UIScrollBarWidgetObj::on_update, &UIScrollBarWidgetObj::on_draw, nullptr, nullptr},
    { UI_WIDGET_BUTTON,      "UIButton",     sizeof(UIButtonWidgetObj),    nullptr,                              &UIButtonWidgetObj::startup,     &UIButtonWidgetObj::cleanup,     &UIButtonWidgetObj::on_event,     nullptr,                          &UIButtonWidgetObj::on_draw,    nullptr, nullptr},
    { UI_WIDGET_SLIDER,      "UISlider",     sizeof(UISliderWidgetObj),    nullptr,                              &UISliderWidgetObj::startup,     &UISliderWidgetObj::cleanup,     &UISliderWidgetObj::on_event,     nullptr,                          &UISliderWidgetObj::on_draw,    nullptr, nullptr},
    { UI_WIDGET_TOGGLE,      "UIToggle",     sizeof(UIToggleWidgetObj),    nullptr,                              &UIToggleWidgetObj::startup,     &UIToggleWidgetObj::cleanup,     &UIToggleWidgetObj::on_event,     &UIToggleWidgetObj::on_update,    &UIToggleWidgetObj::on_draw,    nullptr, nullptr},
    { UI_WIDGET_PANEL,       "UIPanel",      sizeof(UIPanelWidgetObj),     &UIPanelWidgetObj::default_layout,    &UIPanelWidgetObj::startup,      &UIPanelWidgetObj::cleanup,      nullptr,                          nullptr,                          &UIPanelWidgetObj::on_draw,     nullptr, nullptr},
    { UI_WIDGET_IMAGE,       "UIImage",      sizeof(UIImageWidgetObj),     nullptr,                              &UIImageWidgetObj::startup,      &UIImageWidgetObj::cleanup,      nullptr,                          nullptr,                          &UIImageWidgetObj::on_draw,     nullptr, nullptr},
    { UI_WIDGET_TEXT,        "UIText",       sizeof(UITextWidgetObj),      &UITextWidgetObj::default_layout,     &UITextWidgetObj::startup,       &UITextWidgetObj::cleanup,       &UITextWidgetObj::on_event,       nullptr,                          &UITextWidgetObj::on_draw,      UITextWidgetObj::wrap_size, UITextWidgetObj::wrap_limit},
    { UI_WIDGET_TEXT_EDIT,   "UITextEdit",   sizeof(UITextEditWidgetObj),  &UITextEditWidgetObj::default_layout, &UITextEditWidgetObj::startup,   &UITextEditWidgetObj::cleanup,   &UITextEditWidgetObj::on_event,   nullptr,                          &UITextEditWidgetObj::on_draw,  nullptr, nullptr, &UITextEditWidgetObj::cursor_hint},
    { UI_WIDGET_DROPDOWN,    "UIDropdown",   sizeof(UIDropdownWidgetObj),  nullptr,                              &UIDropdownWidgetObj::startup,   &UIDropdownWidgetObj::cleanup,   &UIDropdownWidgetObj::on_event,   nullptr,                          &UIDropdownWidgetObj::on_draw,  nullptr, nullptr},
};
// clang-format on

static_assert(sizeof(sWidgetMeta) / sizeof(*sWidgetMeta) == UI_WIDGET_TYPE_COUNT);

UILayoutInfo widget_default_layout(UIWidgetType type)
{
    UILayoutInfo defaultLayout = {};

    if (sWidgetMeta[(int)type].defaultLayout)
        defaultLayout = sWidgetMeta[(int)type].defaultLayout();

    return defaultLayout;
}

CursorType widget_cursor_hint(UIWidgetObj* obj)
{
    LD_ASSERT(obj);

    CursorType hint = CURSOR_TYPE_DEFAULT;

    if (sWidgetMeta[(int)obj->type].cursorHint)
        hint = sWidgetMeta[(int)obj->type].cursorHint(obj);

    return hint;
}

void widget_startup(UIWidgetObj* obj)
{
    LD_ASSERT(obj);

    if (sWidgetMeta[(int)obj->type].startup)
        sWidgetMeta[(int)obj->type].startup(obj);
}

void widget_cleanup(UIWidgetObj* obj)
{
    LD_ASSERT(obj);

    if (sWidgetMeta[(int)obj->type].cleanup)
        sWidgetMeta[(int)obj->type].cleanup(obj);
}

void widget_on_update(UIWidgetObj* obj, float delta)
{
    LD_ASSERT(obj);

    if (sWidgetMeta[(int)obj->type].onUpdate)
        sWidgetMeta[(int)obj->type].onUpdate(obj, delta);

    if (obj->userCB.onUpdate)
        obj->userCB.onUpdate(UIWidget(obj), delta);
}

bool widget_on_event(UIWidgetObj* obj, const UIEvent& event)
{
    LD_ASSERT(obj);

    bool libHandled = false;
    bool userHandled = false;

    if (sWidgetMeta[(int)obj->type].onEvent)
        libHandled = sWidgetMeta[(int)obj->type].onEvent(obj, event);

    if (obj->userCB.onEvent)
        userHandled = obj->userCB.onEvent(UIWidget(obj), event);

    return libHandled || userHandled;
}

} // namespace LD