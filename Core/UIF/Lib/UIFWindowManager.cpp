#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UIF/UIFWindow.h>
#include <Ludens/UIF/UIFWindowManager.h>

#define INVALID_WINDOW_AREA 0
#define WINDOW_AREA_MARGIN 6.0f
#define TOP_BAR_HEIGHT 25.0f

namespace LD {
namespace UIF {

struct AreaNode
{
    AreaNode* lch;                          /// left or top child area
    AreaNode* rch;                          /// right or bottom child area
    Window window;                          /// leaf nodes represent a window
    WindowAreaID areaID;
    Rect area;

    void invalidate(Rect newArea)
    {
        LD_ASSERT(window && !lch && !rch); // only leaf nodes are windows

        window.set_pos(area.get_pos());
        window.set_size(area.get_size());
    }
};

/// @brief Window Manager Implementation.
class WindowManagerObj
{
public:
    WindowManagerObj(const WindowManagerInfo& wmInfo);
    WindowManagerObj(const WindowManagerObj&) = delete;
    ~WindowManagerObj();

    WindowManagerObj& operator=(const WindowManagerObj&) = delete;

    void update(float delta);

    Window create_window(const Vec2& extent, const char* name);

    AreaNode* alloc_node(const Rect& area);
    AreaNode* get_root();
    AreaNode* get_node(WindowAreaID areaID, AreaNode* root);

    WindowAreaID split_right(WindowAreaID areaID, float ratio);

    void render(ScreenRenderComponent renderer, AreaNode* node);

    void get_workspace_windows_recursive(std::vector<Window>& windows, AreaNode* node);

private:
    Context mCtx;
    PoolAllocator mNodePA;
    AreaNode* mRoot;
    WindowAreaID mAreaIDCounter;
};

WindowManagerObj::WindowManagerObj(const WindowManagerInfo& wmInfo)
    : mAreaIDCounter(1), mRoot(nullptr)
{
    UIF::ContextInfo ctxI{};
    ctxI.fontAtlas = wmInfo.fontAtlas;
    ctxI.fontAtlasImage = wmInfo.fontAtlasImage;
    mCtx = UIF::Context::create(ctxI);

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_MISC;
    paI.blockSize = sizeof(AreaNode);
    paI.pageSize = 16;
    paI.isMultiPage = true;
    mNodePA = PoolAllocator::create(paI);

    Rect rootArea(0, TOP_BAR_HEIGHT, wmInfo.screenSize.x, wmInfo.screenSize.y - TOP_BAR_HEIGHT);

    mRoot = alloc_node(rootArea);
    mRoot->areaID = mAreaIDCounter++;
    mRoot->window = create_window(rootArea.get_size(), "window");
    mRoot->window.set_pos(rootArea.get_pos());
}

WindowManagerObj::~WindowManagerObj()
{
    // TODO: delete windows recusrively

    PoolAllocator::destroy(mNodePA);

    UIF::Context::destroy(mCtx);
}

void WindowManagerObj::update(float delta)
{
    // updates the actual window layout
    mCtx.update(delta);
}

Window WindowManagerObj::create_window(const Vec2& extent, const char* name)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_Y;
    layoutI.childGap = 0.0f;
    layoutI.childPadding = {16, 16, 16, 16};
    layoutI.sizeX = UISize::fixed(extent.x);
    layoutI.sizeY = UISize::fixed(extent.y);

    WindowInfo windowI{};
    windowI.name = name;
    windowI.defaultMouseControls = false;

    return mCtx.add_window(layoutI, windowI);
}

AreaNode* WindowManagerObj::alloc_node(const Rect& area)
{
    AreaNode* node = (AreaNode*)mNodePA.allocate();
    node->window = {};
    node->lch = nullptr;
    node->rch = nullptr;
    node->area = area;
    node->areaID = INVALID_WINDOW_AREA;

    return node;
}

AreaNode* WindowManagerObj::get_root()
{
    return mRoot;
}

// NOTE: When creating and destroy areas, existing AreaNode* can get invalidated.
//       This silly recursive search grabs the latest AreaNode* by ID matching.
AreaNode* WindowManagerObj::get_node(WindowAreaID areaID, AreaNode* node)
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

WindowAreaID WindowManagerObj::split_right(WindowAreaID areaID, float ratio)
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

void WindowManagerObj::render(ScreenRenderComponent renderer, AreaNode* node)
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

void WindowManagerObj::get_workspace_windows_recursive(std::vector<Window>& windows, AreaNode* node)
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

WindowManager WindowManager::create(const WindowManagerInfo& wmInfo)
{
    WindowManagerObj* obj = heap_new<WindowManagerObj>(MEMORY_USAGE_MISC, wmInfo);

    return {obj};
}

void WindowManager::destroy(WindowManager wm)
{
    WindowManagerObj* obj = wm;

    heap_delete<WindowManagerObj>(obj);
}

void WindowManager::update(float delta)
{
    mObj->update(delta);
}

void WindowManager::render(ScreenRenderComponent renderer)
{
    AreaNode* root = mObj->get_root();

    mObj->render(renderer, root);
}

WindowAreaID WindowManager::get_root_area()
{
    return mObj->get_root()->areaID;
}

Window WindowManager::get_area_window(WindowAreaID areaID)
{
    AreaNode* root = mObj->get_root();
    AreaNode* node = mObj->get_node(areaID, root);

    return node ? node->window : Window{};
}

void WindowManager::get_workspace_windows(std::vector<Window>& windows)
{
    windows.clear();

    mObj->get_workspace_windows_recursive(windows, mObj->get_root());
}

WindowAreaID WindowManager::split_right(WindowAreaID areaID, float ratio)
{
    return mObj->split_right(areaID, std::clamp(ratio, 0.05f, 0.95f));
}

} // namespace UIF
} // namespace LD