#pragma once

#include "LuaTest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Lua/LuaConfig.h>

using namespace LD;

#define TEST_LUA_CONFIG_NAME "test_config"

TEST_CASE("LuaConfig primitives")
{
    LuaConfig cfg;
    CHECK((bool)cfg == false);
    {
        LuaConfigValue values[] = {
            {"width", LUA_CONFIG_TYPE_I32, 1},
            {"height", LUA_CONFIG_TYPE_I32, 1},
            {"is_fullscreen", LUA_CONFIG_TYPE_BOOL, 1},
            {"fps_target", LUA_CONFIG_TYPE_F64, 1},
            {"app_name", LUA_CONFIG_TYPE_STRING, 1},
            {"camera2D_pos", LUA_CONFIG_TYPE_VEC2, 1},
            {"camera3D_pos", LUA_CONFIG_TYPE_VEC3, 1},
            {"clear_color", LUA_CONFIG_TYPE_VEC4, 1},
        };

        LuaConfigInfo cfgI;
        cfgI.name = TEST_LUA_CONFIG_NAME;
        cfgI.valueCount = sizeof(values) / sizeof(*values);
        cfgI.values = values;
        cfg = LuaConfig::create(cfgI);
    }
    CHECK((bool)cfg == true);

    const char* src = R"(return {
        width = 1600,
        height = 900,
        is_fullscreen = false,
        fps_target = 60.0,
        app_name = "ludens",
        camera2D_pos = { 3.141, -2.718 },
        camera3D_pos = { 1.0, 0.0, -3.0 },
        clear_color = { 0.1, 0.2, 0.3, 1.0 },
    })";

    bool result = cfg.load(src);
    CHECK(result);

    int ret;
    bool b;
    int32_t i32;
    Vec2 v2;
    Vec3 v3;
    Vec4 v4;
    double f64;
    const char* string;

    ret = cfg.get_i32("width", &i32);
    CHECK(ret == 1);
    CHECK(i32 == 1600);

    ret = cfg.get_i32("height", &i32);
    CHECK(ret == 1);
    CHECK(i32 == 900);

    ret = cfg.get_f64("fps_target", &f64);
    CHECK(ret == 1);
    CHECK(f64 == 60.0);

    ret = cfg.get_string("app_name", &string);
    CHECK(ret == 1);
    CHECK(strncmp(string, "ludens", 6) == 0);

    ret = cfg.get_bool("is_fullscreen", &b);
    CHECK(ret == 1);
    CHECK(b == false);

    ret = cfg.get_vec2("camera2D_pos", &v2);
    CHECK(ret == 1);
    CHECK(v2 == Vec2(3.141f, -2.718f));

    ret = cfg.get_vec3("camera3D_pos", &v3);
    CHECK(ret == 1);
    CHECK(v3 == Vec3(1.0f, 0.0f, -3.0f));

    ret = cfg.get_vec4("clear_color", &v4);
    CHECK(ret == 1);
    CHECK(v4 == Vec4(0.1f, 0.2f, 0.3f, 1.0f));

    LuaConfig::destroy(cfg);
}

TEST_CASE("LuaConfig array")
{
    LuaConfig cfg;
    CHECK((bool)cfg == false);
    {
        LuaConfigValue values[] = {
            {"foo", LUA_CONFIG_TYPE_F64, 3},  // this looks the same as a single LUA_CONFIG_TYPE_VEC3
            {"bar", LUA_CONFIG_TYPE_VEC3, 2}, // an array of Vec3
            {"nar", LUA_CONFIG_TYPE_BOOL, 3}, // an array of booleans
        };

        LuaConfigInfo cfgI;
        cfgI.name = TEST_LUA_CONFIG_NAME;
        cfgI.valueCount = sizeof(values) / sizeof(*values);
        cfgI.values = values;
        cfg = LuaConfig::create(cfgI);
    }
    CHECK((bool)cfg == true);

    const char* src = R"(
        local t = { 1.0, 2.0, 3.0 };
        return {
            foo = t,
            bar = { t, { t[1] + 3, t[2] + 3, t[3] + 3 } },
            nar = { true, false, nil },
        }
    )";
    cfg.load(src);

    int ret;

    double foo[3];
    ret = cfg.get_f64("foo", foo);
    CHECK(ret == 3);
    CHECK(foo[0] == 1.0);
    CHECK(foo[1] == 2.0);
    CHECK(foo[2] == 3.0);

    Vec3 bar[2];
    ret = cfg.get_vec3("bar", bar);
    CHECK(ret == 2);
    CHECK(bar[0] == Vec3(1.0f, 2.0f, 3.0f));
    CHECK(bar[1] == Vec3(4.0f, 5.0f, 6.0f));

    bool nar[3];
    ret = cfg.get_bool("nar", nar);
    CHECK(ret == 3);
    CHECK(nar[0] == true);
    CHECK(nar[1] == false);
    CHECK(nar[2] == false);
}

TEST_CASE("LuaConfig reload")
{
    LuaConfig cfg;
    CHECK((bool)cfg == false);
    {
        LuaConfigValue values[] = {
            {"width", LUA_CONFIG_TYPE_I32, 1},
            {"height", LUA_CONFIG_TYPE_I32, 1},
            {"name", LUA_CONFIG_TYPE_STRING, 1},
        };

        LuaConfigInfo cfgI;
        cfgI.name = TEST_LUA_CONFIG_NAME;
        cfgI.valueCount = sizeof(values) / sizeof(*values);
        cfgI.values = values;
        cfg = LuaConfig::create(cfgI);
    }
    CHECK((bool)cfg == true);

    const char* src1 = R"(return {
        width = 123,
        height = 456,
        name = "name1",
    })";
    cfg.load(src1);

    int ret;
    int32_t i32;
    const char* str;

    ret = cfg.get_i32("width", &i32);
    CHECK(ret == 1);
    CHECK(i32 == 123);

    ret = cfg.get_i32("height", &i32);
    CHECK(ret == 1);
    CHECK(i32 == 456);

    ret = cfg.get_string("name", &str);
    CHECK(ret == 1);
    CHECK(strncmp(str, "name1", 5) == 0);

    // reload with new lua source
    const char* src2 = R"(return {
        width = -123,
        -- height = 456,
        name = "name2",
    })";

    cfg.load(src2);

    ret = cfg.get_i32("width", &i32);
    CHECK(ret == 1);
    CHECK(i32 == -123);

    ret = cfg.get_i32("height", &i32);
    CHECK(ret == (int)LUA_CONFIG_ERROR_NOT_FOUND);

    ret = cfg.get_string("name", &str);
    CHECK(ret == 1);
    CHECK(strncmp(str, "name2", 5) == 0);

    LuaConfig::destroy(cfg);
}

TEST_CASE("LuaConfig runtime array length")
{
    // TODO: array length varying for each load()
}

TEST_CASE("LuaConfig errors")
{
    LuaConfigValue value;
    value.arrayCount = 1;
    value.name = "foo";
    value.type = LUA_CONFIG_TYPE_I32;

    LuaConfigInfo cfgI{};
    cfgI.name = TEST_LUA_CONFIG_NAME;
    cfgI.valueCount = 1;
    cfgI.values = &value;
    LuaConfig cfg = LuaConfig::create(cfgI);

    int32_t i32;

    // ignore all queries before first load
    int ret = cfg.get_i32("foo", &i32);
    CHECK(ret == (int)LUA_CONFIG_ERROR_NOT_LOADED);

    cfg.load("return { bar = 2 }");

    // value for "foo" not found in lua code
    ret = cfg.get_i32("foo", &i32);
    CHECK(ret == (int)LUA_CONFIG_ERROR_NOT_FOUND);

    // did not register "bar" in LuaConfigInfo, ignore the value in lua code
    ret = cfg.get_i32("bar", &i32);
    CHECK(ret == (int)LUA_CONFIG_ERROR_NOT_REGISTERED);

    LuaConfig::destroy(cfg);
}