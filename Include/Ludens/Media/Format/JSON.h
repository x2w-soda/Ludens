#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/View.h>
#include <Ludens/System/FileSystem.h>

#include <cstdint>
#include <string>

namespace LD {

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

} // namespace LD
