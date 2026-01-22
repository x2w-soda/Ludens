#include "LuaTest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Memory/Memory.h>

using namespace LD;

struct TestLuaModule
{
    static int sum(lua_State* l)
    {
        LuaState L(l);
        int lhs = L.to_integer(-1);
        int rhs = L.to_integer(-2);
        L.pop(2);
        L.push_integer(lhs + rhs);

        return 1;
    }
};

TEST_CASE("LuaModule")
{
    LuaState L = LuaState::create(sTestStateInfo);

    LuaModuleValue values[] = {
        {.type = LUA_TYPE_FN, .name = "sum", .fn = &TestLuaModule::sum},
        {.type = LUA_TYPE_NUMBER, .name = "pi", .number = 3.14},
    };

    LuaModuleNamespace modNS;
    modNS.name = nullptr;
    modNS.valueCount = sizeof(values) / sizeof(*values);
    modNS.values = values;

    LuaModuleInfo modI;
    modI.name = "test";
    modI.spaceCount = 1;
    modI.spaces = &modNS;

    LuaModule mod = LuaModule::create(modI);
    mod.load(L);

    const char* src = R"(
    local test = require 'test'
    return test.sum(2, 5), test.pi
)";
    bool success = L.do_string(src);
    CHECK(success);
    CHECK(L.to_integer(-2) == 7);
    CHECK(L.to_number(-1) == 3.14);

    LuaModule::destroy(mod);
    LuaState::destroy(L);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("LuaModule namespace")
{
    LuaState L = LuaState::create(sTestStateInfo);

    LuaModuleValue values[] = {
        {.type = LUA_TYPE_FN, .name = "sum", .fn = &TestLuaModule::sum},
        {.type = LUA_TYPE_NUMBER, .name = "pi", .number = 3.14},
    };

    LuaModuleNamespace modNS;
    modNS.name = "math";
    modNS.valueCount = sizeof(values) / sizeof(*values);
    modNS.values = values;

    LuaModuleInfo modI;
    modI.name = "test";
    modI.spaceCount = 1;
    modI.spaces = &modNS;

    LuaModule mod = LuaModule::create(modI);
    mod.load(L);

    const char* src = R"(
    local test = require 'test'
    return test.math.sum(4, 9)
)";
    bool success = L.do_string(src);
    CHECK(success);

    LuaModule::destroy(mod);
    LuaState::destroy(L);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}
