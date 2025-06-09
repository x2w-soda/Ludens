#include "LuaTest.h"
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Lua/LuaState.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestLuaConfig.h"
#include "TestLuaModule.h"
#include "TestLuaState.h"
#include <Extra/doctest/doctest.h>

namespace {

LuaStateInfo sTestStateInfo{
    .openLibs = true,
};

} // namespace