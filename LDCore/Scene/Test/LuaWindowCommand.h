#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Event/WindowEvent.h>
#include <Ludens/Lua/LuaState.h>

namespace LD {

// TODO: fun stuff such as
// - LUA_WINDOW_COMMAND_TYPE_INPUT_TEXT
// - LUA_WINDOW_COMMAND_TYPE_DRAG_MOUSE
enum LuaWindowCommandType
{
    LUA_WINDOW_COMMAND_TYPE_EVENT,
    LUA_WINDOW_COMMAND_TYPE_ENUM_COUNT
};

struct LuaWindowCommand
{
    const LuaWindowCommandType type;
    String error;

    LuaWindowCommand() = delete;
    LuaWindowCommand(LuaWindowCommandType type)
        : type(type) {}

public: // static
    static View get_name(LuaWindowCommandType type);
    static LuaWindowCommandType get_type(const char* name);
    static LuaWindowCommand* create(LuaState L, String& err);
    static void destroy(LuaWindowCommand* cmd);
    static bool execute(LuaWindowCommand* cmd, String& err);
};

// inject single window event
struct LuaWindowCommandEvent : LuaWindowCommand
{
    EventType eventType;
    union
    {
        WindowResizeEvent resize;
        WindowKeyDownEvent keyDown;
        WindowKeyUpEvent keyUp;
        WindowMousePositionEvent mousePos;
        WindowMouseDownEvent mouseDown;
        WindowMouseUpEvent mouseUp;
        WindowScrollEvent scroll;
    };

    LuaWindowCommandEvent()
        : LuaWindowCommand(LUA_WINDOW_COMMAND_TYPE_EVENT) {}
};

} // namespace LD