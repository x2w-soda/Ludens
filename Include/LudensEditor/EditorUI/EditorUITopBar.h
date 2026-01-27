#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EditorUITopBarInfo
{
    EditorContext ctx;
    UILayer groundLayer; // top bar live in the ground layer
    UILayer floatLayer;  // list menus live in the float layer
    float barHeight;     // top bar height
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