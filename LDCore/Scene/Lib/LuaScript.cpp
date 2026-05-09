#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/ComponentViews.h>
#include <Ludens/WindowRegistry/Input.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

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

static inline ComponentBase* get_component_base(LuaState& L, DataRegistry* outReg);
static void get_or_create_component_ref(LuaState& L, CUID cuid);
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
static int get_asset(lua_State* l);
static int create_child(lua_State* l);
static int destroy(lua_State* l);

static inline ComponentBase* get_component_base(LuaState& L, DataRegistry* outReg)
{
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_TABLE); // stack top should be component table

    int oldSize = L.size();

    L.get_field(-1, "_cuid");
    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);
    CUID compID(reinterpret_cast<uint64_t>(L.to_userdata(-1)));
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

static void get_or_create_component_ref(LuaState& L, CUID cuid)
{
    int oldSize = L.size();

    L.get_global("ludens");
    L.get_field(-1, "get_or_create_component_ref");
    L.push_light_userdata(reinterpret_cast<void*>((uint64_t)cuid));

    LuaError error = L.pcall(1, 1, 0);
    if (error)
    {
        sLog.error("{}", L.to_string(-1));
    }
    LD_ASSERT(error == 0);

    L.remove(-2);

    LD_ASSERT(L.size() == oldSize + 1);
}

/// @brief Component:get_id()
int component_get_id(lua_State* l)
{
    LuaState L(l);

    ComponentBase* base = get_component_base(L, nullptr);

    L.push_light_userdata(reinterpret_cast<void*>((uint64_t)base->cuid));

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

/// @brief ludens.scene.transition
static int scene_transition(lua_State* l)
{
    // TODO: some kind of SceneLoader that takes the Scene index or name.
    auto loadFn = [](SceneObj* obj) -> bool {
        Scene scene(obj);
        ComponentView view = scene.create_component(COMPONENT_TYPE_CAMERA_2D, "dummy", 0);

        String err;
        Camera2DInfo info{};
        info.extent = Vec2(500.0f);
        info.position = Vec2(250.0f);
        info.rotation = 0.0f;
        info.zoom = 1.0f;
        bool ok = ((Camera2DView)view).load(info, Rect(0.0f, 0.0f, 1.0f, 1.0f), err);

        return ok;
    };

    Scene scene(sScene);
    (void)scene.request_transition(loadFn);

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

    KeyCode key;
    L.push_bool(from_string(L.to_string(-1), key) && Input::get_key_down(key));

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

    KeyCode key;
    L.push_bool(from_string(L.to_string(-1), key) && Input::get_key_up(key));

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

    KeyCode key;
    L.push_bool(from_string(L.to_string(-1), key) && Input::get_key(key));

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

    MouseButton btn;
    L.push_bool(from_string(L.to_string(-1), btn) && Input::get_mouse_down(btn));

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

    MouseButton btn;
    L.push_bool(from_string(L.to_string(-1), btn) && Input::get_mouse_up(btn));

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

    MouseButton btn;
    L.push_bool(from_string(L.to_string(-1), btn) && Input::get_mouse(btn));

    return 1;
}

/// @brief ludens.C.get_component(cuid)
/// @note 64-bit cuid marshalled as light userdata.
static int get_component(lua_State* l)
{
    LuaState L(l);

    LD_ASSERT(L.get_type(-1) == LUA_TYPE_LIGHTUSERDATA);
    CUID compID(reinterpret_cast<uint64_t>(L.to_userdata(-1)));

    ComponentType type;
    void* comp = sScene->active->registry.get_component_data(compID, &type);

    if (!comp)
    {
        L.push_nil();
        L.push_nil();
        return 2;
    }

    View ffiTypeV = get_component_type_name(type);
    String ffiType((const char*)ffiTypeV.data, ffiTypeV.size);
    ffiType.push_back('*');

    L.push_string(ffiType.c_str());
    L.push_light_userdata(comp);
    return 2;
}

/// @brief ludens.C.get_asset(suid)
/// @note 32-bit suid fits into Lua number.
static int get_asset(lua_State* l)
{
    LuaState L(l);

    LD_ASSERT(L.size() == 1 && L.get_type(-1) == LUA_TYPE_NUMBER);
    SUID suid((uint32_t)L.to_number(-1));

    if (suid.type() != SERIAL_TYPE_ASSET)
    {
        L.push_nil();
        return 1;
    }

    Asset asset = AssetManager::get().get_asset(suid);
    if (!asset)
    {
        L.push_nil();
        return 1;
    }

    // AssetObj* are from PoolAllocators, so they have stable address
    // that is not returned to the OS even upon destruction.
    L.push_light_userdata(asset.unwrap());
    return 1;
}

/// @brief ludens.C.create_child(compID, compType, params)
static int create_child(lua_State* l)
{
    LD_PROFILE_SCOPE;

    LuaState L(l);

    LD_ASSERT(L.size() == 3);
    LD_ASSERT(L.get_type(1) == LUA_TYPE_LIGHTUSERDATA);
    LD_ASSERT(L.get_type(2) == LUA_TYPE_STRING);
    LD_ASSERT(L.get_type(3) == LUA_TYPE_TABLE);

    CUID parentID(reinterpret_cast<uint64_t>(L.to_userdata(1)));
    String compTypeStr(L.to_string(2));

    // TODO: At some point we would use a type table instead of string.
    //       This is kinda lazy.
    ComponentType compType = COMPONENT_TYPE_ENUM_COUNT;
    for (int i = 0; i < (int)COMPONENT_TYPE_ENUM_COUNT; i++)
    {
        View candidate = get_component_brief_type_name((ComponentType)i);

        if (compTypeStr == candidate)
        {
            compType = (ComponentType)i;
            break;
        }
    }

    if (compType == COMPONENT_TYPE_ENUM_COUNT)
    {
        L.push_nil();
        return 1;
    }

    String compName = compTypeStr;

    L.get_field(3, "name");
    if (L.get_type(-1) == LUA_TYPE_STRING)
        compName = String(L.to_string(-1));
    L.pop(1);

    Scene scene(sScene);
    ComponentView childV = scene.create_component(compType, compName.c_str(), parentID);
    if (!childV)
    {
        L.push_nil();
        return 1;
    }

    // TODO: startup from Scene public API?
    String err;
    if (!sScene->active->startup_component(childV.data(), err))
    {
        L.push_nil();
        return 1;
    }

    get_or_create_component_ref(L, childV.cuid());
    return 1;
}

/// @brief ludens.C.destroy(compID)
static int destroy(lua_State* l)
{
    LuaState L(l);

    LD_ASSERT(L.size() == 1);
    LD_ASSERT(L.get_type(1) == LUA_TYPE_LIGHTUSERDATA);

    CUID compID(reinterpret_cast<uint64_t>(L.to_userdata(1)));

    sScene->active->queue_component_destruction(compID);

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

    const LuaModuleValue sceneVals[] = {
        {.type = LUA_TYPE_FN, .name = "transition", .fn = &LuaScript::scene_transition},
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
        {.type = LUA_TYPE_FN, .name = "get_asset",     .fn = &LuaScript::get_asset},
        {.type = LUA_TYPE_FN, .name = "create_child",  .fn = &LuaScript::create_child},
        {.type = LUA_TYPE_FN, .name = "destroy",       .fn = &LuaScript::destroy},
    };
    // clang-format on

    Array<LuaModuleNamespace, 6> spaces;
    spaces[0].name = "application";
    spaces[0].valueCount = sizeof(applicationVals) / sizeof(*applicationVals);
    spaces[0].values = applicationVals;

    spaces[1].name = "scene";
    spaces[1].valueCount = sizeof(sceneVals) / sizeof(*sceneVals);
    spaces[1].values = sceneVals;

    spaces[2].name = "debug";
    spaces[2].valueCount = sizeof(debugVals) / sizeof(*debugVals);
    spaces[2].values = debugVals;

    spaces[3].name = "input";
    spaces[3].valueCount = sizeof(inputVals) / sizeof(*inputVals);
    spaces[3].values = inputVals;

    spaces[4].name = "ui_driver";
    spaces[4].valueCount = sizeof(uiDriverVals) / sizeof(*uiDriverVals);
    spaces[4].values = uiDriverVals;

    // NOTE: these are bindings that use the Lua stack, there are also FFI bindings in LuaScriptFFI.cpp
    spaces[5].name = "C";
    spaces[5].valueCount = sizeof(CVals) / sizeof(*CVals);
    spaces[5].values = CVals;

    LuaModuleInfo modI;
    modI.name = LUDENS_LUA_MODULE_NAME;
    modI.spaceCount = (uint32_t)spaces.size();
    modI.spaces = spaces.data();

    return LuaModule::create(modI); // caller destroys
}

void Context::create(Scene scene)
{
    LD_PROFILE_SCOPE;

    mScene = scene;

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

    String cdef = std::format("local ffi = require 'ffi' ffi.cdef [[ {} ]]", LuaScript::get_ffi_cdef()).c_str();
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
    // - empty ludens.componentRefs table
    // - empty ludnes.assetRefs table
    // - ComponentRef mechanism

    if (!mL.do_string(R"(
local ffi = require 'ffi'
_G.ludens.scripts = {}
_G.ludens.componentRefs = {}
_G.ludens.assetRefs = {}

_G.ludens.ComponentRef = {
    is_valid = function (compRef)
        -- TODO
        return ffi.cast('void*', compRef.cuid) ~= nil
    end,
    get_child = function (compRef, childName)
        local cuid = ffi.C.ffi_get_child_id_by_name(compRef.cuid, childName)
        return _G.ludens.create_component_ref(cuid)
    end,
    get_parent = function (compRef)
        local cuid = ffi.C.ffi_get_parent_id(compRef.cuid)
        return _G.ludens.create_component_ref(cuid)
    end,
    create_child = function (compRef, compType, params)
        return _G.ludens.C.create_child(compRef.cuid, compType, params)
    end,
    destroy = function (compRef)
        _G.ludens.C.destroy(compRef.cuid)
    end,
    __index = function (compRef, k)
        local method = _G.ludens.ComponentRef[k]
        if method ~= nil then
            return method
        end

        -- TODO: refactor when we have 3D components, this assumes 2D components.
        if k == 'transform' then
            local proxy = rawget(compRef, '__transform_proxy')
            if not proxy then
                proxy = _G.ludens.create_transform_2d_proxy(compRef.cdata.__private_transform)
                rawset(compRef, '__transform_proxy', proxy)
            end
            return proxy
        end

        return compRef.cdata[k]
    end,
    __newindex = function (compRef, k, v)
        if k == 'transform' then -- deep copy
            local proxy = rawget(compRef, '__transform_proxy')
            if not proxy then
                proxy = _G.ludens.create_transform_2d_proxy(compRef.cdata.__private_transform)
                rawset(compRef, '__transform_proxy', proxy)
            end
            local dst = proxy.__ptr
            local src = v.__ptr
            ffi.copy(dst, src, ffi.sizeof('Transform2D'))
            return
        end

        compRef.cdata[k] = v
    end,
}

_G.ludens.get_or_create_component_ref = function (cuid)
    if ffi.cast('void*', cuid) == nil then
        return nil
    end

    local compRef = _G.ludens.componentRefs[cuid]
    if compRef and compRef:is_valid() then
        return compRef        
    end

    -- roundtrip to C++
    local ffiType, compAddr = _G.ludens.C.get_component(cuid);
    compRef = {}
    compRef.cuid = cuid
    compRef.cdata = ffi.cast(ffiType, compAddr)
    setmetatable(compRef, _G.ludens.ComponentRef)
    _G.ludens.componentRefs[cuid] = compRef
    return compRef
end

_G.ludens.AssetRef = {
    is_valid = function (assetRef)
        -- TODO
        return assetRef.suid ~= 0 and assetRef.suid == assetRef.cdata.id
    end,
    __index = function (assetRef, k)
        local method = _G.ludens.AssetRef[k]
        if method then
            return method
        end

        return nil
    end,
}

_G.ludens.get_or_create_asset_ref = function (suid)
    if suid == 0 then
        return nil
    end

    local assetRef = _G.ludens.assetRefs[suid]
    if assetRef and assetRef:is_valid() then
        return assetRef
    end

    -- roundtrip to C++
    local assetAddr = _G.ludens.C.get_asset(suid)
    assetRef = {}
    assetRef.suid = suid
    assetRef.cdata = ffi.cast('AssetObj*', assetAddr)
    setmetatable(assetRef, _G.ludens.AssetRef)
    _G.ludens.assetRefs[suid] = assetRef
    return assetRef
end

-- Proxy for Transform2D* to prevent shallow copy
_G.ludens.Transform2DProxy = {
    __index = function (self, k)
        local ptr = rawget(self, "__ptr")
        return ptr[0][k]
    end,
    __newindex = function (self, k, v)
        local ptr = rawget(self, "__ptr")
        ptr[0][k] = v
    end,
}

_G.ludens.create_transform_2d_proxy = function (transform2DPtr)
    return setmetatable({ __ptr = transform2DPtr }, _G.ludens.Transform2DProxy)
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
    mScene = {};
}

bool Context::update(float delta, String& err)
{
    LD_PROFILE_SCOPE;

    int oldSize = mL.size();

    mL.get_global("ludens");
    mL.push_number((double)delta);
    mL.set_field(-2, "delta");

    bool success = mL.do_string(R"(local ffi = require 'ffi'
for cuid, script in pairs(_G.ludens.scripts) do
    script:update(_G.ludens.delta)
end
)");

    if (!success)
        err = mL.to_string(-1);

    mL.resize(oldSize);

    return success;
}

void Context::create_component_table(CUID compID)
{
    if (!compID)
        return;

    int oldSize = mL.size();

    get_or_create_component_ref(mL, compID);
    mL.pop(1);

    LD_ASSERT(mL.size() == oldSize);
}

void Context::destroy_component_table(CUID compID)
{
    if (!compID)
        return;

    int oldSize = mL.size();

    mL.get_global("ludens");
    mL.get_field(-1, "componentRefs");
    mL.push_light_userdata(reinterpret_cast<void*>((uint64_t)compID));
    mL.push_nil();
    mL.set_table(-3); // ludens.componentRefs[compID] = nil

    // TODO: solve dangling references

    mL.resize(oldSize);
}

bool Context::create_lua_script(CUID compID, AssetID scriptAssetID, String& err)
{
    if (!compID || !scriptAssetID)
        return true; // not an error

    int oldSize = mL.size();

    mL.get_global("ludens");
    mL.get_field(-1, "scripts");
    mL.push_light_userdata(reinterpret_cast<void*>((uint64_t)compID));

    LuaScriptAsset asset = (LuaScriptAsset)AssetManager::get().get_asset(scriptAssetID, ASSET_TYPE_LUA_SCRIPT);
    LD_ASSERT(asset);

    View luaSourceV = asset.get_source();
    String luaSource((char*)luaSourceV.data, luaSourceV.size);

    // this should push the script instance table onto stack
    if (!mL.do_string(luaSource.c_str()))
    {
        err = mL.to_string(-1);
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
    mL.push_light_userdata(reinterpret_cast<void*>((uint64_t)compID));
    mL.push_nil();
    mL.set_table(-3); // ludens.scripts[compID] = nil

    mL.resize(oldSize);
}

bool Context::attach_lua_script(CUID compID, String& err)
{
    LD_ASSERT(compID);

    LuaType type;
    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // call 'attach' lua method on script
    mL.push_light_userdata(reinterpret_cast<void*>((uint64_t)compID));
    mL.get_table(-2);
    LD_ASSERT(mL.get_type(-1) == LUA_TYPE_TABLE); // script instance

    mL.get_field(-1, "attach");
    LD_ASSERT(mL.get_type(-1) == LUA_TYPE_FN); // script attach method

    // arg1 is script instance
    mL.push_value(-2);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    // arg2 is the component
    get_or_create_component_ref(mL, compID);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    LuaError luaError = mL.pcall(2, 0, 0);
    if (luaError != 0)
    {
        err = mL.to_string(-1);
        return false;
    }

    mL.resize(oldSize);
    return true;
}

bool Context::detach_lua_script(CUID compID, String& err)
{
    LD_ASSERT(compID);

    LuaType type;
    int oldSize = mL.size();
    mL.get_global("ludens");
    mL.get_field(-1, "scripts");

    // call 'detach' lua method on script
    mL.push_light_userdata(reinterpret_cast<void*>((uint64_t)compID));
    mL.get_table(-2);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE); // script instance

    mL.get_field(-1, "detach");
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_FN); // script detach method

    // arg1 is script instance
    mL.push_value(-2);
    LD_ASSERT((type = mL.get_type(-1)) == LUA_TYPE_TABLE);

    LuaError luaError = mL.pcall(1, 0, 0);
    if (luaError != 0)
    {
        err = mL.to_string(-1);
        return false;
    }

    mL.resize(oldSize);
    return true;
}

} // namespace LuaScript
} // namespace LD