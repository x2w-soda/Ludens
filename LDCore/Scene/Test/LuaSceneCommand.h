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
    String error;

    LuaSceneCommand() = delete;
    LuaSceneCommand(SceneCommandType type)
        : type(type) {}

    static LuaSceneCommand* create(LuaState L, String& err);
    static void destroy(LuaSceneCommand* cmd);
    static bool execute(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene, String& err);
};

struct LuaSceneCommandLoadScene : LuaSceneCommand
{
    ComponentSubtreeEntry subtree;

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

} // namespace LD