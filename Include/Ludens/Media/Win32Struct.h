#pragma once

#include <cstdint>

// Win32Struct.h
// - defines some structs from Microsoft Windows headers in a platform-agnostic way.
// - sizes and offsets are statically asserted in Win32Struct.cpp.
// - does not drag platform specific headers from Microsoft Windows.

namespace LD {

#pragma pack(push, 2)

/// @brief Format of the header of RT_GROUP_ICON resource.
struct GRPICONDIR
{
    uint16_t idReserved;
    uint16_t idType;
    uint16_t idCount;
};

/// @brief Format of an entry of RT_GROUP_ICON resource.
struct GRPICONDIRENTRY
{
    uint8_t bWidth;
    uint8_t bHeight;
    uint8_t bColorCount;
    uint8_t bReserved;
    uint16_t wPlanes;
    uint16_t wBitCount;
    uint32_t dwBytesInRes;
    uint16_t nId;
};

#pragma pack(pop)

} // namespace LD