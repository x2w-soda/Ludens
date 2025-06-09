#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Lua/LuaState.h>

namespace LD {

struct LuaModuleValue
{
    LuaType type;     /// value type
    const char* name; /// name of value
    union
    {
        const char* string; /// LUA_TYPE_STRING
        double number;      /// LUA_TYPE_NUMBER
        LuaFn fn;           /// LUA_TYPE_FN
    };
};

/// @brief a module namespace is just a table to group values together
struct LuaModuleNamespace
{
    const char* name;             /// if not null, the namespace name, may contain "." for nested namespaces
    const LuaModuleValue* values; /// values beloning to this namespace
    uint32_t valueCount;          /// number of values
};

struct LuaModuleInfo
{
    const char* name;
    const LuaModuleNamespace* spaces;
    uint32_t spaceCount;
};

struct LuaModule : Handle<struct LuaModuleObj>
{
    /// @brief create lua module
    /// @param moduleI module info
    /// @return a handle to a lua module
    static LuaModule create(const LuaModuleInfo& moduleI);

    /// @brief destroy lua module
    static void destroy(LuaModule mod);

    /// @brief load the module into the package.loaded table,
    ///        using module name as key and the module itself as value.
    /// @param L lua state to load the module into
    /// @info https://www.lua.org/manual/5.1/manual.html#5.3
    void load(LuaState& L);
};

} // namespace LD