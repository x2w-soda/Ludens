#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#include <cstdint>
#include <string>

namespace LD {

enum LuaConfigType
{
    LUA_CONFIG_TYPE_BOOL = 0, /// boolean configuration
    LUA_CONFIG_TYPE_I32,      /// 32-bit signed integer configuration
    LUA_CONFIG_TYPE_F64,      /// 64-bit floating point configuration
    LUA_CONFIG_TYPE_STRING,   /// string configuration
    LUA_CONFIG_TYPE_VEC2,     /// math Vec2 configuration, 32-bit floats
    LUA_CONFIG_TYPE_VEC3,     /// math Vec3 configuration, 32-bit floats
    LUA_CONFIG_TYPE_VEC4,     /// math Vec4 configuration, 32-bit floats
};

enum LuaConfigError
{
    /// @brief lua code is never loaded before querying
    LUA_CONFIG_ERROR_NOT_LOADED = 0,

    /// @brief the name is not registered in this configuration
    LUA_CONFIG_ERROR_NOT_REGISTERED = -1,

    /// @brief the name is registered but not found in the lua code
    LUA_CONFIG_ERROR_NOT_FOUND = -2,
};

struct LuaConfigValue
{
    std::string name;    /// the name of the config value
    LuaConfigType type;  /// base type of value
    uint32_t arrayCount; /// if not equal to one, the number of elements in the array table
};

struct LuaConfigInfo
{
    std::string name;             /// the name of the configuration
    const LuaConfigValue* values; /// array of config values
    uint32_t valueCount;          /// number of config values
};

struct LuaConfig : Handle<struct LuaConfigObj>
{
    /// @brief create lua configuration scheme
    /// @param info configuration info
    /// @return lua configuration handle
    static LuaConfig create(const LuaConfigInfo& info);

    /// @brief destroy lua configuration
    /// @param config lua configuration handle
    static void destroy(LuaConfig config);

    /// @brief Evaluates lua code as a configuration file, can be called multiple times
    ///        to update the latest configuration values.
    /// @param src lua 5.1 source code, should return a table as the configuration
    /// @return true if lua code is evaluated without errors
    bool load(const char* src);

    /// @brief get boolean values by name
    /// @param name registered name of the value
    /// @param b pointer to an array of bools, array size declared by LuaConfigValue
    /// @return number of elements written, or a LuaConfigError code on failure
    int get_bool(const char* name, bool* b);

    int get_i32(const char* name, int32_t* i32);

    int get_f64(const char* name, double* f64);

    int get_string(const char* name, const char** string);

    int get_vec2(const char* name, Vec2* v);

    int get_vec3(const char* name, Vec3* v);

    int get_vec4(const char* name, Vec4* v);
};

} // namespace LD