#pragma once

#include <cstddef>

namespace LD {

struct PNGData
{
    /// @brief Test PNG file magic bytes.
    static bool test_magic(const void* fileData, size_t fileSize);
};

} // namespace LD