#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/ComponentViews.h>
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

static_assert(offsetof(AssetObj, type) == 0);
static_assert(offsetof(AssetObj, id) == 4);
static_assert(offsetof(AssetObj, manager) == 8);

static_assert(alignof(AudioSourceComponent) == 8);
static_assert(offsetof(AudioSourceComponent, playback) == 8);
static_assert(offsetof(AudioSourceComponent, playbackState.pan) == 16);
static_assert(offsetof(AudioSourceComponent, playbackState.volumeLinear) == 20);
static_assert(offsetof(AudioSourceComponent, clipID) == 24);

static_assert(alignof(Transform2DComponent) == 8);
static_assert(offsetof(Transform2DComponent, transform) == 8);

static_assert(alignof(Camera2DComponent) == 8);
static_assert(offsetof(Camera2DComponent, transform) == 8);
static_assert(offsetof(Camera2DComponent, camera) == 16);
static_assert(offsetof(Camera2DComponent, viewport) == 24);

static_assert(alignof(Sprite2DComponent) == 8);
static_assert(offsetof(Sprite2DComponent, transform) == 8);
static_assert(offsetof(Sprite2DComponent, draw) == 16);
static_assert(offsetof(Sprite2DComponent, assetID) == 32);

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
} TransformEx;

typedef struct __attribute__((aligned(4))) Transform2D {
    Vec2 position;
    Vec2 scale;
    float rotation;
} Transform2D;

typedef struct __attribute__((aligned(8))) AssetObj {
    uint32_t __private_type;
    uint32_t id;
    void* __private_asset_manager;
} AssetObj;

void* ffi_get_parent_id(void* compID);
void* ffi_get_child_id_by_name(void* compID, const char* name);

typedef struct MeshComponent {
    void* base;
    TransformEx* transform;
} MeshComponent;

typedef struct __attribute__((aligned(8))) AudioSourceComponent {
    void* base;
    void* __private_playback;
    float __private_pan;
    float __private_volumeLinear;
    uint32_t __private_clipAUID;
} AudioSourceComponent;

void ffi_audio_source_component_play(AudioSourceComponent* comp);
void ffi_audio_source_component_pause(AudioSourceComponent* comp);
void ffi_audio_source_component_resume(AudioSourceComponent* comp);
void ffi_audio_source_component_set_pan(AudioSourceComponent* comp, float pan);
void ffi_audio_source_component_set_volume_linear(AudioSourceComponent* comp, float volumeLinear);

typedef struct __attribute__((aligned(8))) Transform2DComponent {
    void* base;
    Transform2D* __private_transform;
} Transform2DComponent;

typedef struct __attribute__((aligned(8))) Camera2DComponent {
    void* base;
    Transform2D* __private_transform;
    void* __private_camera;
    Rect viewport;
} Camera2DComponent;

typedef struct __attribute__((aligned(8))) Sprite2DComponent {
    void* base;
    Transform2D* __private_transform;
    void* __private_drawL;
    void* __private_drawH;
    uint32_t __private_textureID;
} Sprite2DComponent;

void ffi_sprite_2d_component_get_pivot(Sprite2DComponent* comp);
void ffi_sprite_2d_component_set_pivot(Sprite2DComponent* comp, Vec2 pivot);
Rect ffi_sprite_2d_component_get_region(Sprite2DComponent* comp);
void ffi_sprite_2d_component_set_region(Sprite2DComponent* comp, Rect region);
double ffi_sprite_2d_component_get_z_depth(Sprite2DComponent* comp);
void ffi_sprite_2d_component_set_z_depth(Sprite2DComponent* comp, double zDepth);
void ffi_sprite_2d_component_set_texture(Sprite2DComponent* comp, uint32_t suid);
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
    __newindex = function (cdata, k, v)
        if k == 'pan' and tonumber(v) ~= nil then
            ffi.C.ffi_audio_source_component_set_pan(cdata, tonumber(v))
        elseif k == 'volume' and tonumber(v) ~= nil then
            ffi.C.ffi_audio_source_component_set_volume_linear(cdata, tonumber(v))
        end
    end,
})

ffi.metatype("Sprite2DComponent", {
    __index = function (cdata, k)
        if k == 'texture' then
            return _G.ludens.get_or_create_asset_ref(cdata.__private_textureID)
        elseif k == 'region' then
            return ffi.C.ffi_sprite_2d_component_get_region(cdata)
        elseif k == 'pivot' then
            ffi.C.ffi_sprite_2d_component_get_pivot(cdata)
        elseif k == 'z_depth' then
            return ffi.C.ffi_sprite_2d_component_get_z_depth(cdata)
        end
        return nil
    end,
    __newindex = function (cdata, k, v)
        if k == 'texture' then
            local assetRef = v
            ffi.C.ffi_sprite_2d_component_set_texture(cdata, assetRef.cdata.id)
        elseif k == 'region' then
            ffi.C.ffi_sprite_2d_component_set_region(cdata, v)
        elseif k == 'pivot' then
            ffi.C.ffi_sprite_2d_component_set_pivot(cdata, v)
        elseif k == 'z_depth' then
            ffi.C.ffi_sprite_2d_component_set_z_depth(cdata, v)
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

void* ffi_get_parent_id(void* compID)
{
    ComponentBase* base = sScene->active->registry.get_component_base(reinterpret_cast<uint64_t>(compID));
    LD_ASSERT(base);
    ComponentBase* parent = base->parent;
    return parent ? reinterpret_cast<void*>((uint64_t)parent->cuid) : (void*)0;
}

void* ffi_get_child_id_by_name(void* compID, const char* name)
{
    ComponentBase* base = sScene->active->registry.get_component_base(reinterpret_cast<uint64_t>(compID));
    LD_ASSERT(base && base->name);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        LD_ASSERT(child && child->name);

        if (!strcmp(child->name, name))
            return reinterpret_cast<void*>((uint64_t)child->cuid);
    }

    return 0;
}

void ffi_audio_source_component_play(AudioSourceComponent* comp)
{
    LD_ASSERT(comp && comp->base);

    AudioSourceView source(comp);

    if (source)
        source.play();
}

void ffi_audio_source_component_pause(AudioSourceComponent* comp)
{
    LD_ASSERT(comp && comp->base);

    AudioSourceView source(comp);

    if (source)
        source.pause();
}

void ffi_audio_source_component_resume(AudioSourceComponent* comp)
{
    AudioSourceView source(comp);

    if (source)
        source.resume();
}

void ffi_audio_source_component_set_pan(AudioSourceComponent* comp, float pan)
{
    if (!comp->playback)
        return;

    comp->playbackState.pan = std::clamp<float>(pan, 0.0f, 1.0f);
    comp->playback.store(comp->playbackState);
}

void ffi_audio_source_component_set_volume_linear(AudioSourceComponent* comp, float volumeLinear)
{
    if (!comp->playback)
        return;

    comp->playbackState.volumeLinear = std::clamp<float>(volumeLinear, 0.0f, 1.0f);
    comp->playback.store(comp->playbackState);
}

Vec2FFI ffi_sprite_2d_component_get_pivot(Sprite2DComponent* comp)
{
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    Vec2 pivot = sprite.get_pivot();
    return Vec2FFI(pivot.x, pivot.y);
}

void ffi_sprite_2d_component_set_pivot(Sprite2DComponent* comp, Vec2FFI pivot)
{
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    sprite.set_pivot(Vec2(pivot.x, pivot.y));
}

RectFFI ffi_sprite_2d_component_get_region(Sprite2DComponent* comp)
{
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    Rect region = sprite.get_region();
    return RectFFI(region.x, region.y, region.w, region.h);
}

void ffi_sprite_2d_component_set_region(Sprite2DComponent* comp, RectFFI region)
{
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    sprite.set_region(Rect(region.x, region.y, region.w, region.h));
}

double ffi_sprite_2d_component_get_z_depth(Sprite2DComponent* comp)
{
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    return (double)sprite.get_z_depth();
}

void ffi_sprite_2d_component_set_z_depth(Sprite2DComponent* comp, double zDepth)
{
    Sprite2DView sprite(comp);
    LD_ASSERT(sprite);

    uint32_t u32 = 0;
    if (zDepth > 0.0)
        u32 = std::clamp<uint32_t>((uint32_t)zDepth, 0, std::numeric_limits<uint32_t>::max());

    sprite.set_z_depth(u32);
}

void ffi_sprite_2d_component_set_texture(Sprite2DComponent* comp, uint32_t suid)
{
    Sprite2DView sprite(comp);

    if (sprite)
        sprite.set_texture_2d_asset(AssetID(suid));
}

} // extern "C"
} // namespace LuaScript
} // namespace LD