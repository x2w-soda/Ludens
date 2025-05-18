#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Serial/Serial.h>
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

TEST_CASE("Serializer API")
{
    Vec2 v2(2.0f, 3.0f);
    Vec3 v3(v2, 4.0f);
    Vec4 v4(v3, 5.0f);

    Serializer serial;
    serial.write_f32(3.14f);
    serial.write_vec2(v2);
    serial.write_vec3(v3);
    serial.write_vec4(v4);
    CHECK(serial.size() == sizeof(float) * 10);

    float f;
    serial.read_f32(f);
    serial.read_vec2(v2);
    serial.read_vec3(v3);
    serial.read_vec4(v4);
    CHECK(f == 3.14f);
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