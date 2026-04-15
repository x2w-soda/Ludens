#pragma once

#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>

namespace LD {

struct EUIIconStorage
{
    UIImageData image;
    Color bgColor = 0;
};

bool eui_icon(EUIIconStorage& storage, EditorIcon iconType, float iconSize);

} // namespace LD