#include <Ludens/Serial/Format/JSON.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/System/Memory.h>
#include <format>
#include <string>

// NOTE: rapidjson is a header only C++ library,
//       definitely hide the headers from user via pimpl
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace LD {

static_assert((int)JSON_TYPE_NULL == (int)rapidjson::kNullType);
static_assert((int)JSON_TYPE_FALSE == (int)rapidjson::kFalseType);
static_assert((int)JSON_TYPE_TRUE == (int)rapidjson::kTrueType);
static_assert((int)JSON_TYPE_OBJECT == (int)rapidjson::kObjectType);
static_assert((int)JSON_TYPE_ARRAY == (int)rapidjson::kArrayType);
static_assert((int)JSON_TYPE_STRING == (int)rapidjson::kStringType);
static_assert((int)JSON_TYPE_NUMBER == (int)rapidjson::kNumberType);

static const char* get_error_code_cstr(rapidjson::ParseErrorCode code);

struct JSONNodeObj
{
    rapidjson::Value value;
    JSONDocumentObj* doc;
};

struct JSONDocumentObj
{
    rapidjson::Document doc;
    PoolAllocator nodePA;
    JSONNode root;

    inline rapidjson::MemoryPoolAllocator<>& allocator()
    {
        return doc.GetAllocator();
    }

    JSONNodeObj* alloc_node()
    {
        JSONNodeObj* node = (JSONNodeObj*)nodePA.allocate();
        node->doc = this;
        return node;
    }
};

JSONType JSONNode::get_type() const
{
    return static_cast<JSONType>(mObj->value.GetType());
}

bool JSONNode::is_false() const
{
    return mObj->value.GetType() == rapidjson::kFalseType;
}

bool JSONNode::is_true() const
{
    return mObj->value.GetType() == rapidjson::kTrueType;
}

bool JSONNode::is_object() const
{
    return mObj->value.GetType() == rapidjson::kObjectType;
}

bool JSONNode::is_array() const
{
    return mObj->value.GetType() == rapidjson::kArrayType;
}

bool JSONNode::is_string(std::string* str) const
{
    bool match = mObj->value.GetType() == rapidjson::kStringType;

    if (match && str)
    {
        // NOTE: According to RFC 4627, JSON strings can contain Unicode U+0000,
        //       this ensures all null bytes in JSON strings are loaded into std::string
        *str = std::string(mObj->value.GetString(), mObj->value.GetStringLength());
    }

    return match;
}

bool JSONNode::is_number() const
{
    return mObj->value.GetType() == rapidjson::kNumberType;
}

bool JSONNode::is_i32(int32_t* i32) const
{
    bool match = mObj->value.IsInt();

    if (match && i32)
        *i32 = (int32_t)mObj->value.GetInt();

    return match;
}

bool JSONNode::is_i64(int64_t* i64) const
{
    bool match = mObj->value.IsInt64();

    if (match && i64)
        *i64 = mObj->value.GetInt64();

    return match;
}

bool JSONNode::is_u32(uint32_t* u32) const
{
    bool match = mObj->value.IsUint();

    if (match && u32)
        *u32 = (uint32_t)mObj->value.GetUint();

    return match;
}

bool JSONNode::is_u64(uint64_t* u64) const
{
    bool match = mObj->value.IsUint64();

    if (match && u64)
        *u64 = mObj->value.GetUint64();

    return match;
}

int JSONNode::get_size()
{
    if (mObj->value.IsArray())
        return mObj->value.Size();

    if (mObj->value.IsObject())
        return mObj->value.MemberCount();

    return -1;
}

JSONNode JSONNode::get_member(const char* member)
{
    if (!is_object())
        return {};

    rapidjson::Value::MemberIterator ite = mObj->value.FindMember(member);
    if (ite == mObj->value.MemberEnd())
        return {};

    JSONDocumentObj* doc = mObj->doc;
    JSONNodeObj* node = doc->alloc_node();
    node->value = std::move(ite->value);

    return {node};
}

JSONNode JSONNode::get_index(int idx)
{
    if (!is_array() || idx < 0 || idx >= mObj->value.Size())
        return {};

    JSONDocumentObj* doc = mObj->doc;
    JSONNodeObj* node = doc->alloc_node();
    node->value = std::move(mObj->value[idx]);

    return {node};
}

JSONDocument JSONDocument::create()
{
    JSONDocumentObj* obj = heap_new<JSONDocumentObj>(MEMORY_USAGE_SERIAL);

    return {obj};
}

void JSONDocument::destroy(JSONDocument doc)
{
    JSONDocumentObj* obj = doc;

    if (obj->nodePA)
        PoolAllocator::destroy(obj->nodePA);

    heap_delete<JSONDocumentObj>(obj);
}

bool JSONDocument::parse(const char* json, std::string& error)
{
    if (mObj->nodePA)
    {
        PoolAllocator::destroy(mObj->nodePA);
        mObj->nodePA = {};
    }

    error.clear();
    rapidjson::ParseResult result = mObj->doc.Parse(json);

    if (!result)
    {
        const char* errorCstr = get_error_code_cstr(result.Code());
        error = std::format("rapidjson error at offset {}: {}", result.Offset(), errorCstr);
        return false;
    }

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_SERIAL;
    paI.isMultiPage = true;
    paI.blockSize = sizeof(JSONNodeObj);
    paI.pageSize = 64;
    mObj->nodePA = PoolAllocator::create(paI);

    JSONNodeObj* rootNode = mObj->alloc_node();
    rootNode->value.CopyFrom(mObj->doc, mObj->doc.GetAllocator());
    mObj->root = {rootNode};

    return true;
}

JSONNode JSONDocument::get_root()
{
    return mObj->root;
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
    }

    return "unknown";
}

#undef KASE

} // namespace LD