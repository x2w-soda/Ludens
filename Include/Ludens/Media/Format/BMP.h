#pragma once

#include <cstdint>

namespace LD {

#pragma pack(push, 2)

struct BITMAPFILEHEADER
{
    uint16_t bfType;      // the file type, must be 0x4D42 ('BM')
    uint32_t bfSize;      // the byte size of the bitmap file
    uint16_t bfReserved1; // reserved, must be zero
    uint16_t bfReserved2; // reserved, must be zero
    uint32_t bfOffBits;   // the byte offset from the beginning of BITMAPFILEHEADER structure to the bitmap bits
};

struct BITMAPINFOHEADER
{
    uint32_t biSize;         // byte size required by this struct. Does not include color table size
    int32_t biWidth;         // pixel width of the bitmap
    int32_t biHeight;        // pixel height of the bitmap
    uint16_t biPlanes;       // number of planes for target device
    uint16_t biBitCount;     // number of bits per pixel (bpp)
    uint32_t biCompression;  // compression hints
    uint32_t biSizeImage;    // byte size of image, can be 0 for uncompressed RGB
    int32_t biXPelsPerMeter; // horizontal pixels per meter resolution
    int32_t biYPelsPerMeter; // vertical pixels per meter resolution
    uint32_t biClrUsed;      // number of color indices in the color table that are actually used by the bitmap
    uint32_t biClrImportant; // number of color indices that are important for displaying the bitmap
};

#pragma pack(pop)

} // namespace LD