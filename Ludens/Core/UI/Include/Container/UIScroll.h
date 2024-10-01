#pragma once

#include "Core/UI/Include/UIWidget.h"

namespace LD
{

struct UIScrollInfo
{
    UIWidget* Parent;
};

class UIScroll : public UIContainerWidget
{
public:
    UIScroll();
    ~UIScroll();

    void Startup(const UIScrollInfo& info);
    void Cleanup();

    float GetScroll() const;
    void SetScroll(float value);

    UILayoutNode* GetLayoutRoot();

    virtual Rect2D AdjustedRect(const Rect2D&) override;

private:
    static void OnScroll(UIContext* ctx, UIWidget* widget, float dx, float dy);

    float mScroll;            // scroll offset on the main axis
    UILayoutNode mLayoutRoot; // root layout node that contains child widgets
};

} // namespace LD