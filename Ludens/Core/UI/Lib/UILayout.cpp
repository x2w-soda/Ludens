#include <yoga/Yoga.h>
#include "Core/DSA/Include/Vector.h"
#include "Core/UI/Include/UILayout.h"

namespace LD {

UILayoutNode::UILayoutNode()
{
}

UILayoutNode::~UILayoutNode()
{
    LD_DEBUG_ASSERT(mNode == nullptr);
}

void UILayoutNode::Startup(const UILayoutNodeInfo& info)
{
    LD_DEBUG_ASSERT(mNode == nullptr);

    mNode = YGNodeNew();
    mChildCount = 0;
    
    if (info.Width > 0.0f)
        SetWidth(info.Width);
    if (info.Height > 0.0f)
        SetHeight(info.Height);

    SetFlexDirection(info.FlexDirection);
    SetFlexGrow(info.FlexGrow);

    if (info.Parent)
    {
        UILayoutNode& parent = *info.Parent;
        YGNodeInsertChild(parent.mNode, mNode, parent.mChildCount++);
    }
}

void UILayoutNode::Cleanup()
{
    YGNodeFree(mNode);
    mNode = nullptr;
}

void UILayoutNode::CalculateLayout()
{
    YGNodeCalculateLayout(mNode, YGUndefined, YGUndefined, YGDirectionLTR);
}

UILayoutNode& UILayoutNode::SetFlexDirection(UIFlexDirection direction)
{
    YGFlexDirection dir;

    switch (direction)
    {
    case UIFlexDirection::Row:
        dir = YGFlexDirectionRow;
        break;
    case UIFlexDirection::RowReverse:
        dir = YGFlexDirectionRowReverse;
        break;
    case UIFlexDirection::Column:
        dir = YGFlexDirectionColumn;
        break;
    case UIFlexDirection::ColumnReverse:
        dir = YGFlexDirectionColumnReverse;
        break;
    }

    YGNodeStyleSetFlexDirection(mNode, dir);
    return *this;
}

UILayoutNode& UILayoutNode::SetFlexGrow(float grow)
{
    YGNodeStyleSetFlexGrow(mNode, grow);
    return *this;
}

} // namespace LD
