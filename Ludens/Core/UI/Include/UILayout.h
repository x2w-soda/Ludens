#pragma once

#include <yoga/Yoga.h>
#include "Core/Math/Include/Vec2.h"

namespace LD
{

class UILayoutNode;

enum class UIEdge
{
    Left = 0,
    Top,
    Right,
    Bottom,
};

enum class UIFlexDirection
{
    /// the flexbox items are displayed horizontally
    Row = 0,

    /// same as Row but in reverse order
    RowReverse,

    /// the flexbox items are displayed vertically
    Column,

    /// same as Column but in reverse order
    ColumnReverse,
};

struct UILayoutNodeInfo
{
    UILayoutNode* Parent = nullptr;
    UIFlexDirection FlexDirection = UIFlexDirection::Row;
    float FlexGrow;
    float Width = 0.0f;
    float Height = 0.0f;
};

class UILayoutNode
{
public:
    UILayoutNode();
    UILayoutNode(const UILayoutNode&) = delete;
    ~UILayoutNode();

    UILayoutNode& operator=(const UILayoutNode&) = delete;

    void Startup(const UILayoutNodeInfo& info);
    void Cleanup();

    void CalculateLayout();

    /// get position relative to parent layout node 
    inline const UILayoutNode& GetPos(Vec2& pos) const
    {
        pos.x = YGNodeLayoutGetLeft(mNode);
        pos.y = YGNodeLayoutGetTop(mNode);
        return *this;
    }

    inline UILayoutNode& SetSize(const Vec2& size)
    {
        YGNodeStyleSetWidth(mNode, size.x);
        YGNodeStyleSetHeight(mNode, size.y);
        return *this;
    }

    inline const UILayoutNode& GetSize(Vec2& size) const
    {
        size.x = YGNodeLayoutGetWidth(mNode);
        size.y = YGNodeLayoutGetHeight(mNode);
        return *this;
    }

    inline UILayoutNode& SetWidth(float width)
    {
        YGNodeStyleSetWidth(mNode, width);
        return *this;
    }

    inline const UILayoutNode& GetWidth(float& width) const
    {
        width = YGNodeLayoutGetWidth(mNode);
        return *this;
    }

    inline UILayoutNode& SetHeight(float height)
    {
        YGNodeStyleSetHeight(mNode, height);
        return *this;
    }

    inline const UILayoutNode& GetHeight(float& height) const
    {
        height = YGNodeLayoutGetHeight(mNode);
        return *this;
    }

    inline UILayoutNode& SetPadding(float padding)
    {
        YGNodeStyleSetPadding(mNode, YGEdgeAll, padding);
        return *this;
    }

    inline UILayoutNode& SetPadding(float padding, UIEdge edge)
    {
        constexpr YGEdge edges[4]{
            YGEdgeLeft,
            YGEdgeTop,
            YGEdgeRight,
            YGEdgeBottom,
        };

        YGNodeStyleSetPadding(mNode, edges[(int)edge], padding);
        return *this;
    }

    /// the layout system does not do any drawing, border acts exactly
    /// the same as padding and is a hint for the renderer to draw the border.
    inline UILayoutNode& SetBorder(float border)
    {
        YGNodeStyleSetBorder(mNode, YGEdgeAll, border);
        return *this;
    }

    inline const UILayoutNode& GetBorder(float& border) const
    {
        border = YGNodeStyleGetBorder(mNode, YGEdgeAll);
        return *this;
    }

    inline UILayoutNode& SetMargin(float margin)
    {
        YGNodeStyleSetMargin(mNode, YGEdgeAll, margin);
        return *this;
    }

    inline UILayoutNode& SetMargin(float margin, UIEdge edge)
    {
        constexpr YGEdge edges[4]{
            YGEdgeLeft,
            YGEdgeTop,
            YGEdgeRight,
            YGEdgeBottom,
        };

        YGNodeStyleSetMargin(mNode, edges[(int)edge], margin);
        return *this;
    }

    UILayoutNode& SetFlexDirection(UIFlexDirection direction);

    UILayoutNode& SetFlexGrow(float grow);

private:
    YGNodeRef mNode = nullptr;
    size_t mChildCount = 0;
};

} // namespace LD
