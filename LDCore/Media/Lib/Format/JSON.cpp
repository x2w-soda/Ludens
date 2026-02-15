#include <Ludens/DSA/Stack.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Format/JSON.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>

#include <format>
#include <string>

// NOTE: rapidjson is a header only C++ library,
//       definitely hide the headers from user via pimpl
#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/rapidjson.h>

namespace LD {

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

    /// @brief check if node is a JSON number
    bool is_number() const;

    /// @brief check if node is a JSON 32-bit signed integer
    bool get_bool(bool& b) const;

    /// @brief check if node is a JSON 32-bit signed integer
    bool get_i32(int32_t& i32) const;

    /// @brief check if node is a JSON 64-bit signed integer
    bool get_i64(int64_t& i64) const;

    /// @brief check if node is a JSON 32-bit unsigned integer
    bool get_u32(uint32_t& u32) const;

    /// @brief check if node is a JSON 64-bit unsigned integer
    bool get_u64(uint64_t& u64) const;

    /// @brief check if node is a 32-bit floating point
    bool get_f32(float& f32) const;

    /// @brief check if node is a JSON string
    bool get_string(std::string& str) const;

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

static bool parse_json(JSONDocument dst, const View& view, std::string& error);

static_assert((int)JSON_TYPE_NULL == (int)rapidjson::kNullType);
static_assert((int)JSON_TYPE_FALSE == (int)rapidjson::kFalseType);
static_assert((int)JSON_TYPE_TRUE == (int)rapidjson::kTrueType);
static_assert((int)JSON_TYPE_OBJECT == (int)rapidjson::kObjectType);
static_assert((int)JSON_TYPE_ARRAY == (int)rapidjson::kArrayType);
static_assert((int)JSON_TYPE_STRING == (int)rapidjson::kStringType);
static_assert((int)JSON_TYPE_NUMBER == (int)rapidjson::kNumberType);

static const char* get_error_code_cstr(rapidjson::ParseErrorCode code);

struct JSONValueObj
{
    rapidjson::Value value;
    JSONDocumentObj* doc;
};

struct JSONDocumentObj
{
    rapidjson::Document doc;
    PoolAllocator nodePA;
    JSONValue root;
    byte* fileBuffer;

    inline rapidjson::MemoryPoolAllocator<>& allocator()
    {
        return doc.GetAllocator();
    }

    void free_nodes()
    {
        if (!nodePA)
            return;

        for (auto ite = nodePA.begin(); ite; ++ite)
        {
            auto* node = static_cast<JSONValueObj*>(ite.data());
            (&node->value)->~GenericValue();
        }

        PoolAllocator::destroy(nodePA);
        nodePA = {};
    }

    JSONValueObj* alloc_node()
    {
        JSONValueObj* node = (JSONValueObj*)nodePA.allocate();
        node->doc = this;
        new (&node->value) rapidjson::Value();
        return node;
    }
};

JSONType JSONValue::type() const
{
    return static_cast<JSONType>(mObj->value.GetType());
}

bool JSONValue::is_false() const
{
    return mObj->value.GetType() == rapidjson::kFalseType;
}

bool JSONValue::is_true() const
{
    return mObj->value.GetType() == rapidjson::kTrueType;
}

bool JSONValue::is_object() const
{
    return mObj->value.GetType() == rapidjson::kObjectType;
}

bool JSONValue::is_array() const
{
    return mObj->value.GetType() == rapidjson::kArrayType;
}

bool JSONValue::is_number() const
{
    return mObj->value.GetType() == rapidjson::kNumberType;
}

bool JSONValue::get_bool(bool& b) const
{
    if (mObj->value.IsBool())
    {
        b = mObj->value.GetBool();
        return true;
    }

    return false;
}

bool JSONValue::get_i32(int32_t& i32) const
{
    if (mObj->value.IsInt())
    {
        i32 = (int32_t)mObj->value.GetInt();
        return true;
    }

    return false;
}

bool JSONValue::get_i64(int64_t& i64) const
{
    if (mObj->value.IsInt64())
    {
        i64 = mObj->value.GetInt64();
        return true;
    }

    return false;
}

bool JSONValue::get_u32(uint32_t& u32) const
{
    if (mObj->value.IsUint())
    {
        u32 = (uint32_t)mObj->value.GetUint();
        return true;
    }

    return false;
}

bool JSONValue::get_u64(uint64_t& u64) const
{
    if (mObj->value.IsUint64())
    {
        u64 = (uint32_t)mObj->value.GetUint64();
        return true;
    }

    return false;
}

bool JSONValue::get_f32(float& f32) const
{
    if (mObj->value.IsFloat())
    {
        f32 = mObj->value.GetFloat();
        return true;
    }

    return false;
}

bool JSONValue::get_string(std::string& str) const
{
    if (mObj->value.IsString())
    {
        // NOTE: According to RFC 4627, JSON strings can contain Unicode U+0000,
        //       this ensures all null bytes in JSON strings are loaded into std::string
        str = std::string(mObj->value.GetString(), mObj->value.GetStringLength());
        return true;
    }

    return false;
}

int JSONValue::size()
{
    if (mObj->value.IsArray())
        return mObj->value.Size();

    if (mObj->value.IsObject())
        return mObj->value.MemberCount();

    return -1;
}

JSONValue JSONValue::get_member(const char* member)
{
    if (!is_object())
        return {};

    rapidjson::Value::MemberIterator ite = mObj->value.FindMember(member);
    if (ite == mObj->value.MemberEnd())
        return {};

    JSONDocumentObj* doc = mObj->doc;
    JSONValueObj* node = doc->alloc_node();
    node->value = std::move(ite->value);

    return {node};
}

JSONValue JSONValue::get_index(int idx)
{
    if (!is_array() || idx < 0 || idx >= (int)mObj->value.Size())
        return {};

    JSONDocumentObj* doc = mObj->doc;
    JSONValueObj* node = doc->alloc_node();
    node->value = std::move(mObj->value[idx]);

    return {node};
}

JSONDocument JSONDocument::create()
{
    JSONDocumentObj* obj = heap_new<JSONDocumentObj>(MEMORY_USAGE_MEDIA);
    obj->fileBuffer = nullptr;

    return {obj};
}

void JSONDocument::destroy(JSONDocument doc)
{
    JSONDocumentObj* obj = doc;

    obj->free_nodes();

    if (obj->fileBuffer)
        heap_free(obj->fileBuffer);

    heap_delete<JSONDocumentObj>(obj);
}

JSONValue JSONDocument::get_root()
{
    return mObj->root;
}

static bool parse_json(JSONDocument dst, const View& view, std::string& error)
{
    LD_PROFILE_SCOPE;

    JSONDocumentObj* docObj = dst.unwrap();
    docObj->free_nodes();

    error.clear();
    rapidjson::ParseResult result = docObj->doc.Parse(view.data, view.size);

    if (!result)
    {
        const char* errorCstr = get_error_code_cstr(result.Code());
        error = std::format("rapidjson error at offset {}: {}", result.Offset(), errorCstr);
        return false;
    }

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_MEDIA;
    paI.isMultiPage = true;
    paI.blockSize = sizeof(JSONValueObj);
    paI.pageSize = 64; // nodes per page
    docObj->nodePA = PoolAllocator::create(paI);

    JSONValueObj* rootNode = docObj->alloc_node();
    rootNode->value.CopyFrom(docObj->doc, docObj->doc.GetAllocator());
    docObj->root = {rootNode};

    return true;
}

/// @brief JSON reader implementation
struct JSONReaderObj
{
    JSONDocument doc{};
    Stack<JSONValue> scope;

    inline JSONValue get_member(const char* member)
    {
        JSONValue top = scope.top();
        LD_ASSERT(top.is_object());

        return top.get_member(member);
    }

    inline JSONValue get_index(int index)
    {
        JSONValue top = scope.top();
        LD_ASSERT(top.is_array());

        return top.get_index(index);
    }
};

JSONReader JSONReader::create(const View& toml, std::string& err)
{
    JSONDocument doc = JSONDocument::create();

    if (!parse_json(doc, toml, err))
    {
        JSONDocument::destroy(doc);
        return {};
    }

    auto* obj = heap_new<JSONReaderObj>(MEMORY_USAGE_MEDIA);
    obj->doc = doc;

    return JSONReader(obj);
}

void JSONReader::destroy(JSONReader reader)
{
    auto* obj = reader.unwrap();

    LD_ASSERT(obj->scope.empty());

    if (obj->doc)
    {
        JSONDocument::destroy(obj->doc);
        obj->doc = {};
    }

    heap_delete<JSONReaderObj>(obj);
}

bool JSONReader::is_array_scope()
{
    LD_ASSERT(!mObj->scope.empty());

    return mObj->scope.top().is_array();
}

bool JSONReader::is_object_scope()
{
    LD_ASSERT(!mObj->scope.empty());

    return mObj->scope.top().is_object();
}

bool JSONReader::enter_root_object()
{
    JSONValue root = mObj->doc.get_root();

    if (!root || !root.is_object())
        return false;

    mObj->scope.push(root);
    return true;
}

bool JSONReader::enter_root_array(int& size)
{
    JSONValue root = mObj->doc.get_root();

    if (!root || !root.is_array())
        return false;

    size = root.size();
    mObj->scope.push(root);

    return true;
}

bool JSONReader::enter_object(const char* key)
{
    JSONValue top = mObj->scope.top();
    LD_ASSERT(top.is_object());

    JSONValue value = top.get_member(key);
    if (!value || !value.is_object())
        return false;

    mObj->scope.push(value);

    return true;
}

bool JSONReader::enter_object(int index)
{
    JSONValue top = mObj->scope.top();
    LD_ASSERT(top.is_array());

    JSONValue value = top.get_index(index);
    if (!value || !value.is_object())
        return false;

    mObj->scope.push(value);

    return true;
}

bool JSONReader::enter_array(const char* key, int& size)
{
    JSONValue top = mObj->scope.top();
    LD_ASSERT(top.is_object());

    JSONValue value = top.get_member(key);
    if (!value || !value.is_array())
        return false;

    size = value.size();
    mObj->scope.push(value);

    return true;
}

bool JSONReader::enter_array(int index, int& size)
{
    JSONValue top = mObj->scope.top();
    LD_ASSERT(top.is_array());

    JSONValue value = top.get_index(index);
    if (!value || !value.is_array())
        return false;

    size = value.size();
    mObj->scope.push(value);

    return true;
}

void JSONReader::exit()
{
    LD_ASSERT(!mObj->scope.empty());

    mObj->scope.pop();
}

bool JSONReader::read_bool(const char* key, bool& b)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_bool(b);
}

bool JSONReader::read_bool(int index, bool& b)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_bool(b);
}

bool JSONReader::read_i32(const char* key, int32_t& i32)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_i32(i32);
}

bool JSONReader::read_i32(int index, int32_t& i32)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_i32(i32);
}

bool JSONReader::read_i64(const char* key, int64_t& i64)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_i64(i64);
}

bool JSONReader::read_i64(int index, int64_t& i64)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_i64(i64);
}

bool JSONReader::read_u32(const char* key, uint32_t& u32)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_u32(u32);
}

bool JSONReader::read_u32(int index, uint32_t& u32)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_u32(u32);
}

bool JSONReader::read_u64(const char* key, uint64_t& u64)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_u64(u64);
}

bool JSONReader::read_u64(int index, uint64_t& u64)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_u64(u64);
}

bool JSONReader::read_f32(const char* key, float& f32)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_f32(f32);
}

bool JSONReader::read_f32(int index, float& f32)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_f32(f32);
}

bool JSONReader::read_string(const char* key, std::string& str)
{
    JSONValue value = mObj->get_member(key);

    return value && value.get_string(str);
}

bool JSONReader::read_string(int index, std::string& str)
{
    JSONValue value = mObj->get_index(index);

    return value && value.get_string(str);
}

// clang-format off
#define KASE(CODE) case rapidjson::CODE: return #CODE
// clang-format on

const char* get_error_code_cstr(rapidjson::ParseErrorCode code)
{
    switch (code)
    {
        KASE(kParseErrorNone);
        KASE(kParseErrorDocumentEmpty);
        KASE(kParseErrorDocumentRootNotSingular);
        KASE(kParseErrorValueInvalid);
        KASE(kParseErrorObjectMissName);
        KASE(kParseErrorObjectMissColon);
        KASE(kParseErrorObjectMissCommaOrCurlyBracket);
        KASE(kParseErrorArrayMissCommaOrSquareBracket);
        KASE(kParseErrorStringUnicodeEscapeInvalidHex);
        KASE(kParseErrorStringUnicodeSurrogateInvalid);
        KASE(kParseErrorStringEscapeInvalid);
        KASE(kParseErrorStringMissQuotationMark);
        KASE(kParseErrorStringInvalidEncoding);
        KASE(kParseErrorNumberTooBig);
        KASE(kParseErrorNumberMissFraction);
        KASE(kParseErrorNumberMissExponent);
        KASE(kParseErrorTermination);
        KASE(kParseErrorUnspecificSyntaxError);
    }

    return "unknown";
}

#undef KASE

struct RapidJSONEventHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, RapidJSONEventHandler>
{
    JSONCallback callbacks;
    void* user;

    inline bool Null()
    {
        return callbacks.onNull(user);
    }

    inline bool Bool(bool b)
    {
        return callbacks.onBool(b, user);
    }

    inline bool Int(int i)
    {
        return callbacks.onI64((int64_t)i, user);
    }

    inline bool Uint(unsigned u)
    {
        return callbacks.onU64((uint64_t)u, user);
    }

    inline bool Int64(int64_t i)
    {
        return callbacks.onI64((int64_t)i, user);
    }

    inline bool Uint64(uint64_t u)
    {
        return callbacks.onU64((uint64_t)u, user);
    }

    inline bool Double(double d)
    {
        return callbacks.onF64(d, user);
    }

    inline bool String(const char* str, rapidjson::SizeType length, bool copy)
    {
        return callbacks.onString({str, (size_t)length}, user);
    }

    inline bool StartObject()
    {
        return callbacks.onEnterObject(user);
    }

    inline bool EndObject(rapidjson::SizeType memberCount)
    {
        return callbacks.onLeaveObject((size_t)memberCount, user);
    }

    inline bool Key(const char* str, rapidjson::SizeType length, bool copy)
    {
        return callbacks.onKey({str, (size_t)length}, user);
    }

    inline bool StartArray()
    {
        return callbacks.onEnterArray(user);
    }

    inline bool EndArray(rapidjson::SizeType elementCount)
    {
        return callbacks.onLeaveArray((size_t)elementCount, user);
    }
};

bool JSONParser::parse(const View& json, std::string& error, const JSONCallback& callbacks, void* user)
{
    RapidJSONEventHandler eventHandler{};
    eventHandler.callbacks = callbacks;
    eventHandler.user = user;

    rapidjson::MemoryStream memoryStream(json.data, json.size);

    rapidjson::Reader reader;
    rapidjson::ParseResult result = reader.Parse(memoryStream, eventHandler);

    if (!result)
    {
        const char* errorCstr = get_error_code_cstr(result.Code());
        error = std::format("rapidjson error at offset {}: {}", result.Offset(), errorCstr);
        return false;
    }

    return true;
}

} // namespace LD
