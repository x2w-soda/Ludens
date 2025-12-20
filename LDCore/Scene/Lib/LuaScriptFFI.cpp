#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec4.h>

#include "LuaScriptFFI.h"

namespace LD {
namespace LuaScript {

static_assert(alignof(Vec2) == 4);
static_assert(offsetof(Vec2, x) == 0);
static_assert(offsetof(Vec2, y) == 4);

static_assert(alignof(Vec3) == 4);
static_assert(offsetof(Vec3, x) == 0);
static_assert(offsetof(Vec3, y) == 4);
static_assert(offsetof(Vec3, z) == 8);

static_assert(alignof(Vec4) == 16);
static_assert(offsetof(Vec4, x) == 0);
static_assert(offsetof(Vec4, y) == 4);
static_assert(offsetof(Vec4, z) == 8);
static_assert(offsetof(Vec4, w) == 12);

static_assert(alignof(Quat) == 4);
static_assert(offsetof(Quat, x) == 0);
static_assert(offsetof(Quat, y) == 4);
static_assert(offsetof(Quat, z) == 8);
static_assert(offsetof(Quat, w) == 12);

static_assert(alignof(Rect) == 4);
static_assert(offsetof(Rect, x) == 0);
static_assert(offsetof(Rect, y) == 4);
static_assert(offsetof(Rect, w) == 8);
static_assert(offsetof(Rect, h) == 12);

static_assert(alignof(Transform) == 4);
static_assert(offsetof(Transform, rotation) == 0);
static_assert(offsetof(Transform, position) == 12);
static_assert(offsetof(Transform, scale) == 24);
static_assert(offsetof(Transform, quat) == 36);

static_assert(alignof(Transform2D) == 4);
static_assert(offsetof(Transform2D, position) == 0);
static_assert(offsetof(Transform2D, scale) == 8);
static_assert(offsetof(Transform2D, rotation) == 16);

static const char sLuaFFICdef[] = R"(
typedef struct __attribute__((aligned(4))) {
    float x;
    float y;
} Vec2;

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

typedef struct __attribute__((aligned(4))) {
    float x;
    float y;
    float w;
    float h;
} Rect;

typedef struct __attribute__((aligned(4))) {
    Vec3 rotation;
    Vec3 position;
    Vec3 scale;
    Quat __private_quat;
} Transform;

typedef struct __attribute__((aligned(4))) {
    Vec2 position;
    Vec2 scale;
    float rotation;
} Transform2D;

typedef struct {
    Transform transform;
} MeshComponent;

typedef struct {
    Transform2D transform;
    Rect local;
    void* __private_image;
    int32_t zDepth;
    uint32_t __private_auid;
} Sprite2DComponent;
)";

static const char sLuaFFImt[] = R"(local ffi = require 'ffi'
_G.ludens.math = {}

_G.ludens.math.Vec2 = nil
_G.ludens.math.Vec2 = ffi.metatype("Vec2", {
    __add = function (lhs, rhs) return _G.ludens.math.Vec2(lhs.x + rhs.x, lhs.y + rhs.y) end,
    __len = function (v) return math.sqrt(v.x * v.x + v.y * v.y) end,
})

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