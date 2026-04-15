#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UIWidget.h>

#include <algorithm>

#include "UIObj.h"
#include "UIWidgetMeta.h"

namespace LD {

static_assert(sizeof(UIWidgetLayout) <= 96);

/// @brief wrap limit callback, user outputs minimum extent of the wrappable
///        and the maximum extent if unwrapped.
static void ui_layout_wrap_limit_x(UIWidgetObj* obj, float& outMinW, float& outMaxW);

/// @brief wrap sizing callback, given length limit in main axis,
///        user returns the result size on the secondary axis after wrapping.
static float ui_layout_wrap_size_x(UIWidgetObj* obj, float limitW);

static void ui_layout_pass_clear(UIWidgetObj* root);
static void ui_layout_pass_fit_x(UIWidgetObj* root);
static void ui_layout_pass_fit_y(UIWidgetObj* root);
static void ui_layout_pass_grow_shrink_x(UIWidgetObj* root);
static void ui_layout_pass_grow_shrink_y(UIWidgetObj* root);
static void ui_layout_pass_wrap_x(UIWidgetObj* root);
static void ui_layout_pass_pos_align(UIWidgetObj* root);
static void ui_layout_pass_scroll_offset(UIWidgetObj* root, Vec2 offset);

static void ui_layout_grow_x(const Vector<UIWidgetObj*>& growableX, float remainW);
static void ui_layout_grow_y(const Vector<UIWidgetObj*>& growableY, float remainH);
static void ui_layout_shrink_x(Vector<UIWidgetObj*>& shrinkableX, float remainW);

static void ui_layout_wrap_limit_x(UIWidgetObj* obj, float& outMinW, float& outMaxW)
{
    LD_ASSERT(obj->type == UI_WIDGET_TEXT);

    outMinW = outMaxW = 0.0f;

    if (sWidgetMeta[(int)obj->type].wrapLimitX)
        sWidgetMeta[(int)obj->type].wrapLimitX(obj, outMinW, outMaxW);
}

static float ui_layout_wrap_size_x(UIWidgetObj* obj, float limitW)
{
    LD_ASSERT(obj->type == UI_WIDGET_TEXT);

    if (sWidgetMeta[(int)obj->type].wrapSizeX)
        return sWidgetMeta[(int)obj->type].wrapSizeX(obj, limitW);

    return 0.0f;
}

static void ui_layout_pass_clear(UIWidgetObj* root)
{
    root->L->rect.w = 0;
    root->L->rect.h = 0;

    for (UIWidgetObj* child = root->child; child; child = child->next)
        ui_layout_pass_clear(child);
}

static void ui_layout_pass_fit_x(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->L->info;
    UIWidgetObj* widget = (UIWidgetObj*)root;
    float posx = root->L->rect.x + rootLayout.childPadding.left;
    float width = 0.0f;

    for (UIWidgetObj* child = widget->child; child; child = child->next)
    {
        ui_layout_pass_fit_x(child);
        const UILayoutInfo& childLayout = child->L->info;

        if (childLayout.sizeX.type == UI_SIZE_FIXED)
        {
            child->L->rect.w = childLayout.sizeX.extent;
            child->L->minw = child->L->rect.w;
        }
        else if (childLayout.sizeX.type == UI_SIZE_WRAP)
        {
            LD_ASSERT(child->type == UI_WIDGET_TEXT);

            float minw, maxw;
            ui_layout_wrap_limit_x(child, minw, maxw);
            child->L->rect.w = maxw;
            child->L->minw = minw;
        }

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            if (child != root->child)
                posx += rootLayout.childGap;

            posx += child->L->rect.w;
            width = posx - root->L->rect.x;
            root->L->minw += child->L->minw;
        }
        else
        {
            width = std::max(width, child->L->rect.w + rootLayout.childPadding.right);
            root->L->minw = std::max(root->L->minw, child->L->minw);
        }
    }

    switch (rootLayout.sizeX.type)
    {
    case UI_SIZE_FIT:
        root->L->rect.w = width + rootLayout.childPadding.right;
        break;
    case UI_SIZE_FIXED:
        root->L->rect.w = rootLayout.sizeX.extent;
        break;
    default:
        break;
    }
}

void ui_layout_pass_fit_y(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->L->info;
    float posy = root->L->rect.y + rootLayout.childPadding.top;
    float height = 0.0f;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        ui_layout_pass_fit_y(child);
        const UILayoutInfo& childLayout = child->L->info;

        if (childLayout.sizeY.type == UI_SIZE_FIXED)
        {
            child->L->rect.h = childLayout.sizeY.extent;
            child->L->minh = child->L->rect.h;
        }
        else if (childLayout.sizeY.type == UI_SIZE_WRAP)
        {
            LD_ASSERT(child->type == UI_WIDGET_TEXT);
            LD_UNREACHABLE; // TODO:

            // float minh, maxh;
            // ui_layout_wrap_limit_y(child, minh, maxh);
            // child->L->rect.h = maxh;
            // child->L->minh = minh;
        }

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            height = std::max(height, child->L->rect.h + rootLayout.childPadding.bottom);
            root->L->minh = std::max(root->L->minh, child->L->minh);
        }
        else
        {
            if (child != root->child)
                posy += rootLayout.childGap;

            posy += child->L->rect.h;
            height = posy - root->L->rect.y;
            root->L->minh += child->L->minh;
        }
    }

    switch (rootLayout.sizeY.type)
    {
    case UI_SIZE_FIT:
        if (rootLayout.sizeX.type == UI_SIZE_WRAP)
            break;
        root->L->rect.h = height + rootLayout.childPadding.bottom;
        break;
    case UI_SIZE_FIXED:
        root->L->rect.h = rootLayout.sizeY.extent;
        break;
    default:
        break;
    }
}

static void ui_layout_pass_grow_shrink_x(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->L->info;
    float remainW = root->L->rect.w - rootLayout.childPadding.left - rootLayout.childPadding.right;

    Vector<UIWidgetObj*> growableX;
    Vector<UIWidgetObj*> shrinkableX;
    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        const UISize& sizeX = child->L->info.sizeX;

        if (sizeX.type == UI_SIZE_GROW)
            growableX.push_back(child);
        else if (sizeX.type == UI_SIZE_WRAP)
            shrinkableX.push_back(child);
    }

    if (rootLayout.childAxis == UI_AXIS_X && root->child)
    {
        remainW -= (root->get_children_count() - 1) * rootLayout.childGap;
        for (UIWidgetObj* child = root->child; child; child = child->next)
            remainW -= child->L->rect.w;

        ui_layout_grow_x(growableX, remainW);
        ui_layout_shrink_x(shrinkableX, remainW);
    }
    else
    {
        for (UIWidgetObj* child = root->child; child; child = child->next)
        {
            const UISize& sizeX = child->L->info.sizeX;

            if (sizeX.type == UI_SIZE_GROW)
            {
                child->L->rect.w = remainW;
            }
            else if (sizeX.type == UI_SIZE_WRAP)
            {
                float childRemainW = remainW - child->L->rect.w;
                Vector<UIWidgetObj*> v{child};
                ui_layout_shrink_x(v, childRemainW);
            }
        }
    }

    for (UIWidgetObj* child = root->child; child; child = child->next)
        ui_layout_pass_grow_shrink_x(child);
}

void ui_layout_pass_grow_shrink_y(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->L->info;
    float remainH = root->L->rect.h - rootLayout.childPadding.top - rootLayout.childPadding.bottom;

    Vector<UIWidgetObj*> growableY;
    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        const UILayoutInfo& childLayout = child->L->info;

        if (childLayout.sizeY.type == UI_SIZE_GROW)
            growableY.push_back(child);
    }

    if (rootLayout.childAxis == UI_AXIS_Y && root->child)
    {
        remainH -= (root->get_children_count() - 1) * rootLayout.childGap;
        for (UIWidgetObj* child = root->child; child; child = child->next)
            remainH -= child->L->rect.h;

        ui_layout_grow_y(growableY, remainH);
        // TODO: shrink y
    }
    else
    {
        for (UIWidgetObj* child = root->child; child; child = child->next)
        {
            const UILayoutInfo& childLayout = child->L->info;
            const UISize& sizeY = childLayout.sizeY;

            if (sizeY.type == UI_SIZE_GROW)
            {
                child->L->rect.h = remainH;
            }
        }
    }

    for (UIWidgetObj* child = root->child; child; child = child->next)
        ui_layout_pass_grow_shrink_y(child);
}

/// @brief perform wrapping with horizontal axis as the wrap primary axis
static void ui_layout_pass_wrap_x(UIWidgetObj* root)
{
    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        ui_layout_pass_wrap_x(child);
        const UILayoutInfo& childLayout = child->L->info;

        if (childLayout.sizeX.type == UI_SIZE_WRAP)
        {
            LD_ASSERT(child->type == UI_WIDGET_TEXT);

            // ui_layout_pass_grow_shrink_x should have determined width along primary axis
            float wrappedH = ui_layout_wrap_size_x(child, child->L->rect.w);

            child->L->rect.h = wrappedH;
        }
    }
}

static void ui_layout_pass_pos_align(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->L->info;
    float posx = root->L->rect.x + rootLayout.childPadding.left;
    float posy = root->L->rect.y + rootLayout.childPadding.top;
    float childAccW = 0.0f;
    float childAccH = 0.0f;
    int childCount = 0;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        childAccW += child->L->rect.w;
        childAccH += child->L->rect.h;
        childCount++;
    }

    if (childCount == 0)
        return;

    float remainW = 0.0f;
    float remainH = 0.0f;

    // handle alignment along main axis
    if (rootLayout.childAxis == UI_AXIS_X)
    {
        remainW = root->L->rect.w - rootLayout.childPadding.left - rootLayout.childPadding.right - childAccW;
        remainW -= rootLayout.childGap * (childCount - 1);

        switch (rootLayout.childAlignX)
        {
        case UI_ALIGN_CENTER:
            posx += remainW / 2.0f;
            break;
        case UI_ALIGN_END:
            posx += remainW;
            break;
        default:
            break;
        }
    }
    else
    {
        remainH = root->L->rect.h - rootLayout.childPadding.top - rootLayout.childPadding.bottom - childAccH;
        remainH -= rootLayout.childGap * (childCount - 1);

        switch (rootLayout.childAlignY)
        {
        case UI_ALIGN_CENTER:
            posy += remainH / 2.0f;
            break;
        case UI_ALIGN_END:
            posy += remainH;
            break;
        default:
            break;
        }
    }

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        child->L->rect.x = posx;
        child->L->rect.y = posy;

        // handle alignment across main axis for each child.
        if (rootLayout.childAxis == UI_AXIS_X)
        {
            posx += child->L->rect.w + rootLayout.childGap;
            remainH = root->L->rect.h - child->L->rect.h;

            switch (rootLayout.childAlignY)
            {
            case UI_ALIGN_CENTER:
                child->L->rect.y += remainH / 2.0f;
                break;
            case UI_ALIGN_END:
                child->L->rect.y += remainH;
                break;
            default:
                break;
            }
        }
        else // main axis Y
        {
            posy += child->L->rect.h + rootLayout.childGap;
            remainW = root->L->rect.w - child->L->rect.w;

            switch (rootLayout.childAlignX)
            {
            case UI_ALIGN_CENTER:
                child->L->rect.x += remainW / 2.0f;
                break;
            case UI_ALIGN_END:
                child->L->rect.x += remainW;
                break;
            default:
                break;
            }
        }

        ui_layout_pass_pos_align(child);
    }
}

static void ui_layout_pass_scroll_offset(UIWidgetObj* root, Vec2 offset)
{
    const UILayoutInfo& rootLayout = root->L->info;
    (void)rootLayout;

    offset += root->L->childOffset;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        child->L->rect.x += offset.x;
        child->L->rect.y += offset.y;

        ui_layout_pass_scroll_offset(child, offset);
    }
}

static void ui_layout_grow_x(const Vector<UIWidgetObj*>& growableX, float remainW)
{
    if (growableX.empty() || remainW <= 0.0f)
        return;

    while (remainW > 0.0f)
    {
        float smallestW = growableX[0]->L->rect.w;
        float secondSmallestW = smallestW;
        float growW = remainW;

        for (UIWidgetObj* child : growableX)
        {
            if (child->L->rect.w < smallestW)
            {
                secondSmallestW = smallestW;
                smallestW = child->L->rect.w;
            }
            else if (child->L->rect.w > smallestW)
            {
                secondSmallestW = std::min(secondSmallestW, child->L->rect.w);
                growW = secondSmallestW - smallestW;
            }
        }

        growW = std::min(growW, remainW / (float)growableX.size());

        for (UIWidgetObj* child : growableX)
        {
            if (child->L->rect.w == smallestW)
            {
                child->L->rect.w += growW;
                remainW -= growW;
            }
        }
    }
}

void ui_layout_grow_y(const Vector<UIWidgetObj*>& growableY, float remainH)
{
    if (growableY.empty() || remainH <= 0.0f)
        return;

    while (remainH > 0.0f)
    {
        float smallestH = growableY[0]->L->rect.h;
        float secondSmallestH = smallestH;
        float growH = remainH;

        for (UIWidgetObj* child : growableY)
        {
            if (child->L->rect.h < smallestH)
            {
                secondSmallestH = smallestH;
                smallestH = child->L->rect.h;
            }
            else if (child->L->rect.h > smallestH)
            {
                secondSmallestH = std::min(secondSmallestH, child->L->rect.h);
                growH = secondSmallestH - smallestH;
            }
        }

        growH = std::min(growH, remainH / (float)growableY.size());

        for (UIWidgetObj* child : growableY)
        {
            if (child->L->rect.h == smallestH)
            {
                child->L->rect.h += growH;
                remainH -= growH;
            }
        }
    }
}

static void ui_layout_shrink_x(Vector<UIWidgetObj*>& shrinkableX, float remainW)
{
    while (!shrinkableX.empty() && remainW < 0.0f)
    {
        float largestW = shrinkableX[0]->L->rect.w;
        float secondLargestW = largestW;
        float shrinkW = remainW;

        for (size_t i = 1; i < shrinkableX.size(); i++)
        {
            UIWidgetObj* child = shrinkableX[i];

            if (child->L->rect.w > largestW)
            {
                secondLargestW = largestW;
                largestW = child->L->rect.w;
            }
            else if (child->L->rect.w < largestW)
            {
                secondLargestW = std::max(secondLargestW, child->L->rect.w);
                shrinkW = secondLargestW - largestW;
            }
        }

        shrinkW = std::max(shrinkW, remainW / (float)shrinkableX.size());
        if (is_zero_epsilon(shrinkW))
            break;

        Vector<size_t> toErase;

        for (size_t i = 0; i < shrinkableX.size(); i++)
        {
            UIWidgetObj* child = shrinkableX[i];
            float childPrevW = child->L->rect.w;

            if (child->L->rect.w == largestW)
            {
                child->L->rect.w += shrinkW;
                if (child->L->rect.w <= child->L->minw)
                {
                    child->L->rect.w = child->L->minw;
                    toErase.push_back(i);
                }
                remainW -= (child->L->rect.w - childPrevW);
            }
        }

        for (auto ite = toErase.rbegin(); ite != toErase.rend(); ite++)
            shrinkableX.erase(shrinkableX.begin() + *ite);
    }
}

void ui_layout(UIWidgetObj* root)
{
    LD_PROFILE_SCOPE;

    ui_layout_pass_clear(root);
    ui_layout_pass_fit_x(root);
    ui_layout_pass_grow_shrink_x(root);
    ui_layout_pass_wrap_x(root);
    ui_layout_pass_fit_y(root);
    ui_layout_pass_grow_shrink_y(root);
    ui_layout_pass_pos_align(root);
    ui_layout_pass_scroll_offset(root, Vec2(0.0f));
}

} // namespace LD