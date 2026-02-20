#pragma once

#include <Ludens/Camera/Camera.h>

namespace LD {

struct CameraController : Handle<struct CameraControllerObj>
{
    /// @brief create a camera controller for subject camera
    /// @subject valid camera handle that will be controlled
    /// @moveSpeed camera translation speed
    /// @rotSpeed camera rotation speed
    /// @return the controller handle
    static CameraController create(Camera subject, float moveSpeed, float rotSpeed);

    /// @brief destroy a camera controller
    static void destroy(CameraController controller);
    
    void move_forward();
    void move_backward();
    void move_left();
    void move_right();
    void move_world_up();
    void move_world_down();

    void view_pitch(float delta);
    void view_yaw(float delta);

    void update(float delta);
};

} // namespace LD