#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindowManager.h>

#define INVALID_WINDOW_AREA 0
#define WINDOW_AREA_MARGIN 6.0f
#define TOP_BAR_HEIGHT 25.0f

namespace LD {

struct AreaNode
{
    AreaNode* lch;   /// left or top child area
    AreaNode* rch;   /// right or bottom child area
    UIWindow window; /// leaf nodes represent a window
    UIWindowAreaID areaID;
    Rect area;

    void invalidate(Rect newArea)
    {
        LD_ASSERT(window && !lch && !rch); // only leaf nodes are windows

        window.set_pos(area.get_pos());
        window.set_size(area.get_size());
    }
};

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

    AreaNode* alloc_node(const Rect& area);
    AreaNode* get_root();
    AreaNode* get_node(UIWindowAreaID areaID, AreaNode* root);

    UIWindowAreaID split_right(UIWindowAreaID areaID, float ratio);

    void render(ScreenRenderComponent renderer, AreaNode* node);

    void get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node);

private:
    UIContext mCtx;
    PoolAllocator mNodePA;
    UIWindow mTopbar;
    AreaNode* mRoot;
    UIWindowAreaID mAreaIDCounter;
};

UIWindowManagerObj::UIWindowManagerObj(const UIWindowManagerInfo& wmInfo)
    : mAreaIDCounter(1), mRoot(nullptr)
{
    UIContextInfo ctxI{};
    ctxI.fontAtlas = wmInfo.fontAtlas;
    ctxI.fontAtlasImage = wmInfo.fontAtlasImage;
    mCtx = UIContext::create(ctxI);

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_MISC;
    paI.blockSize = sizeof(AreaNode);
    paI.pageSize = 16;
    paI.isMultiPage = true;
    mNodePA = PoolAllocator::create(paI);

    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_X;
    layoutI.childGap = 0.0f;
    layoutI.childPadding = {};
    layoutI.sizeX = UISize::fixed(wmInfo.screenSize.x);
    layoutI.sizeY = UISize::fixed(TOP_BAR_HEIGHT);
    UIWindowInfo windowI{};
    windowI.name = "topbar";
    windowI.defaultMouseControls = false;
    mTopbar = mCtx.add_window(layoutI, windowI, nullptr);
    mTopbar.set_pos(Vec2(0.0f, 0.0f));

    Rect rootArea(0, TOP_BAR_HEIGHT, wmInfo.screenSize.x, wmInfo.screenSize.y - TOP_BAR_HEIGHT);

    mRoot = alloc_node(rootArea);
    mRoot->areaID = mAreaIDCounter++;
    mRoot->window = create_window(rootArea.get_size(), "window");
    mRoot->window.set_pos(rootArea.get_pos());
}

UIWindowManagerObj::~UIWindowManagerObj()
{
    // TODO: delete windows recusrively

    PoolAllocator::destroy(mNodePA);

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
    layoutI.childPadding = {16, 16, 16, 16};
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
    return mTopbar;
}

AreaNode* UIWindowManagerObj::alloc_node(const Rect& area)
{
    AreaNode* node = (AreaNode*)mNodePA.allocate();
    node->window = {};
    node->lch = nullptr;
    node->rch = nullptr;
    node->area = area;
    node->areaID = INVALID_WINDOW_AREA;

    return node;
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

    if (node->areaID == areaID)
        return node;

    AreaNode* match = nullptr;

    if (node->lch && (match = get_node(areaID, node->lch)))
        return match;

    if (node->rch && (match = get_node(areaID, node->rch)))
        return match;

    return nullptr;
}

UIWindowAreaID UIWindowManagerObj::split_right(UIWindowAreaID areaID, float ratio)
{
    AreaNode* node = get_node(areaID, mRoot);
    if (!node)
        return INVALID_WINDOW_AREA;

    Rect leftArea = node->area;
    leftArea.w = node->area.w * ratio;
    leftArea.w -= WINDOW_AREA_MARGIN / 2.0f;

    node->lch = alloc_node(leftArea);
    node->lch->areaID = node->areaID;
    node->lch->window = node->window;
    node->lch->invalidate(leftArea);

    Rect rightArea = node->area;
    rightArea.x += leftArea.w + WINDOW_AREA_MARGIN;
    rightArea.w = node->area.w * (1.0f - ratio) - WINDOW_AREA_MARGIN / 2.0f;

    node->rch = alloc_node(rightArea);
    node->rch->areaID = mAreaIDCounter++;
    node->rch->window = create_window(rightArea.get_size(), "window");
    node->rch->invalidate(rightArea);

    // becomes non-leaf node
    node->areaID = INVALID_WINDOW_AREA;
    node->window = {};

    return node->rch->areaID;
}

void UIWindowManagerObj::render(ScreenRenderComponent renderer, AreaNode* node)
{
    if (!node)
        return;

    if (node->lch)
        render(renderer, node->lch);

    if (node->rch)
        render(renderer, node->rch);

    // render window area on leaf node
    if (node->window)
        node->window.on_draw(renderer);
}

void UIWindowManagerObj::get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node)
{
    if (!node)
        return;

    // only leaf nodes are windows
    if (node->window)
        windows.push_back(node->window);

    if (node->lch)
        get_workspace_windows_recursive(windows, node->lch);

    if (node->rch)
        get_workspace_windows_recursive(windows, node->rch);
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

void UIWindowManager::render(ScreenRenderComponent renderer)
{
    UIWindow topbar = mObj->get_topbar_window();
    topbar.on_draw(renderer);

    AreaNode* root = mObj->get_root();
    mObj->render(renderer, root);
}

UIContext UIWindowManager::get_context()
{
    return mObj->get_context();
}

UIWindowAreaID UIWindowManager::get_root_area()
{
    return mObj->get_root()->areaID;
}

UIWindow UIWindowManager::get_topbar_window()
{
    return mObj->get_topbar_window();
}

UIWindow UIWindowManager::get_area_window(UIWindowAreaID areaID)
{
    AreaNode* root = mObj->get_root();
    AreaNode* node = mObj->get_node(areaID, root);

    return node ? node->window : UIWindow{};
}

void UIWindowManager::get_workspace_windows(std::vector<UIWindow>& windows)
{
    windows.clear();

    mObj->get_workspace_windows_recursive(windows, mObj->get_root());
}

UIWindowAreaID UIWindowManager::split_right(UIWindowAreaID areaID, float ratio)
{
    return mObj->split_right(areaID, std::clamp(ratio, 0.05f, 0.95f));
}

} // namespace LD