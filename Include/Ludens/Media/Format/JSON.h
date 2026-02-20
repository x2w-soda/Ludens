#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Header/View.h>
#include <Ludens/System/FileSystem.h>

#include <cstdint>
#include <string>

namespace LD {

struct Transform2D;
struct TransformEx;

enum JSONType
{
    JSON_TYPE_NULL = 0,
    JSON_TYPE_FALSE,
    JSON_TYPE_TRUE,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY,
    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
};

struct JSONWriter : Handle<struct JSONWriterObj>
{
    static JSONWriter create();
    static void destroy(JSONWriter writer);

    bool begin();
    bool end(std::string& outString);

    bool begin_array();
    bool end_array();

    bool begin_object();
    bool end_object();

    bool key(const char* name);
    bool key(const std::string& str);
    bool write_bool(bool b);
    bool write_i32(int32_t i32);
    bool write_i64(int64_t i64);
    bool write_u32(uint32_t u32);
    bool write_f32(float f32);
    bool write_f64(double f64);
    bool write_string(const char* cstr);
    bool write_string(const std::string& str);
};

struct JSONReader : Handle<struct JSONReaderObj>
{
    static JSONReader create(const View& json, std::string& err);
    static void destroy(JSONReader reader);

    bool is_object_scope();
    bool is_array_scope();

    bool enter_root_object();
    bool enter_root_array(int& size);
    bool enter_object(const char* key);
    bool enter_object(int index);
    bool enter_array(const char* key, int& size);
    bool enter_array(int index, int& size);
    void exit();

    bool read_bool(const char* key, bool& b);
    bool read_bool(int index, bool& b);
    bool read_i32(const char* key, int32_t& i32);
    bool read_i32(int index, int32_t& i32);
    bool read_i64(const char* key, int64_t& i64);
    bool read_i64(int index, int64_t& i64);
    bool read_u32(const char* key, uint32_t& u32);
    bool read_u32(int index, uint32_t& u32);
    bool read_u64(const char* key, uint64_t& u64);
    bool read_u64(int index, uint64_t& u64);
    bool read_f32(const char* key, float& f32);
    bool read_f32(int index, float& f32);
    bool read_string(const char* key, std::string& str);
    bool read_string(int index, std::string& str);
};

struct JSONCallback
{
    bool (*onEnterObject)(void* user);
    bool (*onLeaveObject)(size_t memberCount, void* user);

    // Object key callback. The view is a transient UTF-8 bytestream.
    bool (*onKey)(const View& key, void* user);

    bool (*onEnterArray)(void* user);
    bool (*onLeaveArray)(size_t elementCount, void* user);

    bool (*onNull)(void* user);
    bool (*onBool)(bool b, void* user);
    bool (*onI64)(int64_t i64, void* user);
    bool (*onU64)(uint64_t u64, void* user);
    bool (*onF64)(double f64, void* user);

    // String value callback. The view is a transient UTF-8 bytestream.
    bool (*onString)(const View& string, void* user);
};

/// @brief JSON event based parser, user provides callbacks.
struct JSONParser
{
    static bool parse(const View& json, std::string& error, const JSONCallback& callbacks, void* user);
};

namespace JSONUtil {

bool write_transform(JSONWriter writer, const char* key, const TransformEx& transform);
bool read_transform(JSONReader reader, const char* key, TransformEx& transform);
bool write_transform_2d(JSONWriter writer, const char* key, const Transform2D& transform);
bool read_transform_2d(JSONReader reader, const char* key, Transform2D& transform);
bool write_rect(JSONWriter writer, const char* key, const Rect& rect);
bool read_rect(JSONReader reader, const char* key, Rect& rect);
bool write_vec3(JSONWriter writer, const char* key, const Vec3& vec3);
bool read_vec3(JSONReader reader, const char* key, Vec3& vec3);
bool write_vec2(JSONWriter writer, const char* key, const Vec2& vec2);
bool read_vec2(JSONReader reader, const char* key, Vec2& vec2);

} // namespace JSONUtil
} // namespace LD
