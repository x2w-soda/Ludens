#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/WindowRegistry/Input.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include <array>
#include <cstring>
#include <format>
#include <string>

#include "LuaScript.h"
#include "LuaScriptFFI.h"
#include "SceneObj.h"
#include "UIDriver.h"

#define LUDENS_LUA_SCRIPT_LOG_CHANNEL "LuaScript"
#define LUDENS_LUA_MODULE_NAME "ludens"

namespace LD {

static Log sLog(LUDENS_LUA_SCRIPT_LOG_CHANNEL);

namespace LuaScript {

static KeyCode string_to_keycode(const char* str);
static MouseButton string_to_mouse_button(const char* cstr);
static inline ComponentBase* get_component_base(LuaState& L, DataRegistry* outReg);
static inline void push_component_ref(LuaState& L, CUID compID);
static int component_get_id(lua_State* l);
static int component_get_name(lua_State* l);
static int component_set_name(lua_State* l);
static int application_exit(lua_State* l);
static int debug_log(lua_State* l);
static int input_get_key_down(lua_State* l);
static int input_get_key_up(lua_State* l);
static int input_get_key(lua_State* l);
static int input_get_mouse_down(lua_State* l);
static int input_get_mouse_up(lua_State* l);
static int input_get_mouse(lua_State* l);
static int get_component(lua_State* l);

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

static void push_component_ref(LuaState& L, CUID compID)
{
    std::string str = std::format("return _G.ludens.create_component_ref({})", compID);
    bool ok = L.do_string(str.c_str());
    if (!ok)
    {
        sLog.error("{}", L.to_string(-1));
    }
    LD_ASSERT(ok);
}

/// @brief Component:get_id()
int component_get_id(lua_State* l)
{
    LuaState L(l);

    ComponentBase* base = get_component_base(L, nullptr);
    L.push_number((double)base->cuid);

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

/// @brief ludens.application.exit
static int application_exit(lua_State* l)
{
    WindowRegistry reg = WindowRegistry::get();
    reg.close_window(reg.get_root_id());

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

/// @brief ludens.C.get_component(compID)
static int get_component(lua_State* l)
{
    LuaState L(l);

    LD_ASSERT(L.get_type(-1) == LUA_TYPE_NUMBER);
    CUID compID = (CUID)L.to_number(-1);

    ComponentType type;
    void* comp = sScene->registry.get_component_data(compID, &type);

    if (!comp)
    {
        L.push_nil();
        L.push_nil();
        return 2;
    }

    std::string ffiType(get_component_type_name(type));
    ffiType.push_back('*');

    L.push_string(ffiType.c_str());
    L.push_light_userdata(comp);
    return 2;
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

    const LuaModuleValue uiDriverVals[] = {
        {.type = LUA_TYPE_FN, .name = "install_callback", .fn = &UIDriver::install_callback},
    };

    const LuaModuleValue CVals[] = {
        {.type = LUA_TYPE_FN, .name = "get_component", .fn = &LuaScript::get_component},
    };
    // clang-format on

    std::array<LuaModuleNamespace, 5> spaces;
    spaces[0].name = "application";
    spaces[0].valueCount = sizeof(applicationVals) / sizeof(*applicationVals);
    spaces[0].values = applicationVals;

    spaces[1].name = "debug";
    spaces[1].valueCount = sizeof(debugVals) / sizeof(*debugVals);
    spaces[1].values = debugVals;

    spaces[2].name = "input";
    spaces[2].valueCount = sizeof(inputVals) / sizeof(*inputVals);
    spaces[2].values = inputVals;

    spaces[3].name = "ui_driver";
    spaces[3].valueCount = sizeof(uiDriverVals) / sizeof(*uiDriverVals);
    spaces[3].values = uiDriverVals;

    // NOTE: these are bindings that use the Lua stack, there are also FFI bindings in LuaScriptFFI.cpp
    spaces[4].name = "C";
    spaces[4].valueCount = sizeof(CVals) / sizeof(CVals);
    spaces[4].values = CVals;

    LuaModuleInfo modI;
    modI.name = LUDENS_LUA_MODULE_NAME;
    modI.spaceCount = (uint32_t)spaces.size();
    modI.spaces = spaces.data();

    return LuaModule::create(modI); // caller destroys
}

void Context::create(Scene scene, AssetManager assetManager)
{
    LD_PROFILE_SCOPE;

    mScene = scene;
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

    // Register FFI declarations
    // - Components are accessed via FFI cdata to avoid state duplication
    // - some functions are visible to FFI to call directly

    std::string cdef = std::format("local ffi = require 'ffi' ffi.cdef [[ {} ]]", LuaScript::get_ffi_cdef());
    if (!mL.do_string(cdef.c_str()))
    {
        sLog.error("FFI cdef initialization failed: {}", mL.to_string(-1));
        LD_UNREACHABLE;
    }

    // Register FFI metatype
    // - ffi.metatype for Component cdata structs

    if (!mL.do_string(LuaScript::get_ffi_mt()))
    {
        sLog.error("FFI metatable initialization failed: {}", mL.to_string(-1));
        LD_UNREACHABLE;
    }

    // Bootstrapping for LuaScript runtime
    // - empty ludens.scripts table
    // - empty ludens.components table
    // - ComponentRef mechanism

    if (!mL.do_string(R"(
local ffi = require 'ffi'
_G.ludens.scripts = {}
_G.ludens.components = {}

_G.ludens.ComponentRef = {
    get_child = function (compRef, childName)
        local compID = ffi.C.ffi_get_child_id_by_name(compRef.compID, childName)
        return _G.ludens.create_component_ref(compID)
    end,
    get_parent = function (compRef)
        local compID = ffi.C.ffi_get_parent_id(compRef.compID)
        return _G.ludens.create_component_ref(compID)
    end,
    __index = function (compRef, k)
        local method = _G.ludens.ComponentRef[k]
        if method ~= nil then
            return method
        end

        return compRef.cdata[k]
    end,
    __newindex = function (compRef, k, v)
        compRef.cdata[k] = v
    end,
}

_G.ludens.create_component_ref = function (compID)
    compID = tonumber(compID) -- TODO: handle uint64_t compID, this is a LuaJIT cdata, not native number

    if compID == 0 then
        return nil
    end

    local comp = _G.ludens.components[compID]

    if comp == nil then -- roundtrip to C++, find component address and FFI type
        local ffiType, compAddr = _G.ludens.C.get_component(compID);
        comp = {}
        comp.ffiType = ffiType
        comp.compAddr = compAddr
        _G.ludens.components[compID] = comp
    end

    local compRef = {}
    compRef.compID = compID
    compRef.cdata = ffi.cast(comp.ffiType, comp.compAddr)
    setmetatable(compRef, _G.ludens.ComponentRef)
    return compRef
end
)"))
    {
        sLog.error("Bootstrapping failed: {}", mL.to_string(-1));
        LD_UNREACHABLE;
    }

    mL.clear();
}

void Context::destroy()
{
    LD_PROFILE_SCOPE;

    LuaState::destroy(mL);
    mL = {};
    mAssetManager = {};
    mScene = {};
}

void Context::update(float delta)
{
    LD_PROFILE_SCOPE;

    int oldSize = mL.size();

    mL.get_global("ludens");
    mL.push_number((double)delta);
    mL.set_field(-2, "delta");

    bool ok = mL.do_string(R"(local ffi = require 'ffi'
for compID, script in pairs(_G.ludens.scripts) do
    script:update(_G.ludens.delta)

    -- TODO: This is not enough as soon as a Script is able to modify transforms of arbitrary components.
    ffi.C.ffi_mark_transform_dirty(compID)
end
)");

    if (!ok)
    {
        std::string err(mL.to_string(-1));
        sLog.error("script update failed: {}", err);
        LD_DEBUG_BREAK;
    }

    mL.resize(oldSize);
}

void Context::create_component_table(CUID compID)
{
    if (!compID)
        return;

    int oldSize = mL.size();

    push_component_ref(mL, compID);
    mL.pop(1);

    LD_ASSERT(mL.size() == oldSize);
}

void Context::destroy_component_table(CUID compID)
{
    if (!compID)
        return;

    int oldSize = mL.size();

    mL.get_global("ludens");
    mL.get_field(-1, "components");
    mL.push_number((double)compID);
    mL.push_nil();
    mL.set_table(-3); // ludens.components[compID] = nil

    // TODO: solve dangling references

    mL.resize(oldSize);
}

bool Context::create_lua_script(CUID compID, AssetID scriptAssetID)
{
    if (!compID || !scriptAssetID)
        return true; // not an error

    int oldSize = mL.size();

    mL.get_global("ludens");
    mL.get_field(-1, "scripts");
    mL.push_number((double)compID);

    LuaScriptAsset asset = (LuaScriptAsset)mAssetManager.get_asset(scriptAssetID, ASSET_TYPE_LUA_SCRIPT);
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

    mL.resize(oldSize);
    return true;
}

void Context::destroy_lua_script(CUID compID)
{
    if (!compID)
        return;

    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");
    mL.push_number((double)compID);
    mL.push_nil();
    mL.set_table(-3); // ludens.scripts[compID] = nil

    mL.resize(oldSize);
}

void Context::attach_lua_script(CUID compID)
{
    if (!compID)
        return;

    LuaType type;
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

    // arg2 is the component
    push_component_ref(mL, compID);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    LuaError err = mL.pcall(2, 0, 0);
    if (err != 0)
    {
        sLog.error("script attach failed: {}", mL.to_string(-1));
    }
    LD_ASSERT(err == 0);

    mL.resize(oldSize);
}

void Context::detach_lua_script(CUID compID)
{
    if (!compID)
        return;

    LuaType type;
    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // call 'detach' lua method on script
    mL.push_number((double)compID);
    mL.get_table(-2);
    if ((type = mL.get_type(-1)) == LUA_TYPE_NIL)
    {
        mL.resize(oldSize);
        return;
    }

    mL.get_field(-1, "detach");
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_FN); // script detach method

    // arg1 is script instance
    mL.push_value(-2);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    LuaError err = mL.pcall(1, 0, 0);
    if (err != 0)
    {
        sLog.error("script detach failed: {}", mL.to_string(-1));
    }
    LD_ASSERT(err == 0);

    mL.resize(oldSize);
}

} // namespace LuaScript
} // namespace LD