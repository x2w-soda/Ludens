#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Math/Vec4.h>
#include <cstdint>
#include <cstdlib>

struct lua_State;

namespace LD {

struct LuaState;
struct LuaStateObj;
typedef int (*LuaFn)(lua_State*);

enum LuaType
{
    LUA_TYPE_NIL = 0,
    LUA_TYPE_BOOL,
    LUA_TYPE_LIGHTUSERDATA,
    LUA_TYPE_NUMBER,
    LUA_TYPE_STRING,
    LUA_TYPE_TABLE,
    LUA_TYPE_FN,
    LUA_TYPE_USERDATA,
    LUA_TYPE_THREAD,
};

enum LuaError
{
    /// Runtime errors.
    LUA_ERR_RUNTIME = 2,

    /// Memory allocation error. For such errors, Lua does not call the error handler function.
    LUA_ERR_MEMORY = 4,

    /// Error while running the error handler function.
    LUA_ERR_ERROR = 5,
};

struct LuaStateInfo
{
    bool openLibs;
};

/// @brief A handle for a Lua5.1 state, containing a value stack.
///        Stack indices start from 1 at the bottom. A negative index
///        -x is equivalent to the positive index size() - x + 1.
class LuaState
{
public:
    /// @brief create lua state
    /// @param stateI lua state information
    /// @return a lua state handle
    static LuaState create(const LuaStateInfo& stateI);

    /// @brief destroy lua state
    /// @param state lua state handle to destroy
    static void destroy(LuaState state);

    /// @brief default constructs a null handle
    /// @warning do not call any method on a null handle
    LuaState();

    /// @brief constructs a non-owning handle from native lua state
    /// @param L native Lua 5.1 state
    LuaState(lua_State* L);

    LuaState(const LuaState& other);

    LuaState& operator=(const LuaState& other);

    /// @brief loads and runs the given string
    /// @param str null-terminated Lua 5.1 source code
    /// @return true on success
    bool do_string(const char* str);

    /// @brief loads and runes the give file
    /// @param filepath null-terminated file path to a Lua 5.1 source file
    /// @return true on success
    bool do_file(const char* filepath);

    /// @brief pushes onto the stack the value of the global
    /// @param name name of the global
    void get_global(const char* name);

    /// @brief pops a value and assigns it to a global
    /// @param name name of the global
    void set_global(const char* name);

    /// @brief pushes onto the stack t[k], where t is the table at tIndex and k the value on top of the stack.
    ///        this method pops the key and may trigger a metamethod for the "index" event.
    /// @param tIndex the stack index of the table
    void get_table(int tIndex);

    /// @brief performs t[k] = v, where t is the table at tIndex, v at index -1, and k at index -2.
    ///        this method pops two values off the stack.
    /// @param tIndex the stack index of the table
    void set_table(int tIndex);

    /// @brief pushes onto the stack t[i] for all i1 <= i && i <= i2, where t is the table at tIndex.
    ///        this method pushes i2 - i1 + 1 elements onto the stack, with t[i2] at the very top.
    /// @param tIndex the stack index of the table
    /// @param i1 table index start
    /// @param i2 table index end, inclusive
    void get_table_indices(int tIndex, int i1, int i2);

    /// @brief pops a table from the stack and sets it as the new metatable for the value at the given acceptable index.
    /// @param tIndex the stack index of the table or user data
    void set_meta_table(int tIndex);

    /// @brief pushes onto the stack the value of t[k], where t is the table at tIndex.
    /// @param tIndex the stack index of the table
    /// @param k the key value
    void get_field(int tIndex, const char* k);

    /// @brief pops a value v and performs t[k] = v, where t is the table at tIndex
    /// @param tIndex the stack index of the table
    /// @param key the key value
    void set_field(int tIndex, const char* k);

    /// @brief get the type of a value at stack index
    /// @index stack index of the value to query
    LuaType get_type(int index);

    /// @brief get the number of elements on the stack
    /// @return number of elements on the stack, zero if stack is empty
    int size();

    /// @brief check whether the stack is empty
    /// @return true if stack is empty, false otherwise
    bool empty();

    /// @brief pop all values off the stack
    void clear();

    /// @brief pop values off the stack
    /// @param n number of values to pop
    void pop(int n);

    /// @brief Resize the stack.
    /// @param n The new size of the stack, padding with nil values.
    void resize(int n);

    /// @brief pushes an integer onto the stack
    void push_integer(int32_t i);

    /// @brief pushes a number onto the stack
    void push_number(double num);

    /// @brief pushes a string onto the stack
    /// @param cstr a null-terminated c string
    void push_string(const char* cstr);

    /// @brief pushes a string onto the stack
    /// @param str a string
    /// @param len length of the string in bytes
    void push_lstring(const char* str, size_t len);

    /// @brief pushes a boolean onto the stack
    void push_bool(bool b);

    /// @brief pushes a function onto the stack
    void push_fn(LuaFn fn);

    /// @brief pushes a new empty table onto the stack
    void push_table();

    /// @brief pushes a user data
    /// @param size byte size of ther user data
    /// @return address of the user data allocated by Lua
    void* push_userdata(size_t size);

    /// @brief pushes a light user data
    /// @param data light user data such as a pointer
    void push_light_userdata(void* data);

    /// @brief pushes a nil value onto the stack
    void push_nil();

    /// @brief pushes a Vec2 as a table with 'x', 'y' fields
    void push_vec2(const Vec2& v);

    /// @brief pushes a Vec3 as a table with 'x', 'y', 'z' fields
    void push_vec3(const Vec3& v);

    /// @brief pushes a Vec4 as a table with 'x', 'y', 'z', 'w' fields
    void push_vec4(const Vec4& v);

    /// @brief call function
    /// @param nargs number of arguments
    /// @param nresults number of results
    /// @info https://www.lua.org/manual/5.1/manual.html#lua_call
    void call(int nargs, int nresults);

    /// @brief Protected call function
    /// @param nargs Number of arguments
    /// @param nresults Number of results
    /// @param handlerIndex If not zero, the stack index of the error handler function.
    /// @return zero on success, or one of LUA_ERR_RUNTIME, LUA_ERR_MEMORY, LUA_ERR_ERROR.
    /// @info https://www.lua.org/manual/5.1/manual.html#lua_pcall
    LuaError pcall(int nargs, int nresults, int handlerIndex);

    /// @brief Generates a Lua error. The error message must be on stack top.
    /// @warning This function does a long jump, and therefore never returns.
    void error();

    /// @brief get the integer at stack index
    int32_t to_integer(int index);

    /// @brief get the number at stack index
    double to_number(int index);

    /// @brief get the boolean at stack index
    bool to_bool(int index);

    /// @brief get the string at stack index
    /// @return a transient null-terminated c string
    /// @warning Returned pointer is owned by lua VM, never cache it.
    const char* to_string(int index);

    /// @brief get the user data at stack index. If the value is a light user data, returns its pointer.
    /// @return address of the user data
    void* to_userdata(int index);

    /// @brief get the Vec2 at stack index
    Vec2 to_vec2(int index);

    /// @brief get the Vec3 at stack index
    Vec3 to_vec3(int index);

    /// @brief get the Vec4 at stack index
    Vec4 to_vec4(int index);

private:
    LuaState(LuaStateObj* obj, lua_State* L);

    LuaStateObj* mObj;
    lua_State* mL;
};

} // namespace LD
