#pragma once

#include <string>

namespace LD {

template <typename TErrorEnum>
struct TError
{
    std::string str;
    TErrorEnum type;
};

} // namespace LD