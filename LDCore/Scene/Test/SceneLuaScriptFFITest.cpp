#include <Extra/doctest/doctest.h>
#include <LDCore/Scene/Lib/LuaScriptFFI.h>
#include <Ludens/Lua/LuaState.h>

#include <format>
#include <iostream>
#include <string>

using namespace LD;

TEST_CASE("LuaScript FFI")
{
    LuaState L = LuaState::create({.openLibs = true});

    std::string str = std::format("local ffi = require 'ffi' ffi.cdef [[ {} ]]", LuaScript::get_ffi_cdef());
    bool ok = L.do_string(str.c_str());
    if (!ok)
    {
        std::cout << L.to_string(-1) << std::endl;
    }
    CHECK(ok);

    LuaState::destroy(L);
}