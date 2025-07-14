#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UI.h>
#include <cstdint>
#include <vector>

namespace LD {

struct UICallback
{
    void (*on_press)(void*, UIElement element, MouseButton btn);
    void (*on_release)(void*, UIElement element, MouseButton btn);
    void (*on_enter)(void*, UIElement element);
    void (*on_leave)(void*, UIElement element);
    void (*on_drag)(void* user, UIElement element, MouseButton btn, const Vec2& dragPos, bool begin);
};

struct UIContextObj;

struct UIElementObj
{
    UIContextObj* ctx;
    void* user;
    UICallback userCallback;
    UIElement parent;
    UILayoutInfo layout;
    std::vector<UIElementObj*> children;
    Rect rect;
    float minw;
    float minh;
    bool isHidden;

    UIElementObj() = default;
    UIElementObj(const UIElementObj&) = delete;
    ~UIElementObj()
    {
        for (UIElementObj* child : children)
            heap_delete<UIElementObj>(child);

        children.clear();
    }

    UIElementObj& operator=(const UIElementObj&) = delete;
};

struct UIContextObj
{
    std::vector<UIElementObj*> windows; /// all windows in context
    UIElementObj* dragElement;          /// the element begin dragged
    UIElementObj* pressElement;         /// the element pressed and not yet released
    UIElementObj* cursorElement;        /// the element under mouse cursor
    Vec2 cursorPos;                     /// mouse cursor position
    Vec2 dragStartPos;                  /// mouse cursor drag start position
    MouseButton dragMouseButton;        /// mouse button used for dragging
};

static void ui_layout(UIElementObj* rootObj);
static void ui_layout_pass_fit_x(UIElementObj* rootObj);
static void ui_layout_pass_fit_y(UIElementObj* rootObj);
static void ui_layout_pass_grow_shrink_x(UIElementObj* rootObj);
static void ui_layout_pass_grow_shrink_y(UIElementObj* rootObj);
static void ui_layout_pass_wrap_x(UIElementObj* rootObj);
static void ui_layout_grow_x(const std::vector<UIElementObj*>& growableX, float remainW);
static void ui_layout_grow_y(const std::vector<UIElementObj*>& growableY, float remainH);
static void ui_layout_shrink_x(std::vector<UIElementObj*>& shrinkableX, float remainW);

static void ui_layout_pass_fit_x(UIElementObj* rootObj)
{
    const UILayoutInfo& rootLayout = rootObj->layout;
    float posx = rootObj->rect.x + rootLayout.childPadding.left;
    float width = 0.0f;

    for (UIElementObj* child : rootObj->children)
    {
        ui_layout_pass_fit_x(child);
        const UILayoutInfo& childLayout = child->layout;

        if (childLayout.sizeX.type == UI_SIZE_FIXED)
        {
            child->rect.w = childLayout.sizeX.extent;
            child->minw = child->rect.w;
        }
        else if (childLayout.sizeX.type == UI_SIZE_WRAP_PRIMARY)
        {
            float minw, maxw;
            childLayout.sizeX.wrapLimitFn(child->user, minw, maxw);
            child->rect.w = maxw;
            child->minw = minw;
        }

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            if (child != rootObj->children[0])
                posx += rootLayout.childGap;

            posx += child->rect.w;
            width = posx - rootObj->rect.x;
            rootObj->minw += child->minw;
        }
        else
        {
            width = std::max(width, childLayout.sizeX.extent + rootLayout.childPadding.right);
            rootObj->minw = std::max(rootObj->minw, child->minw);
        }
    }

    switch (rootLayout.sizeX.type)
    {
    case UI_SIZE_FIT:
        rootObj->rect.w = width + rootLayout.childPadding.right;
        break;
    case UI_SIZE_FIXED:
        rootObj->rect.w = rootLayout.sizeX.extent;
        break;
    }
}

void ui_layout_pass_fit_y(UIElementObj* rootObj)
{
    const UILayoutInfo& rootLayout = rootObj->layout;
    float posy = rootObj->rect.y + rootLayout.childPadding.top;
    float height = 0.0f;

    for (UIElementObj* child : rootObj->children)
    {
        ui_layout_pass_fit_y(child);
        const UILayoutInfo& childLayout = child->layout;

        if (childLayout.sizeY.type == UI_SIZE_FIXED)
        {
            child->rect.h = childLayout.sizeY.extent;
            child->minh = child->rect.h;
        }
        else if (childLayout.sizeY.type == UI_SIZE_WRAP_PRIMARY)
        {
            float minh, maxh;
            childLayout.sizeY.wrapLimitFn(child->user, minh, maxh);
            child->rect.h = maxh;
            child->minh = minh;
        }

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            height = std::max(height, childLayout.sizeY.extent + rootLayout.childPadding.bottom);
            rootObj->minh = std::max(rootObj->minh, child->minh);
        }
        else
        {
            if (child != rootObj->children[0])
                posy += rootLayout.childGap;

            posy += child->rect.h;
            height = posy - rootObj->rect.y;
            rootObj->minh += child->minh;
        }
    }

    switch (rootLayout.sizeY.type)
    {
    case UI_SIZE_FIT:
        rootObj->rect.h = height + rootLayout.childPadding.bottom;
        break;
    case UI_SIZE_FIXED:
        rootObj->rect.h = rootLayout.sizeY.extent;
        break;
    }
}

static void ui_layout_pass_grow_shrink_x(UIElementObj* rootObj)
{
    const UILayoutInfo& rootLayout = rootObj->layout;
    float posx = rootObj->rect.x + rootLayout.childPadding.left;
    float posy = rootObj->rect.y + rootLayout.childPadding.top;
    float remainW = rootObj->rect.w - rootLayout.childPadding.left - rootLayout.childPadding.right;

    std::vector<UIElementObj*> growableX;
    std::vector<UIElementObj*> shrinkableX;
    for (UIElementObj* child : rootObj->children)
    {
        ui_layout_pass_grow_shrink_x(child);

        const UISize& sizeX = child->layout.sizeX;

        if (sizeX.type == UI_SIZE_GROW)
            growableX.push_back(child);
        else if (sizeX.type == UI_SIZE_WRAP_PRIMARY)
            shrinkableX.push_back(child);
    }

    if (rootLayout.childAxis == UI_AXIS_X)
    {
        remainW -= (rootObj->children.size() - 1) * rootLayout.childGap;
        for (UIElementObj* child : rootObj->children)
            remainW -= child->rect.w;

        ui_layout_grow_x(growableX, remainW);
        ui_layout_shrink_x(shrinkableX, remainW);
    }
    else
    {
        for (UIElementObj* child : rootObj->children)
        {
            const UISize& sizeX = child->layout.sizeX;

            if (sizeX.type == UI_SIZE_GROW)
            {
                child->rect.w = remainW;
            }
            else if (sizeX.type == UI_SIZE_WRAP_PRIMARY)
            {
                float childRemainW = remainW - child->rect.w;
                std::vector<UIElementObj*> v{child};
                ui_layout_shrink_x(v, childRemainW);
            }
        }
    }
}

void ui_layout_pass_grow_shrink_y(UIElementObj* rootObj)
{
    const UILayoutInfo& rootLayout = rootObj->layout;
    float remainH = rootObj->rect.h - rootLayout.childPadding.top - rootLayout.childPadding.bottom;

    std::vector<UIElementObj*> growableY;
    for (UIElementObj* child : rootObj->children)
    {
        ui_layout_pass_grow_shrink_y(child);

        if (child->layout.sizeY.type == UI_SIZE_GROW)
            growableY.push_back(child);
    }

    if (rootLayout.childAxis == UI_AXIS_Y)
    {
        remainH -= (rootObj->children.size() - 1) * rootLayout.childGap;
        for (UIElementObj* child : rootObj->children)
            remainH -= child->rect.h;

        ui_layout_grow_y(growableY, remainH);
        // TODO: shrink y
    }
    else
    {
        for (UIElementObj* child : rootObj->children)
        {
            const UISize& sizeY = child->layout.sizeY;

            if (sizeY.type == UI_SIZE_GROW)
            {
                child->rect.h = remainH;
            }
        }
    }
}

/// @brief perform wrapping with horizontal axis as the wrap primary axis
static void ui_layout_pass_wrap_x(UIElementObj* rootObj)
{
    const UILayoutInfo& rootLayout = rootObj->layout;

    for (UIElementObj* child : rootObj->children)
    {
        ui_layout_pass_wrap_x(child);
        const UILayoutInfo& childLayout = child->layout;

        if (childLayout.sizeX.type == UI_SIZE_WRAP_PRIMARY)
        {
            // ui_layout_pass_grow_shrink_x should have determined width along primary axis
            float wrappedH = childLayout.sizeX.wrapSizeFn(child->user, child->rect.w);

            LD_ASSERT(childLayout.sizeY.type == UI_SIZE_WRAP_SECONDARY);
            child->rect.h = wrappedH;
        }
    }
}

static void ui_layout_pass_pos(UIElementObj* rootObj)
{
    const UILayoutInfo& rootLayout = rootObj->layout;
    float posx = rootObj->rect.x + rootLayout.childPadding.left;
    float posy = rootObj->rect.y + rootLayout.childPadding.top;

    for (UIElementObj* child : rootObj->children)
    {
        child->rect.x = posx;
        child->rect.y = posy;

        ui_layout_pass_pos(child);

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            posx += child->rect.w + rootLayout.childGap;
        }
        else
        {
            posy += child->rect.h + rootLayout.childGap;
        }
    }
}

static void ui_layout_grow_x(const std::vector<UIElementObj*>& growableX, float remainW)
{
    if (growableX.empty() || remainW <= 0.0f)
        return;

    while (remainW > 0.0f)
    {
        float smallestW = growableX[0]->rect.w;
        float secondSmallestW = smallestW;
        float growW = remainW;

        for (UIElementObj* child : growableX)
        {
            if (child->rect.w < smallestW)
            {
                secondSmallestW = smallestW;
                smallestW = child->rect.w;
            }
            else if (child->rect.w > smallestW)
            {
                secondSmallestW = std::min(secondSmallestW, child->rect.w);
                growW = secondSmallestW - smallestW;
            }
        }

        growW = std::min(growW, remainW / (float)growableX.size());

        for (UIElementObj* child : growableX)
        {
            if (child->rect.w == smallestW)
            {
                child->rect.w += growW;
                remainW -= growW;
            }
        }
    }
}

void ui_layout_grow_y(const std::vector<UIElementObj*>& growableY, float remainH)
{
    if (growableY.empty() || remainH <= 0.0f)
        return;

    while (remainH > 0.0f)
    {
        float smallestH = growableY[0]->rect.h;
        float secondSmallestH = smallestH;
        float growH = remainH;

        for (UIElementObj* child : growableY)
        {
            if (child->rect.h < smallestH)
            {
                secondSmallestH = smallestH;
                smallestH = child->rect.h;
            }
            else if (child->rect.h > smallestH)
            {
                secondSmallestH = std::min(secondSmallestH, child->rect.h);
                growH = secondSmallestH - smallestH;
            }
        }

        growH = std::min(growH, remainH / (float)growableY.size());

        for (UIElementObj* child : growableY)
        {
            if (child->rect.h == smallestH)
            {
                child->rect.h += growH;
                remainH -= growH;
            }
        }
    }
}

static void ui_layout_shrink_x(std::vector<UIElementObj*>& shrinkableX, float remainW)
{
    while (!shrinkableX.empty() && remainW < 0.0f)
    {
        float largestW = shrinkableX[0]->rect.w;
        float secondLargestW = largestW;
        float shrinkW = remainW;

        for (UIElementObj* child : shrinkableX)
        {
            if (child->rect.w > largestW)
            {
                secondLargestW = largestW;
                largestW = child->rect.w;
            }
            else if (child->rect.w < largestW)
            {
                secondLargestW = std::max(secondLargestW, child->rect.w);
                shrinkW = secondLargestW - largestW;
            }
        }

        shrinkW = std::max(shrinkW, remainW / (float)shrinkableX.size());

        std::vector<size_t> toErase;

        for (size_t i = 0; i < shrinkableX.size(); i++)
        {
            UIElementObj* child = shrinkableX[i];
            float childPrevW = child->rect.w;

            if (child->rect.w == largestW)
            {
                child->rect.w += shrinkW;
                if (child->rect.w <= child->minw)
                {
                    child->rect.w = child->minw;
                    toErase.push_back(i);
                }
                remainW -= (child->rect.w - childPrevW);
            }
        }

        for (auto ite = toErase.rbegin(); ite != toErase.rend(); ite++)
            shrinkableX.erase(shrinkableX.begin() + *ite);
    }
}

static void ui_layout(UIElementObj* root)
{
    LD_PROFILE_SCOPE;

    root->rect.w = 0;
    root->rect.h = 0;
    root->minw = 0;
    root->minh = 0;

    ui_layout_pass_fit_x(root);
    ui_layout_pass_grow_shrink_x(root);
    ui_layout_pass_wrap_x(root);
    ui_layout_pass_fit_y(root);
    ui_layout_pass_grow_shrink_y(root);
    ui_layout_pass_pos(root);
}

/// @brief get the element at position
/// @param root the root element to search recursively
/// @param pos global position
/// @return the element at position, or null if position is out of bounds
static UIElementObj* get_element_at_pos(UIElementObj* root, const Vec2& pos)
{
    if (!root->rect.contains(pos))
        return nullptr;

    for (UIElementObj* child : root->children)
    {
        if (!child->rect.contains(pos))
            continue;

        UIElementObj* result = get_element_at_pos(child, pos);

        if (result)
            return result;
    }

    return root;
}

UIElement UIElement::add_child(const UILayoutInfo& layoutI, void* user)
{
    LD_ASSERT(!(layoutI.sizeX.type == UI_SIZE_WRAP_PRIMARY && layoutI.sizeY.type != UI_SIZE_WRAP_SECONDARY));
    LD_ASSERT(layoutI.sizeY.type != UI_SIZE_WRAP_PRIMARY);

    UIElementObj* child = heap_new<UIElementObj>(MEMORY_USAGE_UI);
    child->layout = layoutI;
    child->user = user;
    child->ctx = mObj->ctx;
    child->rect = {};
    child->minw = 0.0f;
    child->minh = 0.0f;
    child->isHidden = false;

    mObj->children.push_back(child);

    return {child};
}

void UIElement::set_on_press(void (*on_press)(void* user, UIElement element, MouseButton btn))
{
    mObj->userCallback.on_press = on_press;
}

void UIElement::set_on_release(void (*on_release)(void* user, UIElement element, MouseButton btn))
{
    mObj->userCallback.on_release = on_release;
}

void UIElement::set_on_enter(void (*on_enter)(void* user, UIElement element))
{
    mObj->userCallback.on_enter = on_enter;
}

void UIElement::set_on_leave(void (*on_leave)(void* user, UIElement element))
{
    mObj->userCallback.on_leave = on_leave;
}

void UIElement::set_on_drag(void (*on_drag)(void* user, UIElement element, MouseButton btn, const Vec2& dragPos, bool begin))
{
    mObj->userCallback.on_drag = on_drag;
}

Rect UIElement::get_rect() const
{
    return mObj->rect;
}

void UIElement::set_user(void* user)
{
    mObj->user = user;
}

void* UIElement::get_user()
{
    return mObj->user;
}

bool UIElement::is_hovered() const
{
    return mObj->ctx->cursorElement == mObj;
}

bool UIElement::is_pressed() const
{
    return mObj->ctx->pressElement == mObj;
}

void LD::UIWindow::hide()
{
    mObj->isHidden = true;
}

void UIWindow::show()
{
    mObj->isHidden = false;
}

bool UIWindow::is_hidden()
{
    return mObj->isHidden;
}

void UIWindow::set_pos(const Vec2& pos)
{
    mObj->rect.x = pos.x;
    mObj->rect.y = pos.y;
}

void UIWindow::set_size(const Vec2& size)
{
    mObj->layout.sizeX = UISize::fixed(size.x);
    mObj->layout.sizeY = UISize::fixed(size.y);
}

void UIWindow::get_children(std::vector<UIElement>& children)
{
    children.resize(mObj->children.size());
    for (size_t i = 0; i < mObj->children.size(); i++)
        children[i] = {mObj->children[i]};
}

UIContext UIContext::create()
{
    UIContextObj* ctxObj = heap_new<UIContextObj>(MEMORY_USAGE_UI);

    return {ctxObj};
}

void UIContext::destroy(UIContext ctx)
{
    UIContextObj* obj = ctx;

    for (UIElementObj* window : obj->windows)
        heap_delete<UIElementObj>(window);

    obj->windows.clear();

    heap_delete<UIContextObj>(obj);
}

void UIContext::input_mouse_position(const Vec2& pos)
{
    LD_PROFILE_SCOPE;

    mObj->cursorPos = pos;

    if (mObj->dragElement)
    {
        UIElementObj* de = mObj->dragElement;
        de->userCallback.on_drag(de->user, {de}, mObj->dragMouseButton, mObj->cursorPos, false);
    }

    UIElementObj* prev = mObj->cursorElement;
    UIElementObj* next = nullptr;

    // last drawn window takes input first
    for (auto ite = mObj->windows.rbegin(); ite != mObj->windows.rend(); ite++)
    {
        UIElementObj* window = *ite;

        if (window->isHidden || !window->rect.contains(pos))
            continue;

        next = get_element_at_pos(window, pos);

        if (next)
        {
            if (next != prev && prev && prev->userCallback.on_leave)
                prev->userCallback.on_leave(prev->user, {prev});

            if (next != prev && next->userCallback.on_enter)
                next->userCallback.on_enter(next->user, {next});

            mObj->cursorElement = next;
            return;
        }
    }

    if (prev && prev->userCallback.on_leave)
        prev->userCallback.on_leave(prev->user, {prev});

    mObj->cursorElement = nullptr;
}

void UIContext::input_mouse_press(MouseButton btn)
{
    UIElementObj* e = mObj->cursorElement;
    if (!e)
        return;

    if (e->userCallback.on_drag)
    {
        mObj->dragStartPos = mObj->cursorPos;
        mObj->dragElement = e;
        mObj->dragMouseButton = btn;

        e->userCallback.on_drag(e->user, {e}, btn, mObj->cursorPos, true);
    }

    if (e->userCallback.on_press)
    {
        e->userCallback.on_press(e->user, {e}, btn);
        mObj->pressElement = e;
    }
}

void UIContext::input_mouse_release(MouseButton btn)
{
    mObj->dragElement = nullptr;
    mObj->pressElement = nullptr;

    UIElementObj* e = mObj->cursorElement;
    if (!e)
        return;

    if (e->userCallback.on_release)
        e->userCallback.on_release(e->user, {e}, btn);
}

void UIContext::layout()
{
    LD_PROFILE_SCOPE;

    for (UIElementObj* window : mObj->windows)
    {
        ui_layout(window);
    }
}

UIWindow UIContext::add_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user)
{
    UIElementObj* windowObj = heap_new<UIElementObj>(MEMORY_USAGE_UI);
    windowObj->layout = layoutI;
    windowObj->user = user;
    windowObj->ctx = mObj;

    mObj->windows.push_back(windowObj);

    return {windowObj};
}

void UIContext::get_windows(std::vector<UIWindow>& windows)
{
    windows.resize(mObj->windows.size());
    for (size_t i = 0; i < mObj->windows.size(); i++)
        windows[i] = {mObj->windows[i]};
}

} // namespace LD