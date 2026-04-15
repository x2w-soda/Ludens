#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

struct EUITextStorage
{
    UITextData text;
    UIPanelData panel;
    float radius = 0.4f;
};

bool eui_text(EUITextStorage& storage, const char* label, float height, Rect* outRect = nullptr);

struct EUITextBreadcrumbStorage
{
    UITextData text;
    UIPanelData panel;

    /// @brief Build breadcrumb text using '/' as separator
    void build(const char* text);
};

int eui_text_breadcrumb(EUITextBreadcrumbStorage& storage, float height, Color hlColor);

} // namespace LD