#pragma once

#include <string_view>
#include <cstdint>

namespace LD {

template <typename ETokenType>
struct Token
{
    ETokenType type;
    Token* next;
    std::string_view span;
};

} // namespace LD