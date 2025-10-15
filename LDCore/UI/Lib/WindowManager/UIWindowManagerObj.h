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

    UIWindow create_window(Hash32 layer, const Vec2& extent, const char* name);

    UIContext get_context();

    UIWMAreaID create_float(const UIWMClientInfo& clientI);

    UIWMAreaID get_area_id();

    AreaNode* set_root(AreaNode* root);
    AreaNode* get_root();
    AreaNode* get_node(UIWMAreaID areaID);
    AreaNode* get_ground_node(UIWMAreaID areaID, AreaNode* root);
    AreaNode* get_float_node(UIWMAreaID areaID);

    void get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node);

    inline float get_top_bar_height() const { return mTopBarHeight; }
    inline float get_bottom_bar_height() const { return mBottomBarHeight; }
    inline RImage get_icon_atlas() const { return mIconAtlasImage; };
    inline Hash32 get_float_layer_hash() const { return mFloatLayerHash; }
    inline Hash32 get_ground_layer_hash() const { return mGroundLayerHash; }

public:
    const UIWindowManagerInfo::Icon icons;

private:
    UIContext mCtx;
    AreaNode* mRoot;
    std::vector<AreaNode*> mFloats;
    UIWMAreaID mAreaIDCounter;
    RImage mIconAtlasImage;
    Hash32 mGroundLayerHash;
    Hash32 mFloatLayerHash;
    float mTopBarHeight;
    float mBottomBarHeight;
};

} // namespace LD