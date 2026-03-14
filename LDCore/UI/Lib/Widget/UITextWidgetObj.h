#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/UI/Widget/UITextWidget.h>

namespace LD {

struct UIWidgetObj;

struct UITextWidgetObj
{
    UIWidgetObj* base;
    UITextStorage* storage;
    FontAtlas fontAtlas;
    RImage fontImage;

    static void startup(UIWidgetObj* obj, void* info);
    static void cleanup(UIWidgetObj* obj);
};

} // namespace LD