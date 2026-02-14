#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Header/View.h>
#include <Ludens/System/FileSystem.h>

#include <cstdint>
#include <string>

namespace LD {

struct TransformEx;
struct Transform2D;

struct TOMLWriter : Handle<struct TOMLWriterObj>
{
    static TOMLWriter create();
    static void destroy(TOMLWriter writer);

    bool is_array_scope();
    bool is_table_scope();
    bool is_inline_table_scope();
    bool is_array_table_scope();

    TOMLWriter begin();
    TOMLWriter end(std::string& outString);

    inline TOMLWriter begin_array(const char* name) { return key(name).begin_array(); }
    TOMLWriter begin_array();
    TOMLWriter end_array();

    inline TOMLWriter begin_table(const char* name) { return key(name).begin_table(); }
    TOMLWriter begin_table();
    TOMLWriter end_table();

    inline TOMLWriter begin_inline_table(const char* name) { return key(name).begin_inline_table(); }
    TOMLWriter begin_inline_table();
    TOMLWriter end_inline_table();

    TOMLWriter begin_array_table(const char* name);
    TOMLWriter end_array_table();

    TOMLWriter key(const char* name);
    TOMLWriter key(const std::string& str);
    TOMLWriter value_bool(bool b);
    TOMLWriter value_i32(int32_t i32);
    TOMLWriter value_i64(int64_t i64);
    TOMLWriter value_u32(uint32_t u32);
    TOMLWriter value_f32(float f32);
    TOMLWriter value_f64(double f64);
    TOMLWriter value_string(const char* cstr);
    TOMLWriter value_string(const std::string& str);
};

struct TOMLReader : Handle<struct TOMLReaderObj>
{
    static TOMLReader create(View toml, std::string& err);
    static void destroy(TOMLReader reader);

    bool is_array_scope();
    bool is_table_scope();

    bool enter_array(const char* key, int& arraySize);
    bool enter_table(const char* key);
    bool enter_table(int index);
    void exit();

    void get_keys(Vector<std::string>& keys);

    bool read_bool(const char* key, bool& b);
    bool read_bool(int index, bool& b);
    bool read_i32(const char* key, int32_t& i32);
    bool read_i32(int index, int32_t& i32);
    bool read_i64(const char* key, int64_t& i64);
    bool read_i64(int index, int64_t& i64);
    bool read_u32(const char* key, uint32_t& u32);
    bool read_u32(int index, uint32_t& u32);
    bool read_f32(const char* key, float& f32);
    bool read_f32(int index, float& f32);
    bool read_f64(const char* key, double& f64);
    bool read_f64(int index, double& f64);
    bool read_string(const char* key, std::string& str);
    bool read_string(int index, std::string& str);
};

namespace TOMLUtil {

bool write_transform(TOMLWriter writer, const char* key, const TransformEx& transform);
bool read_transform(TOMLReader reader, const char* key, TransformEx& transform);
bool write_transform_2d(TOMLWriter writer, const char* key, const Transform2D& transform);
bool read_transform_2d(TOMLReader reader, const char* key, Transform2D& transform);

/// @brief Save rect as inline table.
bool write_rect(TOMLWriter writer, const char* key, const Rect& rect);

/// @brief Load rect from toml table value.
bool read_rect(TOMLReader reader, const char* key, Rect& rect);

/// @brief Save Vec3 as inline table.
bool write_vec3(TOMLWriter writer, const char* key, const Vec3& vec3);

/// @brief Load Vec3 from toml table.
bool read_vec3(TOMLReader reader, const char* key, Vec3& vec3);

/// @brief Save Vec2 as inline table.
bool write_vec2(TOMLWriter writer, const char* key, const Vec2& vec2);

/// @brief Load Vec2 from toml table.
bool read_vec2(TOMLReader reader, const char* key, Vec2& vec2);

} // namespace TOMLUtil
} // namespace LD
