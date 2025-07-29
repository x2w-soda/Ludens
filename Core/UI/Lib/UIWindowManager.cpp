#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindowManager.h>

#define INVALID_WINDOW_AREA 0
#define WINDOW_AREA_MARGIN 6.0f
#define TOPBAR_HEIGHT 25.0f

namespace LD {

enum SplitAxis
{
    SPLIT_AXIS_X,
    SPLIT_AXIS_Y,
};

struct AreaNode
{
    AreaNode* lch;   /// left or top child area
    AreaNode* rch;   /// right or bottom child area
    UIWindow window; /// leaf nodes represent a window
    void (*onWindowResize)(UIWindow window, const Vec2& size);
    UIWindowAreaID areaID;
    Rect area;
    SplitAxis splitAxis;
    float splitRatio;

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

    void resize(AreaNode* node);

    UIWindow create_window(const Vec2& extent, const char* name);

    UIContext get_context();

    UIWindow get_topbar_window();

    AreaNode* alloc_node(const Rect& area);
    AreaNode* get_root();
    AreaNode* get_node(UIWindowAreaID areaID, AreaNode* root);

    UIWindowAreaID split_right(UIWindowAreaID areaID, float ratio);

    void render(ScreenRenderComponent renderer, AreaNode* node);

    void get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node);

    void split_area(SplitAxis axis, float ratio, const Rect& area, Rect& tl, Rect& br);

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
    layoutI.sizeY = UISize::fixed(TOPBAR_HEIGHT);
    UIWindowInfo windowI{};
    windowI.name = "topbar";
    windowI.defaultMouseControls = false;
    mTopbar = mCtx.add_window(layoutI, windowI, nullptr);
    mTopbar.set_pos(Vec2(0.0f, 0.0f));

    Rect rootArea(0, TOPBAR_HEIGHT, wmInfo.screenSize.x, wmInfo.screenSize.y - TOPBAR_HEIGHT);

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

void UIWindowManagerObj::resize(AreaNode* node)
{
    if (!node)
        return;

    if (!node->lch && !node->rch)
    {
        node->window.set_pos(node->area.get_pos());
        node->window.set_size(node->area.get_size());

        if (node->onWindowResize)
            node->onWindowResize(node->window, node->area.get_size());

        return;
    }

    Rect tl, br;
    split_area(node->splitAxis, node->splitRatio, node->area, tl, br);

    if (node->lch)
    {
        node->lch->area = tl;
        resize(node->lch);
    }

    if (node->rch)
    {
        node->rch->area = br;
        resize(node->rch);
    }
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
    node->onWindowResize = nullptr;
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

    Rect leftArea, rightArea;
    split_area(SPLIT_AXIS_X, ratio, node->area, leftArea, rightArea);

    node->lch = alloc_node(leftArea);
    node->lch->areaID = node->areaID;
    node->lch->window = node->window;
    node->lch->invalidate(leftArea);

    node->rch = alloc_node(rightArea);
    node->rch->areaID = mAreaIDCounter++;
    node->rch->window = create_window(rightArea.get_size(), "window");
    node->rch->invalidate(rightArea);

    // becomes non-leaf node
    node->areaID = INVALID_WINDOW_AREA;
    node->window = {};
    node->splitAxis = SPLIT_AXIS_X;
    node->splitRatio = ratio;

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

void UIWindowManagerObj::split_area(SplitAxis axis, float ratio, const Rect& area, Rect& tl, Rect& br)
{
    if (axis == SPLIT_AXIS_X)
    {
        tl = area;
        tl.w = area.w * ratio;
        tl.w -= WINDOW_AREA_MARGIN / 2.0f;

        br = area;
        br.x += tl.w + WINDOW_AREA_MARGIN;
        br.w = area.w * (1.0f - ratio) - WINDOW_AREA_MARGIN / 2.0f;
    }
    else // SPLIT_AXIS_Y
    {
        tl = area;
        tl.h = area.h * ratio;
        tl.h -= WINDOW_AREA_MARGIN / 2.0f;

        br = area;
        br.y += tl.h + WINDOW_AREA_MARGIN;
        br.h = area.h * (1.0f - ratio) - WINDOW_AREA_MARGIN / 2.0f;
    }
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
    root->area.set_size(screenSize.x, screenSize.y - TOPBAR_HEIGHT);

    mObj->resize(root);
}

void UIWindowManager::render(ScreenRenderComponent renderer)
{
    UIWindow topbar = mObj->get_topbar_window();
    topbar.on_draw(renderer);

    AreaNode* root = mObj->get_root();
    mObj->render(renderer, root);
}

void UIWindowManager::set_on_window_resize(UIWindowAreaID areaID, void (*onWindowResize)(UIWindow window, const Vec2& size))
{
    AreaNode* node = mObj->get_node(areaID, mObj->get_root());

    if (!node)
        return;

    node->onWindowResize = onWindowResize;
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