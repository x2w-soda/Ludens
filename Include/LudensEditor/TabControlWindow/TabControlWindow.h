#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor window containing tabs.
struct TabControlWindow : Handle<struct TabControlWindowObj>
{
    TabControlWindow() = default;
    TabControlWindow(const EditorWindowObj* obj) { mObj = (TabControlWindowObj*)obj; }

    /// @brief Create tab control window.
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy tab control window.
    static void destroy(EditorWindow window);

    // TODO: API for multiple tabs...
    void set_tab_name(const char* name);

    /// @brief Check if tab control window is dragged.
    bool has_drag(MouseButton& dragBtn, Vec2& screenPos, bool& dragBegin);
};

} // namespace LD