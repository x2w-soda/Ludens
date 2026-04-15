#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

class EUIScroll
{
public:
    void push(const UILayoutInfo* scrollLayout = nullptr);
    void pop();

    Color bgColor = 0;
    Color barColor = 0xFFFFFFFF;

private:
    UIScrollData mScroll;
    UIScrollBarData mScrollBar;
    UIScrollWidget mScrollW;
};

} // namespace LD