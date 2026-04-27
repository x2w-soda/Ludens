#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

struct EditorUIModalInfo
{
    EditorContext ctx;
    const char* layerName;
    Vec2 screenSize;
    bool isVisible;
};

struct EditorUIModal : Handle<struct EditorUIModalObj>
{
    static EditorUIModal create(const EditorUIModalInfo& modalI);
    static void destroy(EditorUIModal modal);

    void pre_update(const EditorUpdateTick& tick);
    void update(const EditorUpdateTick& tick);
    void post_update();

    /// @brief Show a single window in modal.
    EditorWindow show_window(EditorWindowType type);

    /// @brief Hide the modal.
    void hide();
};

} // namespace LD