#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/UI/Widget/UIImageWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIImageWidgetObj : UIWidgetBaseObj<UIImageData>
{
    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD