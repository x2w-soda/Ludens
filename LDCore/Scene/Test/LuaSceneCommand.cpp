#include <Ludens/Scene/Scene.h>

#include "LuaSceneCommand.h"

#define MEMORY_USAGE MEMORY_USAGE_MISC

namespace LD {

struct LuaSceneCommandMeta
{
    LuaSceneCommand* (*create)(LuaState L, String& err);
    void (*destroy)(LuaSceneCommand* luaCmd);
    bool (*execute)(LuaSceneCommand* luaCmd, SceneCommandQueue cmdQ, Scene& scene);
    void (*check)(LuaSceneCommand* luaCmd, SceneCommand* cmd);
};

static void lua_scene_command_response_fn(SceneCommand* cmd, void* user);
static void extract_property_table(LuaState L, Vector<PropertyNameValue>& props);

static LuaSceneCommand* load_scene_create(LuaState L, String& err);
static void load_scene_destroy(LuaSceneCommand* cmd);
static bool load_scene_execute(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene);
static LuaSceneCommand* set_props_create(LuaState L, String& err);
static void set_props_destroy(LuaSceneCommand* cmd);
static bool set_props_execute(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene);
static LuaSceneCommand* get_props_create(LuaState L, String& err);
static void get_props_destroy(LuaSceneCommand* cmd);
static bool get_props_execute(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene);
static void get_props_check(LuaSceneCommand* luaCmd, SceneCommand* cmd);
static LuaSceneCommand* create_component_create(LuaState L, String& err);
static void create_component_destroy(LuaSceneCommand* cmd);
static bool create_component_execute(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene);
static void create_component_check(LuaSceneCommand* luaCmd, SceneCommand* cmd);

static LuaSceneCommandMeta sCommandMeta[] = {
    {&load_scene_create, &load_scene_destroy, &load_scene_execute},
    {&set_props_create, &set_props_destroy, &set_props_execute},
    {&get_props_create, &get_props_destroy, &get_props_execute, &get_props_check},
    {&create_component_create, &create_component_destroy, &create_component_execute, &create_component_check},
};

static void lua_scene_command_response_fn(SceneCommand* cmd, void* user)
{
    auto* luaCmd = (LuaSceneCommand*)user;

    if (sCommandMeta[(int)cmd->type].check)
        sCommandMeta[(int)cmd->type].check(luaCmd, cmd);
}

static void extract_value_table(LuaState L, Value64& val)
{
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE);

    Vec2 v2;
    if (L.peek_vec2(-1, v2))
    {
        val.set_vec2(v2);
        return;
    }

    val = {}; // unknown
}

static void extract_property_table(LuaState L, Vector<PropertyNameValue>& props)
{
    int oldSize = L.size();

    L.push_nil();
    while (L.next(-2))
    {
        PropertyNameValue prop;
        prop.name = L.to_string(-2);

        switch (L.get_type(-1))
        {
        case LUA_TYPE_NUMBER:
            prop.value.set_f64(L.to_number(-1));
            break;
        case LUA_TYPE_STRING:
            prop.value.set_string(L.to_string(-1));
            break;
        case LUA_TYPE_BOOL:
            prop.value.set_bool(L.to_bool(-1));
            break;
        case LUA_TYPE_TABLE:
            extract_value_table(L, prop.value);
            break;
        default:
            LD_UNREACHABLE;
            break;
        }

        props.emplace_back(std::move(prop));

        L.pop(1);
    }

    L.resize(oldSize);
}

static LuaSceneCommand* load_scene_create(LuaState L, String& err)
{
    auto* luaCmd = heap_new<LuaSceneCommandLoadScene>(MEMORY_USAGE);
    int oldSize = L.size();

    if (!L.get_field_type(-1, "subtree", LUA_TYPE_TABLE))
        return nullptr;

    size_t compCount = L.get_objlen(-1);
    Vector<ComponentEntry>& components = luaCmd->subtree.components;
    components.resize(compCount);

    for (size_t i = 0; i < compCount; i++)
    {
        int oldSize2 = L.size();

        L.push_number(i + 1);
        L.get_table(-2);
        LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE);

        if (!L.get_field_type(-1, "parent", LUA_TYPE_NUMBER))
            return nullptr;
        components[i].parentIndex = (int32_t)L.to_number(-1);
        L.pop(1);

        if (!L.peek_string_field(-1, "name", components[i].name))
            return nullptr;

        String str;
        if (!L.peek_string_field(-1, "type", str))
            return nullptr;

        components[i].type = get_component_type(str.c_str());
        LD_ASSERT(components[i].type != COMPONENT_TYPE_ENUM_COUNT);

        if (L.peek_string_field(-1, "script_name", str))
        {
            AssetManager AM = AssetManager::get();
            components[i].scriptID = AM.get_id_from_name(str.c_str(), nullptr);
            LD_ASSERT(components[i].scriptID);
        }

        L.resize(oldSize2);
    }

    L.resize(oldSize);
    return luaCmd;
}

static void load_scene_destroy(LuaSceneCommand* cmd)
{
    heap_delete<LuaSceneCommandLoadScene>((LuaSceneCommandLoadScene*)cmd);
}

static bool load_scene_execute(LuaSceneCommand* base, SceneCommandQueue cmdQ, Scene& scene)
{
    auto* cmd = (SceneCommandLoadScene*)cmdQ.enqueue(SCENE_COMMAND_TYPE_LOAD_SCENE);
    auto* luaCmd = (LuaSceneCommandLoadScene*)base;
    cmd->subtree = luaCmd->subtree; // maybe std::move this?

    cmdQ.poll_commands(scene);

    // NOTE: startup right after load
    return scene.startup();
}

static LuaSceneCommand* set_props_create(LuaState L, String& err)
{
    auto* luaCmd = heap_new<LuaSceneCommandSetProps>(MEMORY_USAGE);
    int oldSize = L.size();

    if (!L.get_field_type(-1, "path", LUA_TYPE_STRING))
        return nullptr;

    luaCmd->compPath = L.to_string(-1);
    L.pop(1);

    if (!L.get_field_type(-1, "props", LUA_TYPE_TABLE))
        return nullptr;

    extract_property_table(L, luaCmd->props);

    L.resize(oldSize);
    return luaCmd;
}

static void set_props_destroy(LuaSceneCommand* cmd)
{
    heap_delete<LuaSceneCommandSetProps>((LuaSceneCommandSetProps*)cmd);
}

static bool set_props_execute(LuaSceneCommand* base, SceneCommandQueue cmdQ, Scene& scene)
{
    auto* cmd = (SceneCommandSetProps*)cmdQ.enqueue(SCENE_COMMAND_TYPE_SET_PROPS);
    auto* luaCmd = (LuaSceneCommandSetProps*)base;
    cmd->compPath = luaCmd->compPath;
    cmd->props = luaCmd->props;

    cmdQ.poll_commands(scene);
    return true;
}

static LuaSceneCommand* get_props_create(LuaState L, String& err)
{
    auto* luaCmd = heap_new<LuaSceneCommandGetProps>(MEMORY_USAGE);
    int oldSize = L.size();

    if (!L.get_field_type(-1, "path", LUA_TYPE_STRING))
        return nullptr;

    luaCmd->compPath = L.to_string(-1);
    L.pop(1);

    if (!L.get_field_type(-1, "props", LUA_TYPE_TABLE))
        return nullptr;

    extract_property_table(L, luaCmd->props);

    L.resize(oldSize);
    return luaCmd;
}

static void get_props_destroy(LuaSceneCommand* cmd)
{
    heap_delete<LuaSceneCommandGetProps>((LuaSceneCommandGetProps*)cmd);
}

static bool get_props_execute(LuaSceneCommand* base, SceneCommandQueue cmdQ, Scene& scene)
{
    auto* cmd = (SceneCommandGetProps*)cmdQ.enqueue(SCENE_COMMAND_TYPE_GET_PROPS);
    auto* luaCmd = (LuaSceneCommandGetProps*)base;
    cmd->compPath = luaCmd->compPath;
    cmd->props = luaCmd->props; // NOTE: lua command holds the expected values

    cmdQ.poll_commands(scene);
    return true;
}

static void get_props_check(LuaSceneCommand* luaCmdBase, SceneCommand* cmdBase)
{
    auto* luaCmd = (LuaSceneCommandGetProps*)luaCmdBase;
    auto* cmd = (SceneCommandGetProps*)cmdBase;

    if (cmd->props.size() != luaCmd->props.size())
    {
        luaCmd->error = std::format("query expected {} properties, found {}", luaCmd->props.size(), cmd->props.size()).c_str();
        return;
    }

    for (size_t i = 0; i < luaCmd->props.size(); i++)
    {
        const Value64& actual = cmd->props[i].value;
        Value64 expected = luaCmd->props[i].value;
        (void)Value64::narrow(actual.type, expected); // narrow before comparison

        if (actual != expected)
        {
            luaCmd->error = std::format("property value mismatch, expected {}, found {}", expected.print().c_str(), actual.print().c_str()).c_str();
            return;
        }
    }
}

static LuaSceneCommand* create_component_create(LuaState L, String& err)
{
    auto* luaCmd = heap_new<LuaSceneCommandCreateComponent>(MEMORY_USAGE);
    int oldSize = L.size();
    String str;

    if (!L.peek_string_field(-1, "parent_path", luaCmd->parentPath))
        return nullptr;

    if (!L.peek_string_field(-1, "name", luaCmd->compName))
        return nullptr;

    if (!L.peek_string_field(-1, "type", str))
        return nullptr;

    luaCmd->compType = get_component_type(str.c_str());

    if (!L.get_field_type(-1, "props", LUA_TYPE_TABLE))
        return nullptr;

    extract_property_table(L, luaCmd->props);

    L.resize(oldSize);
    return luaCmd;
}

static void create_component_destroy(LuaSceneCommand* cmd)
{
    heap_delete<LuaSceneCommandCreateComponent>((LuaSceneCommandCreateComponent*)cmd);
}

static bool create_component_execute(LuaSceneCommand* luaCmdBase, SceneCommandQueue cmdQ, Scene& scene)
{
    auto* cmd = (SceneCommandCreateComponent*)cmdQ.enqueue(SCENE_COMMAND_TYPE_CREATE_COMPONENT);
    auto* luaCmd = (LuaSceneCommandCreateComponent*)luaCmdBase;
    cmd->parentPath = luaCmd->parentPath;
    cmd->props = luaCmd->props;
    cmd->compName = luaCmd->compName;
    cmd->compType = luaCmd->compType;

    cmdQ.poll_commands(scene);
    return true;
}

static void create_component_check(LuaSceneCommand* luaCmdBase, SceneCommand* cmdBase)
{
    auto* luaCmd = (LuaSceneCommandCreateComponent*)luaCmdBase;
    auto* cmd = (SceneCommandCreateComponent*)cmdBase;

    if (luaCmd->expectSuccess != (bool)cmd->result.compView)
    {
        luaCmd->error = ("SceneCommandCreateComponent " + std::string(cmd->result.compView ? "succeeded" : "failed")).c_str();
    }
}

//
// PUBLIC API
//

LuaSceneCommand* LuaSceneCommand::create(LuaState L, String& err)
{
    if (!L.get_field_type(-1, "scene_command", LUA_TYPE_STRING))
    {
        err = "not a scene command";
        return nullptr;
    }

    String cmdName = L.to_string(-1);
    SceneCommandType cmdType = SceneCommand::get_type(cmdName.c_str());
    if (cmdType == SCENE_COMMAND_TYPE_ENUM_COUNT)
    {
        err = std::format("unknown command type: {}", cmdName.c_str()).c_str();
        return nullptr;
    }
    L.pop(1);

    LuaSceneCommand* cmd = sCommandMeta[(int)cmdType].create(L, err);
    if (!cmd)
        return nullptr;

    return cmd;
}

void LuaSceneCommand::destroy(LuaSceneCommand* cmd)
{
    sCommandMeta[(int)cmd->type].destroy(cmd);
}

bool LuaSceneCommand::execute(LuaSceneCommand* cmd, SceneCommandQueue cmdQ, Scene& scene, String& err)
{
    cmdQ.set_response_callback(&lua_scene_command_response_fn, cmd);

    if (!sCommandMeta[(int)cmd->type].execute(cmd, cmdQ, scene))
    {
        err = cmd->error;
        return false;
    }

    return true;
}

} // namespace LD
