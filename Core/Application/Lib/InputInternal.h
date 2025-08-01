#pragma once

#include <cstdint>

#define PRESSED_BIT 0x1
#define PRESSED_THIS_FRAME_BIT 0x2
#define RELEASED_THIS_FRAME_BIT 0x4

namespace LD {
namespace Input {

/// @brief clears "this frame" states for all input.
///        currently the frame boundary is determined by glfwPollEvents.
void frame_boundary();

extern uint8_t sKeyState[];
extern uint8_t sMouseState[];
extern float sMouseCursorDeltaX;
extern float sMouseCursorDeltaY;
extern float sMouseCursorX;
extern float sMouseCursorY;

} // namespace Input
} // namespace LD