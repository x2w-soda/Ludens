#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Platform.h>
#include <Ludens/Lua/LuaState.h>

#include <cstddef>
#include <cstdint>
#include <iostream>

#include "LuaTest.h"

using namespace LD;

#ifdef LD_PLATFORM_WIN32
# define FFI_EXPORT __declspec(dllexport)
#elif defined(LD_PLATFORM_LINUX)
# define FFI_EXPORT __attribute__((visibility("default")))
#endif

const char sLuaFFI_cdef[] = R"(
local ffi = require 'ffi'

ffi.cdef[[
typedef struct __attribute__((aligned(8))) {
    union { float x; float r; };
    union { float y; float g; };
} Vec2;

typedef struct {
    union { Vec2 position; Vec2 pos; };
    Vec2 scale;
    union { float rotation; float rot; };
} Transform2D;
]]

ffi.cdef[[
typedef struct {
    Transform2D transform;
    uint32_t assetID;
} Sprite2D;

typedef struct {
    int32_t _value;
} Box;

float vec2_length_sq(Vec2* v);
]])";

const char sLuaFFI_mt[] = R"(
local ffi = require 'ffi'

_G.Vec2 = nil
local mt = {
    __len = function(v) return math.sqrt(v.x * v.x + v.y * v.y) end,
    __add = function(lhs, rhs) return _G.Vec2(lhs.x + rhs.x, lhs.y + rhs.y) end,
}
_G.Vec2 = ffi.metatype("Vec2", mt)

_G.Transform2D = ffi.metatype("Transform2D", {})

_G.Box = nil
mt = {
    __index = function (t, k) -- read field with side effects
        if k == 'value' then
            _G.index_ctr._value = _G.index_ctr._value + 1
            return t._value
        end
        return nil
    end,
    __newindex = function (t, k, v) -- write field with side effects
        if k == 'value' then
            _G.newindex_ctr._value = _G.newindex_ctr._value + 1
            t._value = v
        end
    end,
}
_G.Box = ffi.metatype("Box", mt)
)";

const char sLuaFFI_test1[] = R"(
local ffi = require 'ffi'
local v1 = ffi.cast('Vec2*', _G.v1)

return #(v1 + Vec2(4.0, 20.0))
)";

const char sLuaFFI_test2[] = R"(
local ffi = require 'ffi'
local sprite = ffi.cast('Sprite2D*', _G.sprite)

sprite.transform.pos.x = sprite.transform.pos.x * 2.0
return sprite.transform.pos.x
)";

const char sLuaFFI_test3[] = R"(
local ffi = require 'ffi'
local cdata = ffi.cast('Vec2*', _G.v1)
print(cdata)
return tonumber(ffi.cast('uintptr_t', cdata))
)";

const char sLuaFFI_test4[] = R"(
local ffi = require 'ffi'
local v1 = ffi.cast('Vec2*', _G.v1)
return ffi.C.vec2_length_sq(v1);
)";

namespace {

struct Box
{
    int32_t value;

    Box(int32_t value) : value(value) {}
};

struct Sprite2D
{
    Transform2D transform;
    uint32_t assetID;
};

} // namespace

extern "C" {
FFI_EXPORT float vec2_length_sq(Vec2* v)
{
    return v->x * v->x + v->y * v->y;
}
} // extern "C"

TEST_CASE("Lua FFI")
{
    LuaState L = LuaState::create(sTestStateInfo);
    CHECK(L.do_string(sLuaFFI_cdef));
    CHECK(L.do_string(sLuaFFI_mt));

    Vec2 v1(3.0f, 4.0f);
    L.push_light_userdata(&v1);
    L.set_global("v1");
    CHECK(L.do_string(sLuaFFI_test1));
    CHECK(L.size() == 1);
    CHECK(L.to_number(-1) == 25.0f);
    L.clear();

    CHECK(L.do_string(sLuaFFI_test3));
    CHECK(L.size() == 1);
    CHECK(L.get_type(-1) == LUA_TYPE_NUMBER);
    void* ptr = (void*)((uintptr_t)L.to_number(-1));
    CHECK(ptr == &v1);
    L.clear();

    CHECK(L.do_string(sLuaFFI_test4));
    CHECK(L.size() == 1);
    CHECK(L.get_type(-1) == LUA_TYPE_NUMBER);
    CHECK(L.to_number(-1) == 25.0f);
    L.clear();

    Sprite2D sprite{};
    sprite.transform.position.x = 123.0f;
    L.push_light_userdata(&sprite);
    L.set_global("sprite");
    CHECK(L.do_string(sLuaFFI_test2));
    CHECK(L.size() == 1);
    CHECK(L.to_number(-1) == 246.0f);
    L.clear();

    CHECK(sprite.transform.position.x == 246.0f);

    Box box(0xBeef), indexCtr(0), newindexCtr(0);
    L.push_light_userdata(&box);
    L.set_global("box");
    L.push_light_userdata(&indexCtr);
    L.set_global("index_ctr");
    L.push_light_userdata(&newindexCtr);
    L.set_global("newindex_ctr");

    {
        CHECK(L.do_string(R"(
local ffi = require 'ffi'
local box = ffi.cast('Box*', _G.box)
_G.index_ctr = ffi.cast('Box*', _G.index_ctr)
_G.newindex_ctr = ffi.cast('Box*', _G.newindex_ctr)

return box.value
)"));
        CHECK(L.to_number(-1) == 0xBeef);
        L.clear();

        CHECK(box.value == 0xBeef);
        CHECK(indexCtr.value == 1);
        CHECK(newindexCtr.value == 0);
    }

    {
        CHECK(L.do_string(R"(
local ffi = require 'ffi'
local box = ffi.cast('Box*', _G.box)
_G.index_ctr = ffi.cast('Box*', _G.index_ctr)
_G.newindex_ctr = ffi.cast('Box*', _G.newindex_ctr)

box.value = 0xCAFE
box.value = box.value + 1

return box.value
)"));
        CHECK(L.to_number(-1) == 0xCAFF);
        L.clear();

        CHECK(box.value == 0xCAFF);
        CHECK(indexCtr.value == 3);
        CHECK(newindexCtr.value == 2);
    }

    LuaState::destroy(L);
}
