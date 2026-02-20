#pragma once

#include <Ludens/Camera/Camera2D.h>

namespace LD {

struct Camera2DController : Handle<struct Camera2DControllerObj>
{
    static Camera2DController create(Camera2D subject);
    static void destroy(Camera2DController controller);

    /// @brief Update interpolations.
    /// @param delta Delta time in seconds.
    /// @param mousePos If not null, the mouse position within subject camera extent,
    ///        enables 'zoom towards cursor' behavior.
    void update(float delta, const Vec2* mousePos);

    /// @brief Accumulate target zoom exponent.
    void accumulate_zoom_exp(float zoomExpDelta);
};

} // namespace LD