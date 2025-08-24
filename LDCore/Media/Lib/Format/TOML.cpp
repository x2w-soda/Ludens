#include <Ludens/Media/Format/TOML.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/System/FileSystem.h>
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

struct TOMLValueObj
{
    toml::value val;
    TOMLDocumentObj* doc;
};

/// @brief Toml document implementation.
struct TOMLDocumentObj
{
    PoolAllocator valuePA;
    byte* fileBuffer;
    toml::value root;

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

bool TOMLValue::is_i64(int64_t& i64) const
{
    if (get_type() != TOML_TYPE_INT)
        return false;

    i64 = (int64_t)mObj->val.as_integer();
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

bool TOMLValue::is_f64(double& f64) const
{
    TOMLType type = get_type();

    switch (type)
    {
    case TOML_TYPE_FLOAT:
        f64 = (double)mObj->val.as_floating();
        return true;
    case TOML_TYPE_INT:
        f64 = (double)mObj->val.as_integer();
        return true;
    }

    return false;
}

bool TOMLValue::is_f32(float& f32) const
{
    TOMLType type = get_type();

    switch (type)
    {
    case TOML_TYPE_FLOAT:
        f32 = (float)mObj->val.as_floating();
        return true;
    case TOML_TYPE_INT:
        f32 = (float)mObj->val.as_integer();
        return true;
    }

    return false;
}

bool TOMLValue::is_string(std::string& string) const
{
    if (get_type() != TOML_TYPE_STRING)
        return false;

    string = mObj->val.as_string();
    return true;
}

int TOMLValue::get_size()
{
    TOMLType type = get_type();

    if (type != TOML_TYPE_ARRAY)
        return -1;

    return (int)mObj->val.size();
}

TOMLValue TOMLValue::get_index(int idx)
{
    if (!is_array_type() || idx < 0 || idx >= get_size())
        return {};

    TOMLValueObj* value = mObj->doc->alloc_value();
    value->val = mObj->val.at((size_t)idx);

    return TOMLValue(value);
}

TOMLValue TOMLValue::get_key(const char* key)
{
    if (!is_table_type() || !mObj->val.contains(key))
        return {};

    TOMLValueObj* value = mObj->doc->alloc_value();
    value->val = mObj->val.at(key);

    return TOMLValue(value);
}

TOMLDocument TOMLDocument::create()
{
    auto* obj = heap_new<TOMLDocumentObj>(MEMORY_USAGE_MEDIA);

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

    std::string source(toml, len);
    const auto& result = toml::try_parse_str(source, toml::spec::default_version());

    error.clear();

    if (result.is_ok())
    {
        mObj->root = result.unwrap();
    }
    else
    {
        const std::vector<toml::error_info>& errors = result.unwrap_err();
        if (!errors.empty())
            error = toml::format_error(errors[0]);
    }

    return result.is_ok();
}

TOMLValue TOMLDocument::get(const char* name)
{
    if (!mObj->root.contains(name))
        return {};

    TOMLValueObj* value = mObj->alloc_value();
    value->val = toml::find(mObj->root, name);

    return TOMLValue(value);
}

} // namespace LD
