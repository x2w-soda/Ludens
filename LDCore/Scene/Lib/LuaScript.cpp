#include "LuaScript.h"
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Input.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <array>
#include <cstring>
#include <string>

#define LUDENS_LUA_SCRIPT_LOG_CHANNEL "LuaScript"
#define LUDENS_LUA_MODULE_NAME "ludens"

namespace LD {

static Log sLog(LUDENS_LUA_SCRIPT_LOG_CHANNEL);

namespace LuaScript {

static KeyCode string_to_keycode(const char* str);
static MouseButton string_to_mouse_button(const char* cstr);
static inline ComponentBase* get_component_base(LuaState& L, DataRegistry* outReg);
static inline bool push_script_table(LuaState& L, CUID compID);
static int transform_get_position(lua_State* l);
static int transform_set_position(lua_State* l);
static int transform_get_rotation(lua_State* l);
static int transform_set_rotation(lua_State* l);
static int transform_get_scale(lua_State* l);
static int transform_set_scale(lua_State* l);
static int transform2d_get_position(lua_State* l);
static int transform2d_set_position(lua_State* l);
static int transform2d_get_rotation(lua_State* l);
static int transform2d_set_rotation(lua_State* l);
static int transform2d_get_scale(lua_State* l);
static int transform2d_set_scale(lua_State* l);
static int component_get_id(lua_State* l);
static int component_get_name(lua_State* l);
static int component_set_name(lua_State* l);
static void push_transform_table(DataRegistry reg, LuaState L, CUID compID, Transform* transform);
static void push_transform2d_table(DataRegistry reg, LuaState L, CUID compID, Transform2D* transform);
static void push_audio_source_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void push_camera_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void push_mesh_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void push_sprite2d_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void install_component_base(DataRegistry reg, LuaState& L, CUID compID);
static int application_exit(lua_State* l);
static int debug_log(lua_State* l);
static int input_get_key_down(lua_State* l);
static int input_get_key_up(lua_State* l);
static int input_get_key(lua_State* l);
static int input_get_mouse_down(lua_State* l);
static int input_get_mouse_up(lua_State* l);
static int input_get_mouse(lua_State* l);
static int audio_source_component_play(lua_State* l);
static int audio_source_component_pause(lua_State* l);
static int audio_source_component_resume(lua_State* l);

static KeyCode string_to_keycode(const char* cstr)
{
    size_t len = strlen(cstr);

    if (len == 1)
    {
        char c = *cstr;

        if ('a' <= c && c <= 'z')
            return static_cast<KeyCode>(c - 'a' + KEY_CODE_A);

        return KEY_CODE_ENUM_LAST; // failed to resolve
    }

    std::string str(cstr);
    if (str == "space")
    {
        return KEY_CODE_SPACE;
    }

    return KEY_CODE_ENUM_LAST; // failed to resolve
}

static MouseButton string_to_mouse_button(const char* cstr)
{
    std::string str(cstr);

    if (str == "lmb")
        return MOUSE_BUTTON_LEFT;

    if (str == "rmb")
        return MOUSE_BUTTON_RIGHT;

    return MOUSE_BUTTON_ENUM_LAST; // failed to resolve
}

static inline ComponentBase* get_component_base(LuaState& L, DataRegistry* outReg)
{
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE); // stack top should be component table

    int oldSize = L.size();

    L.get_field(-1, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    CUID compID = (CUID)L.to_number(-1);
    L.pop(1);

    L.get_field(-1, "_reg");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);
    DataRegistry reg((DataRegistryObj*)L.to_userdata(-1));

    if (outReg)
        *outReg = reg;

    ComponentBase* base = reg.get_component_base(compID);
    LD_ASSERT(base);

    L.resize(oldSize);

    return base;
}

/// @brief try and push ludens.scripts[compID], or nil on failure
static inline bool push_script_table(LuaState& L, CUID compID)
{
    int oldSize = L.size();

    L.get_global("ludens");
    L.get_field(-1, "scripts");
    L.push_number((double)compID);
    L.get_table(-2);

    LuaType type = L.get_type(-1);

    if (L.get_type(-1) == LUA_TYPE_TABLE) // script table
    {
        L.remove(-2);
        L.remove(-2);
        LD_ASSERT(L.size() == oldSize + 1);

        return true;
    }

    L.resize(oldSize);
    L.push_nil();
    return false;
}

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
    LD_PROFILE_SCOPE;
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
    LD_PROFILE_SCOPE;
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
    LD_PROFILE_SCOPE;
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
    LD_PROFILE_SCOPE;
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
    LD_PROFILE_SCOPE;
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
    LD_PROFILE_SCOPE;
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

/// @brief Transform2D:get_position()
static int transform2d_get_position(lua_State* l)
{
    LuaState L(l);

    L.get_field(-1, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform2D* transform = (Transform2D*)L.to_userdata(-1);
    L.push_vec2(transform->position);

    return 1;
}

/// @brief Transform2D:set_position(Vec2)
static int transform2d_set_position(lua_State* l)
{
    LuaState L(l);

    L.get_field(-2, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform2D* transform = (Transform2D*)L.to_userdata(-1);
    transform->scale = L.to_vec2(-2);
    L.pop(1);

    DataRegistry reg;
    CUID compID;
    get_transform_cuid(L, -2, compID, reg);
    reg.mark_component_transform_dirty(compID);

    return 0;
}

/// @brief Transform2D:get_rotation()
static int transform2d_get_rotation(lua_State* l)
{
    LuaState L(l);

    L.get_field(-1, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform2D* transform = (Transform2D*)L.to_userdata(-1);
    L.push_number((double)transform->rotation);

    return 1;
}

/// @brief Transform2D:set_rotation(number)
static int transform2d_set_rotation(lua_State* l)
{
    LuaState L(l);

    L.get_field(-2, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform2D* transform = (Transform2D*)L.to_userdata(-1);
    transform->scale = (float)L.to_number(-2);
    L.pop(1);

    DataRegistry reg;
    CUID compID;
    get_transform_cuid(L, -2, compID, reg);
    reg.mark_component_transform_dirty(compID);

    return 0;
}

/// @brief Transform2D:get_scale()
static int transform2d_get_scale(lua_State* l)
{
    LuaState L(l);

    L.get_field(-1, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform2D* transform = (Transform2D*)L.to_userdata(-1);
    L.push_vec2(transform->scale);

    return 1;
}

/// @brief Transform2D:set_scale(Vec2)
static int transform2d_set_scale(lua_State* l)
{
    LuaState L(l);

    L.get_field(-2, "_ud");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);

    Transform2D* transform = (Transform2D*)L.to_userdata(-1);
    transform->scale = L.to_vec2(-2);
    L.pop(1);

    DataRegistry reg;
    CUID compID;
    get_transform_cuid(L, -2, compID, reg);
    reg.mark_component_transform_dirty(compID);

    return 0;
}

/// @brief Component:get_id()
int component_get_id(lua_State* l)
{
    LuaState L(l);

    ComponentBase* base = get_component_base(L, nullptr);
    L.push_number((double)base->id);

    return 1;
}

/// @brief Component:get_name()
static int component_get_name(lua_State* l)
{
    LuaState L(l);

    ComponentBase* base = get_component_base(L, nullptr);
    L.push_string(base->name);

    return 1;
}

/// @brief Component:set_name(string)
static int component_set_name(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
        return 0;

    L.push_value(-2);
    ComponentBase* base = get_component_base(L, nullptr);
    LD_ASSERT(base && base->name);
    L.pop(1);

    heap_free(base->name);
    base->name = heap_strdup(L.to_string(-1), MEMORY_USAGE_MISC);

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

/// @brief Pushes a lua table representing a Transform2D.
void push_transform2d_table(DataRegistry reg, LuaState L, CUID compID, Transform2D* transform)
{
    L.push_table(); // Transform2D

    L.push_light_userdata(transform);
    L.set_field(-2, "_ud");

    L.push_light_userdata(reg.unwrap());
    L.set_field(-2, "_reg");

    L.push_number((double)compID);
    L.set_field(-2, "_cuid");

    L.push_fn(&transform2d_get_position);
    L.set_field(-2, "get_position");

    L.push_fn(&transform2d_set_position);
    L.set_field(-2, "set_position");

    L.push_fn(&transform2d_get_rotation);
    L.set_field(-2, "get_rotation");

    L.push_fn(&transform2d_set_rotation);
    L.set_field(-2, "set_rotation");

    L.push_fn(&transform2d_get_scale);
    L.set_field(-2, "get_scale");

    L.push_fn(&transform2d_set_scale);
    L.set_field(-2, "set_scale");
}

static void push_audio_source_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    L.push_table(); // audio source component
    install_component_base(reg, L, compID);

    L.push_light_userdata((void*)scene.unwrap());
    L.set_field(-2, "_scene");

    L.push_fn(&audio_source_component_play);
    L.set_field(-2, "play");

    L.push_fn(&audio_source_component_pause);
    L.set_field(-2, "pause");

    L.push_fn(&audio_source_component_resume);
    L.set_field(-2, "resume");
}

static void push_camera_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    CameraComponent* cameraC = (CameraComponent*)comp;

    L.push_table(); // camera component
    install_component_base(reg, L, compID);

    push_transform_table(reg, L, compID, &cameraC->transform);
    L.set_field(-2, "transform");

    // TODO:
}

static void push_mesh_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    MeshComponent* meshC = (MeshComponent*)comp;

    L.push_table(); // mesh component
    install_component_base(reg, L, compID);

    push_transform_table(reg, L, compID, &meshC->transform);
    L.set_field(-2, "transform");
}

void push_sprite2d_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    Sprite2DComponent* spriteC = (Sprite2DComponent*)comp;

    L.push_table(); // Sprite2D component
    install_component_base(reg, L, compID);

    push_transform2d_table(reg, L, compID, &spriteC->transform);
    L.set_field(-2, "transform");
}

static void install_component_base(DataRegistry reg, LuaState& L, CUID compID)
{
    int oldSize = L.size();

    // TODO: use metatable instead
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE);

    L.push_light_userdata(reg.unwrap());
    L.set_field(-2, "_reg");

    L.push_number((double)compID);
    L.set_field(-2, "_cuid");

    L.push_fn(&component_get_id);
    L.set_field(-2, "get_id");

    L.push_fn(&component_get_name);
    L.set_field(-2, "get_name");

    L.push_fn(&component_set_name);
    L.set_field(-2, "set_name");

    LD_ASSERT(L.size() == oldSize);
}

// clang-format off
struct
{
    ComponentType type;
    void (*push_table)(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
} sComponents[] = {
    {COMPONENT_TYPE_DATA,         nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE, &push_audio_source_component_table},
    {COMPONENT_TYPE_TRANSFORM,    nullptr},
    {COMPONENT_TYPE_CAMERA,       nullptr},
    {COMPONENT_TYPE_MESH,         &push_mesh_component_table},
    {COMPONENT_TYPE_SPRITE_2D,    &push_sprite2d_component_table},
};
// clang-format on

static_assert(sizeof(sComponents) / sizeof(*sComponents) == COMPONENT_TYPE_ENUM_COUNT);

/// @brief ludens.application.exit
static int application_exit(lua_State* l)
{
    Application app = Application::get();

    app.exit();
    return 0;
}

/// @brief ludens.debug.log
static int debug_log(lua_State* l)
{
    LuaState L(l);

    int nargs = L.size();

    // call string.format
    L.get_global("string");
    L.get_field(-1, "format");
    L.remove(-2);
    L.insert(1);

    L.pcall(nargs, 1, 0);
    sLog.debug("{}", L.to_string(-1));

    return 0;
}

/// @brief ludens.input.get_key_down
static int input_get_key_down(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
    {
        L.push_bool(false);
        return 1;
    }

    KeyCode key = string_to_keycode(L.to_string(-1));
    L.push_bool(Input::get_key_down(key));

    return 1;
}

/// @brief ludens.input.get_key_up
static int input_get_key_up(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
    {
        L.push_bool(false);
        return 1;
    }

    KeyCode key = string_to_keycode(L.to_string(-1));
    L.push_bool(Input::get_key_up(key));

    return 1;
}

/// @brief ludens.input.get_key
static int input_get_key(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
    {
        L.push_bool(false);
        return 1;
    }

    KeyCode key = string_to_keycode(L.to_string(-1));
    L.push_bool(Input::get_key(key));

    return 1;
}

/// @brief ludens.input.get_mouse_down
static int input_get_mouse_down(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
    {
        L.push_bool(false);
        return 1;
    }

    MouseButton btn = string_to_mouse_button(L.to_string(-1));
    L.push_bool(Input::get_mouse_down(btn));

    return 1;
}

/// @brief ludens.input.get_mouse_up
static int input_get_mouse_up(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
    {
        L.push_bool(false);
        return 1;
    }

    MouseButton btn = string_to_mouse_button(L.to_string(-1));
    L.push_bool(Input::get_mouse_up(btn));

    return 1;
}

/// @brief ludens.input.get_mouse
static int input_get_mouse(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_STRING)
    {
        L.push_bool(false);
        return 1;
    }

    MouseButton btn = string_to_mouse_button(L.to_string(-1));
    L.push_bool(Input::get_mouse(btn));

    return 1;
}

static int audio_source_component_play(lua_State* l)
{
    LD_PROFILE_SCOPE;

    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_TABLE)
        return 0;

    L.get_field(-1, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    CUID compID = (CUID)L.to_number(-1);

    L.get_field(-2, "_scene");
    Scene scene((SceneObj*)L.to_userdata(-1));
    Scene::IAudioSource source(scene, compID);
    source.play();

    return 0;
}

static int audio_source_component_pause(lua_State* l)
{
    LD_PROFILE_SCOPE;

    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_TABLE)
        return 0;

    L.get_field(-1, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    CUID compID = (CUID)L.to_number(-1);

    L.get_field(-2, "_scene");
    Scene scene((SceneObj*)L.to_userdata(-1));
    Scene::IAudioSource source(scene, compID);
    source.pause();

    return 0;
}

static int audio_source_component_resume(lua_State* l)
{
    LD_PROFILE_SCOPE;

    LuaState L(l);

    if (L.get_type(-1) != LUA_TYPE_TABLE)
        return 0;

    L.get_field(-1, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    CUID compID = (CUID)L.to_number(-1);

    L.get_field(-2, "_scene");
    Scene scene((SceneObj*)L.to_userdata(-1));
    Scene::IAudioSource source(scene, compID);
    source.resume();

    return 0;
}

//
// PUBLIC API
//

const char* get_log_channel_name()
{
    return LUDENS_LUA_SCRIPT_LOG_CHANNEL;
}

LuaModule create_ludens_module()
{

    // clang-format off
    const LuaModuleValue applicationVals[] = {
        {.type = LUA_TYPE_FN, .name = "exit", .fn = &LuaScript::application_exit},
    };

    const LuaModuleValue debugVals[] = {
        {.type = LUA_TYPE_FN, .name = "log", .fn = &LuaScript::debug_log},
    };

    const LuaModuleValue inputVals[] = {
        {.type = LUA_TYPE_FN, .name = "get_key_down",   .fn = &LuaScript::input_get_key_down},
        {.type = LUA_TYPE_FN, .name = "get_key_up",     .fn = &LuaScript::input_get_key_up},
        {.type = LUA_TYPE_FN, .name = "get_key",        .fn = &LuaScript::input_get_key},
        {.type = LUA_TYPE_FN, .name = "get_mouse_down", .fn = &LuaScript::input_get_mouse_down},
        {.type = LUA_TYPE_FN, .name = "get_mouse_up",   .fn = &LuaScript::input_get_mouse_up},
        {.type = LUA_TYPE_FN, .name = "get_mouse",      .fn = &LuaScript::input_get_mouse},
    };
    // clang-format on

    std::array<LuaModuleNamespace, 3> spaces;
    spaces[0].name = "application";
    spaces[0].valueCount = sizeof(applicationVals) / sizeof(*applicationVals);
    spaces[0].values = applicationVals;

    spaces[1].name = "debug";
    spaces[1].valueCount = sizeof(debugVals) / sizeof(*debugVals);
    spaces[1].values = debugVals;

    spaces[2].name = "input";
    spaces[2].valueCount = sizeof(inputVals) / sizeof(*inputVals);
    spaces[2].values = inputVals;

    LuaModuleInfo modI;
    modI.name = LUDENS_LUA_MODULE_NAME;
    modI.spaceCount = (uint32_t)spaces.size();
    modI.spaces = spaces.data();

    return LuaModule::create(modI); // caller destroys
}

// stack top should be ludens.scripts
void create_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, ComponentType type, void* comp)
{
    L.push_number((double)compID);
    L.get_table(-2);                             // ludens.scripts[compID]
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE); // script instance table missing

    sComponents[(int)type].push_table(scene, reg, L, compID, comp);
    L.set_field(-2, "_comp");
    L.pop(1);
}

// stack top should be ludens.scripts
void destroy_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID)
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