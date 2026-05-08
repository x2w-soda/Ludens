#include <Ludens/DSA/ViewUtil.h>

#include <algorithm>
#include <iterator>
#include <ranges>

namespace LD {

struct KeyCodeMeta
{
    KeyCode key;
    View str;
};

static const KeyCodeMeta sKeyCodeMeta[] = {
    {KEY_CODE_SPACE, "space"},
    {KEY_CODE_APOSTROPHE, "apostrophe"},
    {KEY_CODE_COMMA, "comma"},
    {KEY_CODE_MINUS, "minus"},
    {KEY_CODE_PERIOD, "period"},
    {KEY_CODE_SLASH, "slash"},
    {KEY_CODE_0, "0"},
    {KEY_CODE_1, "1"},
    {KEY_CODE_2, "2"},
    {KEY_CODE_3, "3"},
    {KEY_CODE_4, "4"},
    {KEY_CODE_5, "5"},
    {KEY_CODE_6, "6"},
    {KEY_CODE_7, "7"},
    {KEY_CODE_8, "8"},
    {KEY_CODE_9, "9"},
    {KEY_CODE_SEMICOLON, "semicolon"},
    {KEY_CODE_EQUAL, "equal"},
    {KEY_CODE_A, "a"},
    {KEY_CODE_B, "b"},
    {KEY_CODE_C, "c"},
    {KEY_CODE_D, "d"},
    {KEY_CODE_E, "e"},
    {KEY_CODE_F, "f"},
    {KEY_CODE_G, "g"},
    {KEY_CODE_H, "h"},
    {KEY_CODE_I, "i"},
    {KEY_CODE_J, "j"},
    {KEY_CODE_K, "k"},
    {KEY_CODE_L, "l"},
    {KEY_CODE_M, "m"},
    {KEY_CODE_N, "n"},
    {KEY_CODE_O, "o"},
    {KEY_CODE_P, "p"},
    {KEY_CODE_Q, "q"},
    {KEY_CODE_R, "r"},
    {KEY_CODE_S, "s"},
    {KEY_CODE_T, "t"},
    {KEY_CODE_U, "u"},
    {KEY_CODE_V, "v"},
    {KEY_CODE_W, "w"},
    {KEY_CODE_X, "x"},
    {KEY_CODE_Y, "y"},
    {KEY_CODE_Z, "z"},
    {KEY_CODE_LEFT_BRACKET, "left_bracket"},
    {KEY_CODE_BACKSLASH, "backslash"},
    {KEY_CODE_RIGHT_BRACKET, "right_bracket"},
    {KEY_CODE_GRAVE_ACCENT, "grave_accent"},
    {KEY_CODE_WORLD_1, "world_1"},
    {KEY_CODE_WORLD_2, "world_2"},
    {KEY_CODE_ESCAPE, "escape"},
    {KEY_CODE_ENTER, "enter"},
    {KEY_CODE_TAB, "tab"},
    {KEY_CODE_BACKSPACE, "backspace"},
    {KEY_CODE_INSERT, "insert"},
    {KEY_CODE_DELETE, "delete"},
    {KEY_CODE_RIGHT, "right"},
    {KEY_CODE_LEFT, "left"},
    {KEY_CODE_DOWN, "down"},
    {KEY_CODE_UP, "up"},
    {KEY_CODE_PAGE_UP, "page_up"},
    {KEY_CODE_PAGE_DOWN, "page_down"},
    {KEY_CODE_HOME, "home"},
    {KEY_CODE_END, "end"},
    {KEY_CODE_CAPS_LOCK, "caps_lock"},
    {KEY_CODE_SCROLL_LOCK, "scroll_lock"},
    {KEY_CODE_NUM_LOCK, "num_lock"},
    {KEY_CODE_PRINT_SCREEN, "print_screen"},
    {KEY_CODE_PAUSE, "pause"},
    {KEY_CODE_F1, "f1"},
    {KEY_CODE_F2, "f2"},
    {KEY_CODE_F3, "f3"},
    {KEY_CODE_F4, "f4"},
    {KEY_CODE_F5, "f5"},
    {KEY_CODE_F6, "f6"},
    {KEY_CODE_F7, "f7"},
    {KEY_CODE_F8, "f8"},
    {KEY_CODE_F9, "f9"},
    {KEY_CODE_F10, "f10"},
    {KEY_CODE_F11, "f11"},
    {KEY_CODE_F12, "f12"},
    {KEY_CODE_F13, "f13"},
    {KEY_CODE_F14, "f14"},
    {KEY_CODE_F15, "f15"},
    {KEY_CODE_F16, "f16"},
    {KEY_CODE_F17, "f17"},
    {KEY_CODE_F18, "f18"},
    {KEY_CODE_F19, "f19"},
    {KEY_CODE_F20, "f20"},
    {KEY_CODE_F21, "f21"},
    {KEY_CODE_F22, "f22"},
    {KEY_CODE_F23, "f23"},
    {KEY_CODE_F24, "f24"},
    {KEY_CODE_F25, "f25"},
    {KEY_CODE_KEYPAD_0, "keypad_0"},
    {KEY_CODE_KEYPAD_1, "keypad_1"},
    {KEY_CODE_KEYPAD_2, "keypad_2"},
    {KEY_CODE_KEYPAD_3, "keypad_3"},
    {KEY_CODE_KEYPAD_4, "keypad_4"},
    {KEY_CODE_KEYPAD_5, "keypad_5"},
    {KEY_CODE_KEYPAD_6, "keypad_6"},
    {KEY_CODE_KEYPAD_7, "keypad_7"},
    {KEY_CODE_KEYPAD_8, "keypad_8"},
    {KEY_CODE_KEYPAD_9, "keypad_9"},
    {KEY_CODE_KEYPAD_DECIMAL, "keypad_decimal"},
    {KEY_CODE_KEYPAD_DIVIDE, "keypad_divide"},
    {KEY_CODE_KEYPAD_MULTIPLY, "keypad_multiply"},
    {KEY_CODE_KEYPAD_SUBTRACT, "keypad_subtract"},
    {KEY_CODE_KEYPAD_ADD, "keypad_add"},
    {KEY_CODE_KEYPAD_ENTER, "keypad_enter"},
    {KEY_CODE_KEYPAD_EQUAL, "keypad_equal"},
    {KEY_CODE_LEFT_SHIFT, "left_shift"},
    {KEY_CODE_LEFT_CONTROL, "left_control"},
    {KEY_CODE_LEFT_ALT, "left_alt"},
    {KEY_CODE_LEFT_SUPER, "left_super"},
    {KEY_CODE_RIGHT_SHIFT, "right_shift"},
    {KEY_CODE_RIGHT_CONTROL, "right_control"},
    {KEY_CODE_RIGHT_ALT, "right_alt"},
    {KEY_CODE_RIGHT_SUPER, "right_super"},
    {KEY_CODE_MENU, "menu"},
};

struct MouseButtonMeta
{
    MouseButton btn;
    View str;
};

static const MouseButtonMeta sMouseButtonMeta[] = {
    {MOUSE_BUTTON_LEFT, "lmb"},
    {MOUSE_BUTTON_RIGHT, "rmb"},
    {MOUSE_BUTTON_MIDDLE, "mmb"},
};

static_assert(std::size(sMouseButtonMeta) == (size_t)MOUSE_BUTTON_ENUM_LAST);

View to_string(KeyCode key)
{
    auto it = std::ranges::lower_bound(sKeyCodeMeta, key, {}, &KeyCodeMeta::key);

    if (it != std::end(sKeyCodeMeta) && it->key == key)
        return it->str;

    return {};
}

// TODO: probably shouldn't linear probe
bool from_string(View str, KeyCode& key)
{
    for (const KeyCodeMeta& meta : sKeyCodeMeta)
    {
        if (meta.str == str)
        {
            key = meta.key;
            return true;
        }
    }

    return false;
}

View to_string(MouseButton btn)
{
    return sMouseButtonMeta[(int)btn].str;
}

bool from_string(View str, MouseButton& btn)
{
    for (int i = 0; i < (int)MOUSE_BUTTON_ENUM_LAST; i++)
    {
        if (str == sMouseButtonMeta[i].str)
        {
            btn = (MouseButton)i;
            return true;
        }
    }

    return false;
}

} // namespace LD