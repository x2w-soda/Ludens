#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/System/Memory.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <toml.hpp> // hide
#include <vector>

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

static toml::value get_toml_value(TOMLType type)
{
    switch (type)
    {
    case TOML_TYPE_BOOL:
        return toml::value(toml::type_config::boolean_type{});
    case TOML_TYPE_INT:
        return toml::value(toml::type_config::integer_type{});
    case TOML_TYPE_FLOAT:
        return toml::value(toml::type_config::floating_type{});
    case TOML_TYPE_STRING:
        return toml::value(toml::type_config::string_type{});
    case TOML_TYPE_OFFSET_DATETIME:
        return toml::value(toml::offset_datetime{});
    case TOML_TYPE_LOCAL_DATETIME:
        return toml::value(toml::local_datetime{});
    case TOML_TYPE_LOCAL_DATE:
        return toml::value(toml::local_date{});
    case TOML_TYPE_LOCAL_TIME:
        return toml::value(toml::local_time{});
    case TOML_TYPE_ARRAY: // TODO: this is templated
    case TOML_TYPE_TABLE: // TODO: this is templated
    default:
        LD_UNREACHABLE;
    }

    return toml::value();
}

struct TOMLValueObj
{
    TOMLValueObj* child;
    TOMLValueObj* next;
    toml::value val;
    TOMLDocumentObj* doc;
    const char* selfKey;
    int selfIndex;

    /// @brief Check for child with key.
    TOMLValueObj* table_get_child(const std::string& key);
};

TOMLValueObj* TOMLValueObj::table_get_child(const std::string& key)
{
    LD_ASSERT(val.type() == toml::value_t::table);

    for (TOMLValueObj* c = child; c; c = c->next)
    {
        LD_ASSERT(c->selfKey);
        if (std::string(c->selfKey) == key)
            return c;
    }

    return nullptr;
}

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

            if (node->selfKey)
                heap_free((void*)node->selfKey);
        }

        PoolAllocator::destroy(valuePA);
        valuePA = {};
    }

    TOMLValueObj* alloc_value()
    {
        TOMLValueObj* obj = (TOMLValueObj*)valuePA.allocate();
        obj->doc = this;
        obj->selfKey = nullptr;
        obj->selfIndex = -1;
        obj->child = nullptr;
        obj->next = nullptr;
        new (&obj->val) toml::value();
        return obj;
    }

    void consolidate(TOMLValueObj* root)
    {
        if (!root)
            return;

        if (root->val.is_table())
        {
            for (TOMLValueObj* child = root->child; child; child = child->next)
            {
                consolidate(child);
                LD_ASSERT(child->selfKey);
                root->val.as_table()[child->selfKey] = child->val;
            }
        }
        else if (root->val.is_array())
        {
            int size = root->val.size();

            for (TOMLValueObj* child = root->child; child; child = child->next)
            {
                consolidate(child);
                LD_ASSERT(0 <= child->selfIndex && child->selfIndex < size);
                root->val.as_array()[child->selfIndex] = child->val;
            }
        }
    }
};

TOMLType TOMLValue::get_type() const
{
    return (TOMLType)mObj->val.type();
}

bool TOMLValue::is_bool(bool& boolean) const
{
    if (get_type() != TOML_TYPE_BOOL)
        return false;

    boolean = mObj->val.as_boolean();
    return true;
}

bool TOMLValue::set_bool(bool boolean)
{
    if (get_type() != TOML_TYPE_BOOL)
        return false;

    mObj->val.as_boolean() = boolean;
    return true;
}

bool TOMLValue::is_i64(int64_t& i64) const
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    i64 = (int64_t)mObj->val.as_integer();
    return true;
}

bool TOMLValue::set_i64(int64_t i64)
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    mObj->val.as_integer() = i64;
    return true;
}

bool TOMLValue::is_i32(int32_t& i32) const
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    // safe downcast to 32-bit
    i32 = (int32_t)mObj->val.as_integer();
    return true;
}

bool TOMLValue::set_i32(int32_t i32)
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    // safe upcast to 64-bit signed.
    mObj->val.as_integer() = (int64_t)i32;
    return true;
}

bool TOMLValue::is_u32(uint32_t& u32) const
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    // safe downcast to 32-bit unsigned.
    u32 = (uint32_t)mObj->val.as_integer();
    return true;
}

bool TOMLValue::set_u32(uint32_t u32)
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    // safe upcast to 64-bit signed.
    mObj->val.as_integer() = (int64_t)u32;
    return true;
}

bool TOMLValue::is_f64(double& f64) const
{
    if (get_type() != TOML_TYPE_FLOAT)
        return false;

    f64 = (double)mObj->val.as_floating();
    return true;
}

bool TOMLValue::set_f64(double f64)
{
    if (get_type() != TOML_TYPE_FLOAT)
        return false;

    mObj->val.as_floating() = f64;
    return true;
}

bool TOMLValue::is_f32(float& f32) const
{
    if (get_type() != TOML_TYPE_FLOAT)
        return false;

    f32 = (float)mObj->val.as_floating();
    return true;
}

bool TOMLValue::set_f32(float f32)
{
    if (get_type() != TOML_TYPE_FLOAT)
        return false;

    mObj->val.as_floating() = (double)f32;
    return true;
}

bool TOMLValue::is_string(std::string& string) const
{
    if (get_type() != TOML_TYPE_STRING)
        return false;

    string = mObj->val.as_string();
    return true;
}

bool TOMLValue::set_string(const std::string& string)
{
    if (get_type() != TOML_TYPE_STRING)
        return false;

    mObj->val.as_string() = string;
    return true;
}

int TOMLValue::get_size()
{
    TOMLType type = get_type();

    if (type != TOML_TYPE_ARRAY && type != TOML_TYPE_TABLE)
        return -1;

    return (int)mObj->val.size();
}

TOMLValue TOMLValue::get_index(int idx)
{
    if (!is_array_type() || idx < 0 || idx >= get_size())
        return {};

    TOMLValueObj* value = mObj->doc->alloc_value();
    value->selfIndex = idx;
    value->val = mObj->val.at((size_t)idx);
    value->next = mObj->child;
    mObj->child = value;

    return TOMLValue(value);
}

bool TOMLValue::has_key(const char* key, const TOMLType* typeMatch)
{
    if (!is_table_type())
        return false;

    auto& table = mObj->val.as_table();
    if (!table.contains(key))
        return false;

    if (typeMatch)
        return (TOMLType)table[key].type() == *typeMatch;

    return true;
}

TOMLValue TOMLValue::set_key(const char* key, TOMLType type)
{
    if (!is_table_type())
        return {};

    auto& table = mObj->val.as_table();

    TOMLValueObj* obj = mObj->table_get_child(key);
    if (obj)
    {
        obj->val = get_toml_value(type);
    }
    else
    {
        obj = mObj->doc->alloc_value();
        obj->val = get_toml_value(type);
        obj->selfKey = heap_strdup(key, MEMORY_USAGE_MEDIA);
        obj->next = mObj->child;
        mObj->child = obj;
    }

    // this will eventually be done during consolidate(),
    // but we try to keep the DOM as syncrhonized as we can.
    table[key] = obj->val;

    return TOMLValue(obj);
}

TOMLValue TOMLValue::get_key(const char* key)
{
    if (!is_table_type() || !mObj->val.contains(key))
        return {};

    TOMLValueObj* obj = mObj->table_get_child(key);

    if (!obj)
    {
        obj = mObj->doc->alloc_value();
        obj->selfKey = heap_strdup(key, MEMORY_USAGE_MEDIA);
        obj->val = mObj->val.at(key);
        obj->next = mObj->child;
        mObj->child = obj;
    }

    return TOMLValue(obj);
}

int TOMLValue::get_keys(std::vector<std::string>& keys)
{
    if (!is_table_type())
        return 0;

    keys.resize(get_size());
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

TOMLDocument TOMLDocument::create_from_file(const std::filesystem::path& path)
{
    if (!FS::exists(path))
        return {};

    TOMLDocument doc = TOMLDocument::create();
    TOMLDocumentObj* obj = doc.unwrap();

    uint64_t fileSize = FS::get_file_size(path);
    obj->fileBuffer = (byte*)heap_malloc(fileSize, MEMORY_USAGE_MEDIA);
    bool ok = FS::read_file(path, fileSize, obj->fileBuffer);

    if (!ok)
    {
        TOMLDocument::destroy(doc);
        return {};
    }

    std::string error;
    ok = doc.parse((const char*)obj->fileBuffer, fileSize, error);

    return doc;
}

void TOMLDocument::destroy(TOMLDocument doc)
{
    TOMLDocumentObj* obj = doc.unwrap();

    obj->free_values();

    if (obj->fileBuffer)
        heap_free(obj->fileBuffer);

    heap_delete<TOMLDocumentObj>(obj);
}

bool TOMLDocument::parse(const char* toml, size_t len, std::string& error)
{
    if (mObj->valuePA)
        PoolAllocator::destroy(mObj->valuePA);

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_MEDIA;
    paI.blockSize = sizeof(TOMLValueObj);
    paI.pageSize = 64; // TOML values per page
    paI.isMultiPage = true;
    mObj->valuePA = PoolAllocator::create(paI);
    mObj->root.child = nullptr;
    mObj->root.next = nullptr;
    mObj->root.doc = nullptr;
    mObj->root.selfIndex = -1;
    mObj->root.selfKey = nullptr;

    std::string source(toml, len);
    const auto& result = toml::try_parse_str(source, toml::spec::default_version());

    error.clear();

    if (result.is_ok())
    {
        mObj->root.val = result.unwrap();
    }
    else
    {
        const std::vector<toml::error_info>& errors = result.unwrap_err();
        if (!errors.empty())
            error = toml::format_error(errors[0]);
    }

    return result.is_ok();
}

void TOMLDocument::consolidate()
{
    if (!mObj->root.val.is_table())
        return;

    mObj->consolidate(&mObj->root);
}

TOMLValue TOMLDocument::get(const char* name)
{
    if (!mObj->root.val.is_table() || !mObj->root.val.contains(name))
        return {};

    TOMLValueObj* obj = mObj->root.table_get_child(name);

    if (!obj)
    {
        obj = mObj->alloc_value();
        obj->val = toml::find(mObj->root.val, name);
        obj->selfKey = heap_strdup(name, MEMORY_USAGE_MEDIA);
        obj->next = mObj->root.child;
        mObj->root.child = obj;
    }

    return TOMLValue(obj);
}

bool TOMLDocument::save_to_disk(const FS::Path& path)
{
    LD_PROFILE_SCOPE;

    if (!mObj->root.val.is_table())
        return false;

    mObj->consolidate(&mObj->root);

    std::string str = toml::format(mObj->root.val);
    return FS::write_file(path, str.size(), (byte*)str.data());
}

} // namespace LD
