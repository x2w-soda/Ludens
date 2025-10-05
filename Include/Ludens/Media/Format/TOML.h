#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <cstdint>
#include <string>
#include <vector>

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
    TOMLType get_type() const;

    inline bool is_bool_type() const { return get_type() == TOML_TYPE_BOOL; }
    inline bool is_int_type() const { return get_type() == TOML_TYPE_INT; }
    inline bool is_float_type() const { return get_type() == TOML_TYPE_FLOAT; }
    inline bool is_string_type() const { return get_type() == TOML_TYPE_STRING; }
    inline bool is_table_type() const { return get_type() == TOML_TYPE_TABLE; }
    inline bool is_array_type() const { return get_type() == TOML_TYPE_ARRAY; }

    /// @brief Check if value is a TOML bool.
    /// @param boolean Output boolean upon success.
    bool is_bool(bool& boolean) const;

    /// @brief Set TOML boolean value.
    /// @return True on success.
    bool set_bool(bool boolean);

    /// @brief Check if value is a TOML int that is castable to i64.
    /// @param i64 Output 64-bit signed integer upon success.
    bool is_i64(int64_t& i64) const;

    /// @brief Set TOML 64-bit int value.
    /// @return True on success.
    bool set_i64(int64_t i64);

    /// @brief Check if value is a TOML int that is castable to i32.
    /// @param i32 Output 32-bit signed integer upon success.
    bool is_i32(int32_t& i32) const;

    /// @brief Set TOML 32-bit int value.
    /// @return True on success.
    bool set_i32(int32_t i32);

    /// @brief Check if value is a TOML int that is castable to u32.
    /// @param u32 Output 32-bit unsigned integer upon success.
    bool is_u32(uint32_t& u32) const;

    /// @brief Set TOML 32-bit unsigned int value.
    /// @return True on success.
    bool set_u32(uint32_t u32);

    /// @brief Check if value is a TOML floating point.
    /// @param f64 Output 64-bit floating point number on success.
    /// @note TOML integers will be implicitly casted to float.
    bool is_f64(double& f64) const;

    /// @brief Set TOML 64-bit floating point value.
    /// @return True on success.
    bool set_f64(double f64);

    /// @brief Check if value is a TOML floating point.
    /// @param f32 Output 32-bit floating point number on success.
    /// @note TOML integers will be implicitly casted to float.
    bool is_f32(float& f32) const;

    /// @brief Set TOML 32-bit floating point value.
    /// @return True on success.
    bool set_f32(float f32);

    /// @brief Check if value is a TOML string.
    /// @param string Output string upon success.
    bool is_string(std::string& string) const;

    /// @brief Set TOML string value.
    /// @return True on success.
    bool set_string(const std::string& string);

    /// @brief Get array size or table size.
    /// @return Non-negative size, or negative value on failure.
    int get_size();

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

    /// @brief Set key of TOML table. May override existing key.
    /// @return The value associated with key on success.
    TOMLValue set_key(const char* key, TOMLType type);
    
    /// @brief Lookup key in TOML table.
    TOMLValue get_key(const char* key);

    /// @brief Shorthand for table get_key.
    inline TOMLValue operator[](const char* key)
    {
        return get_key(key);
    }

    /// @brief Get all keys in a table.
    int get_keys(std::vector<std::string>& keys);
};

/// @brief TOML document handle.
struct TOMLDocument : Handle<struct TOMLDocumentObj>
{
    /// @brief Create empty TOML document.
    static TOMLDocument create();

    /// @brief Create TOML document from file on disk.
    static TOMLDocument create_from_file(const FS::Path& path);

    /// @brief Destroy TOML document.
    static void destroy(TOMLDocument doc);

    /// @brief Load TOML document from string.
    bool parse(const char* toml, size_t len, std::string& error);

    /// @brief Consolidate all value modifications to TOML DOM.
    void consolidate();

    /// @brief Get value by name.
    TOMLValue get(const char* name);

    /// @brief Save TOML document to disk after consolidation.
    bool save_to_disk(const FS::Path& path);
};

} // namespace LD
