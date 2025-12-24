#include <Ludens/Event/Event.h>
#include <cstdint>
#include <cstring>

#include <GLFW/glfw3.h>

#include "InputInternal.h"

namespace LD {
namespace Input {

// keycodes are defined to be identical to GLFW
static_assert(GLFW_KEY_LAST < KEY_CODE_ENUM_LAST);
static_assert(GLFW_MOUSE_BUTTON_LEFT == MOUSE_BUTTON_LEFT);
static_assert(GLFW_MOUSE_BUTTON_RIGHT == MOUSE_BUTTON_RIGHT);
static_assert(GLFW_MOUSE_BUTTON_MIDDLE == MOUSE_BUTTON_MIDDLE);

uint8_t sKeyState[KEY_CODE_ENUM_LAST];
uint8_t sMouseState[MOUSE_BUTTON_ENUM_LAST];
float sMouseCursorDeltaX;
float sMouseCursorDeltaY;
float sMouseCursorX;
float sMouseCursorY;

void frame_boundary()
{
    for (int i = 0; i < KEY_CODE_ENUM_LAST; i++)
        sKeyState[i] &= ~(PRESSED_THIS_FRAME_BIT | RELEASED_THIS_FRAME_BIT);

    for (int i = 0; i < MOUSE_BUTTON_ENUM_LAST; i++)
        sMouseState[i] &= ~(PRESSED_THIS_FRAME_BIT | RELEASED_THIS_FRAME_BIT);
}

bool get_key(KeyCode key)
{
    return sKeyState[key] & PRESSED_BIT;
}

bool get_key_down(KeyCode key)
{
    return sKeyState[key] & PRESSED_THIS_FRAME_BIT;
}

bool get_key_up(KeyCode key)
{
    return sKeyState[key] & RELEASED_THIS_FRAME_BIT;
}

bool get_mouse(MouseButton button)
{
    return sMouseState[button] & PRESSED_BIT;
}

bool get_mouse_down(MouseButton button)
{
    return sMouseState[button] & PRESSED_THIS_FRAME_BIT;
}

bool get_mouse_up(MouseButton button)
{
    return sMouseState[button] & RELEASED_THIS_FRAME_BIT;
}

void get_mouse_position(float& x, float& y)
{
    x = sMouseCursorX;
    y = sMouseCursorY;
}

bool get_mouse_motion(float& dx, float& dy)
{
    dx = sMouseCursorDeltaX;
    dy = sMouseCursorDeltaY;

    return dx || dy;
}

} // namespace Input
} // namespace LD
