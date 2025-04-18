#include "InputInternal.h"
#include <GLFW/glfw3.h>
#include <Ludens/Application/Input.h>
#include <cstdint>
#include <cstring>

namespace LD {
namespace Input {

// keycodes are defined to be identical to GLFW
static_assert(GLFW_KEY_LAST == KEYCODE_ENUM_LAST);

uint8_t sKeyState[KEYCODE_ENUM_LAST];

void frame_boundary()
{
    for (int i = 0; i < KEYCODE_ENUM_LAST; i++)
        sKeyState[i] &= ~(KEY_PRESSED_THIS_FRAME_BIT | KEY_RELEASED_THIS_FRAME_BIT);
}

bool get_key(KeyCode key)
{
    return sKeyState[key] & KEY_PRESSED_BIT;
}

bool get_key_down(KeyCode key)
{
    return sKeyState[key] & KEY_PRESSED_THIS_FRAME_BIT;
}

bool get_key_up(KeyCode key)
{
    return sKeyState[key] & KEY_RELEASED_THIS_FRAME_BIT;
}

} // namespace Input
} // namespace LD
