#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/UI/UIWindow.h>

#include <string>

namespace LD {

/// @brief User of a UIWidget subtree, provides bindings and drives the subtree.
class UIDriver
{
public:
    UIDriver() = default;
    UIDriver(const UIDriver&) = delete;
    ~UIDriver() = default;

    UIDriver& operator=(const UIDriver&) = delete;

    bool connect(UIWindow window, LuaState luaState, const char* luaSource, std::string& err);
    bool disconnect();

    bool attach(std::string& err);
    bool detach(std::string& err);

    inline UIWindow get_window() { return mWindow; }

    static int install_callback(lua_State* l);

private:
    void push_driver_table();

    static void ui_button_on_click(UIButtonWidget w, MouseButton btn, void* user);

private:
    LuaState mL{};
    UIWindow mWindow{};
    HashMap<void*, int> mCallbackRefs;
    int mScriptRef = 0;
};

} // namespace LD