#include <Ludens/Application/Application.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWindowManager.h>
#include <cstring>

#define INVALID_WINDOW_AREA 0
#define WINDOW_AREA_MARGIN 6.0f
#define WINDOW_TAB_HEIGHT 22.0f
#define TOPBAR_HEIGHT 22.0f

namespace LD {

struct AreaNode;

enum SplitAxis
{
    SPLIT_AXIS_X,
    SPLIT_AXIS_Y,
};

static void split_area(SplitAxis axis, float ratio, const Rect& area, Rect& tl, Rect& br, Rect& split);
static void invalidate(AreaNode* node);

struct AreaTab
{
    UIWindow window;
    UITextWidget titleText;

    AreaTab(UIContext ctx, const Vec2& pos);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

AreaTab::AreaTab(UIContext ctx, const Vec2& pos)
{
    UILayoutInfo layoutI{};
    layoutI.childAxis = UIAxis::UI_AXIS_Y;
    layoutI.childGap = 0.0f;
    layoutI.childPadding = {.left = 10.0f, .right = 10.0f};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(WINDOW_TAB_HEIGHT);

    UIWindowInfo windowI{};
    windowI.name = "windowTab";
    windowI.defaultMouseControls = false;

    window = ctx.add_window(layoutI, windowI, this);
    window.set_pos(pos);
    window.set_on_draw(AreaTab::on_draw);

    UITextWidgetInfo textWI{};
    textWI.cstr = nullptr;
    textWI.fontSize = WINDOW_TAB_HEIGHT * 0.7f; // TODO:
    textWI.hoverHL = false;
    titleText = window.node().add_text({}, textWI, this);
}

void AreaTab::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    AreaTab& self = *(AreaTab*)widget.get_user();
    UITheme theme = self.window.get_theme();

    Rect rect = widget.get_rect();
    renderer.draw_rect(rect, theme.get_surface_color());

    self.titleText.on_draw(renderer);
}

struct AreaNode
{
    AreaNode* lch;         /// left or top child area
    AreaNode* rch;         /// right or bottom child area
    AreaTab* tab;          /// corresponding window tab
    UIWindow window;       /// leaf nodes represent a window
    UIWindow splitControl; /// non-leaf nodes represent a split
    void (*onWindowResize)(UIWindow window, const Vec2& size);
    UIWindowAreaID areaID;
    Rect area;
    SplitAxis splitAxis;
    float splitRatio;

    // non-recursive, triggers optional window resize callback for user
    void invalidate_area(Rect newArea)
    {
        LD_ASSERT(window && !lch && !rch); // only leaf nodes are windows
        LD_ASSERT(tab && tab->window && tab->titleText);

        area = newArea;

        tab->window.set_pos(area.get_pos());

        Rect windowArea(area.x, area.y + WINDOW_TAB_HEIGHT, area.w, area.h - WINDOW_TAB_HEIGHT);
        window.set_rect(windowArea);

        if (onWindowResize)
            onWindowResize(window, windowArea.get_size());
    }

    // recursive, subtrees are invalidated
    void invalidate_split_ratio(float newRatio)
    {
        LD_ASSERT(splitControl && lch && rch); // only non-leaf nodes are splits

        newRatio = std::clamp<float>(newRatio, 0.05f, 0.95f);
        splitRatio = newRatio;

        Rect tl, br, splitArea;
        split_area(splitAxis, splitRatio, area, tl, br, splitArea);
        splitControl.set_rect(splitArea);

        lch->area = tl;
        invalidate(lch);

        rch->area = br;
        invalidate(rch);
    }

    static void split_control_on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void split_control_on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    static void split_control_on_enter(UIWidget widget);
    static void split_control_on_leave(UIWidget widget);
};

void AreaNode::split_control_on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    AreaNode& self = *(AreaNode*)widget.get_user();
    Rect area = widget.get_rect();
    UITheme theme = widget.get_theme();

    Color color = theme.get_background_color();
    if (widget.is_hovered())
        color = theme.get_surface_color();

    if (self.splitAxis == SPLIT_AXIS_X)
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

    if (btn != MOUSE_BUTTON_LEFT)
        return;

    float ratio;

    if (node->splitAxis == SPLIT_AXIS_X)
        ratio = (dragPos.x - node->area.x) / node->area.w;
    else
        ratio = (dragPos.y - node->area.y) / node->area.h;

    node->invalidate_split_ratio(ratio);
}

void AreaNode::split_control_on_enter(UIWidget widget)
{
    Application app = Application::get();
    AreaNode* node = (AreaNode*)widget.get_user();

    app.hint_cursor_shape(node->splitAxis == SPLIT_AXIS_X ? CURSOR_TYPE_HRESIZE : CURSOR_TYPE_VRESIZE);
}

void AreaNode::split_control_on_leave(UIWidget widget)
{
    Application app = Application::get();

    app.hint_cursor_shape(CURSOR_TYPE_DEFAULT);
}

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
    UIWindowAreaID split_bottom(UIWindowAreaID areaID, float ratio);

    void render(ScreenRenderComponent renderer, AreaNode* node);

    void get_workspace_windows_recursive(std::vector<UIWindow>& windows, AreaNode* node);

private:
    UIContext mCtx;
    PoolAllocator mNodePA;
    UIWindow mTopbarWindow;
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

    mRoot = alloc_node(rootArea);
    mRoot->areaID = mAreaIDCounter++;
    mRoot->window = create_window(rootArea.get_size(), "window");
    mRoot->window.set_pos(rootArea.get_pos() + Vec2(0.0f, WINDOW_TAB_HEIGHT));
    mRoot->tab = heap_new<AreaTab>(MEMORY_USAGE_UI, mCtx, rootArea.get_pos());
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

AreaNode* UIWindowManagerObj::alloc_node(const Rect& area)
{
    AreaNode* node = (AreaNode*)mNodePA.allocate();
    memset(node, 0, sizeof(AreaNode));
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

    ratio = std::clamp(ratio, 0.05f, 0.95f);

    Rect leftArea, rightArea, splitArea;
    split_area(SPLIT_AXIS_X, ratio, node->area, leftArea, rightArea, splitArea);

    node->lch = alloc_node(leftArea);
    node->lch->areaID = node->areaID;
    node->lch->window = node->window;
    node->lch->tab = node->tab;
    node->lch->invalidate_area(leftArea);

    node->rch = alloc_node(rightArea);
    node->rch->areaID = mAreaIDCounter++;
    node->rch->window = create_window(rightArea.get_size(), "window");
    node->rch->tab = heap_new<AreaTab>(MEMORY_USAGE_UI, mCtx, rightArea.get_pos());
    node->rch->invalidate_area(rightArea);

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(WINDOW_AREA_MARGIN);
    layoutI.sizeY = UISize::fixed(node->area.h);
    UIWindowInfo windowI{};
    windowI.name = "splitControl";
    windowI.defaultMouseControls = false;

    // becomes non-leaf node
    node->areaID = INVALID_WINDOW_AREA;
    node->window = {};
    node->tab = {};
    node->splitControl = mCtx.add_window(layoutI, windowI, node);
    node->splitControl.set_rect(splitArea);
    node->splitControl.set_on_draw(&AreaNode::split_control_on_draw);
    node->splitControl.set_on_drag(&AreaNode::split_control_on_drag);
    node->splitControl.set_on_enter(&AreaNode::split_control_on_enter);
    node->splitControl.set_on_leave(&AreaNode::split_control_on_leave);
    node->splitAxis = SPLIT_AXIS_X;
    node->splitRatio = ratio;

    return node->rch->areaID;
}

UIWindowAreaID UIWindowManagerObj::split_bottom(UIWindowAreaID areaID, float ratio)
{
    AreaNode* node = get_node(areaID, mRoot);
    if (!node)
        return INVALID_WINDOW_AREA;

    ratio = std::clamp(ratio, 0.05f, 0.95f);

    Rect topArea, bottomArea, splitArea;
    split_area(SPLIT_AXIS_Y, ratio, node->area, topArea, bottomArea, splitArea);

    node->lch = alloc_node(topArea);
    node->lch->areaID = node->areaID;
    node->lch->window = node->window;
    node->lch->tab = node->tab;
    node->lch->invalidate_area(topArea);

    node->rch = alloc_node(bottomArea);
    node->rch->areaID = mAreaIDCounter++;
    node->rch->window = create_window(bottomArea.get_size(), "window");
    node->rch->tab = heap_new<AreaTab>(MEMORY_USAGE_UI, mCtx, bottomArea.get_pos());
    node->rch->invalidate_area(bottomArea);

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(node->area.w);
    layoutI.sizeY = UISize::fixed(WINDOW_AREA_MARGIN);
    UIWindowInfo windowI{};
    windowI.name = "splitControl";
    windowI.defaultMouseControls = false;

    // becomes non-leaf node
    node->areaID = INVALID_WINDOW_AREA;
    node->window = {};
    node->splitControl = mCtx.add_window(layoutI, windowI, node);
    node->splitControl.set_rect(splitArea);
    node->splitControl.set_on_draw(&AreaNode::split_control_on_draw);
    node->splitControl.set_on_drag(&AreaNode::split_control_on_drag);
    node->splitControl.set_on_enter(&AreaNode::split_control_on_enter);
    node->splitControl.set_on_leave(&AreaNode::split_control_on_leave);
    node->splitAxis = SPLIT_AXIS_Y;
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

    if (node->lch || node->rch)
        node->splitControl.on_draw(renderer);

    // render window area on leaf node
    if (node->window && node->tab)
    {
        node->window.on_draw(renderer);
        node->tab->window.on_draw(renderer);
    }
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

static void split_area(SplitAxis axis, float ratio, const Rect& area, Rect& tl, Rect& br, Rect& split)
{
    if (axis == SPLIT_AXIS_X)
    {
        tl = area;
        tl.w = area.w * ratio;
        tl.w -= WINDOW_AREA_MARGIN / 2.0f;

        split = Rect(tl.x + tl.w, tl.y, WINDOW_AREA_MARGIN, tl.h);

        br = area;
        br.x += tl.w + WINDOW_AREA_MARGIN;
        br.w = area.w * (1.0f - ratio) - WINDOW_AREA_MARGIN / 2.0f;
    }
    else // SPLIT_AXIS_Y
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

/// @brief recursive invalidation to notify new split ratio and window area
static void invalidate(AreaNode* node)
{
    if (!node)
        return;

    if (!node->lch && !node->rch)
    {
        // invalidate leaf node window area
        node->invalidate_area(node->area);
        return;
    }

    node->invalidate_split_ratio(node->splitRatio);
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
    root->area = Rect(0, TOPBAR_HEIGHT, screenSize.x, screenSize.y - TOPBAR_HEIGHT);
    invalidate(root);
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

    if (!node || !node->tab)
        return;

    node->tab->titleText.set_text(title);
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
    return mObj->split_right(areaID, ratio);
}

UIWindowAreaID UIWindowManager::split_bottom(UIWindowAreaID areaID, float ratio)
{
    return mObj->split_bottom(areaID, ratio);
}

} // namespace LD