#include "LuaScript.h"
#include <Ludens/Application/Application.h> // TODO: move application module forward in CMake dependency
#include <Ludens/DataRegistry/DataComponent.h>

#define LUDENS_LUA_MODULE_NAME "ludens"

namespace LD {
namespace LuaScript {

static int transform_get_position(lua_State* l);
static int transform_set_position(lua_State* l);
static int transform_get_rotation(lua_State* l);
static int transform_set_rotation(lua_State* l);
static int transform_get_scale(lua_State* l);
static int transform_set_scale(lua_State* l);
static void push_transform_table(DataRegistry reg, LuaState L, CUID compID, Transform* transform);
static void push_mesh_component_table(DataRegistry reg, LuaState L, CUID compID, void* comp);

static inline void get_transform_cuid(LuaState L, int tIndex, CUID& compID, DataRegistry& reg)
{
    L.get_field(tIndex, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    compID = (CUID)L.to_number(-1);
    L.pop(1);

    L.get_field(tIndex, "_reg");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);
    reg = DataRegistry((DataRegistryObj*)L.to_userdata(-1));
}

/// @brief Transform:get_position()
static int transform_get_position(lua_State* l)
{
    LuaState L(l);

    L.get_field(-1, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform* transform = (Transform*)L.to_userdata(-1);
    L.push_vec3(transform->position);

    return 1;
}

/// @brief Transform:set_position(Vec3)
static int transform_set_position(lua_State* l)
{
    LuaState L(l);

    L.get_field(-2, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform* transform = (Transform*)L.to_userdata(-1);
    transform->position = L.to_vec3(-2);
    L.pop(1);

    DataRegistry reg;
    CUID compID;
    get_transform_cuid(L, -2, compID, reg);
    reg.mark_component_transform_dirty(compID);

    return 0;
}

/// @brief Transform:get_rotation()
static int transform_get_rotation(lua_State* l)
{
    LuaState L(l);

    L.get_field(-1, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform* transform = (Transform*)L.to_userdata(-1);
    L.push_vec3(transform->rotation);

    return 1;
}

/// @brief Transform:set_rotation(Vec3)
static int transform_set_rotation(lua_State* l)
{
    LuaState L(l);

    L.get_field(-2, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform* transform = (Transform*)L.to_userdata(-1);
    transform->rotation = L.to_vec3(-2);
    transform->quat = Quat::from_euler(transform->rotation);
    L.pop(1);

    DataRegistry reg;
    CUID compID;
    get_transform_cuid(L, -2, compID, reg);
    reg.mark_component_transform_dirty(compID);

    return 0;
}

/// @brief Transform:get_scale()
static int transform_get_scale(lua_State* l)
{
    LuaState L(l);

    L.get_field(-1, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform* transform = (Transform*)L.to_userdata(-1);
    L.push_vec3(transform->scale);

    return 1;
}

/// @brief Transform:set_scale(Vec3)
static int transform_set_scale(lua_State* l)
{
    LuaState L(l);

    L.get_field(-2, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform* transform = (Transform*)L.to_userdata(-1);
    transform->scale = L.to_vec3(-2);
    L.pop(1);

    DataRegistry reg;
    CUID compID;
    get_transform_cuid(L, -2, compID, reg);
    reg.mark_component_transform_dirty(compID);

    return 0;
}

/// @brief Pushes a lua table representing a Transform.
static void push_transform_table(DataRegistry reg, LuaState L, CUID compID, Transform* transform)
{
    L.push_table(); // transform

    L.push_light_userdata(transform);
    L.set_field(-2, "_ud");

    L.push_light_userdata(reg.unwrap());
    L.set_field(-2, "_reg");

    L.push_number((double)compID);
    L.set_field(-2, "_cuid");

    L.push_fn(&transform_get_position);
    L.set_field(-2, "get_position");

    L.push_fn(&transform_set_position);
    L.set_field(-2, "set_position");

    L.push_fn(&transform_get_rotation);
    L.set_field(-2, "get_rotation");

    L.push_fn(&transform_set_rotation);
    L.set_field(-2, "set_rotation");

    L.push_fn(&transform_get_scale);
    L.set_field(-2, "get_scale");

    L.push_fn(&transform_set_scale);
    L.set_field(-2, "set_scale");
}

static void push_mesh_component_table(DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    MeshComponent* meshC = (MeshComponent*)comp;

    L.push_table(); // mesh component
    push_transform_table(reg, L, compID, &meshC->transform);
    L.set_field(-2, "transform");
}

// clang-format off
struct
{
    ComponentType type;
    void (*push_table)(DataRegistry reg, LuaState L, CUID compID, void* comp);
} sComponents[] = {
    {COMPONENT_TYPE_DATA,       nullptr},
    {COMPONENT_TYPE_TRANSFORM,  nullptr},
    {COMPONENT_TYPE_MESH,       &push_mesh_component_table},
    {COMPONENT_TYPE_TEXTURE_2D, nullptr},
};
// clang-format on

static int exit_application(lua_State* l)
{
    Application app = Application::get();

    app.exit();
    return 0;
}

LuaModule create_ludens_module()
{
    LuaModuleValue values[] = {
        {.type = LUA_TYPE_FN, .name = "exit_application", .fn = &LuaScript::exit_application},
    };

    LuaModuleNamespace space;
    space.name = nullptr;
    space.valueCount = sizeof(values) / sizeof(*values);
    space.values = values;

    LuaModuleInfo modI;
    modI.name = LUDENS_LUA_MODULE_NAME;
    modI.spaceCount = 1;
    modI.spaces = &space;

    return LuaModule::create(modI); // caller destroys
}

// stack top should be ludens.scripts
void create_component_table(DataRegistry reg, LuaState L, CUID compID, ComponentType type, void* comp)
{
    L.push_number((double)compID);
    L.get_table(-2);                             // ludens.scripts[compID]
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE); // script instance table missing

    sComponents[(int)type].push_table(reg, L, compID, comp);
    L.set_field(-2, "_comp");
    L.pop(1);
}

// stack top should be ludens.scripts
void destroy_component_table(DataRegistry reg, LuaState L, CUID compID)
{
    L.push_number((double)compID);
    L.get_table(-2);                             // ludens.scripts[compID]
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE); // script instance table missing

    L.push_nil();
    L.set_field(-2, "_comp"); // ludens.scripts[compID]._comp = nil
    L.pop(1);
}

} // namespace LuaScript
} // namespace LD