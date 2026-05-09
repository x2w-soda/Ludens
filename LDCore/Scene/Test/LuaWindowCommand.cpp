#include <Ludens/Header/Assert.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include <Ludens/DSA/ViewUtil.h>

#include "LuaWindowCommand.h"

#include <format>

#define MEMORY_USAGE MEMORY_USAGE_MISC

namespace LD {

struct LuaWindowCommandMeta
{
    LuaWindowCommand* (*create)(LuaState L, String& err);
    void (*destroy)(LuaWindowCommand* cmd);
    bool (*execute)(LuaWindowCommand* cmd, String& err);
    View name;
};

static LuaWindowCommand* event_cmd_create(LuaState L, String& err);
static void event_cmd_destroy(LuaWindowCommand* cmd);
static bool event_cmd_execute(LuaWindowCommand* cmd, String& err);

static LuaWindowCommandMeta sCommandMeta[] = {
    {&event_cmd_create, &event_cmd_destroy, &event_cmd_execute, "event"},
};

static_assert(sizeof(sCommandMeta) / sizeof(*sCommandMeta) == (int)LUA_WINDOW_COMMAND_TYPE_ENUM_COUNT);

static LuaWindowCommand* event_cmd_create(LuaState L, String& err)
{
    auto* cmd = heap_new<LuaWindowCommandEvent>(MEMORY_USAGE);

    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE);

    String str;
    if (!L.peek_string_field(-1, "type", str))
        return nullptr;

    // TODO:
    if (str != "key_down")
        return nullptr;

    cmd->eventType = EVENT_TYPE_WINDOW_KEY_DOWN;
    cmd->keyDown = WindowKeyDownEvent(0, KEY_CODE_X, 0);

    return cmd;
}

static void event_cmd_destroy(LuaWindowCommand* base)
{
    heap_delete<LuaWindowCommandEvent>((LuaWindowCommandEvent*)base);
}

static bool event_cmd_execute(LuaWindowCommand* base, String&)
{
    auto* cmd = (LuaWindowCommandEvent*)base;

    WindowRegistry reg = WindowRegistry::get();
    reg.inject_event(reg.get_root_id(), &cmd->resize);

    return true;
}

//
// Public API
//

View LuaWindowCommand::get_name(LuaWindowCommandType type)
{
    return sCommandMeta[(int)type].name;
}

LuaWindowCommandType LuaWindowCommand::get_type(const char* cstr)
{
    View name(cstr);

    for (int i = 0; i < (int)LUA_WINDOW_COMMAND_TYPE_ENUM_COUNT; i++)
    {
        if (name == sCommandMeta[i].name)
            return (LuaWindowCommandType)i;
    }

    return LUA_WINDOW_COMMAND_TYPE_ENUM_COUNT;
}

LuaWindowCommand* LuaWindowCommand::create(LuaState L, String& err)
{
    int oldSize = L.size();

    String cmdName;
    if (!L.peek_string_field(-1, "window_command", cmdName))
        return nullptr;

    LuaWindowCommandType cmdType = LuaWindowCommand::get_type(cmdName.c_str());
    if (cmdType == LUA_WINDOW_COMMAND_TYPE_ENUM_COUNT)
    {
        err = std::format("unknown command type: {}", cmdName.c_str()).c_str();
        return nullptr;
    }

    LuaWindowCommand* cmd = sCommandMeta[(int)cmdType].create(L, err);
    if (!cmd)
    {
        err = std::format("failed to create command type: {}", cmdName.c_str()).c_str();
        return nullptr;
    }

    L.resize(oldSize);

    return cmd;
}

void LuaWindowCommand::destroy(LuaWindowCommand* cmd)
{
    sCommandMeta[(int)cmd->type].destroy(cmd);
}

bool LuaWindowCommand::execute(LuaWindowCommand* cmd, String& err)
{
    return sCommandMeta[(int)cmd->type].execute(cmd, err);
}

} // namespace LD