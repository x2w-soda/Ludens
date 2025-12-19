#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/RenderServer/ScreenLayer.h>

namespace LD {

/// @brief Used by the scene to render 2D components into screen layers.
class ScreenRenderer
{
public:
    /// @brief In-place startup.
    void startup();

    /// @brief In-place cleanup.
    void cleanup();

    /// @brief Iterate 2D components in registry and renders to screen layers.
    /// @warning The scene guarantees registry is not mutated during screen render.
    void render(DataRegistry registry);

    inline ScreenLayer get_layer()
    {
        return mLayer;
    }

private:
    ScreenLayer mLayer;
};

} // namespace LD