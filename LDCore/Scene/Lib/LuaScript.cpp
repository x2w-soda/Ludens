#include <Ludens/Application/Application.h>
#include <Ludens/Application/Input.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>

#include <array>
#include <cstring>
#include <format>
#include <string>

#include "LuaScript.h"
#include "LuaScriptFFI.h"

#define LUDENS_LUA_SCRIPT_LOG_CHANNEL "LuaScript"
#define LUDENS_LUA_MODULE_NAME "ludens"

namespace LD {

static Log sLog(LUDENS_LUA_SCRIPT_LOG_CHANNEL);

namespace LuaScript {

static KeyCode string_to_keycode(const char* str);
static MouseButton string_to_mouse_button(const char* cstr);
static inline ComponentBase* get_component_base(LuaState& L, DataRegistry* outReg);
static inline bool push_script_table(LuaState& L, CUID compID);
static inline void push_component_table(LuaState& L, const char* ffiCast, void* comp);
static int component_get_id(lua_State* l);
static int component_get_name(lua_State* l);
static int component_set_name(lua_State* l);
static void push_audio_source_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void push_camera_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void push_mesh_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
static void push_sprite2d_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp);
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

static inline void push_component_table(LuaState& L, const char* ffiCast, void* comp)
{
    L.push_light_userdata(comp);
    L.set_global("tmp"); // TODO: this ugly

    std::string str = std::format("local ffi = require 'ffi' return ffi.cast(\"{}\", _G.tmp)", ffiCast);
    bool ok = L.do_string(str.c_str());
    LD_ASSERT(ok);

    LuaType type = L.get_type(-1);
    LD_ASSERT(type == LUA_TYPE_CDATA);
}

static inline void get_transform_cuid(LuaState L, int tIndex, CUID& compID)
{
    L.get_field(tIndex, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    compID = (CUID)L.to_number(-1);
    L.pop(1);
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

static void push_audio_source_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    AudioSourceComponent* sourceC = (AudioSourceComponent*)comp;

    L.push_table(); // // TODO: FFI

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

    L.push_table(); // TODO: FFI
}

static void push_mesh_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    MeshComponent* meshC = (MeshComponent*)comp;

    push_component_table(L, "MeshComponent*", comp);
}

void push_sprite2d_component_table(Scene scene, DataRegistry reg, LuaState L, CUID compID, void* comp)
{
    Sprite2DComponent* spriteC = (Sprite2DComponent*)comp;

    L.push_table(); // TODO: FFI
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

void Context::startup(Scene scene, DataRegistry registry, AssetManager assetManager)
{
    LD_PROFILE_SCOPE;

    mScene = scene;
    mRegistry = registry;
    mAssetManager = assetManager;

    LuaStateInfo stateI{};
    stateI.openLibs = true;
    mL = LuaState::create(stateI);

    LuaModule ludensLuaModule = LuaScript::create_ludens_module();
    ludensLuaModule.load(mL);
    LuaModule::destroy(ludensLuaModule);

    if (!mL.do_string("_G.ludens = require 'ludens'"))
    {
        sLog.error("module initialization failed: {}", mL.to_string(-1));
        LD_UNREACHABLE;
    }

    std::string cdef = std::format("local ffi = require 'ffi' ffi.cdef [[ {} ]]", LuaScript::get_ffi_cdef());
    if (!mL.do_string(cdef.c_str()))
    {
        sLog.error("FFI cdef initialization failed: {}", mL.to_string(-1));
        LD_UNREACHABLE;
    }

    if (!mL.do_string(LuaScript::get_ffi_mt()))
    {
        sLog.error("FFI metatable initialization failed: {}", mL.to_string(-1));
        LD_UNREACHABLE;
    }

    mL.get_global("ludens");
    mL.push_table();
    mL.set_field(-2, "scripts");
    mL.clear();
}

void Context::cleanup()
{
    LD_PROFILE_SCOPE;

    LuaState::destroy(mL);
    mL = {};
    mAssetManager = {};
    mRegistry = {};
    mScene = {};
}

void Context::set_registry(DataRegistry registry)
{
    mRegistry = registry;
}

void Context::update(float delta)
{
    LD_PROFILE_SCOPE;

    int oldSize1 = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // TODO: iterate scripts in Lua to avoid pcall overhead.

    for (auto ite = mRegistry.get_component_scripts(); ite; ++ite)
    {
        auto* script = (ComponentScriptSlot*)ite.data();
        if (!script->isEnabled)
            continue;

        CUID componentID = script->componentID;

        int oldSize2 = mL.size();
        mL.push_number((double)componentID);
        mL.get_table(-2);

        mL.get_field(-1, "update");
        LD_ASSERT(mL.get_type(-1) == LUA_TYPE_FN);

        // arg1 is the script instance (lua table)
        mL.push_number((double)componentID);
        mL.get_table(-4);

        // arg2 is the component (lua table) the script is attached to
        mL.get_field(-1, "_comp");
        LD_ASSERT(mL.get_type(-1) == LUA_TYPE_CDATA);

        // arg3 is the frame delta time
        mL.push_number((double)delta);

        // Script:update(comp, delta)
        {
            LD_PROFILE_SCOPE_NAME("LuaScript pcall");

            LuaError err = mL.pcall(3, 0, 0);
            LD_ASSERT(err == 0);

            if (err != 0)
            {
                sLog.warn("script update error: {}", mL.to_string(-1));
            }
        }

        // NOTE: This is a pessimistic assumption that all script updates
        //       write to component transform.
        mRegistry.mark_component_transform_dirty(componentID);

        mL.resize(oldSize2);
    }

    mL.resize(oldSize1);
}

bool Context::create_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return true; // not an error

    int oldSize = mL.size();
    CUID compID = scriptSlot->componentID;
    AUID assetID = scriptSlot->assetID;

    mL.get_global("ludens");
    mL.get_field(-1, "scripts");
    mL.push_number((double)compID);

    LuaScriptAsset asset = (LuaScriptAsset)mAssetManager.get_asset(assetID, ASSET_TYPE_LUA_SCRIPT);
    LD_ASSERT(asset);

    const char* luaSource = asset.get_source();

    // this should push the script instance table onto stack
    bool isScriptValid = mL.do_string(luaSource);
    if (!isScriptValid)
    {
        printf("Error: %s\n", mL.to_string(-1)); // TODO: error control flow
        mL.resize(oldSize);
        return false;
    }

    mL.set_table(-3); // store script instance as ludens.scripts[compID]

    ComponentType type;
    void* comp = mRegistry.get_component(compID, &type);

    // create and store table for component type
    LuaScript::create_component_table(mScene, mRegistry, mL, compID, type, comp);

    mL.resize(oldSize);
    return true;
}

void Context::destroy_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    CUID compID = scriptSlot->componentID;
    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // destroy component lua table representation
    LuaScript::destroy_component_table(mScene, mRegistry, mL, compID);

    mL.push_number((double)compID);
    mL.push_nil();
    mL.set_table(-3);

    mL.resize(oldSize);
}

void Context::attach_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    LuaType type;
    CUID compID = scriptSlot->componentID;
    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // call 'attach' lua method on script
    mL.push_number((double)compID);
    mL.get_table(-2);
    LD_ASSERT(mL.get_type(-1) == LUA_TYPE_TABLE); // script instance

    mL.get_field(-1, "attach");
    LD_ASSERT(mL.get_type(-1) == LUA_TYPE_FN); // script attach method

    // arg1 is script instance
    mL.push_value(-2);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    // arg2 is the component cdata from FFI
    mL.get_field(-3, "_comp");
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_CDATA);

    mL.call(2, 0);

    mL.resize(oldSize);
}

void Context::detach_lua_script(ComponentScriptSlot* scriptSlot)
{
    if (!scriptSlot)
        return;

    LuaType type;
    CUID compID = scriptSlot->componentID;
    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // call 'detach' lua method on script
    mL.push_number((double)compID);
    mL.get_table(-2);
    mL.get_field(-1, "detach");
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_FN); // script detach method

    // arg1 is script instance
    mL.push_value(-2);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    mL.call(1, 0);

    mL.resize(oldSize);
}

} // namespace LuaScript
} // namespace LD