#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Lua/LuaState.h>
#include <cstddef>

#include "LuaTest.h"

using namespace LD;

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

namespace {

struct Sprite2D
{
    Transform2D transform;
    uint32_t assetID;
};

} // namespace

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

    Sprite2D sprite{};
    sprite.transform.position.x = 123.0f;
    L.push_light_userdata(&sprite);
    L.set_global("sprite");
    CHECK(L.do_string(sLuaFFI_test2));
    CHECK(L.size() == 1);
    CHECK(L.to_number(-1) == 246.0f);
    L.clear();
    
    CHECK(sprite.transform.position.x == 246.0f);

    LuaState::destroy(L);
}