#include "UIObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UILayout.h>
#include <Ludens/UI/UIWidget.h>
#include <algorithm>

namespace LD {

static void ui_layout_pass_fit_x(UIWidgetObj* root);
static void ui_layout_pass_fit_y(UIWidgetObj* root);
static void ui_layout_pass_grow_shrink_x(UIWidgetObj* root);
static void ui_layout_pass_grow_shrink_y(UIWidgetObj* root);
static void ui_layout_pass_wrap_x(UIWidgetObj* root);
static void ui_layout_grow_x(const std::vector<UIWidgetObj*>& growableX, float remainW);
static void ui_layout_grow_y(const std::vector<UIWidgetObj*>& growableY, float remainH);
static void ui_layout_shrink_x(std::vector<UIWidgetObj*>& shrinkableX, float remainW);

static void ui_layout_pass_fit_x(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->layout.info;
    UIWidgetObj* widget = (UIWidgetObj*)root;
    float posx = root->layout.rect.x + rootLayout.childPadding.left;
    float width = 0.0f;

    for (UIWidgetObj* child = widget->child; child; child = child->next)
    {
        ui_layout_pass_fit_x(child);
        const UILayoutInfo& childLayout = child->layout.info;

        if (childLayout.sizeX.type == UI_SIZE_FIXED)
        {
            child->layout.rect.w = childLayout.sizeX.extent;
            child->layout.minw = child->layout.rect.w;
        }
        else if (childLayout.sizeX.type == UI_SIZE_WRAP_PRIMARY)
        {
            float minw, maxw;
            childLayout.sizeX.wrapLimitFn(child, minw, maxw);
            child->layout.rect.w = maxw;
            child->layout.minw = minw;
        }

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            if (child != root->child)
                posx += rootLayout.childGap;

            posx += child->layout.rect.w;
            width = posx - root->layout.rect.x;
            root->layout.minw += child->layout.minw;
        }
        else
        {
            width = std::max(width, childLayout.sizeX.extent + rootLayout.childPadding.right);
            root->layout.minw = std::max(root->layout.minw, child->layout.minw);
        }
    }

    switch (rootLayout.sizeX.type)
    {
    case UI_SIZE_FIT:
        root->layout.rect.w = width + rootLayout.childPadding.right;
        break;
    case UI_SIZE_FIXED:
        root->layout.rect.w = rootLayout.sizeX.extent;
        break;
    }
}

void ui_layout_pass_fit_y(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->layout.info;
    float posy = root->layout.rect.y + rootLayout.childPadding.top;
    float height = 0.0f;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        ui_layout_pass_fit_y(child);
        const UILayoutInfo& childLayout = child->layout.info;

        if (childLayout.sizeY.type == UI_SIZE_FIXED)
        {
            child->layout.rect.h = childLayout.sizeY.extent;
            child->layout.minh = child->layout.rect.h;
        }
        else if (childLayout.sizeY.type == UI_SIZE_WRAP_PRIMARY)
        {
            float minh, maxh;
            childLayout.sizeY.wrapLimitFn(child, minh, maxh);
            child->layout.rect.h = maxh;
            child->layout.minh = minh;
        }

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            height = std::max(height, childLayout.sizeY.extent + rootLayout.childPadding.bottom);
            root->layout.minh = std::max(root->layout.minh, child->layout.minh);
        }
        else
        {
            if (child != root->child)
                posy += rootLayout.childGap;

            posy += child->layout.rect.h;
            height = posy - root->layout.rect.y;
            root->layout.minh += child->layout.minh;
        }
    }

    switch (rootLayout.sizeY.type)
    {
    case UI_SIZE_FIT:
        root->layout.rect.h = height + rootLayout.childPadding.bottom;
        break;
    case UI_SIZE_FIXED:
        root->layout.rect.h = rootLayout.sizeY.extent;
        break;
    }
}

static void ui_layout_pass_grow_shrink_x(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->layout.info;
    float posx = root->layout.rect.x + rootLayout.childPadding.left;
    float posy = root->layout.rect.y + rootLayout.childPadding.top;
    float remainW = root->layout.rect.w - rootLayout.childPadding.left - rootLayout.childPadding.right;

    std::vector<UIWidgetObj*> growableX;
    std::vector<UIWidgetObj*> shrinkableX;
    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        ui_layout_pass_grow_shrink_x(child);

        const UISize& sizeX = child->layout.info.sizeX;

        if (sizeX.type == UI_SIZE_GROW)
            growableX.push_back(child);
        else if (sizeX.type == UI_SIZE_WRAP_PRIMARY)
            shrinkableX.push_back(child);
    }

    if (rootLayout.childAxis == UI_AXIS_X && root->child)
    {
        remainW -= (root->get_children_count() - 1) * rootLayout.childGap;
        for (UIWidgetObj* child = root->child; child; child = child->next)
            remainW -= child->layout.rect.w;

        ui_layout_grow_x(growableX, remainW);
        ui_layout_shrink_x(shrinkableX, remainW);
    }
    else
    {
        for (UIWidgetObj* child = root->child; child; child = child->next)
        {
            const UISize& sizeX = child->layout.info.sizeX;

            if (sizeX.type == UI_SIZE_GROW)
            {
                child->layout.rect.w = remainW;
            }
            else if (sizeX.type == UI_SIZE_WRAP_PRIMARY)
            {
                float childRemainW = remainW - child->layout.rect.w;
                std::vector<UIWidgetObj*> v{child};
                ui_layout_shrink_x(v, childRemainW);
            }
        }
    }
}

void ui_layout_pass_grow_shrink_y(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->layout.info;
    float remainH = root->layout.rect.h - rootLayout.childPadding.top - rootLayout.childPadding.bottom;

    std::vector<UIWidgetObj*> growableY;
    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        const UILayoutInfo& childLayout = child->layout.info;

        ui_layout_pass_grow_shrink_y(child);

        if (childLayout.sizeY.type == UI_SIZE_GROW)
            growableY.push_back(child);
    }

    if (rootLayout.childAxis == UI_AXIS_Y && root->child)
    {
        remainH -= (root->get_children_count() - 1) * rootLayout.childGap;
        for (UIWidgetObj* child = root->child; child; child = child->next)
            remainH -= child->layout.rect.h;

        ui_layout_grow_y(growableY, remainH);
        // TODO: shrink y
    }
    else
    {
        for (UIWidgetObj* child = root->child; child; child = child->next)
        {
            const UILayoutInfo& childLayout = child->layout.info;
            const UISize& sizeY = childLayout.sizeY;

            if (sizeY.type == UI_SIZE_GROW)
            {
                child->layout.rect.h = remainH;
            }
        }
    }
}

/// @brief perform wrapping with horizontal axis as the wrap primary axis
static void ui_layout_pass_wrap_x(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->layout.info;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        ui_layout_pass_wrap_x(child);
        const UILayoutInfo& childLayout = child->layout.info;

        if (childLayout.sizeX.type == UI_SIZE_WRAP_PRIMARY)
        {
            // ui_layout_pass_grow_shrink_x should have determined width along primary axis
            float wrappedH = childLayout.sizeX.wrapSizeFn(child, child->layout.rect.w);

            LD_ASSERT(childLayout.sizeY.type == UI_SIZE_WRAP_SECONDARY);
            child->layout.rect.h = wrappedH;
        }
    }
}

static void ui_layout_pass_pos(UIWidgetObj* root)
{
    const UILayoutInfo& rootLayout = root->layout.info;
    float posx = root->layout.rect.x + rootLayout.childPadding.left;
    float posy = root->layout.rect.y + rootLayout.childPadding.top;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        child->layout.rect.x = posx;
        child->layout.rect.y = posy;

        ui_layout_pass_pos(child);

        if (rootLayout.childAxis == UI_AXIS_X)
        {
            posx += child->layout.rect.w + rootLayout.childGap;
        }
        else
        {
            posy += child->layout.rect.h + rootLayout.childGap;
        }
    }
}

static void ui_layout_grow_x(const std::vector<UIWidgetObj*>& growableX, float remainW)
{
    if (growableX.empty() || remainW <= 0.0f)
        return;

    while (remainW > 0.0f)
    {
        float smallestW = growableX[0]->layout.rect.w;
        float secondSmallestW = smallestW;
        float growW = remainW;

        for (UIWidgetObj* child : growableX)
        {
            if (child->layout.rect.w < smallestW)
            {
                secondSmallestW = smallestW;
                smallestW = child->layout.rect.w;
            }
            else if (child->layout.rect.w > smallestW)
            {
                secondSmallestW = std::min(secondSmallestW, child->layout.rect.w);
                growW = secondSmallestW - smallestW;
            }
        }

        growW = std::min(growW, remainW / (float)growableX.size());

        for (UIWidgetObj* child : growableX)
        {
            if (child->layout.rect.w == smallestW)
            {
                child->layout.rect.w += growW;
                remainW -= growW;
            }
        }
    }
}

void ui_layout_grow_y(const std::vector<UIWidgetObj*>& growableY, float remainH)
{
    if (growableY.empty() || remainH <= 0.0f)
        return;

    while (remainH > 0.0f)
    {
        float smallestH = growableY[0]->layout.rect.h;
        float secondSmallestH = smallestH;
        float growH = remainH;

        for (UIWidgetObj* child : growableY)
        {
            if (child->layout.rect.h < smallestH)
            {
                secondSmallestH = smallestH;
                smallestH = child->layout.rect.h;
            }
            else if (child->layout.rect.h > smallestH)
            {
                secondSmallestH = std::min(secondSmallestH, child->layout.rect.h);
                growH = secondSmallestH - smallestH;
            }
        }

        growH = std::min(growH, remainH / (float)growableY.size());

        for (UIWidgetObj* child : growableY)
        {
            if (child->layout.rect.h == smallestH)
            {
                child->layout.rect.h += growH;
                remainH -= growH;
            }
        }
    }
}

static void ui_layout_shrink_x(std::vector<UIWidgetObj*>& shrinkableX, float remainW)
{
    while (!shrinkableX.empty() && remainW < 0.0f)
    {
        float largestW = shrinkableX[0]->layout.rect.w;
        float secondLargestW = largestW;
        float shrinkW = remainW;

        for (UIWidgetObj* child : shrinkableX)
        {
            if (child->layout.rect.w > largestW)
            {
                secondLargestW = largestW;
                largestW = child->layout.rect.w;
            }
            else if (child->layout.rect.w < largestW)
            {
                secondLargestW = std::max(secondLargestW, child->layout.rect.w);
                shrinkW = secondLargestW - largestW;
            }
        }

        shrinkW = std::max(shrinkW, remainW / (float)shrinkableX.size());

        std::vector<size_t> toErase;

        for (size_t i = 0; i < shrinkableX.size(); i++)
        {
            UIWidgetObj* child = shrinkableX[i];
            float childPrevW = child->layout.rect.w;

            if (child->layout.rect.w == largestW)
            {
                child->layout.rect.w += shrinkW;
                if (child->layout.rect.w <= child->layout.minw)
                {
                    child->layout.rect.w = child->layout.minw;
                    toErase.push_back(i);
                }
                remainW -= (child->layout.rect.w - childPrevW);
            }
        }

        for (auto ite = toErase.rbegin(); ite != toErase.rend(); ite++)
            shrinkableX.erase(shrinkableX.begin() + *ite);
    }
}

void ui_layout(UIWidgetObj* root)
{
    LD_PROFILE_SCOPE;

    root->layout.rect.w = 0;
    root->layout.rect.h = 0;
    root->layout.minw = 0;
    root->layout.minh = 0;

    ui_layout_pass_fit_x(root);
    ui_layout_pass_grow_shrink_x(root);
    ui_layout_pass_wrap_x(root);
    ui_layout_pass_fit_y(root);
    ui_layout_pass_grow_shrink_y(root);
    ui_layout_pass_pos(root);
}

} // namespace LD