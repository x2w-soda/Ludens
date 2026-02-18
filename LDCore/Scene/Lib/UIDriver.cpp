#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>

#include "UIDriver.h"

namespace LD {

static int ui_driver_get_widget(lua_State* l);
static void push_widget_proxy(LuaState L, UIDriver* driver, const char* pathCstr);

/// @brief ui:get_widget('path/to/widget')
static int ui_driver_get_widget(lua_State* l)
{
    LD_PROFILE_SCOPE;

    LuaState L(l);
    LD_ASSERT(L.size() == 2);

    L.get_field(-2, "__user");
    UIDriver* driver = (UIDriver*)L.to_userdata(-1);
    LD_ASSERT(driver);
    L.pop(1);

    const char* pathStr = L.to_string(-1);
    push_widget_proxy(L, driver, pathStr);
    return 1;
}

static void push_widget_proxy(LuaState L, UIDriver* driver, const char* pathCstr)
{
    if (!pathCstr)
    {
        L.push_nil();
        return;
    }

    FS::Path widgetPath(pathCstr);
    UIWidget widget = (UIWidget)driver->get_window();

    for (const auto& path : widgetPath)
    {
        UIWidget child = widget.get_child_by_name(path.string());

        if (!child)
        {
            L.push_nil();
            return;
        }

        widget = child;
    }

    widget.set_user(driver);

    // the widget proxy table is responsible for caching the Lua functions
    // so that the UIDriver may invoke the Lua callback later.
    bool ok = L.do_string(R"(
local proxyMeta = {
    __newindex = function (proxy, k, v)
        if type(k) == 'string' and type(v) == 'function' then
            _G.ludens.ui_driver.install_callback(proxy.__widget, k, v)
            return
        end
        rawset(proxy, k, v)
    end,
}
local proxy = {}

setmetatable(proxy, proxyMeta)
return proxy
)");
    LD_ASSERT(ok && L.get_type(-1) == LUA_TYPE_TABLE);

    // NOTE: This is only possible since UIWidgetObj address is stable.
    //       Will have to refactor once we add widget create/destroy API in Lua
    //       to solve dangling references on Lua side.
    L.push_light_userdata(widget.unwrap());
    L.set_field(-2, "__widget");
}

bool UIDriver::connect(UIWindow window, LuaState luaState, const char* luaSource, std::string& err)
{
    LD_PROFILE_SCOPE;

    mL = luaState;
    mWindow = window;

    bool ok = mL.do_string(luaSource);
    if (!ok)
    {
        err = mL.to_string(-1);
        return false;
    }

    if (!mL.get_type(-1) == LUA_TYPE_TABLE)
    {
        err = "UIDriver expected a lua script table";
        return false;
    }

    // cache the UI lua script table in registry.
    int luaReg = mL.get_registry_index();
    mScriptRef = mL.ref(luaReg);

    return true;
}

bool UIDriver::disconnect()
{
    LD_PROFILE_SCOPE;

    // release the UI lua script table for GC.
    int luaReg = mL.get_registry_index();
    mL.unref(luaReg, mScriptRef);
    mL = {};

    return true;
}

bool UIDriver::attach(std::string& err)
{
    LD_PROFILE_SCOPE;

    err.clear();
    int oldSize = mL.size();
    int luaReg = mL.get_registry_index();
    mL.push_integer(mScriptRef);
    mL.get_table(luaReg);

    if (mL.get_type(-1) != LUA_TYPE_TABLE)
    {
        err = "UIDriver::attach missing script table";
        mL.resize(oldSize);
        return false;
    }

    mL.get_field(-1, "attach");
    if (mL.get_type(-1) != LUA_TYPE_FN)
    {
        err = "UIDriver::attach missing script:attach function";
        mL.resize(oldSize);
        return false;
    }

    mL.push_integer(mScriptRef);
    mL.get_table(luaReg);

    push_driver_table();

    {
        LD_PROFILE_SCOPE_NAME("pcall");
        LuaError luaError = mL.pcall(2, 0, -1); // script:attach(ui)
        if (luaError)
        {
            err = mL.to_string(-1);
            mL.resize(oldSize);
            return false;
        }
    }

    mL.resize(oldSize);
    return true;
}

bool UIDriver::detach(std::string& err)
{
    LD_PROFILE_SCOPE;

    err.clear();
    int oldSize = mL.size();
    int luaReg = mL.get_registry_index();
    mL.push_integer(mScriptRef);
    mL.get_table(luaReg);

    if (mL.get_type(-1) != LUA_TYPE_TABLE)
    {
        err = "UIDriver::detach missing script table";
        mL.resize(oldSize);
        return false;
    }

    mL.get_field(-1, "detach");
    if (mL.get_type(-1) != LUA_TYPE_FN)
    {
        err = "UIDriver::detach missing script:detach function";
        mL.resize(oldSize);
        return false;
    }

    mL.push_integer(mScriptRef);
    mL.get_table(luaReg);

    LuaError luaError = mL.pcall(1, 0, -1); // script:detach
    if (luaError)
    {
        err = mL.to_string(-1);
        mL.resize(oldSize);
        return false;
    }

    mL.resize(oldSize);
    return true;
}

/// @brief _G.ludens.ui_driver.install_callback(widget, callbackStr, callbackFn)
int UIDriver::install_callback(lua_State* l)
{
    LD_PROFILE_SCOPE;

    LuaState L(l);

    LD_ASSERT(L.size() == 3);
    LuaType type = L.get_type(-3);
    LD_ASSERT(L.get_type(1) == LUA_TYPE_LIGHTUSERDATA);
    LD_ASSERT(L.get_type(2) == LUA_TYPE_STRING);
    LD_ASSERT(L.get_type(3) == LUA_TYPE_FN);

    const char* callbackCstr = L.to_string(2);
    UIWidget widget((UIWidgetObj*)L.to_userdata(1));
    UIDriver* driver = (UIDriver*)widget.get_user();
    LD_ASSERT(widget && driver && callbackCstr);

    // establish UIWidget -> UIDriver
    std::string callbackStr(callbackCstr);
    if (callbackStr == "on_click" && widget.get_type() == UI_WIDGET_BUTTON)
    {
        UIButtonWidget buttonW = (UIButtonWidget)widget;
        buttonW.set_on_click(&UIDriver::ui_button_on_click);
    }
    else // TODO: warn
        return 0;

    // establish UIDriver -> Lua Function
    // note that we can only use widget address as key due to UIWidgetObj address stability.
    driver->mCallbackRefs[widget.unwrap()] = L.ref(L.get_registry_index());

    return 0;
}

void UIDriver::push_driver_table()
{
    LD_PROFILE_SCOPE;

    mL.push_table();
    mL.push_fn(&ui_driver_get_widget);
    mL.set_field(-2, "get_widget");
    mL.push_light_userdata(this);
    mL.set_field(-2, "__user");
}

void UIDriver::ui_button_on_click(UIButtonWidget w, MouseButton btn, void* user)
{
    UIDriver* driver = (UIDriver*)user;

    auto it = driver->mCallbackRefs.find(w.unwrap());
    LD_ASSERT(it != driver->mCallbackRefs.end());

    LuaState L = driver->mL;
    L.push_integer(it->second);
    L.get_table(driver->mL.get_registry_index());

    LD_ASSERT(L.get_type(-1) == LUA_TYPE_FN);
    L.pcall(0, 0, -1);
}

} // namespace LD