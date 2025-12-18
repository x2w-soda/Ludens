#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Header/Math/Quat.h>

#include "LuaScriptFFI.h"

namespace LD {
namespace LuaScript {

static_assert(sizeof(Vec3) == 12);
static_assert(alignof(Vec3) == 4);
static_assert(offsetof(Vec3, x) == 0);
static_assert(offsetof(Vec3, y) == 4);
static_assert(offsetof(Vec3, z) == 8);

static_assert(sizeof(Vec4) == 16);
static_assert(alignof(Vec4) == 16);
static_assert(offsetof(Vec4, x) == 0);
static_assert(offsetof(Vec4, y) == 4);
static_assert(offsetof(Vec4, z) == 8);
static_assert(offsetof(Vec4, w) == 12);

static_assert(sizeof(Quat) == 16);
static_assert(alignof(Quat) == 4);
static_assert(offsetof(Quat, x) == 0);
static_assert(offsetof(Quat, y) == 4);
static_assert(offsetof(Quat, z) == 8);
static_assert(offsetof(Quat, w) == 12);

static const char sLuaFFICdef[] = R"(
typedef struct __attribute__((aligned(4))) {
    float x;
    float y;
    float z;
} Vec3;

typedef struct __attribute__((aligned(16))) {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct __attribute__((aligned(4))) {
    float x;
    float y;
    float z;
    float w;
} Quat;

typedef struct {
    Vec3 rotation;
    Vec3 position;
    Vec3 scale;
    Quat __internal_quat__;
} Transform;

typedef struct {
    Transform transform;
} MeshComponent;
)";

static const char sLuaFFImt[] = R"(local ffi = require 'ffi'
_G.ludens.math = {}
_G.ludens.math.Vec3 = nil
_G.ludens.math.Vec3 = ffi.metatype("Vec3", {
    __add = function (lhs, rhs) return _G.ludens.math.Vec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z) end,
    __len = function (v) return math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z) end,
})

_G.ludens.math.Vec4 = nil
_G.ludens.math.Vec4 = ffi.metatype("Vec4", {
    __add = function (lhs, rhs) return _G.ludens.math.Vec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w) end,
    __len = function (v) return math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w) end,
})
)";

const char* get_ffi_cdef()
{
    return sLuaFFICdef;
}

const char* get_ffi_mt()
{
    return sLuaFFImt;
}

} // namespace LuaScript
} // namespace LD