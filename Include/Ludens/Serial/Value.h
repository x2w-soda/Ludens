#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Transform.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace LD {

enum ValueType : uint16_t
{
    VALUE_TYPE_F32 = 0,
    VALUE_TYPE_F64,
    VALUE_TYPE_I8,
    VALUE_TYPE_U8,
    VALUE_TYPE_I16,
    VALUE_TYPE_U16,
    VALUE_TYPE_I32,
    VALUE_TYPE_U32,
    VALUE_TYPE_I64,
    VALUE_TYPE_U64,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_VEC2,
    VALUE_TYPE_VEC3,
    VALUE_TYPE_VEC4,
    VALUE_TYPE_RECT,
    VALUE_TYPE_STRING,
    VALUE_TYPE_TRANSFORM_2D,
    VALUE_TYPE_TRANSFORM,
    VALUE_TYPE_ENUM_COUNT,
};

union Value8
{
    bool b8;
    float f32;
    double f64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
};

union Value16
{
    bool b8[16];
    float f32[4];
    double f64[2];
    int16_t i16[8];
    int32_t i32[4];
    int64_t i64[2];
    uint16_t u16[8];
    uint32_t u32[4];
    uint64_t u64[2];
    Rect rect;
};

class Value64
{
public:
    ValueType type = VALUE_TYPE_ENUM_COUNT;

    union
    {
        Value16 v16;
        Transform2D transform2D;
        TransformEx transformEx;
        std::string str;
    };

    Value64() {}
    Value64(const Value64&);
    Value64(Value64&&) noexcept;
    explicit Value64(const char* cstr);
    explicit Value64(const std::string& str);
    explicit Value64(float f32);
    explicit Value64(double f64);
    explicit Value64(uint32_t u32);
    explicit Value64(uint64_t u64);
    explicit Value64(bool b8);
    explicit Value64(Vec2 vec2);
    explicit Value64(Vec3 vec3);
    explicit Value64(Vec4 vec4);
    explicit Value64(Rect rect);
    explicit Value64(Transform2D transform2D);
    explicit Value64(TransformEx transformEx);
    ~Value64();

    Value64& operator=(const Value64&);
    Value64& operator=(Value64&&) noexcept;

    bool operator==(const Value64& other) const;

    void set_u32(uint32_t u32);
    uint32_t get_u32() const;
    void set_f32(float f32);
    float get_f32() const;
    void set_f64(double f64);
    double get_f64() const;
    void set_bool(bool b8);
    bool get_bool() const;
    void set_vec2(Vec2 vec2);
    Vec2 get_vec2() const;
    void set_vec3(Vec3 vec3);
    Vec3 get_vec3() const;
    void set_vec4(Vec4 vec4);
    Vec4 get_vec4() const;
    void set_rect(Rect rect);
    Rect get_rect() const;
    void set_transform_2d(const Transform2D& transform);
    Transform2D get_transform_2d() const;
    void set_transform(const TransformEx& transform);
    TransformEx get_transform() const;
    void set_string(const std::string& str);
    std::string get_string() const;

private:
    void destroy();
    void copy_from(const Value64& other);
    void move_from(Value64&& other);
};

/// @brief Get byte size of value type, or zero for varying length values.
size_t get_value_byte_size(ValueType type);

/// @brief Get static C string for value type name.
const char* get_value_cstr(ValueType type);

} // namespace LD
