#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/View.h>
#include <Ludens/System/FileSystem.h>

#include <cstdint>
#include <string>

namespace LD {

enum TOMLType
{
    TOML_TYPE_EMPTY = 0,
    TOML_TYPE_BOOL,
    TOML_TYPE_INT,
    TOML_TYPE_FLOAT,
    TOML_TYPE_STRING,
    TOML_TYPE_OFFSET_DATETIME,
    TOML_TYPE_LOCAL_DATETIME,
    TOML_TYPE_LOCAL_DATE,
    TOML_TYPE_LOCAL_TIME,
    TOML_TYPE_ARRAY,
    TOML_TYPE_TABLE,
};

struct TOMLValue : Handle<struct TOMLValueObj>
{
    /// @brief Get value data type.
    TOMLType type() const;

    inline bool is_bool() const { return type() == TOML_TYPE_BOOL; }
    inline bool is_int() const { return type() == TOML_TYPE_INT; }
    inline bool is_float() const { return type() == TOML_TYPE_FLOAT; }
    inline bool is_string() const { return type() == TOML_TYPE_STRING; }
    inline bool is_table() const { return type() == TOML_TYPE_TABLE; }
    inline bool is_array() const { return type() == TOML_TYPE_ARRAY; }

    /// @brief Check if value is a TOML bool.
    /// @param boolean Output boolean upon success.
    bool get_bool(bool& boolean) const;

    /// @brief Check if value is a TOML int that is castable to i64.
    /// @param i64 Output 64-bit signed integer upon success.
    bool get_i64(int64_t& i64) const;

    /// @brief Check if value is a TOML int that is castable to i32.
    /// @param i32 Output 32-bit signed integer upon success.
    bool get_i32(int32_t& i32) const;

    /// @brief Check if value is a TOML int that is castable to u32.
    /// @param u32 Output 32-bit unsigned integer upon success.
    bool get_u32(uint32_t& u32) const;

    /// @brief Check if value is a TOML floating point.
    /// @param f64 Output 64-bit floating point number on success.
    /// @note TOML integers will be implicitly casted to float.
    bool get_f64(double& f64) const;

    /// @brief Check if value is a TOML floating point.
    /// @param f32 Output 32-bit floating point number on success.
    /// @note TOML integers will be implicitly casted to float.
    bool get_f32(float& f32) const;

    /// @brief Check if value is a TOML string.
    /// @param string Output string upon success.
    bool get_string(std::string& string) const;

    /// @brief Get array size or table size.
    /// @return Non-negative size, or negative value on failure.
    int size();

    /// @brief Index into a TOML array.
    /// @param idx Array index.
    TOMLValue get_index(int idx);

    /// @brief Shorthand for array get_index.
    inline TOMLValue operator[](int idx)
    {
        return get_index(idx);
    }

    /// @brief Check if table contains a key.
    /// @param typeMatch If not null, checks if the value matches the type.
    /// @return True if value is table type, contains the key, and satisfies optional type matching.
    bool has_key(const char* key, const TOMLType* typeMatch);

    /// @brief Lookup key in TOML table.
    TOMLValue get_key(const char* key);

    /// @brief Lookup key in TOML table with expected type.
    TOMLValue get_key(const char* key, TOMLType type);

    /// @brief Shorthand for table get_key.
    inline TOMLValue operator[](const char* key)
    {
        return get_key(key);
    }

    /// @brief Get all keys in a table.
    int get_keys(Vector<std::string>& keys);
};

/// @brief TOML document handle.
struct TOMLDocument : Handle<struct TOMLDocumentObj>
{
    /// @brief Create empty TOML document.
    static TOMLDocument create();

    /// @brief Destroy TOML document, all TOML values from this document becomes out of date.
    static void destroy(TOMLDocument doc);

    /// @brief Get value under root TOML table.
    TOMLValue get(const char* name);
};

/// @brief TOML DOM parser.
struct TOMLParser
{
    static bool parse(TOMLDocument dst, const View& view, std::string& error);

    static bool parse_from_file(TOMLDocument dst, const FS::Path& path, std::string& error);
};

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

namespace TOMLUtil {

/// @brief Save rect as inline table.
/// @return True on success.
bool save_rect_table(const Rect& rect, TOMLWriter writer);

/// @brief Load rect from toml table value.
/// @return True on success.
bool load_rect_table(Rect& rect, TOMLValue table);

} // namespace TOMLUtil
} // namespace LD
