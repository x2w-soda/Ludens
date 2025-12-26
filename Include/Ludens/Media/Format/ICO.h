#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <Ludens/Media/Bitmap.h>
#include <Ludens/System/FileSystem.h>

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

struct ICOData
{
    /// @brief Create bitmaps from .ico file.
    /// @param icoPath Path to an .ico file on disk.
    /// @param outBitmaps Outputs bitmaps found in .ico file.
    /// @warning Caller's responsibility to destroy output Bitmap images.
    static void create_bitmaps_from_file(const FS::Path& icoPath, std::vector<Bitmap>& outBitmaps);

    /// @brief Create bitmaps from .ico data in RAM.
    /// @param icoData Data from .ico file.
    /// @param icoSize Byte size of icoData.
    /// @param outBitmaps Outputs bitmaps found in .ico file.
    /// @warning Caller's responsibility to destroy output Bitmap images.
    static void create_bitmaps_from_file_data(const void* icoData, size_t icoSize, std::vector<Bitmap>& outBitmaps);
};

} // namespace LD