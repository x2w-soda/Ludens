#include <Ludens/Header/Types.h>
#include <Ludens/Serial/Value.h>
#include <type_traits>

namespace LD {

// clang-format off
struct
{
    ValueType type;
    const char* cstr;
    size_t byteSize;
} sValueTable[] = {
    {VALUE_TYPE_F32,  "f32",  4},
    {VALUE_TYPE_F64,  "f64",  8},
    {VALUE_TYPE_I8,   "i8",   1},
    {VALUE_TYPE_U8,   "u8",   1},
    {VALUE_TYPE_I16,  "i16",  2},
    {VALUE_TYPE_U16,  "u16",  2},
    {VALUE_TYPE_I32,  "i32",  4},
    {VALUE_TYPE_U32,  "u32",  4},
    {VALUE_TYPE_I64,  "i64",  8},
    {VALUE_TYPE_U64,  "u64",  8},
    {VALUE_TYPE_BOOL, "bool", 1},
};
// clang-format on

static_assert(IsTrivial<Value>);
static_assert(sizeof(sValueTable) / sizeof(*sValueTable) == VALUE_TYPE_ENUM_COUNT);

size_t get_value_byte_size(ValueType type)
{
    return sValueTable[(int)type].byteSize;
}

const char* get_value_cstr(ValueType type)
{
    return sValueTable[(int)type].cstr;
}

} // namespace LD