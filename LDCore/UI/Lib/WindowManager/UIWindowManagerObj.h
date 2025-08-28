#pragma once

#include <Ludens/UI/UIWindowManager.h>

#define WINDOW_AREA_MARGIN 6.0f
#define INVALID_WINDOW_AREA 0
#define WINDOW_TAB_HEIGHT 22.0f
#define TOPBAR_HEIGHT 22.0f

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

    UIWindow get_topbar_window();

    UIWindowAreaID get_area_id();

    AreaNode* set_root(AreaNode* root);
    AreaNode* get_root();
    AreaNode* get_node(UIWindowAreaID areaID, AreaNode* root);

    void render(ScreenRenderComponent renderer, AreaNode* node);

    void get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node);

private:
    UIContext mCtx;
    UIWindow mTopbarWindow;
    AreaNode* mRoot;
    UIWindowAreaID mAreaIDCounter;
};

} // namespace LD