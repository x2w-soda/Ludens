#pragma once

#include <cstddef>
#include <cstdint>

namespace LD {

enum ValueType : uint16_t
{
    VALUE_TYPE_F32 = 0,
    VALUE_TYPE_F64,
    VALUE_TYPE_I8,
    VALUE_TYPE_U8,
    VALUE_TYPE_I16,
    VALUE_TYPE_U16,
    VALUE_TYPE_I32,
    VALUE_TYPE_U32,
    VALUE_TYPE_I64,
    VALUE_TYPE_U64,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_ENUM_COUNT,
};

/// @brief Tagged union of pointers.
struct Value
{
    ValueType type;
    union
    {
        bool* b;
        float* f32;
        double* f64;
        int8_t* i8;
        int16_t* i16;
        int32_t* i32;
        int64_t* i64;
        uint8_t* u8;
        uint16_t* u16;
        uint32_t* u32;
        uint64_t* u64;
    };
};

/// @brief Get byte size of value type, or zero for varying length values.
size_t get_value_byte_size(ValueType type);

/// @brief Get static C string for value type name.
const char* get_value_cstr(ValueType type);

} // namespace LD
