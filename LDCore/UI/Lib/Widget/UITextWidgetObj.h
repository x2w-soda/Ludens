#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct UIWidgetObj;

struct UITextWidgetObj
{
    UIWidgetObj* base;
    const char* value;
    FontAtlas fontAtlas;
    RImage fontImage;
    Color fgColor;
    Color bgColor;
    float fontSize;

    static void cleanup(UIWidgetObj* base);
};

} // namespace LD