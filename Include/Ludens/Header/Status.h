#pragma once

#include <Ludens/DSA/String.h>

namespace LD {

template <typename TStatusEnum>
struct TStatus
{
    String str;
    TStatusEnum type;
};

} // namespace LD