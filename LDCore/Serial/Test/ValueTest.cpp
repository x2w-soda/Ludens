#include <Extra/doctest/doctest.h>
#include <Ludens/Serial/Value.h>

using namespace LD;

TEST_CASE("Value64 sanity check")
{
    Value64 val;
    CHECK(val.type == VALUE_TYPE_ENUM_COUNT);

    val = Value64(3.0f);
    CHECK(val.type == VALUE_TYPE_F32);
    CHECK(val.get_f32() == 3.0f);
    val.set_f32(-3.14f);
    CHECK(is_equal_epsilon(val.get_f32(), -3.14f));

    val = Value64(6.0);
    CHECK(val.type == VALUE_TYPE_F64);
    CHECK(val.get_f64() == 6.0);
    val.set_f64(-3.14);
    CHECK(is_equal_epsilon(val.get_f64(), -3.14));

    val = Value64((uint32_t)30);
    CHECK(val.type == VALUE_TYPE_U32);
    CHECK(val.get_u32() == 30);
    val.set_u32(100);
    CHECK(val.get_u32() == 100);

    val = Value64(Vec2(1.0f, -2.0f));
    CHECK(val.type == VALUE_TYPE_VEC2);
    CHECK(val.get_vec2() == Vec2(1.0f, -2.0f));
    val.set_vec2(Vec2(1.5f, 2.5f));
    CHECK(val.get_vec2() == Vec2(1.5f, 2.5f));

    val = Value64(Vec3(1.0f, -2.0f, 3.0f));
    CHECK(val.type == VALUE_TYPE_VEC3);
    CHECK(val.get_vec3() == Vec3(1.0f, -2.0f, 3.0f));
    val.set_vec3(Vec3(1.5f, 2.5f, -3.5f));
    CHECK(val.get_vec3() == Vec3(1.5f, 2.5f, -3.5f));

    val = Value64(Vec4(1.0f, -2.0f, 3.0f, -4.0f));
    CHECK(val.type == VALUE_TYPE_VEC4);
    CHECK(val.get_vec4() == Vec4(1.0f, -2.0f, 3.0f, -4.0f));
    val.set_vec4(Vec4(1.5f, 2.5f, -3.5f, -4.5f));
    CHECK(val.get_vec4() == Vec4(1.5f, 2.5f, -3.5f, -4.5f));

    val = Value64(Rect(1.0f, 2.0f, 3.0f, 4.0f));
    CHECK(val.type == VALUE_TYPE_RECT);
    CHECK(val.get_rect() == Rect(1.0f, 2.0f, 3.0f, 4.0f));
    val.set_rect(Rect(1.5f, 2.5f, 3.5f, 4.5f));
    CHECK(val.get_rect() == Rect(1.5f, 2.5f, 3.5f, 4.5f));

    val = Value64("hello");
    CHECK(val.type == VALUE_TYPE_STRING);
    CHECK(val.get_string() == "hello");
    val.set_string("world");
    CHECK(val.get_string() == "world");
}
