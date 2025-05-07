#pragma once

#include <Ludens/Header/KeyCode.h>

namespace LD {
namespace Input {

/// @brief returns true while the key is pressed down
bool get_key(KeyCode key);

/// @brief returns true during the exact frame where the user pressed down the key
bool get_key_down(KeyCode key);

/// @brief returns true during the exact frame the user released the key
bool get_key_up(KeyCode key);

/// @brief returns true while the mouse button is pressed down
bool get_mouse(MouseButton button);

/// @brief returns true during the exact frame where the user pressed down the mouse button
bool get_mouse_down(MouseButton button);

/// @brief returns true during the exact frame where the user released the mouse button
bool get_mouse_up(MouseButton button);

/// @brief get the mouse cursor position in screen space
void get_mouse_position(float& x, float& y);

/// @brief get the mouse cursor position difference with the last frame
bool get_mouse_motion(float& dx, float& dy);

} // namespace Input
} // namespace LD