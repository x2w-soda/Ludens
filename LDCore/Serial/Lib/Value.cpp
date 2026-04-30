#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Serial/Value.h>
#include <type_traits>

namespace LD {

// clang-format off
struct
{
    ValueType type;
    const char* cstr;
    size_t byteSize;
} sValueTable[] = {
    {VALUE_TYPE_F32,          "f32",          4},
    {VALUE_TYPE_F64,          "f64",          8},
    {VALUE_TYPE_I8,           "i8",           1},
    {VALUE_TYPE_U8,           "u8",           1},
    {VALUE_TYPE_I16,          "i16",          2},
    {VALUE_TYPE_U16,          "u16",          2},
    {VALUE_TYPE_I32,          "i32",          4},
    {VALUE_TYPE_U32,          "u32",          4},
    {VALUE_TYPE_I64,          "i64",          8},
    {VALUE_TYPE_U64,          "u64",          8},
    {VALUE_TYPE_BOOL,         "bool",         1},
    {VALUE_TYPE_VEC2,         "vec2",         8},
    {VALUE_TYPE_VEC3,         "vec3",         12},
    {VALUE_TYPE_VEC4,         "vec4",         16},
    {VALUE_TYPE_RECT,         "rect",         16},
    {VALUE_TYPE_STRING,       "string",       0},
    {VALUE_TYPE_TRANSFORM_2D, "transform_2d", sizeof(Transform2D)},
    {VALUE_TYPE_TRANSFORM,    "transform",    sizeof(TransformEx)},
};
// clang-format on

static_assert(IsTrivial<Value8>);
static_assert(sizeof(Value8) == 8);
static_assert(alignof(Value8) == 8);
static_assert(IsTrivial<Value16>);
static_assert(sizeof(Value16) == 16);
static_assert(alignof(Value16) == 8);
static_assert(sizeof(Value64) == 64);
static_assert(alignof(Value64) == 8);
static_assert(sizeof(sValueTable) / sizeof(*sValueTable) == VALUE_TYPE_ENUM_COUNT);

size_t get_value_byte_size(ValueType type)
{
    return sValueTable[(int)type].byteSize;
}

const char* get_value_cstr(ValueType type)
{
    return sValueTable[(int)type].cstr;
}

Value64::Value64(const Value64& other)
    : type(other.type)
{
    copy_from(other);
}

Value64::Value64(Value64&& other) noexcept
    : type(other.type)
{
    move_from(std::move(other));
}

Value64::Value64(const std::string& s)
    : type(VALUE_TYPE_STRING)
{
    new (&str) std::string(s);
}

Value64::Value64(float f32)
    : type(VALUE_TYPE_F32)
{
    v16.f32[0] = f32;
}

Value64::Value64(double f64)
    : type(VALUE_TYPE_F64)
{
    v16.f64[0] = f64;
}

Value64::Value64(uint32_t u32)
    : type(VALUE_TYPE_U32)
{
    v16.u32[0] = u32;
}

Value64::Value64(uint64_t u64)
    : type(VALUE_TYPE_U64)
{
    v16.u64[0] = u64;
}

Value64::Value64(bool b8)
    : type(VALUE_TYPE_BOOL)
{
    v16.b8[0] = b8;
}

Value64::Value64(Vec2 vec2)
    : type(VALUE_TYPE_VEC2)
{
    v16.f32[0] = vec2.x;
    v16.f32[1] = vec2.y;
}

Value64::Value64(Vec3 vec3)
    : type(VALUE_TYPE_VEC3)
{
    v16.f32[0] = vec3.x;
    v16.f32[1] = vec3.y;
    v16.f32[2] = vec3.z;
}

Value64::Value64(Vec4 vec4)
    : type(VALUE_TYPE_VEC4)
{
    v16.f32[0] = vec4.x;
    v16.f32[1] = vec4.y;
    v16.f32[2] = vec4.z;
    v16.f32[3] = vec4.w;
}

Value64::Value64(Rect rect)
    : type(VALUE_TYPE_RECT)
{
    v16.rect = rect;
}

Value64::Value64(Transform2D transform2D)
    : type(VALUE_TYPE_TRANSFORM_2D)
{
    this->transform2D = transform2D;
}

Value64::Value64(TransformEx transformEx)
    : type(VALUE_TYPE_TRANSFORM)
{
    this->transformEx = transformEx;
}

Value64::~Value64()
{
    destroy();
}

Value64& Value64::operator=(const Value64& other)
{
    if (this != &other)
    {
        destroy();
        type = other.type;
        copy_from(other);
    }

    return *this;
}

Value64& Value64::operator=(Value64&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        type = other.type;
        move_from(std::move(other));
    }

    return *this;
}

bool Value64::operator==(const Value64& other) const
{
    if (type != other.type)
        return false;

    bool eq = false;

    switch (type)
    {
    case VALUE_TYPE_F32:
        eq = is_equal_epsilon<float>(v16.f32[0], other.v16.f32[0]);
        break;
    case VALUE_TYPE_F64:
        eq = is_equal_epsilon<double>(v16.f64[0], other.v16.f64[0]);
        break;
    case VALUE_TYPE_I8:
    case VALUE_TYPE_U8:
    case VALUE_TYPE_BOOL:
        eq = !memcmp(&v16, &other.v16, 1);
        break;
    case VALUE_TYPE_I16:
    case VALUE_TYPE_U16:
        eq = !memcmp(&v16, &other.v16, 2);
        break;
    case VALUE_TYPE_I32:
    case VALUE_TYPE_U32:
        eq = !memcmp(&v16, &other.v16, 4);
        break;
    case VALUE_TYPE_I64:
    case VALUE_TYPE_U64:
        eq = !memcmp(&v16, &other.v16, 8);
        break;
    case VALUE_TYPE_VEC2:
        eq = is_equal_epsilon<float>(v16.f32[0], other.v16.f32[0]) &&
             is_equal_epsilon<float>(v16.f32[1], other.v16.f32[1]);
        break;
    case VALUE_TYPE_VEC3:
        eq = get_vec3() == other.get_vec3();
        break;
    case VALUE_TYPE_VEC4:
        eq = get_vec4() == other.get_vec4();
        break;
    case VALUE_TYPE_RECT:
        eq = v16.rect == other.v16.rect;
        break;
    case VALUE_TYPE_STRING:
        eq = str == other.str;
        break;
    case VALUE_TYPE_TRANSFORM_2D:
        eq = transform2D == other.transform2D;
        break;
    case VALUE_TYPE_TRANSFORM:
        break; // TODO:
    default:
        break;
    }

    return eq;
}

void Value64::set_u32(uint32_t u32)
{
    destroy();
    type = VALUE_TYPE_U32;
    v16.u32[0] = u32;
}

uint32_t Value64::get_u32() const
{
    LD_ASSERT(type == VALUE_TYPE_U32);

    return v16.u32[0];
}

void Value64::set_f32(float f32)
{
    destroy();
    type = VALUE_TYPE_F32;
    v16.f32[0] = f32;
}

float Value64::get_f32() const
{
    LD_ASSERT(type == VALUE_TYPE_F32);

    return v16.f32[0];
}

void Value64::set_f64(double f64)
{
    destroy();
    type = VALUE_TYPE_F64;
    v16.f64[0] = f64;
}

double Value64::get_f64() const
{
    LD_ASSERT(type == VALUE_TYPE_F64);

    return v16.f64[0];
}

void Value64::set_bool(bool b8)
{
    destroy();
    type = VALUE_TYPE_BOOL;
    v16.b8[0] = b8;
}

bool Value64::get_bool() const
{
    LD_ASSERT(type == VALUE_TYPE_BOOL);

    return v16.b8[0];
}

void Value64::set_vec2(Vec2 vec2)
{
    destroy();
    type = VALUE_TYPE_VEC2;
    v16.f32[0] = vec2.x;
    v16.f32[1] = vec2.y;
}

Vec2 Value64::get_vec2() const
{
    LD_ASSERT(type == VALUE_TYPE_VEC2);

    return Vec2(v16.f32[0], v16.f32[1]);
}

void Value64::set_vec3(Vec3 vec3)
{
    destroy();
    type = VALUE_TYPE_VEC3;
    v16.f32[0] = vec3.x;
    v16.f32[1] = vec3.y;
    v16.f32[2] = vec3.z;
}

Vec3 Value64::get_vec3() const
{
    LD_ASSERT(type == VALUE_TYPE_VEC3);

    return Vec3(v16.f32[0], v16.f32[1], v16.f32[2]);
}

void Value64::set_vec4(Vec4 vec4)
{
    destroy();
    type = VALUE_TYPE_VEC4;
    v16.f32[0] = vec4.x;
    v16.f32[1] = vec4.y;
    v16.f32[2] = vec4.z;
    v16.f32[3] = vec4.w;
}

Vec4 Value64::get_vec4() const
{
    LD_ASSERT(type == VALUE_TYPE_VEC4);

    return Vec4(v16.f32[0], v16.f32[1], v16.f32[2], v16.f32[3]);
}

void Value64::set_rect(Rect rect)
{
    destroy();
    type = VALUE_TYPE_RECT;
    v16.rect = rect;
}

Rect Value64::get_rect() const
{
    LD_ASSERT(type == VALUE_TYPE_RECT);

    return v16.rect;
}

void Value64::set_transform_2d(const Transform2D& transform)
{
    destroy();
    type = VALUE_TYPE_TRANSFORM_2D;
    transform2D = transform;
}

Transform2D Value64::get_transform_2d() const
{
    LD_ASSERT(type == VALUE_TYPE_TRANSFORM_2D);

    return transform2D;
}

void Value64::set_transform(const TransformEx& transform)
{
    destroy();
    type = VALUE_TYPE_TRANSFORM;
    transformEx = transform;
}

TransformEx Value64::get_transform() const
{
    LD_ASSERT(type == VALUE_TYPE_TRANSFORM);

    return transformEx;
}

void Value64::set_string(const std::string& s)
{
    destroy();
    type = VALUE_TYPE_STRING;
    new (&str) std::string(s);
}

std::string Value64::get_string() const
{
    LD_ASSERT(type == VALUE_TYPE_STRING);

    return str;
}

void Value64::destroy()
{
    if (type == VALUE_TYPE_STRING)
        str.~basic_string();

    type = VALUE_TYPE_ENUM_COUNT;
}

void Value64::copy_from(const Value64& other)
{
    LD_ASSERT(type == other.type);

    switch (other.type)
    {
    case VALUE_TYPE_STRING:
        new (&str) std::string(other.str);
        break;
    case VALUE_TYPE_TRANSFORM_2D:
        transform2D = other.transform2D;
        break;
    case VALUE_TYPE_TRANSFORM:
        transformEx = other.transformEx;
        break;
    case VALUE_TYPE_ENUM_COUNT:
        break;
    default:
        v16 = other.v16;
        break;
    }
}

void Value64::move_from(Value64&& other)
{
    switch (other.type)
    {
    case VALUE_TYPE_STRING:
        new (&str) std::string(std::move(other.str));
        break;
    case VALUE_TYPE_TRANSFORM_2D:
        transform2D = std::move(other.transform2D);
        break;
    case VALUE_TYPE_TRANSFORM:
        transformEx = std::move(other.transformEx);
        break;
    case VALUE_TYPE_ENUM_COUNT:
        break;
    default:
        v16 = other.v16;
        break;
    }

    other.destroy();
}

} // namespace LD