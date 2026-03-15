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
    { UI_WIDGET_WINDOW,    "UIWindow",   sizeof(UIWindowObj),         nullptr,                       nullptr,                       &UIWindowObj::on_draw },
    { UI_WIDGET_SCROLL,    "UIScroll",   sizeof(UIScrollWidgetObj),   &UIScrollWidgetObj::startup,   &UIScrollWidgetObj::cleanup,   nullptr, },
    { UI_WIDGET_BUTTON,    "UIButton",   sizeof(UIButtonWidgetObj),   &UIButtonWidgetObj::startup,   &UIButtonWidgetObj::cleanup,   &UIButtonWidget::on_draw },
    { UI_WIDGET_SLIDER,    "UISlider",   sizeof(UISliderWidgetObj),   &UISliderWidgetObj::startup,   &UISliderWidgetObj::cleanup,   &UISliderWidget::on_draw },
    { UI_WIDGET_TOGGLE,    "UIToggle",   sizeof(UIToggleWidgetObj),   &UIToggleWidgetObj::startup,   &UIToggleWidgetObj::cleanup,   &UIToggleWidget::on_draw },
    { UI_WIDGET_PANEL,     "UIPanel",    sizeof(UIPanelWidgetObj),    &UIPanelWidgetObj::startup,    &UIPanelWidgetObj::cleanup,    &UIPanelWidget::on_draw },
    { UI_WIDGET_IMAGE,     "UIImage",    sizeof(UIImageWidgetObj),    &UIImageWidgetObj::startup,    &UIImageWidgetObj::cleanup,    &UIImageWidget::on_draw },
    { UI_WIDGET_TEXT,      "UIText",     sizeof(UITextWidgetObj),     &UITextWidgetObj::startup,     &UITextWidgetObj::cleanup,     &UITextWidget::on_draw },
    { UI_WIDGET_TEXT_EDIT, "UITextEdit", sizeof(UITextEditWidgetObj), &UITextEditWidgetObj::startup, &UITextEditWidgetObj::cleanup, &UITextEditWidget::on_draw },
};
// clang-format on

static_assert(sizeof(sWidgetMeta) / sizeof(*sWidgetMeta) == UI_WIDGET_TYPE_COUNT);
static_assert(IsTrivial<UIScrollWidgetObj>);
static_assert(IsTrivial<UIPanelWidgetObj>);
static_assert(IsTrivial<UIImageWidgetObj>);
static_assert(IsTrivial<UISliderWidgetObj>);

void widget_startup(UIWidgetObj* obj, void* storage)
{
    LD_ASSERT(obj);

    if (sWidgetMeta[(int)obj->type].startup)
        sWidgetMeta[(int)obj->type].startup(obj, storage);
}

void widget_cleanup(UIWidgetObj* obj)
{
    LD_ASSERT(obj);

    if (sWidgetMeta[(int)obj->type].cleanup)
        sWidgetMeta[(int)obj->type].cleanup(obj);
}

void widget_on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget.unwrap();
    LD_ASSERT(obj);

    if (sWidgetMeta[(int)obj->type].onDraw)
        sWidgetMeta[(int)obj->type].onDraw(widget, renderer);
}

} // namespace LD