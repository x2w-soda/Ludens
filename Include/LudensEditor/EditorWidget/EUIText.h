#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

class EUIText
{
public:
    float radius = 0.4f;

    bool update(const char* label, float height, Rect* outRect = nullptr);

private:
    UITextData mText;
    UIPanelData mPanel;
};

class EUITextBreadcrumb
{
public:
    /// @brief Build breadcrumb text using '/' as separator
    void build(const char* text);

    String update(float height, Color hlColor);

private:
    UITextData mText;
    UIPanelData mPanel;
};

} // namespace LD