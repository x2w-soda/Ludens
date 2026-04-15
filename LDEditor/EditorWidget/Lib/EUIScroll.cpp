#include <LudensEditor/EditorWidget/EUIScroll.h>

#include "EUI.h"

namespace LD {

void EUIScroll::push(const UILayoutInfo* scrollLayout)
{
    UILayoutInfo layoutI(UISize::grow(), UISize::grow());
    layoutI.childAxis = UI_AXIS_X;
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);

    mScroll.bgColor = bgColor;
    mScrollW = ui_push_scroll(&mScroll);

    mScrollBar.contentExtent = mScrollW.get_child_rect_union().h;
    mScrollBar.scrollExtent = mScrollW.get_rect().h;

    if (scrollLayout)
        ui_top_layout(*scrollLayout);
}

void EUIScroll::pop()
{
    ui_pop(); // scroll container

    mScrollBar.bgColor = Color::lift(bgColor, 0.04f);
    mScrollBar.barColor = barColor;
    mScrollBar.axis = UI_AXIS_Y;
    UIScrollBarWidget barW = ui_push_scroll_bar(&mScrollBar);
    UILayoutInfo layoutI(UISize::fixed(8.0f), UISize::grow());
    ui_top_layout(layoutI);

    if (ui_top_is_dragged())
        mScrollW.set_scroll_offset_y_normalized(mScrollBar.get_ratio());
    else
        mScrollBar.set_ratio(mScrollW.get_scroll_offset_y_normalized());

    ui_pop(); // scroll bar

    ui_pop(); // panel
}

} // namespace LD