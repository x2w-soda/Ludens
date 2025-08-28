#include "WindowManager/AreaNode.h"
#include "WindowManager/AreaTab.h"
#include "WindowManager/UIWindowManagerObj.h"
#include <Ludens/Application/Application.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWindowManager.h>
#include <cstring>
#include <vector>

namespace LD {

struct AreaTab;
struct AreaNode;

static void delete_node(AreaNode* node)
{
    if (!node)
        return;

    delete_node(node->get_lch());
    delete_node(node->get_rch());

    node->cleanup();
    heap_delete<AreaNode>(node);
}

UIWindowManagerObj::UIWindowManagerObj(const UIWindowManagerInfo& wmInfo)
    : mAreaIDCounter(1), mRoot(nullptr)
{
    UIContextInfo ctxI{};
    ctxI.fontAtlas = wmInfo.fontAtlas;
    ctxI.fontAtlasImage = wmInfo.fontAtlasImage;
    ctxI.theme = wmInfo.theme;
    mCtx = UIContext::create(ctxI);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.childGap = 6.0f;
    layoutI.childPadding = {.left = 6.0f};
    layoutI.sizeX = UISize::fixed(wmInfo.screenSize.x);
    layoutI.sizeY = UISize::fixed(TOPBAR_HEIGHT);
    UIWindowInfo windowI{};
    windowI.name = "topbar";
    windowI.defaultMouseControls = false;
    mTopbarWindow = mCtx.add_window(layoutI, windowI, nullptr);
    mTopbarWindow.set_pos(Vec2(0.0f, 0.0f));

    Rect rootArea(0, TOPBAR_HEIGHT, wmInfo.screenSize.x, wmInfo.screenSize.y - TOPBAR_HEIGHT);

    UIWindowAreaID areaID = mAreaIDCounter++;
    UIWindow rootWindow = create_window(rootArea.get_size(), "window");
    rootWindow.set_pos(rootArea.get_pos());
    mCtx.layout(); // force root window size

    mRoot = heap_new<AreaNode>(MEMORY_USAGE_UI);
    mRoot->startup_as_leaf(mCtx, areaID, rootArea, rootWindow);
    mRoot->set_area(rootArea);
    mRoot->invalidate();
}

UIWindowManagerObj::~UIWindowManagerObj()
{
    delete_node(mRoot);

    UIContext::destroy(mCtx);
}

void UIWindowManagerObj::update(float delta)
{
    // updates the actual window layout
    mCtx.update(delta);
}

UIWindow UIWindowManagerObj::create_window(const Vec2& extent, const char* name)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_Y;
    layoutI.childGap = 0.0f;
    layoutI.childPadding = {};
    layoutI.sizeX = UISize::fixed(extent.x);
    layoutI.sizeY = UISize::fixed(extent.y);

    UIWindowInfo windowI{};
    windowI.name = name;
    windowI.defaultMouseControls = false;

    return mCtx.add_window(layoutI, windowI, nullptr);
}

UIContext UIWindowManagerObj::get_context()
{
    return mCtx;
}

UIWindow UIWindowManagerObj::get_topbar_window()
{
    return mTopbarWindow;
}

UIWindowAreaID UIWindowManagerObj::get_area_id()
{
    return mAreaIDCounter++;
}

AreaNode* UIWindowManagerObj::set_root(AreaNode* root)
{
    return mRoot = root;
}

AreaNode* UIWindowManagerObj::get_root()
{
    return mRoot;
}

// NOTE: When creating and destroy areas, existing AreaNode* can get invalidated.
//       This silly recursive search grabs the latest AreaNode* by ID matching.
AreaNode* UIWindowManagerObj::get_node(UIWindowAreaID areaID, AreaNode* node)
{
    if (!node)
        return nullptr;

    if (node->get_area_id() == areaID)
        return node;

    AreaNode* match = nullptr;
    AreaNode* lch = node->get_lch();
    AreaNode* rch = node->get_rch();

    if (lch && (match = get_node(areaID, lch)))
        return match;

    if (rch && (match = get_node(areaID, rch)))
        return match;

    return nullptr;
}

void UIWindowManagerObj::render(ScreenRenderComponent renderer, AreaNode* node)
{
    if (!node)
        return;

    AreaNode* lch = node->get_lch();
    AreaNode* rch = node->get_rch();

    if (lch)
        render(renderer, lch);

    if (rch)
        render(renderer, rch);

    node->draw(renderer);
}

void UIWindowManagerObj::get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node)
{
    if (!node)
        return;

    // only leaf nodes are windows
    if (node->get_type() == AREA_NODE_TYPE_LEAF)
    {
        AreaTab* tab = node->get_active_tab();
        windows.push_back(tab->client);
    }

    if (node->get_lch())
        get_workspace_windows_recursive(windows, node->get_lch());

    if (node->get_rch())
        get_workspace_windows_recursive(windows, node->get_rch());
}

UIWindowManager UIWindowManager::create(const UIWindowManagerInfo& wmInfo)
{
    UIWindowManagerObj* obj = heap_new<UIWindowManagerObj>(MEMORY_USAGE_MISC, wmInfo);

    return {obj};
}

void UIWindowManager::destroy(UIWindowManager wm)
{
    UIWindowManagerObj* obj = wm;

    heap_delete<UIWindowManagerObj>(obj);
}

void UIWindowManager::update(float delta)
{
    mObj->update(delta);
}

void UIWindowManager::resize(const Vec2& screenSize)
{
    UIWindow topbar = mObj->get_topbar_window();
    topbar.set_size(Vec2(screenSize.x, TOPBAR_HEIGHT));

    AreaNode* root = mObj->get_root();
    root->set_area(Rect(0, TOPBAR_HEIGHT, screenSize.x, screenSize.y - TOPBAR_HEIGHT));
    root->invalidate();
}

void UIWindowManager::render(ScreenRenderComponent renderer)
{
    AreaNode* root = mObj->get_root();
    mObj->render(renderer, root);

    UIWindow topbar = mObj->get_topbar_window();
    topbar.on_draw(renderer);
}

void UIWindowManager::set_window_title(UIWindowAreaID areaID, const char* title)
{
    AreaNode* node = mObj->get_node(areaID, mObj->get_root());

    if (!node || node->get_type() != AREA_NODE_TYPE_LEAF)
        return;

    AreaTab* tab = node->get_active_tab();
    tab->titleText.set_text(title);
}

void UIWindowManager::set_on_window_resize(UIWindowAreaID areaID, void (*onWindowResize)(UIWindow window, const Vec2& size))
{
    AreaNode* node = mObj->get_node(areaID, mObj->get_root());

    if (!node || node->get_type() != AREA_NODE_TYPE_LEAF)
        return;

    AreaTab* tab = node->get_active_tab();
    tab->onWindowResize = onWindowResize;
}

UIContext UIWindowManager::get_context()
{
    return mObj->get_context();
}

UIWindowAreaID UIWindowManager::get_root_area()
{
    return mObj->get_root()->get_area_id();
}

UIWindow UIWindowManager::get_topbar_window()
{
    return mObj->get_topbar_window();
}

UIWindow UIWindowManager::get_area_window(UIWindowAreaID areaID)
{
    AreaNode* node = mObj->get_node(areaID, mObj->get_root());

    if (!node || node->get_type() != AREA_NODE_TYPE_LEAF)
        return {};

    AreaTab* tab = node->get_active_tab();
    return tab->client;
}

void UIWindowManager::get_workspace_windows(std::vector<UIWindow>& windows)
{
    windows.clear();

    mObj->get_workspace_windows_recursive(windows, mObj->get_root());
}

UIWindowAreaID UIWindowManager::split_right(UIWindowAreaID areaID, float ratio)
{
    AreaNode* node = mObj->get_node(areaID, mObj->get_root());

    if (node && node->get_type() == AREA_NODE_TYPE_LEAF)
    {
        return node->split_right(mObj, ratio);
    }

    return INVALID_WINDOW_AREA;
}

UIWindowAreaID UIWindowManager::split_bottom(UIWindowAreaID areaID, float ratio)
{
    AreaNode* node = mObj->get_node(areaID, mObj->get_root());

    if (node && node->get_type() == AREA_NODE_TYPE_LEAF)
    {
        return node->split_bottom(mObj, ratio);
    }

    return INVALID_WINDOW_AREA;
}

} // namespace LD