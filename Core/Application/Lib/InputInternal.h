#pragma once

#define KEY_PRESSED_BIT 0x1
#define KEY_PRESSED_THIS_FRAME_BIT 0x2
#define KEY_RELEASED_THIS_FRAME_BIT 0x4

namespace LD {
namespace Input {

/// @brief clears "this frame" states for all input.
///        currently the frame boundary is determined by glfwPollEvents.
void frame_boundary();

} // namespace Input
} // namespace LD