#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EditorUIModalInfo
{
    EditorContext ctx;
    const char* layerName;
    Vec2 screenSize;
};

struct EditorUIModal : Handle<struct EditorUIModalObj>
{
    static EditorUIModal create(const EditorUIModalInfo& modalI);
    static void destroy(EditorUIModal modal);

    void on_imgui(float delta, const Vec2& screenSize);
};

} // namespace LD