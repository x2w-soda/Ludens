#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>

#include <algorithm>

#include "LuaScriptFFI.h"
#include "SceneObj.h"

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

static_assert(alignof(TransformEx) == 4);
static_assert(offsetof(TransformEx, position) == 0);
static_assert(offsetof(TransformEx, scale) == 12);
static_assert(offsetof(TransformEx, rotation) == 24);
static_assert(offsetof(TransformEx, rotationEuler) == 40);

static_assert(alignof(Transform2D) == 4);
static_assert(offsetof(Transform2D, position) == 0);
static_assert(offsetof(Transform2D, scale) == 8);
static_assert(offsetof(Transform2D, rotation) == 16);

static_assert(alignof(AudioSourceComponent) == 8);
static_assert(offsetof(AudioSourceComponent, playback) == 8);
static_assert(offsetof(AudioSourceComponent, clipID) == 16);
static_assert(offsetof(AudioSourceComponent, pan) == 20);
static_assert(offsetof(AudioSourceComponent, volumeLinear) == 24);

static const char sLuaFFICdef[] = R"(
typedef struct __attribute__((aligned(4))) Vec2 {
    float x;
    float y;
} Vec2;

typedef struct __attribute__((aligned(4))) Vec3 {
    float x;
    float y;
    float z;
} Vec3;

typedef struct __attribute__((aligned(16))) Vec4 {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct __attribute__((aligned(4))) Quat {
    float x;
    float y;
    float z;
    float w;
} Quat;

typedef struct __attribute__((aligned(4))) Rect {
    float x;
    float y;
    float w;
    float h;
} Rect;

typedef struct __attribute__((aligned(4))) TransformEx {
    Vec3 position;
    Vec3 scale;
    Quat __private_quat;
    Vec3 rotation;
} Transform;

typedef struct __attribute__((aligned(4))) Transform2D {
    Vec2 position;
    Vec2 scale;
    float rotation;
} Transform2D;

uint32_t ffi_get_parent_id(uint32_t compID);
uint32_t ffi_get_child_id_by_name(uint32_t compID, const char* name);

typedef struct MeshComponent {
    void* base;
    Transform transform;
} MeshComponent;

typedef struct __attribute__((aligned(8))) AudioSourceComponent {
    void* base;
    void* __private_playback;
    uint32_t __private_clipAUID;
    float __private_pan;
    float __private_volumeLinear;
} AudioSourceComponent;

void ffi_audio_source_component_play(AudioSourceComponent* comp);
void ffi_audio_source_component_pause(AudioSourceComponent* comp);
void ffi_audio_source_component_resume(AudioSourceComponent* comp);
void ffi_audio_source_component_set_pan(AudioSourceComponent* comp, float pan);
void ffi_audio_source_component_set_volume_linear(AudioSourceComponent* comp, float volumeLinear);

// TODO:
typedef struct Sprite2DComponent {
    void* base;
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

ffi.metatype("AudioSourceComponent", {
    __index = function (t, k)
        if k == 'pan' then
            return t.cdata.__private_pan
        elseif k == 'play' then
            return function (comp)
                ffi.C.ffi_audio_source_component_play(comp.cdata)
            end
        elseif k == 'pause' then
            return function (comp)
                ffi.C.ffi_audio_source_component_pause(comp.cdata)
            end
        elseif k == 'resume' then
            return function (comp)
                ffi.C.ffi_audio_source_component_resume(comp.cdata)
            end
        end
        return nil
    end,
    __newindex = function (t, k, v)
        if k == 'pan' and tonumber(v) ~= nil then
            ffi.C.ffi_audio_source_component_set_pan(t, tonumber(v))
        elseif k == 'volume' and tonumber(v) ~= nil then
            ffi.C.ffi_audio_source_component_set_volume_linear(t, tonumber(v))
        end
    end,
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

extern "C" {

uint32_t ffi_get_parent_id(uint32_t compID)
{
    ComponentBase* base = sScene->registry.get_component_base(compID);
    LD_ASSERT(base);
    ComponentBase* parent = base->parent;
    return parent ? parent->cuid : 0;
}

uint32_t ffi_get_child_id_by_name(uint32_t compID, const char* name)
{
    ComponentBase* base = sScene->registry.get_component_base(compID);
    LD_ASSERT(base && base->name);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        LD_ASSERT(child && child->name);

        if (!strcmp(child->name, name))
            return child->cuid;
    }

    return 0;
}

void ffi_audio_source_component_play(AudioSourceComponent* comp)
{
    LD_ASSERT(comp && comp->base);

    Scene::AudioSource source(comp);

    if (source)
        source.play();
}

void ffi_audio_source_component_pause(AudioSourceComponent* comp)
{
    LD_ASSERT(comp && comp->base);

    Scene::AudioSource source(comp);

    if (source)
        source.pause();
}

void ffi_audio_source_component_resume(AudioSourceComponent* comp)
{
    Scene::AudioSource source(comp);

    if (source)
        source.resume();
}

void ffi_audio_source_component_set_pan(AudioSourceComponent* comp, float pan)
{
    if (!comp->playback)
        return;

    comp->pan = std::clamp<float>(pan, 0.0f, 1.0f);

    AudioPlayback::Accessor accessor = comp->playback.access();
    accessor.set_pan(pan);
}

void ffi_audio_source_component_set_volume_linear(AudioSourceComponent* comp, float volumeLinear)
{
    if (!comp->playback)
        return;

    comp->volumeLinear = std::clamp<float>(volumeLinear, 0.0f, 1.0f);

    AudioPlayback::Accessor accessor = comp->playback.access();
    accessor.set_volume_linear(volumeLinear);
}

void ffi_sprite_2d_component_set_z_depth(Sprite2DComponent* comp, uint32_t zDepth)
{
    Scene::Sprite2D sprite(comp);

    if (sprite)
        sprite.set_z_depth(zDepth);
}

} // extern "C"
} // namespace LuaScript
} // namespace LD