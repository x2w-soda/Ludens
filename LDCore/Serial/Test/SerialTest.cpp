#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
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

    static bool serialize(Serializer& serial, const Foo& foo)
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

        return true;
    }

    static bool deserialize(Deserializer& serial, Foo& foo)
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

        return true;
    }
};

TEST_CASE("integer serialization")
{
    Serializer serializer;
    serializer.write_i8(std::numeric_limits<int8_t>::min());
    serializer.write_i8(std::numeric_limits<int8_t>::max());
    serializer.write_i16(std::numeric_limits<int16_t>::min());
    serializer.write_i16(std::numeric_limits<int16_t>::max());
    serializer.write_i32(std::numeric_limits<int32_t>::min());
    serializer.write_i32(std::numeric_limits<int32_t>::max());
    serializer.write_i64(std::numeric_limits<int64_t>::min());
    serializer.write_i64(std::numeric_limits<int64_t>::max());
    CHECK(serializer.size() == 30);

    serializer.write_u8(std::numeric_limits<uint8_t>::min());
    serializer.write_u8(std::numeric_limits<uint8_t>::max());
    serializer.write_u16(std::numeric_limits<uint16_t>::min());
    serializer.write_u16(std::numeric_limits<uint16_t>::max());
    serializer.write_u32(std::numeric_limits<uint32_t>::min());
    serializer.write_u32(std::numeric_limits<uint32_t>::max());
    serializer.write_u64(std::numeric_limits<uint64_t>::min());
    serializer.write_u64(std::numeric_limits<uint64_t>::max());
    CHECK(serializer.size() == 60);

    size_t dataSize;
    const byte* data = serializer.view(dataSize);
    Deserializer deserializer(data, dataSize);

    int8_t i8min, i8max;
    int16_t i16min, i16max;
    int32_t i32min, i32max;
    int64_t i64min, i64max;
    deserializer.read_i8(i8min);
    deserializer.read_i8(i8max);
    deserializer.read_i16(i16min);
    deserializer.read_i16(i16max);
    deserializer.read_i32(i32min);
    deserializer.read_i32(i32max);
    deserializer.read_i64(i64min);
    deserializer.read_i64(i64max);
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
    deserializer.read_u8(u8min);
    deserializer.read_u8(u8max);
    deserializer.read_u16(u16min);
    deserializer.read_u16(u16max);
    deserializer.read_u32(u32min);
    deserializer.read_u32(u32max);
    deserializer.read_u64(u64min);
    deserializer.read_u64(u64max);
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

    size_t dataSize;
    const byte* data = serial.view(dataSize);
    Deserializer deserializer(data, dataSize);

    float f32;
    double f64;
    deserializer.read_f32(f32);
    deserializer.read_f64(f64);
    deserializer.read_vec2(v2);
    deserializer.read_vec3(v3);
    deserializer.read_vec4(v4);
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

    size_t dataSize;
    const byte* data = serial.view(dataSize);
    Deserializer deserializer(data, dataSize);

    Foo f2;
    deserialize(deserializer, f2);
    CHECK(f2.name == "gameobject");
    CHECK(f2.health == 100);
    CHECK(f2.hitbox == f.hitbox);
    CHECK(f2.hurtbox == f.hurtbox);
}

TEST_CASE("chunk serialization")
{
    Foo f;
    f.name = "object";
    f.health = 123;
    f.hitbox = {1.0f, 2.0f, 3.0f, 4.0f};
    f.hurtbox = {5.0f, 6.0f, 7.0f, 8.0f};

    uint32_t chunkSize;
    Serializer serial;
    serial.write_chunk_begin("Foo.");
    serial.write_i32(1234);
    {
        serial.write_chunk_begin("Foo1");
        serialize(serial, f);
        chunkSize = serial.write_chunk_end();
        CHECK(chunkSize == 46);
    }
    serial.write_i32(5678);
    chunkSize = serial.write_chunk_end();
    CHECK(chunkSize == 62);

    std::string name;
    name.resize(4);

    size_t dataSize;
    const byte* data = serial.view(dataSize);
    Deserializer deserializer(data, dataSize);

    deserializer.read_chunk(name.data(), chunkSize);
    CHECK(chunkSize == 62);
    CHECK(name == "Foo.");

    int32_t i32;
    deserializer.read_i32(i32);
    CHECK(i32 == 1234);

    deserializer.read_chunk(name.data(), chunkSize);
    CHECK(chunkSize == 46);
    CHECK(name == "Foo1");

    Foo f2;
    deserialize(deserializer, f2);
    CHECK(f2.name == "object");
    CHECK(f2.health == 123);
    CHECK(f2.hitbox == f.hitbox);
    CHECK(f2.hurtbox == f.hurtbox);

    deserializer.read_i32(i32);
    CHECK(i32 == 5678);
}