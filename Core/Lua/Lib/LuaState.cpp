#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/System/Memory.h>
#include <cstring>

// hide Lua headers from user
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace LD {

static Log sLog("lua");

struct LuaStateObj
{
    lua_State* L;
};

static void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    (void)ud;

    if (osize == 0)
        return heap_malloc(nsize, MEMORY_USAGE_LUA);

    if (nsize == 0)
    {
        heap_free(ptr);
        return NULL;
    }

    void* base = heap_malloc(nsize, MEMORY_USAGE_LUA);
    memcpy(base, ptr, std::min(osize, nsize));
    heap_free(ptr);

    return base;
}

LuaState LuaState::create(const LuaStateInfo& stateI)
{
    LuaStateObj* obj = (LuaStateObj*)heap_malloc(sizeof(LuaStateObj), MEMORY_USAGE_LUA);

    obj->L = lua_newstate(&lua_alloc, obj);

    if (stateI.openLibs)
        luaL_openlibs(obj->L);

    return LuaState(obj, obj->L);
}

void LuaState::destroy(LuaState state)
{
    LuaStateObj* obj = state.mObj;

    if (obj)
    {
        lua_close(obj->L);
        heap_free(obj);
    }

    state.mObj = nullptr;
    state.mL = nullptr;
}

LuaState::LuaState()
    : mObj(nullptr), mL(nullptr)
{
}

LuaState::LuaState(lua_State* L)
    : mObj(nullptr), mL(L)
{
}

LuaState::LuaState(LuaStateObj* obj, lua_State* L)
    : mObj(obj), mL(L)
{
}

LuaState::LuaState(const LuaState& other)
    : mObj(other.mObj), mL(other.mL)
{
}

LuaState& LuaState::operator=(const LuaState& other)
{
    mObj = other.mObj;
    mL = other.mL;

    return *this;
}

bool LuaState::do_string(const char* str)
{
    int ret = luaL_dostring(mL, str);

    return ret == 0;
}

bool LuaState::do_file(const char* filepath)
{
    int ret = luaL_dofile(mL, filepath);

    return ret == 0;
}

void LuaState::get_global(const char* name)
{
    lua_getglobal(mL, name);
}

void LuaState::set_global(const char* name)
{
    lua_setglobal(mL, name);
}

void LuaState::get_table(int tIndex)
{
    lua_gettable(mL, tIndex);
}

void LuaState::set_table(int tIndex)
{
    lua_settable(mL, tIndex);
}

void LuaState::get_table_indices(int tIndex, int i1, int i2)
{
    if (tIndex > 0)
        tIndex = -(size() - tIndex + 1);

    for (int i = i1; i <= i2; i++)
    {
        lua_pushinteger(mL, i);
        lua_gettable(mL, tIndex - 1 - (i - i1));
    }
}

void LuaState::get_field(int tIndex, const char* k)
{
    lua_getfield(mL, tIndex, k);
}

void LuaState::set_field(int tIndex, const char* k)
{
    lua_setfield(mL, tIndex, k);
}

LuaType LuaState::get_type(int index)
{
    int type = lua_type(mL, index);

    // clang-format off
    switch (type)
    {
    case LUA_TNIL:           return LUA_TYPE_NIL;
    case LUA_TBOOLEAN:       return LUA_TYPE_BOOL;
    case LUA_TLIGHTUSERDATA: return LUA_TYPE_LIGHTUSERDATA;
    case LUA_TNUMBER:        return LUA_TYPE_NUMBER;
    case LUA_TSTRING:        return LUA_TYPE_STRING;
    case LUA_TTABLE:         return LUA_TYPE_TABLE;
    case LUA_TFUNCTION:      return LUA_TYPE_FN;
    case LUA_TUSERDATA:      return LUA_TYPE_USERDATA;
    case LUA_TTHREAD:        return LUA_TYPE_THREAD;
    }
    // clang-format on

    sLog.error("unknown native lua type {}", type);

    return (LuaType)type;
}

int LuaState::size()
{
    return lua_gettop(mL);
}

bool LuaState::empty()
{
    return lua_gettop(mL) == 0;
}

void LuaState::clear()
{
    lua_pop(mL, lua_gettop(mL));
}

void LuaState::pop(int n)
{
    lua_pop(mL, n);
}

void LuaState::push_integer(int32_t i)
{
    lua_pushinteger(mL, (lua_Integer)i);
}

void LuaState::push_number(double num)
{
    lua_pushnumber(mL, (lua_Number)num);
}

void LuaState::push_string(const char* cstr)
{
    lua_pushlstring(mL, cstr, strlen(cstr));
}

void LuaState::push_lstring(const char* str, size_t len)
{
    lua_pushlstring(mL, str, len);
}

void LuaState::push_bool(bool b)
{
    lua_pushboolean(mL, (int)b);
}

void LuaState::push_fn(LuaFn fn)
{
    lua_pushcfunction(mL, fn);
}

void LuaState::push_table()
{
    lua_createtable(mL, 0, 0);
}

void LuaState::push_nil()
{
    lua_pushnil(mL);
}

void LuaState::call(int nargs, int nresults)
{
    lua_call(mL, nargs, nresults);
}

int32_t LuaState::to_integer(int index)
{
    return (int32_t)lua_tointeger(mL, index);
}

double LuaState::to_number(int index)
{
    return (double)lua_tonumber(mL, index);
}

bool LuaState::to_bool(int index)
{
    return (bool)lua_toboolean(mL, index);
}

const char* LuaState::to_string(int index)
{
    return lua_tostring(mL, index);
}

} // namespace LD