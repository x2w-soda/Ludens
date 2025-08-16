#pragma once

#include "LuaTest.h"
#include <string>
#include <Extra/doctest/doctest.h>
#include <Ludens/Lua/LuaState.h>

using namespace LD;

static int lua_error_handler(lua_State* l)
{
    LuaState L(l);

    if (L.get_type(-1) == LUA_TYPE_STRING)
    {
        std::string msg(L.to_string(-1));
        msg.push_back('!'); // emphasize the pain of error handling
        L.push_string(msg.c_str());
    }

    return 1;
}

static int lua_int_div(lua_State* l)
{
    LuaState L(l);

    int32_t lhs = L.to_integer(-2);
    int32_t rhs = L.to_integer(-1);

    if (rhs == 0)
    {
        L.push_string("division by zero");
        L.error();
    }

    L.push_integer(lhs / rhs);

    return 1;
}

TEST_CASE("Lua pcall")
{
    LuaState L = LuaState::create(sTestStateInfo);
    LuaError err;

    // call non-function
    L.push_string("foo");
    err = L.pcall(0, 0, 0);
    CHECK(err == LUA_ERR_RUNTIME);
    L.clear();

    // success, without handler
    L.push_fn(&lua_int_div);
    L.push_integer(30);
    L.push_integer(5);
    err = L.pcall(2, 1, 0);
    CHECK(err == 0);
    CHECK(L.to_integer(-1) == 6);
    CHECK(L.size() == 1);
    L.clear();

    // catch VM error, without handler
    L.push_fn(&lua_int_div);
    L.push_integer(30);
    L.push_integer(0);
    err = L.pcall(2, 1, 0);
    CHECK(err == LUA_ERR_RUNTIME);
    CHECK(L.size() == 1); // error message
    std::string msg(L.to_string(-1));
    CHECK(msg == "division by zero");
    L.clear();

    // catch VM error, with handler
    L.push_fn(&lua_error_handler);
    L.push_fn(&lua_int_div);
    L.push_integer(30);
    L.push_integer(0);
    err = L.pcall(2, 1, -4);
    CHECK(err == LUA_ERR_RUNTIME);
    CHECK(L.size() == 2);
    msg = std::string(L.to_string(-1));
    CHECK(msg == "division by zero!");
    L.clear();

    LuaState::destroy(L);
}