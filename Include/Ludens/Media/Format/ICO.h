#pragma once

#include <cstdint>

namespace LD {

struct ICONDIR
{
    uint16_t idReserved;
    uint16_t idType;
    uint16_t idCount;
};

struct ICONDIRENTRY
{
    uint8_t bWidth;
    uint8_t bHeight;
    uint8_t bColorCount;
    uint8_t bReserved;
    uint16_t wPlanes;
    uint16_t wBitCount;
    uint32_t dwBytesInRes;
    uint32_t dwImageOffset;
};

} // namespace LD