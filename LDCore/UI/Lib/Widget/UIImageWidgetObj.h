#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct UIWidgetObj;

struct UIImageWidgetObj
{
    UIWidgetObj* base;
    RImage imageHandle;
    Rect imageRect;
    Color tint;
};

} // namespace LD