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

/// @brief A node in the DOM tree
struct JSONValue : Handle<struct JSONValueObj>
{
    /// @brief get node data type
    JSONType type() const;

    /// @brief check if node is JSON false value
    bool is_false() const;

    /// @brief check if node is JSON true value
    bool is_true() const;

    /// @brief check if node is a JSON object
    bool is_object() const;

    /// @brief check if node is a JSON array
    bool is_array() const;

    /// @brief check if node is a JSON string
    bool is_string(std::string* str) const;

    /// @brief check if node is a JSON number
    bool is_number() const;

    /// @brief check if node is a JSON 32-bit signed integer
    bool is_i32(int32_t* i32) const;

    /// @brief check if node is a JSON 64-bit signed integer
    bool is_i64(int64_t* i64) const;

    /// @brief check if node is a JSON 32-bit unsigned integer
    bool is_u32(uint32_t* u32) const;

    /// @brief check if node is a JSON 64-bit unsigned integer
    bool is_u64(uint64_t* u64) const;

    /// @brief check if node is a 32-bit floating point
    bool is_f32(float* f32) const;

    /// @brief get number of elements in array or number of members in object
    /// @return the size of the array or object, or a negative value otherwise
    int size();

    /// @brief get the member of an object
    /// @param member C string name of the member
    /// @return the member node, or null on error
    JSONValue get_member(const char* member);

    /// @brief get the element at index in an array
    /// @param idx index into the array
    /// @return the element node, or null on error
    JSONValue get_index(int idx);

    /// @brief Shorthand for get_index method.
    inline JSONValue operator[](int idx) { return get_index(idx); }
};

/// @brief JSON Document Object Model.
struct JSONDocument : Handle<struct JSONDocumentObj>
{
    /// @brief Create empty json document.
    static JSONDocument create();

    /// @brief Destroy json document, all values from this document becomes out of date.
    static void destroy(JSONDocument doc);

    /// @brief Get root document value.
    JSONValue get_root();
};

/// @brief JSON DOM parser.
struct JSONParser
{
    /// @brief Try parsing JSON into document.
    /// @param dst Destination document.
    /// @param view JSON source string.
    /// @param error Output error message upon failure.
    static bool parse(JSONDocument dst, const View& view, std::string& error);
};

struct JSONEventCallback
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
struct JSONEventParser
{
    static bool parse(const void* fileData, size_t fileSize, std::string& error, const JSONEventCallback& callbacks, void* user);
};

} // namespace LD
