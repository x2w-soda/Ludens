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
#include <unordered_set>
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

UIWindowManagerObj::UIWindowManagerObj(const UIWindowManagerInfo& wmI)
    : icons(wmI.icons), mAreaIDCounter(1), mRoot(nullptr), mGroundLayerHash(wmI.groundLayerHash), mFloatLayerHash(wmI.floatLayerHash), mTopBarHeight(wmI.topBarHeight), mBottomBarHeight(wmI.bottomBarHeight)
{
    mIconAtlasImage = wmI.iconAtlasImage;

    UIContextInfo ctxI{};
    ctxI.fontAtlas = wmI.fontAtlas;
    ctxI.fontAtlasImage = wmI.fontAtlasImage;
    ctxI.theme = wmI.theme;
    mCtx = UIContext::create(ctxI);
    mCtx.add_layer(mGroundLayerHash);
    mCtx.add_layer(mFloatLayerHash);

    Rect rootArea(0, mTopBarHeight, wmI.screenSize.x, wmI.screenSize.y - mTopBarHeight - mBottomBarHeight);

    UIWMAreaID areaID = mAreaIDCounter++;
    UIWindow rootWindow = create_window(mGroundLayerHash, rootArea.get_size(), "window");
    rootWindow.set_pos(rootArea.get_pos());
    mCtx.layout(); // force root window size

    mRoot = heap_new<AreaNode>(MEMORY_USAGE_UI);
    mRoot->startup_as_leaf(this, areaID, rootArea, rootWindow);
    mRoot->set_area(rootArea);
    mRoot->invalidate();
}

UIWindowManagerObj::~UIWindowManagerObj()
{
    for (AreaNode* node : mFloats)
        delete_node(node);

    delete_node(mRoot);

    UIContext::destroy(mCtx);
}

void UIWindowManagerObj::update(float delta)
{
    // updates the actual window layout
    mCtx.update(delta);

    std::unordered_set<AreaNode*> toErase;

    for (AreaNode* node : mFloats)
    {
        if (node->get_tab_count() == 0)
        {
            toErase.insert(node);
            delete_node(node);
        }
    }

    std::erase_if(mFloats, [&](AreaNode* node) { return toErase.contains(node); });
}

UIWindow UIWindowManagerObj::create_window(Hash32 layer, const Vec2& extent, const char* name)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_Y;
    layoutI.childGap = 0.0f;
    layoutI.childPadding = {};
    layoutI.sizeX = UISize::fixed(extent.x);
    layoutI.sizeY = UISize::fixed(extent.y);

    UIWindowInfo windowI{};
    windowI.name = name;
    windowI.layer = layer;
    windowI.defaultMouseControls = false;
    windowI.drawWithScissor = true; // TODO: parameterize, not every window needs scissor crop

    return mCtx.add_window(layoutI, windowI, nullptr);
}

UIContext UIWindowManagerObj::get_context()
{
    return mCtx;
}

UIWMAreaID UIWindowManagerObj::create_float(const UIWMClientInfo& clientI)
{
    float border = WINDOW_AREA_MARGIN;

    AreaNode* node = heap_new<AreaNode>(MEMORY_USAGE_UI);
    UIWindow client = clientI.client;
    Rect nodeArea = client.get_rect();
    LD_ASSERT(nodeArea.w > 0.0f && nodeArea.h > 0.0f);

    nodeArea.h += WINDOW_TAB_HEIGHT + border;
    nodeArea.x -= border;
    nodeArea.w += 2 * border;
    node->startup_as_float(this, get_area_id(), nodeArea, client, border, clientI.user);

    mFloats.push_back(node);

    return node->get_area_id();
}

UIWMAreaID UIWindowManagerObj::get_area_id()
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

AreaNode* UIWindowManagerObj::get_node(UIWMAreaID areaID)
{
    AreaNode* node;

    if ((node = get_ground_node(areaID, mRoot)))
        return node;

    return get_float_node(areaID);
}

AreaNode* UIWindowManagerObj::get_ground_node(UIWMAreaID areaID, AreaNode* node)
{
    if (!node)
        return nullptr;

    if (node->get_area_id() == areaID)
        return node;

    AreaNode* match = nullptr;
    AreaNode* lch = node->get_lch();
    AreaNode* rch = node->get_rch();

    if (lch && (match = get_ground_node(areaID, lch)))
        return match;

    if (rch && (match = get_ground_node(areaID, rch)))
        return match;

    return nullptr;
}

AreaNode* UIWindowManagerObj::get_float_node(UIWMAreaID areaID)
{
    for (AreaNode* node : mFloats)
    {
        if (node->get_area_id() == areaID)
            return node;
    }

    return nullptr;
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
    float topBarHeight = mObj->get_top_bar_height();
    float bottomBarHeight = mObj->get_bottom_bar_height();

    AreaNode* root = mObj->get_root();
    root->set_area(Rect(0, topBarHeight, screenSize.x, screenSize.y - topBarHeight - bottomBarHeight));
    root->invalidate();
}

void UIWindowManager::set_window_title(UIWMAreaID areaID, const char* title)
{
    AreaNode* node = mObj->get_node(areaID);

    if (!node || node->get_type() == AREA_NODE_TYPE_SPLIT)
        return;

    AreaTab* tab = node->get_active_tab();
    tab->titleTextW.set_text(title);
}

void UIWindowManager::set_resize_callback(UIWMAreaID areaID, UIWMClientResizeCallback callback)
{
    AreaNode* node = mObj->get_node(areaID);

    if (!node || node->get_type() == AREA_NODE_TYPE_SPLIT)
        return;

    AreaTab* tab = node->get_active_tab();
    tab->onClientResize = callback;
}

void UIWindowManager::set_close_callback(UIWMAreaID areaID, UIWMClientCloseCallback callback)
{
    AreaNode* node = mObj->get_node(areaID);

    if (!node || node->get_type() == AREA_NODE_TYPE_SPLIT)
        return;

    AreaTab* tab = node->get_active_tab();
    tab->onClientClose = callback;
}

UIContext UIWindowManager::get_context()
{
    return mObj->get_context();
}

UIWMAreaID UIWindowManager::get_root_area()
{
    return mObj->get_root()->get_area_id();
}

UIWindow UIWindowManager::get_area_window(UIWMAreaID areaID)
{
    AreaNode* node = mObj->get_node(areaID);

    if (!node || node->get_type() == AREA_NODE_TYPE_SPLIT)
        return {};

    AreaTab* tab = node->get_active_tab();
    return tab->client;
}

void UIWindowManager::get_workspace_windows(std::vector<UIWindow>& windows)
{
    windows.clear();

    mObj->get_workspace_windows_recursive(windows, mObj->get_root());
}

UIWMAreaID UIWindowManager::split_right(UIWMAreaID areaID, float ratio)
{
    AreaNode* node = mObj->get_ground_node(areaID, mObj->get_root());

    if (node && node->get_type() == AREA_NODE_TYPE_LEAF)
    {
        return node->split_right(mObj, ratio);
    }

    return INVALID_WINDOW_AREA;
}

UIWMAreaID UIWindowManager::split_bottom(UIWMAreaID areaID, float ratio)
{
    AreaNode* node = mObj->get_ground_node(areaID, mObj->get_root());

    if (node && node->get_type() == AREA_NODE_TYPE_LEAF)
    {
        return node->split_bottom(mObj, ratio);
    }

    return INVALID_WINDOW_AREA;
}

UIWMAreaID UIWindowManager::create_float(const UIWMClientInfo& clientI)
{
    return mObj->create_float(clientI);
}

void UIWindowManager::set_float_pos_centered(UIWMAreaID areaID)
{
    AreaNode* rootNode = mObj->get_root();
    AreaNode* floatNode = mObj->get_float_node(areaID);
    if (!rootNode || !floatNode)
        return;

    Rect rootArea = rootNode->get_area();
    Rect floatArea = floatNode->get_area();
    set_float_pos(areaID, Vec2((rootArea.w - floatArea.w) / 2.0f, (rootArea.h - floatArea.h) / 2.0f));
}

void UIWindowManager::set_float_pos(UIWMAreaID areaID, const Vec2& pos)
{
    AreaNode* floatNode = mObj->get_float_node(areaID);
    if (!floatNode)
        return;

    Rect nodeArea = floatNode->get_area();
    nodeArea.set_pos(pos.x, pos.y);

    // TODO: update client positions without triggering resize callbacks?
    floatNode->invalidate_area(nodeArea);
}

void UIWindowManager::show_float(UIWMAreaID areaID)
{
    AreaNode* floatNode = mObj->get_float_node(areaID);
    if (!floatNode)
        return;

    floatNode->show();
}

void UIWindowManager::hide_float(UIWMAreaID areaID)
{
    AreaNode* floatNode = mObj->get_float_node(areaID);
    if (!floatNode)
        return;

    floatNode->hide();
}

Hash32 UIWindowManager::get_ground_layer_hash()
{
    return mObj->get_ground_layer_hash();
}

Hash32 UIWindowManager::get_float_layer_hash()
{
    return mObj->get_float_layer_hash();
}

} // namespace LD