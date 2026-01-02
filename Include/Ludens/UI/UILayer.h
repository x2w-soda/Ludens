#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/UI/UIWorkspace.h>

namespace LD {

struct ScreenRenderComponent;

struct UILayer : Handle<struct UILayerObj>
{
    /// @brief Peform layout on all workspaces in this layer.
    void layout();

    /// @brief Render all windows and widgets in a layer.
    /// @param renderer The screen space renderer to draw the layer with.
    void render(ScreenRenderComponent& renderer);

    /// @brief Raise layer to top in context.
    void raise();

    /// @brief Create a workspace under this layer.
    UIWorkspace create_workspace(const Rect& area);

    /// @brief Destroy a workspace.
    void destroy_workspace(UIWorkspace workspace);
};

} // namespace LD