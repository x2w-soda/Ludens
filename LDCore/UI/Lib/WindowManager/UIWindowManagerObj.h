#pragma once

#include <Ludens/UI/UIWindowManager.h>
#include <vector>

#define WINDOW_AREA_MARGIN 6.0f
#define INVALID_WINDOW_AREA 0
#define WINDOW_TAB_HEIGHT 22.0f

namespace LD {

struct AreaNode;

/// @brief Window Manager Implementation.
class UIWindowManagerObj
{
public:
    UIWindowManagerObj(const UIWindowManagerInfo& wmInfo);
    UIWindowManagerObj(const UIWindowManagerObj&) = delete;
    ~UIWindowManagerObj();

    UIWindowManagerObj& operator=(const UIWindowManagerObj&) = delete;

    void update(float delta);

    UIWindow create_window(const Vec2& extent, const char* name);

    UIContext get_context();

    UIWindowAreaID create_float(const Rect& rect);

    UIWindowAreaID get_area_id();

    AreaNode* set_root(AreaNode* root);
    AreaNode* get_root();
    AreaNode* get_node(UIWindowAreaID areaID);
    AreaNode* get_ground_node(UIWindowAreaID areaID, AreaNode* root);
    AreaNode* get_float_node(UIWindowAreaID areaID);

    /// @brief Render non-floating areas.
    void render_ground(ScreenRenderComponent renderer, AreaNode* node);

    /// @brief Render floating areas.
    void render_float(ScreenRenderComponent renderer);

    void get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node);

    inline float get_top_bar_height() const { return mTopBarHeight; }
    inline float get_bottom_bar_height() const { return mBottomBarHeight; }

private:
    UIContext mCtx;
    AreaNode* mRoot;
    std::vector<AreaNode*> mFloats;
    UIWindowAreaID mAreaIDCounter;
    float mTopBarHeight;
    float mBottomBarHeight;
};

} // namespace LD