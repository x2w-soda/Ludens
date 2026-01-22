#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>

#include <cstdint>
#include <iostream>
#include <string>

#include <toml.hpp> // hide

namespace LD {

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
    TOMLValueObj* child;
    TOMLValueObj* next;
    toml::value val;
    TOMLDocumentObj* doc;
};

/// @brief Toml document implementation.
struct TOMLDocumentObj
{
    PoolAllocator valuePA;
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
        obj->child = nullptr;
        obj->next = nullptr;
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
        root.child = nullptr;
        root.next = nullptr;
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
    obj->next = mObj->child;
    mObj->child = obj;

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
    obj->next = mObj->child; // TODO: remove?
    mObj->child = obj;

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

TOMLValue TOMLDocument::get(const char* name)
{
    if (!mObj->root.val.is_table() || !mObj->root.val.contains(name))
        return {};

    TOMLValueObj* obj = mObj->alloc_value();
    obj->val = toml::find(mObj->root.val, name);
    obj->next = mObj->root.child; // TODO: do we need next?
    mObj->root.child = obj;

    return TOMLValue(obj);
}

bool TOMLParser::parse(TOMLDocument dst, const View& view, std::string& error)
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

bool TOMLParser::parse_from_file(TOMLDocument dst, const FS::Path& path, std::string& error)
{
    std::vector<byte> file;
    if (!FS::read_file_to_vector(path, file, error))
        return false;

    return TOMLParser::parse(dst, View((const char*)file.data(), file.size()), error);
}

enum TOMLWriterScopeType
{
    SCOPE_TABLE = 0,
    SCOPE_ARRAY,
    SCOPE_INLINE_TABLE,
    SCOPE_ARRAY_TABLE,
};

struct TOMLWriterScope
{
    TOMLWriterScopeType type;
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
        return scope.size() > 0 && scope.top().type == SCOPE_ARRAY;
    }

    inline bool is_table_scope() const
    {
        return scope.size() > 0 && scope.top().type == SCOPE_TABLE;
    }

    inline bool is_inline_table_scope() const
    {
        return scope.size() > 0 && scope.top().type == SCOPE_INLINE_TABLE;
    }

    inline bool is_array_table_scope() const
    {
        return scope.size() > 0 && scope.top().type == SCOPE_ARRAY_TABLE;
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
        newScope.type = SCOPE_ARRAY;
        newScope.value = heap_new<toml::value>(MEMORY_USAGE_MEDIA);
        *newScope.value = toml::array();

        scope.push(newScope);
    }

    void push_table_scope()
    {
        TOMLWriterScope newScope{};
        newScope.type = SCOPE_TABLE;
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
        LD_ASSERT(scope.size() > 0 && scope.top().type == SCOPE_TABLE);
        LD_ASSERT(!key.empty());

        toml::value* parentTable = scope.top().value;

        TOMLWriterScope newScope;
        newScope.type = SCOPE_INLINE_TABLE;
        newScope.value = heap_new<toml::value>(MEMORY_USAGE_MEDIA);
        *newScope.value = toml::table();
        newScope.value->as_table_fmt().fmt = toml::table_format::oneline;

        parentTable->as_table()[key] = newScope.value;

        scope.push(newScope);
        key.clear();
    }

    void push_array_table_scope(const char* name)
    {
        TOMLWriterScope newScope{};
        newScope.type = SCOPE_ARRAY_TABLE;
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

        TOMLWriterScopeType type = scope.top().type;

        if (type == SCOPE_TABLE)
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
        else if (type == SCOPE_ARRAY || type == SCOPE_ARRAY_TABLE)
        {
            toml::value* parentArray = scope.top().value;
            parentArray->as_array().push_back(std::move(*popValue));
        }

        heap_delete<toml::value>(popValue);
    }

    template <typename TValue>
    void value(TValue value)
    {
        TOMLWriterScopeType scopeType = scope.top().type;
        toml::value* scopeVal = scope.top().value;

        switch (scopeType)
        {
        case SCOPE_TABLE:
        case SCOPE_INLINE_TABLE:
            if (!key.empty())
            {
                scopeVal->as_table()[key] = toml::value(value);
                key.clear();
            }
            break;
        case SCOPE_ARRAY:
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
    LD_ASSERT(mObj->scope.size() == 1 && mObj->scope.top().type == SCOPE_TABLE); // root table scope

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
    LD_ASSERT(is_table_scope());
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

namespace TOMLUtil {

bool save_rect_table(const Rect& rect, TOMLWriter writer)
{
    if (!writer || !writer.is_inline_table_scope())
        return false;

    writer.key("x").value_f32(rect.x);
    writer.key("y").value_f32(rect.y);
    writer.key("w").value_f32(rect.w);
    writer.key("h").value_f32(rect.h);
    return true;
}

bool load_rect_table(Rect& rect, TOMLValue table)
{
    if (!table || !table.is_table())
        return false;

    TOMLValue x = table.get_key("x");
    if (!x || !x.get_f32(rect.x))
        return false;

    TOMLValue y = table.get_key("y");
    if (!y || !y.get_f32(rect.y))
        return false;

    TOMLValue w = table.get_key("w");
    if (!w || !w.get_f32(rect.w))
        return false;

    TOMLValue h = table.get_key("h");
    if (!h || !h.get_f32(rect.h))
        return false;

    return true;
}

} // namespace TOMLUtil
} // namespace LD
