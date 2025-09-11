#pragma once

#include "AreaTab.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Directional.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/UI/UIWindowManager.h>
#include <vector>

namespace LD {

struct UIWindowManagerObj;
struct AreaTab;

enum AreaNodeType
{
    // uninitialized state
    AREA_NODE_TYPE_UNKNOWN,

    // leaf node in workspace, represents a window area
    AREA_NODE_TYPE_LEAF,

    // represents a split partition, has two children nodes
    AREA_NODE_TYPE_SPLIT,

    // represents a floating window area
    AREA_NODE_TYPE_FLOAT,
};

class AreaNode
{
public:
    AreaNode();
    AreaNode(const AreaNode&) = delete;
    ~AreaNode();

    AreaNode& operator=(const AreaNode&) = delete;

    void startup_as_split(UIContext ctx, UIWMAreaID areaID, const Rect& area, Axis axis, float ratio, const Rect& splitArea);
    void startup_as_leaf(UIContext ctx, UIWMAreaID areaID, const Rect& area, UIWindow client);
    void startup_as_float(UIContext ctx, UIWMAreaID areaID, const Rect& area, UIWindow client, float border, void* user);
    void cleanup();

    /// @brief Recusrive invalidation based on current area rect.
    void invalidate();

    // non-recursive, triggers optional window resize callback for user
    void invalidate_area(const Rect& newArea);

    // recursive, subtrees are invalidated
    void invalidate_split_ratio(float newRatio);

    inline AreaNodeType get_type() const { return mType; }
    inline UIWMAreaID get_area_id() const { return mAreaID; }
    inline AreaNode* get_lch() { return mLch; }
    inline AreaNode* get_rch() { return mRch; }
    inline Rect get_area() const { return mArea; }
    inline void set_area(const Rect& area) { mArea = area; }

    inline float get_split_ratio() const
    {
        LD_ASSERT(mType == AREA_NODE_TYPE_SPLIT);
        return mSplitRatio;
    }

    inline AreaTab* get_active_tab()
    {
        LD_ASSERT(mType != AREA_NODE_TYPE_SPLIT);
        return mTabControl.get_active_tab();
    }

    inline size_t get_tab_count() const
    {
        LD_ASSERT(mType != AREA_NODE_TYPE_SPLIT);
        return mTabControl.get_tab_count();
    }

    UIWMAreaID split_right(UIWindowManagerObj* wm, float ratio);
    UIWMAreaID split_bottom(UIWindowManagerObj* wm, float ratio);

    void show();
    void hide();
    void draw(ScreenRenderComponent renderer);

private:
    static void split_control_on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void split_control_on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    static void split_control_on_hover(UIWidget widget, UIEvent event);

    AreaNode* mParent;          /// parent node
    AreaNode* mLch;             /// left or top child area
    AreaNode* mRch;             /// right or bottom child area
    AreaTabControl mTabControl; /// tab control window, leaf nodes only
    UIWindow mSplitControl;     /// split control window, non-leaf nodes only
    UIWMAreaID mAreaID;         /// unique ID per node
    AreaNodeType mType;
    Rect mArea;
    Axis mSplitAxis;
    float mSplitRatio;
};

} // namespace LD