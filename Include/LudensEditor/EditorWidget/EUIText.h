#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

struct EUITextStorage
{
    UITextStorage text;
    UIPanelStorage panel;
    float radius = 0.4f;
};

bool eui_text(EUITextStorage* storage, const char* label, float height, Rect* outRect = nullptr);

} // namespace LD