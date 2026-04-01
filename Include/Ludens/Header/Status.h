#pragma once

#include <string>

namespace LD {

template <typename TStatusEnum>
struct TStatus
{
    std::string str;
    TStatusEnum type;
};

} // namespace LD