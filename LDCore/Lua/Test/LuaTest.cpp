#include "LuaTest.h"
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Lua/LuaState.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>

LD::LuaStateInfo sTestStateInfo{
    .openLibs = true,
};
