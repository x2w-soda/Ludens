#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
#include <Ludens/Serial/Serial.h>

namespace {

struct Vec2
{
    float x, y;

    static void serialize(LD::Serializer& serializer, const Vec2& foo)
    {
        serializer.write_f32(foo.x);
        serializer.write_f32(foo.y);
    }

    static void deserialize(LD::Serializer& serializer, Vec2& foo)
    {
        serializer.read_f32(foo.x);
        serializer.read_f32(foo.y);
    }
};

struct Rect
{
    Vec2 min;
    Vec2 max;

    static void serialize(LD::Serializer& serializer, const Rect& rect)
    {
        Vec2::serialize(serializer, rect.min);
        Vec2::serialize(serializer, rect.max);
    }

    static void deserialize(LD::Serializer& serializer, Rect& rect)
    {
        Vec2::deserialize(serializer, rect.min);
        Vec2::deserialize(serializer, rect.max);
    }
};

} // namespace

TEST_CASE("struct serialization")
{
    Vec2 foo{2.0f, 3.0f};

    LD::Serializer serial;

    LD::serialize(serial, foo);
    CHECK(serial.size() == 8);

    Vec2 restore;
    LD::deserialize(serial, restore);
    CHECK(restore.x == foo.x);
    CHECK(restore.y == foo.y);
}

TEST_CASE("aggregate serialization")
{
    Rect rect;
    rect.min = {2.0f, 3.0f};
    rect.max = {8.0f, 24.0f};

    LD::Serializer serial;

    LD::serialize(serial, rect);
    CHECK(serial.size() == 16);

    Rect restore;
    LD::deserialize(serial, restore);

    CHECK(restore.min.x == rect.min.x);
    CHECK(restore.min.y == rect.min.y);
    CHECK(restore.max.x == rect.max.x);
    CHECK(restore.max.y == rect.max.y);
}