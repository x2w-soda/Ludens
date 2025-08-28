#include "AreaNode.h"
#include "UIWindowManagerObj.h"
#include <Ludens/Application/Application.h>

namespace LD {

static void split_area(Axis axis, float ratio, const Rect& area, Rect& tl, Rect& br, Rect& split)
{
    if (axis == AXIS_X)
    {
        tl = area;
        tl.w = area.w * ratio;
        tl.w -= WINDOW_AREA_MARGIN / 2.0f;

        split = Rect(tl.x + tl.w, tl.y, WINDOW_AREA_MARGIN, tl.h);

        br = area;
        br.x += tl.w + WINDOW_AREA_MARGIN;
        br.w = area.w * (1.0f - ratio) - WINDOW_AREA_MARGIN / 2.0f;
    }
    else
    {
        tl = area;
        tl.h = area.h * ratio;
        tl.h -= WINDOW_AREA_MARGIN / 2.0f;

        split = Rect(tl.x, tl.y + tl.h, tl.w, WINDOW_AREA_MARGIN);

        br = area;
        br.y += tl.h + WINDOW_AREA_MARGIN;
        br.h = area.h * (1.0f - ratio) - WINDOW_AREA_MARGIN / 2.0f;
    }
}

AreaNode::AreaNode()
    : mType(AREA_NODE_TYPE_UNKNOWN), mAreaID(INVALID_WINDOW_AREA)
{
}

AreaNode::~AreaNode()
{
}

UIWindowAreaID AreaNode::split_right(UIWindowManagerObj* wm, float ratio)
{
    LD_ASSERT(mType == AREA_NODE_TYPE_LEAF); // only leaf nodes are splittable

    ratio = std::clamp(ratio, 0.05f, 0.95f);

    UIContext ctx = wm->get_context();
    Rect leftArea, rightArea, splitArea;
    split_area(AXIS_X, ratio, mArea, leftArea, rightArea, splitArea);
    Rect clientArea = rightArea;
    clientArea.y += WINDOW_TAB_HEIGHT;
    clientArea.h -= WINDOW_TAB_HEIGHT;

    // new subtree relationship
    AreaNode* parent = mParent;
    AreaNode* split = heap_new<AreaNode>(MEMORY_USAGE_UI);
    AreaNode* lch = this;
    AreaNode* rch = heap_new<AreaNode>(MEMORY_USAGE_UI);
    split->startup_as_split(ctx, wm->get_area_id(), mArea, AXIS_X, ratio, splitArea);
    rch->startup_as_leaf(ctx, wm->get_area_id(), rightArea, wm->create_window(clientArea.get_size(), "window"));

    split->mParent = parent;
    if (parent)
    {
        bool wasLch = this == parent->mLch;
        if (wasLch)
            parent->mLch = split;
        else
            parent->mRch = split;
    }
    else
        wm->set_root(split);

    lch->mParent = split;
    split->mLch = lch;
    split->mLch->invalidate_area(leftArea);

    rch->mParent = split;
    split->mRch = rch;
    split->mRch->invalidate_area(rightArea);

    return split->mRch->get_area_id();
}

UIWindowAreaID AreaNode::split_bottom(UIWindowManagerObj* wm, float ratio)
{
    LD_ASSERT(mType == AREA_NODE_TYPE_LEAF); // only leaf nodes are splittable

    ratio = std::clamp(ratio, 0.05f, 0.95f);

    UIContext ctx = wm->get_context();
    Rect topArea, bottomArea, splitArea;
    split_area(AXIS_Y, ratio, mArea, topArea, bottomArea, splitArea);

    // new subtree relationship
    AreaNode* parent = mParent;
    AreaNode* split = heap_new<AreaNode>(MEMORY_USAGE_UI);
    AreaNode* lch = this;
    AreaNode* rch = heap_new<AreaNode>(MEMORY_USAGE_UI);
    split->startup_as_split(ctx, wm->get_area_id(), mArea, AXIS_Y, ratio, splitArea);
    rch->startup_as_leaf(ctx, wm->get_area_id(), bottomArea, wm->create_window(bottomArea.get_size(), "window"));

    split->mParent = parent;
    if (parent)
    {
        bool wasLch = this == parent->mLch;
        if (wasLch)
            parent->mLch = split;
        else
            parent->mRch = split;
    }
    else
        wm->set_root(split);

    lch->mParent = split;
    split->mLch = lch;
    split->mLch->invalidate_area(topArea);

    rch->mParent = split;
    split->mRch = rch;
    split->mRch->invalidate_area(bottomArea);

    return split->mRch->get_area_id();
}

void AreaNode::invalidate_area(const Rect& rect)
{
    LD_ASSERT(mType == AREA_NODE_TYPE_LEAF);

    mArea = rect;
    mTabControl.invalidate_area(mArea);
}

void AreaNode::startup_as_split(UIContext ctx, UIWindowAreaID areaID, const Rect& area, Axis axis, float ratio, const Rect& splitArea)
{
    mType = AREA_NODE_TYPE_SPLIT;
    mAreaID = areaID;
    mArea = area;

    UILayoutInfo layoutI{};
    if (axis == AXIS_X)
    {
        layoutI.sizeX = UISize::fixed(WINDOW_AREA_MARGIN);
        layoutI.sizeY = UISize::fixed(splitArea.h);
    }
    else
    {
        layoutI.sizeX = UISize::fixed(splitArea.w);
        layoutI.sizeY = UISize::fixed(WINDOW_AREA_MARGIN);
    }
    UIWindowInfo windowI{};
    windowI.name = "splitControl";
    windowI.defaultMouseControls = false;

    mSplitControl = ctx.add_window(layoutI, windowI, this);
    mSplitControl.set_rect(splitArea);
    mSplitControl.set_on_draw(&AreaNode::split_control_on_draw);
    mSplitControl.set_on_drag(&AreaNode::split_control_on_drag);
    mSplitControl.set_on_enter(&AreaNode::split_control_on_enter);
    mSplitControl.set_on_leave(&AreaNode::split_control_on_leave);
    mSplitAxis = axis;
    mSplitRatio = ratio;
}

void AreaNode::startup_as_leaf(UIContext ctx, UIWindowAreaID areaID, const Rect& area, UIWindow client)
{
    mType = AREA_NODE_TYPE_LEAF;
    mAreaID = areaID;
    mArea = area;
    mParent = nullptr;
    mLch = nullptr;
    mRch = nullptr;

    Vec2 clientPos = area.get_pos();
    clientPos.y += WINDOW_TAB_HEIGHT;
    client.set_pos(clientPos);

    mTabControl.startup(ctx, area);
    mTabControl.add_tab(client);
}

void AreaNode::startup_as_float(UIContext ctx, UIWindowAreaID areaID, const Rect& area, UIWindow client)
{
    mType = AREA_NODE_TYPE_FLOAT;
    mAreaID = areaID;
    mArea = area;
    mParent = nullptr;
    mLch = nullptr;
    mRch = nullptr;

    Vec2 clientPos = area.get_pos();
    clientPos.y += WINDOW_TAB_HEIGHT;
    client.set_pos(clientPos);

    mTabControl.startup(ctx, area);
    mTabControl.add_tab(client);
}

void AreaNode::cleanup()
{
    switch (mType)
    {
    case AREA_NODE_TYPE_UNKNOWN:
        return;
    case AREA_NODE_TYPE_LEAF:
        mTabControl.cleanup();
        break;
    case AREA_NODE_TYPE_SPLIT:
        break;
    }
}

void AreaNode::invalidate()
{
    if (mType == AREA_NODE_TYPE_LEAF)
    {
        // invalidate leaf node window area
        invalidate_area(mArea);
        return;
    }
    else if (mType == AREA_NODE_TYPE_SPLIT)
        invalidate_split_ratio(mSplitRatio);
}

void AreaNode::invalidate_split_ratio(float newRatio)
{
    LD_ASSERT(mType == AREA_NODE_TYPE_SPLIT);

    newRatio = std::clamp<float>(newRatio, 0.05f, 0.95f);
    mSplitRatio = newRatio;

    Rect tl, br, splitArea;
    split_area(mSplitAxis, mSplitRatio, mArea, tl, br, splitArea);
    mSplitControl.set_rect(splitArea);

    mLch->mArea = tl;
    mLch->invalidate();

    mRch->mArea = br;
    mRch->invalidate();
}

void AreaNode::draw(ScreenRenderComponent renderer)
{
    switch (mType)
    {
    case AREA_NODE_TYPE_LEAF:
    case AREA_NODE_TYPE_FLOAT:
        mTabControl.draw(renderer);
        break;
    case AREA_NODE_TYPE_SPLIT:
        mSplitControl.on_draw(renderer);
        break;
    }
}

void AreaNode::split_control_on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    AreaNode& self = *(AreaNode*)widget.get_user();
    Rect area = widget.get_rect();
    UITheme theme = widget.get_theme();

    Color color = theme.get_background_color();
    if (widget.is_hovered())
        color = theme.get_surface_color();

    if (self.mSplitAxis == AXIS_X)
    {
        area.x += 1.0f;
        area.w -= 2.0f;
    }
    else
    {
        area.y += 1.0f;
        area.h -= 2.0f;
    }

    renderer.draw_rect(area, color);
}

void AreaNode::split_control_on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    AreaNode* node = (AreaNode*)widget.get_user();

    LD_ASSERT(node->mArea.w > 0.0f);
    LD_ASSERT(node->mArea.h > 0.0f);

    if (btn != MOUSE_BUTTON_LEFT)
        return;

    float ratio;

    if (node->mSplitAxis == AXIS_X)
        ratio = (dragPos.x - node->mArea.x) / node->mArea.w;
    else
        ratio = (dragPos.y - node->mArea.y) / node->mArea.h;

    node->invalidate_split_ratio(ratio);
}

void AreaNode::split_control_on_enter(UIWidget widget)
{
    Application app = Application::get();
    AreaNode* node = (AreaNode*)widget.get_user();

    app.hint_cursor_shape(node->mSplitAxis == AXIS_X ? CURSOR_TYPE_HRESIZE : CURSOR_TYPE_VRESIZE);
}

void AreaNode::split_control_on_leave(UIWidget widget)
{
    Application app = Application::get();

    app.hint_cursor_shape(CURSOR_TYPE_DEFAULT);
}

} // namespace LD