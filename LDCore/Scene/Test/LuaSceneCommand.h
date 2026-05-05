#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Scene/SceneCommand.h>
#include <Ludens/Serial/Property.h>

namespace LD {

class Scene;

struct LuaSceneCommand
{
    const SceneCommandType type;
    std::string error;

    LuaSceneCommand() = delete;
    LuaSceneCommand(SceneCommandType type)
        : type(type) {}
};

struct LuaSceneCommandLoadScene : LuaSceneCommand
{
    ComponentSubtreeData subtreeData;

    LuaSceneCommandLoadScene()
        : LuaSceneCommand(SCENE_COMMAND_TYPE_LOAD_SCENE) {}
};

struct LuaSceneCommandSetProps : LuaSceneCommand
{
    String compPath;
    Vector<PropertyNameValue> props;

    LuaSceneCommandSetProps()
        : LuaSceneCommand(SCENE_COMMAND_TYPE_SET_PROPS) {}
};

struct LuaSceneCommandGetProps : LuaSceneCommand
{
    String compPath;
    Vector<PropertyNameValue> props; // with expected value

    LuaSceneCommandGetProps()
        : LuaSceneCommand(SCENE_COMMAND_TYPE_GET_PROPS) {}
};

struct LuaSceneCommandCreateComponent : LuaSceneCommand
{
    String parentPath;
    String compName;
    ComponentType compType = COMPONENT_TYPE_ENUM_COUNT;
    Vector<PropertyNameValue> props;
    bool expectSuccess = true; // whether the scene command is expected to fail or succeed

    LuaSceneCommandCreateComponent()
        : LuaSceneCommand(SCENE_COMMAND_TYPE_CREATE_COMPONENT) {}
};

bool parse_lua_scene_commands(LuaState L, Vector<LuaSceneCommand*>& steps, std::string& err);
void free_lua_scene_commands(Vector<LuaSceneCommand*>& steps);
bool execute_lua_scene_command(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene, std::string& err);

} // namespace LD