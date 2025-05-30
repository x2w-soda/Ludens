#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
#include "TestCompression.h"
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Serial/Serial.h>
#include <limits>
#include <string>

using namespace LD;

struct Foo
{
    std::string name;
    Rect hitbox;
    Rect hurtbox;
    int32_t health;

    static void serialize(Serializer& serial, const Foo& foo)
    {
        serial.write_u32((uint32_t)foo.name.size());
        serial.write((byte*)foo.name.data(), foo.name.size());
        serial.write_i32(foo.health);
        serial.write_f32(foo.hitbox.x);
        serial.write_f32(foo.hitbox.y);
        serial.write_f32(foo.hitbox.w);
        serial.write_f32(foo.hitbox.h);
        serial.write_f32(foo.hurtbox.x);
        serial.write_f32(foo.hurtbox.y);
        serial.write_f32(foo.hurtbox.w);
        serial.write_f32(foo.hurtbox.h);
    }

    static void deserialize(Serializer& serial, Foo& foo)
    {
        uint32_t nameLen;
        serial.read_u32(nameLen);
        foo.name.resize(nameLen);
        serial.read((byte*)foo.name.data(), nameLen);
        serial.read_i32(foo.health);
        serial.read_f32(foo.hitbox.x);
        serial.read_f32(foo.hitbox.y);
        serial.read_f32(foo.hitbox.w);
        serial.read_f32(foo.hitbox.h);
        serial.read_f32(foo.hurtbox.x);
        serial.read_f32(foo.hurtbox.y);
        serial.read_f32(foo.hurtbox.w);
        serial.read_f32(foo.hurtbox.h);
    }
};

TEST_CASE("integer serialization")
{
    Serializer serial;
    serial.write_i8(std::numeric_limits<int8_t>::min());
    serial.write_i8(std::numeric_limits<int8_t>::max());
    serial.write_i16(std::numeric_limits<int16_t>::min());
    serial.write_i16(std::numeric_limits<int16_t>::max());
    serial.write_i32(std::numeric_limits<int32_t>::min());
    serial.write_i32(std::numeric_limits<int32_t>::max());
    serial.write_i64(std::numeric_limits<int64_t>::min());
    serial.write_i64(std::numeric_limits<int64_t>::max());
    CHECK(serial.size() == 30);

    serial.write_u8(std::numeric_limits<uint8_t>::min());
    serial.write_u8(std::numeric_limits<uint8_t>::max());
    serial.write_u16(std::numeric_limits<uint16_t>::min());
    serial.write_u16(std::numeric_limits<uint16_t>::max());
    serial.write_u32(std::numeric_limits<uint32_t>::min());
    serial.write_u32(std::numeric_limits<uint32_t>::max());
    serial.write_u64(std::numeric_limits<uint64_t>::min());
    serial.write_u64(std::numeric_limits<uint64_t>::max());
    CHECK(serial.size() == 60);

    int8_t i8min, i8max;
    int16_t i16min, i16max;
    int32_t i32min, i32max;
    int64_t i64min, i64max;
    serial.read_i8(i8min);
    serial.read_i8(i8max);
    serial.read_i16(i16min);
    serial.read_i16(i16max);
    serial.read_i32(i32min);
    serial.read_i32(i32max);
    serial.read_i64(i64min);
    serial.read_i64(i64max);
    CHECK(i8min == std::numeric_limits<int8_t>::min());
    CHECK(i8max == std::numeric_limits<int8_t>::max());
    CHECK(i16min == std::numeric_limits<int16_t>::min());
    CHECK(i16max == std::numeric_limits<int16_t>::max());
    CHECK(i32min == std::numeric_limits<int32_t>::min());
    CHECK(i32max == std::numeric_limits<int32_t>::max());
    CHECK(i64min == std::numeric_limits<int64_t>::min());
    CHECK(i64max == std::numeric_limits<int64_t>::max());

    uint8_t u8min, u8max;
    uint16_t u16min, u16max;
    uint32_t u32min, u32max;
    uint64_t u64min, u64max;
    serial.read_u8(u8min);
    serial.read_u8(u8max);
    serial.read_u16(u16min);
    serial.read_u16(u16max);
    serial.read_u32(u32min);
    serial.read_u32(u32max);
    serial.read_u64(u64min);
    serial.read_u64(u64max);
    CHECK(u8min == std::numeric_limits<uint8_t>::min());
    CHECK(u8max == std::numeric_limits<uint8_t>::max());
    CHECK(u16min == std::numeric_limits<uint16_t>::min());
    CHECK(u16max == std::numeric_limits<uint16_t>::max());
    CHECK(u32min == std::numeric_limits<uint32_t>::min());
    CHECK(u32max == std::numeric_limits<uint32_t>::max());
    CHECK(u64min == std::numeric_limits<uint64_t>::min());
    CHECK(u64max == std::numeric_limits<uint64_t>::max());
}

TEST_CASE("floating point serialization")
{
    Vec2 v2(2.0f, 3.0f);
    Vec3 v3(v2, 4.0f);
    Vec4 v4(v3, 5.0f);

    Serializer serial;
    serial.write_f32(3.14f);
    serial.write_f64(3.1415926535);
    serial.write_vec2(v2);
    serial.write_vec3(v3);
    serial.write_vec4(v4);
    CHECK(serial.size() == 48);

    float f32;
    double f64;
    serial.read_f32(f32);
    serial.read_f64(f64);
    serial.read_vec2(v2);
    serial.read_vec3(v3);
    serial.read_vec4(v4);
    CHECK(f32 == 3.14f);
    CHECK(f64 == 3.1415926535);
    CHECK(v2 == Vec2(2.0f, 3.0f));
    CHECK(v3 == Vec3(2.0f, 3.0f, 4.0f));
    CHECK(v4 == Vec4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST_CASE("struct serialization")
{
    Foo f;
    f.name = "gameobject";
    f.health = 100;
    f.hitbox = {1.0f, 2.0f, 3.0f, 4.0f};
    f.hurtbox = {5.0f, 6.0f, 7.0f, 8.0f};

    Serializer serial;
    serialize(serial, f);

    Foo f2;
    deserialize(serial, f2);

    CHECK(f2.name == "gameobject");
    CHECK(f2.health == 100);
    CHECK(f2.hitbox.x == f.hitbox.x);
    CHECK(f2.hitbox.y == f.hitbox.y);
    CHECK(f2.hitbox.w == f.hitbox.w);
    CHECK(f2.hitbox.h == f.hitbox.h);
    CHECK(f2.hurtbox.x == f.hurtbox.x);
    CHECK(f2.hurtbox.y == f.hurtbox.y);
    CHECK(f2.hurtbox.w == f.hurtbox.w);
    CHECK(f2.hurtbox.h == f.hurtbox.h);
}