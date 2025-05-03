#pragma once

namespace LD {

enum MouseButton : int
{
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_ENUM_LAST,
};

enum KeyCode : int
{
    KEY_CODE_SPACE = 32,
    KEY_CODE_APOSTROPHE = 39, /* ' */
    KEY_CODE_COMMA = 44,      /* , */
    KEY_CODE_MINUS = 45,      /* - */
    KEY_CODE_PERIOD = 46,     /* . */
    KEY_CODE_SLASH = 47,      /* / */
    KEY_CODE_0 = 48,
    KEY_CODE_1 = 49,
    KEY_CODE_2 = 50,
    KEY_CODE_3 = 51,
    KEY_CODE_4 = 52,
    KEY_CODE_5 = 53,
    KEY_CODE_6 = 54,
    KEY_CODE_7 = 55,
    KEY_CODE_8 = 56,
    KEY_CODE_9 = 57,
    KEY_CODE_SEMICOLON = 59, /* ; */
    KEY_CODE_EQUAL = 61,     /* = */
    KEY_CODE_A = 65,
    KEY_CODE_B = 66,
    KEY_CODE_C = 67,
    KEY_CODE_D = 68,
    KEY_CODE_E = 69,
    KEY_CODE_F = 70,
    KEY_CODE_G = 71,
    KEY_CODE_H = 72,
    KEY_CODE_I = 73,
    KEY_CODE_J = 74,
    KEY_CODE_K = 75,
    KEY_CODE_L = 76,
    KEY_CODE_M = 77,
    KEY_CODE_N = 78,
    KEY_CODE_O = 79,
    KEY_CODE_P = 80,
    KEY_CODE_Q = 81,
    KEY_CODE_R = 82,
    KEY_CODE_S = 83,
    KEY_CODE_T = 84,
    KEY_CODE_U = 85,
    KEY_CODE_V = 86,
    KEY_CODE_W = 87,
    KEY_CODE_X = 88,
    KEY_CODE_Y = 89,
    KEY_CODE_Z = 90,
    KEY_CODE_LEFT_BRACKET = 91,  /* [ */
    KEY_CODE_BACKSLASH = 92,     /* \ */
    KEY_CODE_RIGHT_BRACKET = 93, /* ] */
    KEY_CODE_GRAVE_ACCENT = 96,  /* ` */
    KEY_CODE_WORLD_1 = 161,      /* non-US #1 */
    KEY_CODE_WORLD_2 = 162,      /* non-US #2 */

    KEY_CODE_ESCAPE = 256,
    KEY_CODE_ENTER = 257,
    KEY_CODE_TAB = 258,
    KEY_CODE_BACKSPACE = 259,
    KEY_CODE_INSERT = 260,
    KEY_CODE_DELETE = 261,
    KEY_CODE_RIGHT = 262,
    KEY_CODE_LEFT = 263,
    KEY_CODE_DOWN = 264,
    KEY_CODE_UP = 265,
    KEY_CODE_PAGE_UP = 266,
    KEY_CODE_PAGE_DOWN = 267,
    KEY_CODE_HOME = 268,
    KEY_CODE_END = 269,
    KEY_CODE_CAPS_LOCK = 280,
    KEY_CODE_SCROLL_LOCK = 281,
    KEY_CODE_NUM_LOCK = 282,
    KEY_CODE_PRINT_SCREEN = 283,
    KEY_CODE_PAUSE = 284,
    KEY_CODE_F1 = 290,
    KEY_CODE_F2 = 291,
    KEY_CODE_F3 = 292,
    KEY_CODE_F4 = 293,
    KEY_CODE_F5 = 294,
    KEY_CODE_F6 = 295,
    KEY_CODE_F7 = 296,
    KEY_CODE_F8 = 297,
    KEY_CODE_F9 = 298,
    KEY_CODE_F10 = 299,
    KEY_CODE_F11 = 300,
    KEY_CODE_F12 = 301,
    KEY_CODE_F13 = 302,
    KEY_CODE_F14 = 303,
    KEY_CODE_F15 = 304,
    KEY_CODE_F16 = 305,
    KEY_CODE_F17 = 306,
    KEY_CODE_F18 = 307,
    KEY_CODE_F19 = 308,
    KEY_CODE_F20 = 309,
    KEY_CODE_F21 = 310,
    KEY_CODE_F22 = 311,
    KEY_CODE_F23 = 312,
    KEY_CODE_F24 = 313,
    KEY_CODE_F25 = 314,
    KEY_CODE_KEYPAD_0 = 320,
    KEY_CODE_KEYPAD_1 = 321,
    KEY_CODE_KEYPAD_2 = 322,
    KEY_CODE_KEYPAD_3 = 323,
    KEY_CODE_KEYPAD_4 = 324,
    KEY_CODE_KEYPAD_5 = 325,
    KEY_CODE_KEYPAD_6 = 326,
    KEY_CODE_KEYPAD_7 = 327,
    KEY_CODE_KEYPAD_8 = 328,
    KEY_CODE_KEYPAD_9 = 329,
    KEY_CODE_KEYPAD_DECIMAL = 330,
    KEY_CODE_KEYPAD_DIVIDE = 331,
    KEY_CODE_KEYPAD_MULTIPLY = 332,
    KEY_CODE_KEYPAD_SUBTRACT = 333,
    KEY_CODE_KEYPAD_ADD = 334,
    KEY_CODE_KEYPAD_ENTER = 335,
    KEY_CODE_KEYPAD_EQUAL = 336,
    KEY_CODE_LEFT_SHIFT = 340,
    KEY_CODE_LEFT_CONTROL = 341,
    KEY_CODE_LEFT_ALT = 342,
    KEY_CODE_LEFT_SUPER = 343,
    KEY_CODE_RIGHT_SHIFT = 344,
    KEY_CODE_RIGHT_CONTROL = 345,
    KEY_CODE_RIGHT_ALT = 346,
    KEY_CODE_RIGHT_SUPER = 347,
    KEY_CODE_MENU = 348,
    KEY_CODE_ENUM_LAST
};

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