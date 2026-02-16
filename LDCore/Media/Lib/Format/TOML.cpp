#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>

#include <cstdint>
#include <iostream>
#include <string>

#include <toml.hpp> // hide

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
    TOMLValue get_root();
};

static bool parse_toml(TOMLDocument dst, const View& view, std::string& error);
static bool parse_toml_from_file(TOMLDocument dst, const FS::Path& path, std::string& error);

// static paranoia
static_assert((int)TOML_TYPE_EMPTY == (int)toml::value_t::empty);
static_assert((int)TOML_TYPE_BOOL == (int)toml::value_t::boolean);
static_assert((int)TOML_TYPE_INT == (int)toml::value_t::integer);
static_assert((int)TOML_TYPE_FLOAT == (int)toml::value_t::floating);
static_assert((int)TOML_TYPE_STRING == (int)toml::value_t::string);
static_assert((int)TOML_TYPE_OFFSET_DATETIME == (int)toml::value_t::offset_datetime);
static_assert((int)TOML_TYPE_LOCAL_DATETIME == (int)toml::value_t::local_datetime);
static_assert((int)TOML_TYPE_LOCAL_DATE == (int)toml::value_t::local_date);
static_assert((int)TOML_TYPE_LOCAL_TIME == (int)toml::value_t::local_time);
static_assert((int)TOML_TYPE_ARRAY == (int)toml::value_t::array);
static_assert((int)TOML_TYPE_TABLE == (int)toml::value_t::table);

// default to using 64 bit signed integer for TOML ints
static_assert(std::is_same_v<toml::value::config_type::integer_type, std::int64_t>);

// default to using 64-bit IEEE 754 for TOML floats
static_assert(std::is_same_v<toml::value::config_type::floating_type, double>);

// default to using C++ STL basic_string
static_assert(std::is_same_v<toml::value::config_type::string_type, std::string>);

struct TOMLValueObj
{
    toml::value val;
    TOMLDocumentObj* doc;
};

/// @brief Toml document implementation.
struct TOMLDocumentObj
{
    PoolAllocator valuePA{};
    byte* fileBuffer = nullptr;
    TOMLValueObj root;

    void free_values()
    {
        if (!valuePA)
            return;

        for (auto ite = valuePA.begin(); ite; ++ite)
        {
            auto* node = static_cast<TOMLValueObj*>(ite.data());
            (&node->val)->~basic_value();
        }

        PoolAllocator::destroy(valuePA);
        valuePA = {};
    }

    TOMLValueObj* alloc_value()
    {
        TOMLValueObj* obj = (TOMLValueObj*)valuePA.allocate();
        obj->doc = this;
        new (&obj->val) toml::value();
        return obj;
    }

    void reset()
    {
        if (valuePA)
            PoolAllocator::destroy(valuePA);

        PoolAllocatorInfo paI{};
        paI.usage = MEMORY_USAGE_MEDIA;
        paI.blockSize = sizeof(TOMLValueObj);
        paI.pageSize = 64; // TOML values per page
        paI.isMultiPage = true;
        valuePA = PoolAllocator::create(paI);

        root.val = toml::table();
        root.doc = this;
    }
};

TOMLType TOMLValue::type() const
{
    return (TOMLType)mObj->val.type();
}

bool TOMLValue::get_bool(bool& boolean) const
{
    if (type() != TOML_TYPE_BOOL)
        return false;

    boolean = mObj->val.as_boolean();
    return true;
}

bool TOMLValue::get_i64(int64_t& i64) const
{
    if (type() != TOML_TYPE_INT)
        return false;

    i64 = (int64_t)mObj->val.as_integer();
    return true;
}

bool TOMLValue::get_i32(int32_t& i32) const
{
    if (type() != TOML_TYPE_INT)
        return false;

    // safe downcast to 32-bit
    i32 = (int32_t)mObj->val.as_integer();
    return true;
}

bool TOMLValue::get_u32(uint32_t& u32) const
{
    if (type() != TOML_TYPE_INT)
        return false;

    // safe downcast to 32-bit unsigned.
    u32 = (uint32_t)mObj->val.as_integer();
    return true;
}

bool TOMLValue::get_f64(double& f64) const
{
    if (type() == TOML_TYPE_FLOAT)
    {
        f64 = (double)mObj->val.as_floating();
        return true;
    }
    else if (type() == TOML_TYPE_INT)
    {
        f64 = (double)mObj->val.as_integer();
        return true;
    }

    return false;
}

bool TOMLValue::get_f32(float& f32) const
{
    if (type() == TOML_TYPE_FLOAT)
    {
        f32 = (float)mObj->val.as_floating();
        return true;
    }
    else if (type() == TOML_TYPE_INT)
    {
        f32 = (float)mObj->val.as_integer();
        return true;
    }

    return false;
}

bool TOMLValue::get_string(std::string& string) const
{
    if (type() != TOML_TYPE_STRING)
        return false;

    string = mObj->val.as_string();
    return true;
}

int TOMLValue::size()
{
    if (!is_array() && !is_table())
        return -1;

    return (int)mObj->val.size();
}

TOMLValue TOMLValue::get_index(int idx)
{
    if (!is_array() || idx < 0 || idx >= size())
        return {};

    TOMLValueObj* obj = mObj->doc->alloc_value();
    obj->val = mObj->val.at((size_t)idx);

    return TOMLValue(obj);
}

bool TOMLValue::has_key(const char* key, const TOMLType* typeMatch)
{
    if (!is_table())
        return false;

    auto& table = mObj->val.as_table();
    if (!table.contains(key))
        return false;

    if (typeMatch)
        return (TOMLType)table[key].type() == *typeMatch;

    return true;
}

TOMLValue TOMLValue::get_key(const char* key)
{
    if (!is_table() || !mObj->val.contains(key))
        return {};

    TOMLValueObj* obj = mObj->doc->alloc_value();
    obj->val = mObj->val.at(key);

    return TOMLValue(obj);
}

TOMLValue TOMLValue::get_key(const char* key, TOMLType type)
{
    TOMLValue val = get_key(key);

    if (!val || val.type() != type)
        return {};

    return val;
}

int TOMLValue::get_keys(Vector<std::string>& keys)
{
    if (!is_table())
        return 0;

    keys.resize(size());
    int i = 0;

    for (const auto& ite : mObj->val.as_table())
    {
        keys[i++] = ite.first;
    }

    return (int)keys.size();
}

TOMLDocument TOMLDocument::create()
{
    auto* obj = heap_new<TOMLDocumentObj>(MEMORY_USAGE_MEDIA);
    obj->root.val = toml::value();

    return TOMLDocument(obj);
}

void TOMLDocument::destroy(TOMLDocument doc)
{
    TOMLDocumentObj* obj = doc.unwrap();

    obj->free_values();

    if (obj->fileBuffer)
        heap_free(obj->fileBuffer);

    heap_delete<TOMLDocumentObj>(obj);
}

TOMLValue TOMLDocument::get_root()
{
    LD_ASSERT(mObj->root.val.is_table());

    return TOMLValue(&mObj->root);
}

static bool parse_toml(TOMLDocument dst, const View& view, std::string& error)
{
    TOMLDocumentObj* docObj = dst.unwrap();

    docObj->reset();

    // TODO: does toml-11 really not have a parse-from-view API?
    //       this is a redundant clone of the entire input string.
    std::string source(view.data, view.size);

    const auto& result = toml::try_parse_str(source, toml::spec::default_version());

    error.clear();

    if (result.is_ok())
    {
        docObj->root.val = result.unwrap();
    }
    else
    {
        const Vector<toml::error_info>& errors = result.unwrap_err();
        if (!errors.empty())
            error = toml::format_error(errors[0]);
    }

    return result.is_ok();
}

static bool parse_toml_from_file(TOMLDocument dst, const FS::Path& path, std::string& error)
{
    std::vector<byte> file;
    if (!FS::read_file_to_vector(path, file, error))
        return false;

    return parse_toml(dst, View((const char*)file.data(), file.size()), error);
}

enum TOMLScopeType
{
    TOML_SCOPE_TABLE = 0,
    TOML_SCOPE_ARRAY,
    TOML_SCOPE_INLINE_TABLE,
    TOML_SCOPE_ARRAY_TABLE,
};

struct TOMLWriterScope
{
    TOMLScopeType type;
    toml::value* value; // pointer to avoid copying nested hashmaps when stack resizes
    std::string tableName;
};

/// @brief TOML writer implementation.
struct TOMLWriterObj
{
    Stack<TOMLWriterScope> scope;
    std::string key;

    inline bool is_array_scope() const
    {
        return scope.size() > 0 && scope.top().type == TOML_SCOPE_ARRAY;
    }

    inline bool is_table_scope() const
    {
        return scope.size() > 0 && scope.top().type == TOML_SCOPE_TABLE;
    }

    inline bool is_inline_table_scope() const
    {
        return scope.size() > 0 && scope.top().type == TOML_SCOPE_INLINE_TABLE;
    }

    inline bool is_array_table_scope() const
    {
        return scope.size() > 0 && scope.top().type == TOML_SCOPE_ARRAY_TABLE;
    }

    inline bool is_expecting_value() const
    {
        return !key.empty() || is_array_scope();
    }

    inline bool may_begin_array() const
    {
        return is_expecting_value();
    }

    inline bool may_begin_inline_table() const
    {
        return is_expecting_value();
    }

    inline bool may_begin_array_table() const
    {
        return key.empty() && is_table_scope();
    }

    void push_array_scope()
    {
        TOMLWriterScope newScope;
        newScope.type = TOML_SCOPE_ARRAY;
        newScope.value = heap_new<toml::value>(MEMORY_USAGE_MEDIA);
        *newScope.value = toml::array();

        scope.push(newScope);
    }

    void push_table_scope()
    {
        TOMLWriterScope newScope{};
        newScope.type = TOML_SCOPE_TABLE;
        newScope.value = heap_new<toml::value>(MEMORY_USAGE_MEDIA);
        *newScope.value = toml::table();

        if (!key.empty())
        {
            newScope.tableName = key;
            key.clear();
        }

        scope.push(newScope);
    }

    void push_inline_table_scope()
    {
        LD_ASSERT(scope.size() > 0 && (scope.top().type == TOML_SCOPE_TABLE) || (scope.top().type == TOML_SCOPE_INLINE_TABLE));
        LD_ASSERT(!key.empty());

        TOMLWriterScope newScope;
        newScope.type = TOML_SCOPE_INLINE_TABLE;
        newScope.value = heap_new<toml::value>(MEMORY_USAGE_MEDIA);
        *newScope.value = toml::table();
        newScope.value->as_table_fmt().fmt = toml::table_format::oneline;

        if (!key.empty())
        {
            newScope.tableName = key;
            key.clear();
        }

        scope.push(newScope);
        key.clear();
    }

    void push_array_table_scope(const char* name)
    {
        TOMLWriterScope newScope{};
        newScope.type = TOML_SCOPE_ARRAY_TABLE;
        newScope.value = heap_new<toml::value>(MEMORY_USAGE_MEDIA);
        *newScope.value = toml::array(); // an array of tables under the same name
        newScope.value->as_array_fmt().fmt = toml::array_format::array_of_tables;
        newScope.tableName = name;

        scope.push(newScope);
    }

    void pop_scope()
    {
        std::string popTableName = scope.top().tableName;
        toml::value* popValue = scope.top().value;

        scope.pop();

        if (scope.empty())
        {
            heap_delete<toml::value>(popValue);
            return;
        }

        TOMLScopeType type = scope.top().type;

        if (type == TOML_SCOPE_TABLE || type == TOML_SCOPE_INLINE_TABLE)
        {
            toml::value* parentTable = scope.top().value;

            if (!popTableName.empty())
            {
                parentTable->as_table()[popTableName] = std::move(*popValue);
            }
            else if (!key.empty())
            {
                parentTable->as_table()[key] = std::move(*popValue);
                key.clear();
            }
        }
        else if (type == TOML_SCOPE_ARRAY || type == TOML_SCOPE_ARRAY_TABLE)
        {
            toml::value* parentArray = scope.top().value;
            parentArray->as_array().push_back(std::move(*popValue));
        }
        else
            LD_UNREACHABLE;

        heap_delete<toml::value>(popValue);
    }

    template <typename TValue>
    void value(TValue value)
    {
        TOMLScopeType scopeType = scope.top().type;
        toml::value* scopeVal = scope.top().value;

        switch (scopeType)
        {
        case TOML_SCOPE_TABLE:
        case TOML_SCOPE_INLINE_TABLE:
            if (!key.empty())
            {
                scopeVal->as_table()[key] = toml::value(value);
                key.clear();
            }
            break;
        case TOML_SCOPE_ARRAY:
            scopeVal->as_array().push_back(toml::value(value));
            break;
        default:
            LD_UNREACHABLE;
            break;
        }
    }
};

TOMLWriter TOMLWriter::create()
{
    auto* obj = heap_new<TOMLWriterObj>(MEMORY_USAGE_MEDIA);

    return TOMLWriter(obj);
}

void TOMLWriter::destroy(TOMLWriter writer)
{
    auto* obj = writer.unwrap();

    heap_delete<TOMLWriterObj>(obj);
}

bool TOMLWriter::is_array_scope()
{
    return mObj->is_array_scope();
}

bool TOMLWriter::is_table_scope()
{
    return mObj->is_table_scope();
}

bool TOMLWriter::is_inline_table_scope()
{
    return mObj->is_inline_table_scope();
}

bool TOMLWriter::is_array_table_scope()
{
    return mObj->is_array_table_scope();
}

TOMLWriter TOMLWriter::begin()
{
    mObj->push_table_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::end(std::string& outStr)
{
    LD_ASSERT(mObj->scope.size() == 1 && mObj->scope.top().type == TOML_SCOPE_TABLE); // root table scope

    outStr = toml::format(*mObj->scope.top().value);

    mObj->pop_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::begin_table()
{
    mObj->push_table_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::end_table()
{
    LD_ASSERT(is_table_scope());

    mObj->pop_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::begin_array()
{
    LD_ASSERT(mObj->may_begin_array());

    mObj->push_array_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::end_array()
{
    LD_ASSERT(mObj->is_array_scope());

    mObj->pop_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::begin_inline_table()
{
    LD_ASSERT(mObj->may_begin_inline_table());

    mObj->push_inline_table_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::end_inline_table()
{
    LD_ASSERT(is_inline_table_scope());

    mObj->pop_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::begin_array_table(const char* name)
{
    LD_ASSERT(mObj->may_begin_array_table());

    mObj->push_array_table_scope(name);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::end_array_table()
{
    LD_ASSERT(is_array_table_scope());

    mObj->pop_scope();

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::key(const char* name)
{
    LD_ASSERT(is_table_scope() || is_inline_table_scope());
    LD_ASSERT(mObj->key.empty()); // repeated key()

    mObj->key = name;

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::key(const std::string& str)
{
    return key(str.c_str());
}

TOMLWriter TOMLWriter::value_bool(bool b)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(b);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_i32(int32_t i32)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(i32);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_i64(int64_t i64)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(i64);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_u32(uint32_t u32)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(u32);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_f32(float f32)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(f32);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_f64(double f64)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(f64);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_string(const char* cstr)
{
    LD_ASSERT(mObj->is_expecting_value());

    mObj->value(cstr);

    return TOMLWriter(mObj);
}

TOMLWriter TOMLWriter::value_string(const std::string& str)
{
    return value_string(str.c_str());
}

/// @brief TOML reader implementation.
struct TOMLReaderObj
{
    TOMLDocument doc{};
    Stack<TOMLValue> scope;

    inline TOMLValue get_key(const char* key)
    {
        TOMLValue top = scope.top();
        LD_ASSERT(top.is_table());

        return top.get_key(key);
    }

    inline TOMLValue get_index(int index)
    {
        TOMLValue top = scope.top();
        LD_ASSERT(top.is_array());

        return top.get_index(index);
    }
};

TOMLReader TOMLReader::create(const View& toml, std::string& err)
{
    TOMLDocument doc = TOMLDocument::create();

    if (!parse_toml(doc, toml, err))
    {
        TOMLDocument::destroy(doc);
        return {};
    }

    auto* obj = heap_new<TOMLReaderObj>(MEMORY_USAGE_MEDIA);
    obj->doc = doc;
    obj->scope.push(doc.get_root());

    return TOMLReader(obj);
}

void TOMLReader::destroy(TOMLReader reader)
{
    auto* obj = reader.unwrap();

    LD_ASSERT(obj->scope.size() == 1);
    obj->scope.pop();

    if (obj->doc)
    {
        TOMLDocument::destroy(obj->doc);
        obj->doc = {};
    }

    heap_delete<TOMLReaderObj>(obj);
}

bool TOMLReader::is_array_scope()
{
    LD_ASSERT(!mObj->scope.empty());

    return mObj->scope.top().is_array();
}

bool TOMLReader::is_table_scope()
{
    LD_ASSERT(!mObj->scope.empty());

    return mObj->scope.top().is_table();
}

bool TOMLReader::enter_array(const char* key, int& arraySize)
{
    TOMLValue top = mObj->scope.top();
    LD_ASSERT(top.is_table());

    TOMLValue value = top.get_key(key);
    if (!value || !value.is_array())
        return false;

    arraySize = value.size();
    mObj->scope.push(value);

    return true;
}

bool TOMLReader::enter_table(const char* key)
{
    TOMLValue top = mObj->scope.top();
    LD_ASSERT(top.is_table());

    TOMLValue value = top.get_key(key);
    if (!value || !value.is_table())
        return false;

    mObj->scope.push(value);

    return true;
}

bool TOMLReader::enter_table(int index)
{
    TOMLValue top = mObj->scope.top();
    LD_ASSERT(top.is_array());

    TOMLValue value = top.get_index(index);
    if (!value)
        return false;

    mObj->scope.push(value);

    return true;
}

void TOMLReader::exit()
{
    LD_ASSERT(!mObj->scope.empty());

    mObj->scope.pop();
}

void TOMLReader::get_keys(Vector<std::string>& keys)
{
    LD_ASSERT(mObj->scope.top().is_table());

    mObj->scope.top().get_keys(keys);
}

bool TOMLReader::read_bool(const char* key, bool& b)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_bool(b);
}

bool TOMLReader::read_bool(int index, bool& b)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_bool(b);
}

bool TOMLReader::read_i32(const char* key, int32_t& i32)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_i32(i32);
}

bool TOMLReader::read_i32(int index, int32_t& i32)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_i32(i32);
}

bool TOMLReader::read_i64(const char* key, int64_t& i64)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_i64(i64);
}

bool TOMLReader::read_i64(int index, int64_t& i64)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_i64(i64);
}

bool TOMLReader::read_u32(const char* key, uint32_t& u32)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_u32(u32);
}

bool TOMLReader::read_u32(int index, uint32_t& u32)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_u32(u32);
}

bool TOMLReader::read_f32(const char* key, float& f32)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_f32(f32);
}

bool TOMLReader::read_f32(int index, float& f32)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_f32(f32);
}

bool TOMLReader::read_f64(const char* key, double& f64)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_f64(f64);
}

bool TOMLReader::read_f64(int index, double& f64)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_f64(f64);
}

bool TOMLReader::read_string(const char* key, std::string& str)
{
    TOMLValue value = mObj->get_key(key);

    return value && value.get_string(str);
}

bool TOMLReader::read_string(int index, std::string& str)
{
    TOMLValue value = mObj->get_index(index);

    return value && value.get_string(str);
}

namespace TOMLUtil {

bool write_transform(TOMLWriter writer, const char* key, const TransformEx& transform)
{
    if (!writer)
        return false;

    writer.begin_inline_table(key);
    write_vec3(writer, "position", transform.position);
    write_vec3(writer, "rotation", transform.rotationEuler);
    write_vec3(writer, "scale", transform.scale);
    writer.end_inline_table();

    return true;
}

bool read_transform(TOMLReader reader, const char* key, TransformEx& transform)
{
    if (!reader || !reader.enter_table(key))
        return false;

    if (!read_vec3(reader, "position", transform.position) ||
        !read_vec3(reader, "rotation", transform.rotationEuler) ||
        !read_vec3(reader, "scale", transform.scale))
    {
        reader.exit();
        return false;
    }

    transform.rotation = Quat::from_euler(transform.rotationEuler);
    reader.exit();
    return true;
}

bool write_transform_2d(TOMLWriter writer, const char* key, const Transform2D& transform)
{
    if (!writer)
        return false;

    writer.begin_inline_table(key);
    write_vec2(writer, "position", transform.position);
    write_vec2(writer, "scale", transform.scale);
    writer.key("rotation").value_f32(transform.rotation);
    writer.end_inline_table();

    return true;
}

bool read_transform_2d(TOMLReader reader, const char* key, Transform2D& transform)
{
    if (!reader || !reader.enter_table(key))
        return false;

    if (!read_vec2(reader, "position", transform.position) ||
        !read_vec2(reader, "scale", transform.scale) ||
        !reader.read_f32("rotation", transform.rotation))
    {
        reader.exit();
        return false;
    }

    reader.exit();
    return true;
}

bool write_rect(TOMLWriter writer, const char* key, const Rect& rect)
{
    if (!writer)
        return false;

    writer.begin_inline_table(key);
    writer.key("x").value_f32(rect.x);
    writer.key("y").value_f32(rect.y);
    writer.key("w").value_f32(rect.w);
    writer.key("h").value_f32(rect.h);
    writer.end_inline_table();

    return true;
}

bool read_rect(TOMLReader reader, const char* key, Rect& rect)
{
    if (!reader || !reader.enter_table(key))
        return false;

    if (!reader.read_f32("x", rect.x) ||
        !reader.read_f32("y", rect.y) ||
        !reader.read_f32("w", rect.w) ||
        !reader.read_f32("h", rect.h))
    {
        reader.exit();
        return false;
    }

    reader.exit();
    return true;
}

bool write_vec3(TOMLWriter writer, const char* key, const Vec3& vec3)
{
    if (!writer)
        return false;

    writer.begin_inline_table(key);
    writer.key("x").value_f32(vec3.x);
    writer.key("y").value_f32(vec3.y);
    writer.key("z").value_f32(vec3.z);
    writer.end_inline_table();

    return true;
}

bool read_vec3(TOMLReader reader, const char* key, Vec3& vec3)
{
    if (!reader)
        return false;

    int size;

    if (reader.enter_table(key))
    {
        if (!reader.read_f32("x", vec3.x) ||
            !reader.read_f32("y", vec3.y) ||
            !reader.read_f32("z", vec3.z))
        {
            reader.exit();
            return false;
        }

        reader.exit();
        return true;
    }
    else if (reader.enter_array(key, size))
    {
        if (size != 3 ||
            !reader.read_f32(0, vec3.x) ||
            !reader.read_f32(1, vec3.y) ||
            !reader.read_f32(2, vec3.z))
        {
            reader.exit();
            return false;
        }

        reader.exit();
        return true;
    }

    return false;
}

bool write_vec2(TOMLWriter writer, const char* key, const Vec2& vec2)
{
    if (!writer)
        return false;

    writer.begin_inline_table(key);
    writer.key("x").value_f32(vec2.x);
    writer.key("y").value_f32(vec2.y);
    writer.end_inline_table();

    return true;
}

bool read_vec2(TOMLReader reader, const char* key, Vec2& vec2)
{
    if (!reader)
        return false;

    int size;

    if (reader.enter_table(key))
    {
        if (!reader.read_f32("x", vec2.x) ||
            !reader.read_f32("y", vec2.y))
        {
            reader.exit();
            return false;
        }

        reader.exit();
        return true;
    }
    else if (reader.enter_array(key, size))
    {
        if (size != 2 ||
            !reader.read_f32(0, vec2.x) ||
            !reader.read_f32(1, vec2.y))
        {
            reader.exit();
            return false;
        }

        reader.exit();
        return true;
    }

    return false;
}

} // namespace TOMLUtil
} // namespace LD
