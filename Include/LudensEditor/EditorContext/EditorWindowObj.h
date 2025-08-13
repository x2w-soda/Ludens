#pragma once

#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

/// @brief Base class for editor window implementations.
struct EditorWindowObj
{
    /// Shared context among editor windows.
    EditorContext editorCtx;

    /// Root window widget from the UIWindowManager.
    /// The user data must be the EditorWindowObj instance pointer.
    UIWindow root;

    /// @brief Allows derived class to draw additional overlays in a separate pass.
    /// @param renderer The screen space renderer, its sampled image should be
    ///                 a blurred version of the entire viewport.
    virtual void on_draw_overlay(ScreenRenderComponent renderer) {}
};

} // namespace LD