#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/Lua/LuaState.h>
#include <limits>

TEST_CASE("LuaState primitives")
{
    LuaState L = LuaState::create(sTestStateInfo);

    L.push_number(3.1415);
    double num = L.to_number(-1);
    CHECK(is_zero_epsilon(num - 3.1415));
    L.pop(1);
    CHECK(L.empty());

    L.push_bool(true);
    L.push_bool(false);
    CHECK(L.to_bool(-1) == false);
    CHECK(L.to_bool(-2) == true);
    L.pop(2);
    CHECK(L.empty());

    L.push_integer(-12345);
    L.push_integer(0);
    L.push_integer(std::numeric_limits<int32_t>::max());
    L.push_integer(std::numeric_limits<int32_t>::min());
    CHECK(L.to_integer(-1) == std::numeric_limits<int32_t>::min());
    CHECK(L.to_integer(-2) == std::numeric_limits<int32_t>::max());
    CHECK(L.to_integer(-3) == 0);
    CHECK(L.to_integer(-4) == -12345);
    CHECK(L.size() == 4);

    L.clear();
    CHECK(L.empty());

    LuaState::destroy(L);
}

TEST_CASE("LuaState types")
{
    LuaState L = LuaState::create(sTestStateInfo);

    L.push_integer(123);
    CHECK(L.get_type(-1) == LUA_TYPE_NUMBER);

    L.push_number(3.14);
    CHECK(L.get_type(-1) == LUA_TYPE_NUMBER);

    L.push_bool(true);
    CHECK(L.get_type(-1) == LUA_TYPE_BOOL);

    L.push_fn([](lua_State*) -> int { return 0; });
    CHECK(L.get_type(-1) == LUA_TYPE_FN);

    L.push_table();
    CHECK(L.get_type(-1) == LUA_TYPE_TABLE);

    L.push_string("cstr");
    CHECK(L.get_type(-1) == LUA_TYPE_STRING);

    L.push_nil();
    CHECK(L.get_type(-1) == LUA_TYPE_NIL);

    L.clear();
    LuaState::destroy(L);
}

TEST_CASE("LuaState functions")
{
    LuaState L = LuaState::create(sTestStateInfo);

    auto get_123 = [](lua_State* l) -> int {
        LuaState L(l);
        L.push_integer(1);
        L.push_integer(2);
        L.push_integer(3);
        return 3;
    };

    // 0 args, 3 results
    L.push_fn(get_123);
    L.call(0, 3);
    CHECK(L.size() == 3);
    CHECK(L.to_integer(-1) == 3);
    CHECK(L.to_integer(-2) == 2);
    CHECK(L.to_integer(-3) == 1);
    L.clear();

    auto sub = [](lua_State* l) -> int {
        LuaState L(l);
        int32_t lhs = L.to_integer(-2);
        int32_t rhs = L.to_integer(-1);
        L.pop(2);
        L.push_integer(lhs - rhs);
        return 1;
    };

    // 2 args, 1 result
    L.push_fn(sub);
    L.push_integer(5);
    L.push_integer(17);
    L.call(2, 1);

    CHECK(L.size() == 1);
    CHECK(L.to_integer(-1) == -12);

    LuaState::destroy(L);
}

TEST_CASE("LuaState tables")
{
    LuaState L = LuaState::create(sTestStateInfo);

    L.push_table();
    L.push_integer(33);
    L.set_field(-2, "x");
    L.set_global("t");
    CHECK(L.empty());

    L.get_global("t");
    L.get_field(-1, "x");
    CHECK(L.to_integer(-1) == 33);
    L.pop(1);

    L.push_integer(1);
    L.push_integer(100);
    L.set_table(-3); // t[1] = 100
    CHECK(L.size() == 1);

    L.push_integer(2);
    L.push_bool(true);
    L.set_table(-3); // t[2] = true
    CHECK(L.size() == 1);

    L.push_integer(1);
    L.get_table(-2);
    CHECK(L.get_type(-1) == LUA_TYPE_NUMBER);
    CHECK(L.to_number(-1) == 100);
    L.pop(1);

    L.push_integer(2);
    L.get_table(-2);
    CHECK(L.get_type(-1) == LUA_TYPE_BOOL);
    CHECK(L.to_bool(-1) == true);
    L.pop(1);

    // pushes t[1] and t[2] onto the stack
    L.get_table_indices(-1, 1, 2);
    CHECK(L.size() == 3);
    CHECK(L.get_type(-2) == LUA_TYPE_NUMBER);
    CHECK(L.to_number(-2) == 100);
    CHECK(L.get_type(-1) == LUA_TYPE_BOOL);
    CHECK(L.to_bool(-1) == true);
    L.pop(2);

    // achieve the same using positive tIndex
    L.get_table_indices(1, 1, 2);
    CHECK(L.size() == 3);
    CHECK(L.get_type(-2) == LUA_TYPE_NUMBER);
    CHECK(L.to_number(-2) == 100);
    CHECK(L.get_type(-1) == LUA_TYPE_BOOL);
    CHECK(L.to_bool(-1) == true);

    LuaState::destroy(L);
}

TEST_CASE("LuaState do_string")
{
    LuaState L = LuaState::create(sTestStateInfo);

    L.do_string("x = 123\ny = x * -3");

    L.get_global("x");
    L.get_global("y");
    CHECK(L.to_integer(-1) == -369);
    CHECK(L.to_integer(-2) == 123);
    L.pop(2);

    LuaState::destroy(L);
}
