#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EditorUITopBarInfo
{
    EditorContext ctx;
    const char* groundLayerName; // top bar lives in the ground layer
    float barHeight;             // top bar height
    Vec2 screenSize;
};

/// @brief Editor top bar menu UI.
struct EditorUITopBar : Handle<struct EditorTopBarObj>
{
    static EditorUITopBar create(const EditorUITopBarInfo& topBarI);
    static void destroy(EditorUITopBar topBar);

    void on_imgui(float delta);
};

} // namespace LD