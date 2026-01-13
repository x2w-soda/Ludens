#include <Ludens/WindowRegistry/Input.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include <GLFW/glfw3.h>

namespace LD {
namespace Input {

bool get_key(KeyCode key)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_key(reg.get_root_id(), key);
}

bool get_key_down(KeyCode key)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_key_down(reg.get_root_id(), key);
}

bool get_key_up(KeyCode key)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_key_up(reg.get_root_id(), key);
}

bool get_mouse(MouseButton button)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_mouse(reg.get_root_id(), button);
}

bool get_mouse_down(MouseButton button)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_mouse_down(reg.get_root_id(), button);
}

bool get_mouse_up(MouseButton button)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_mouse_up(reg.get_root_id(), button);
}

void get_mouse_position(float& x, float& y)
{
    WindowRegistry reg = WindowRegistry::get();

    reg.get_window_mouse_position(reg.get_root_id(), x, y);
}

bool get_mouse_motion(float& dx, float& dy)
{
    WindowRegistry reg = WindowRegistry::get();

    return reg.get_window_mouse_motion(reg.get_root_id(), dx, dy);
}

} // namespace Input
} // namespace LD
