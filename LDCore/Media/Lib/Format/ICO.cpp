#include <Ludens/Media/Format/BMP.h>
#include <Ludens/Media/Format/ICO.h>
#include <Ludens/Media/Format/PNG.h>
#include <Ludens/Profiler/Profiler.h>
#include <cstddef>

namespace LD {

// must be identical to ICO file format
// https://en.wikipedia.org/wiki/ICO_(file_format)#File_structure

static_assert(sizeof(ICONDIR) == 6);
static_assert(offsetof(ICONDIR, idReserved) == 0);
static_assert(offsetof(ICONDIR, idType) == 2);
static_assert(offsetof(ICONDIR, idCount) == 4);

static_assert(sizeof(ICONDIRENTRY) == 16);
static_assert(offsetof(ICONDIRENTRY, bWidth) == 0);
static_assert(offsetof(ICONDIRENTRY, bHeight) == 1);
static_assert(offsetof(ICONDIRENTRY, bColorCount) == 2);
static_assert(offsetof(ICONDIRENTRY, bReserved) == 3);
static_assert(offsetof(ICONDIRENTRY, wPlanes) == 4);
static_assert(offsetof(ICONDIRENTRY, wBitCount) == 6);
static_assert(offsetof(ICONDIRENTRY, dwBytesInRes) == 8);
static_assert(offsetof(ICONDIRENTRY, dwImageOffset) == 12);

/// @brief Try and restore full BMP data from ICO DIB data.
/// @param dibData Original ICO DIB data
/// @param dibSize Original ICO DIB byte size
/// @param outBMPData Outputs full BMP data on success.
/// @param outANDmask Outputs ICO AND mask address on success. Address is from original buffer, not from outBMPData.
/// @param outHasAlpha Outputs whehter BMP has alpha channel, ICO AND mask is used to derive alpha for 3-Channel BMP data.
/// @return True on success.
static bool patch_ico_dib(const byte* dibData, uint64_t dibSize, std::vector<byte>& outBMPData, const uint8_t** outANDmask, bool* outHasAlpha)
{
    if (!dibData || dibSize < sizeof(BITMAPINFOHEADER))
        return false;

    const BITMAPINFOHEADER* srcInfoHeader = (const BITMAPINFOHEADER*)dibData;
    std::vector<byte> dibCopy(dibData, dibData + dibSize);
    BITMAPINFOHEADER* dstInfoHeader = (BITMAPINFOHEADER*)dibCopy.data();
    dstInfoHeader->biHeight /= 2;

    // Number of colors is 2^N for bit count of N, but biClrUsed takes precedence.
    // N = 16 and N = 32 do not use color tables.
    uint32_t colorTableSize = 0;
    if (srcInfoHeader->biBitCount <= 8)
    {
        uint32_t colors = srcInfoHeader->biClrUsed ? srcInfoHeader->biClrUsed : (1u << srcInfoHeader->biBitCount);
        colorTableSize = colors * 4;
    }

    const uint32_t pixelOffset = sizeof(BITMAPFILEHEADER) + srcInfoHeader->biSize + colorTableSize;

    BITMAPFILEHEADER dstFileHeader{};
    dstFileHeader.bfType = 0x4D42;
    dstFileHeader.bfOffBits = pixelOffset;
    dstFileHeader.bfSize = static_cast<uint32_t>(sizeof(BITMAPFILEHEADER) + dibCopy.size());

    outBMPData.resize(dstFileHeader.bfSize);
    memcpy(outBMPData.data(), &dstFileHeader, sizeof(dstFileHeader));
    memcpy(outBMPData.data() + sizeof(dstFileHeader), dibCopy.data(), dibCopy.size());

    const int width = (int)dstInfoHeader->biWidth;
    const int height = (int)dstInfoHeader->biHeight;
    const int bpp = (int)dstInfoHeader->biBitCount;

    const uint32_t xorStride = ((width * bpp + 31) / 32) * 4;
    const uint32_t xorSize = xorStride * height;
    const uint32_t andStride = ((width + 31) / 32) * 4; // alpha mask is guaranteed 1 bit per pixel
    const uint32_t andSize = andStride * height;
    const uint32_t andOffset = dstInfoHeader->biSize + colorTableSize + xorSize;

    if (andOffset + andSize > dibSize)
        return false; // DIB data not large enough to contain AND mask.

    *outANDmask = dibData + andOffset;
    *outHasAlpha = bpp == 32;

    return true;
}

/// @brief The ico AND mask is a bitmask, we have to apply this to the BMP bitmap data manually.
static void patch_ico_AND_mask(uint8_t* rgba, int width, int height, const uint8_t* andMask)
{
    // Byte stride for a scanline. Since the AND mask is a bitmask, a width of 32 pixels
    // only correspond to 4 bytes of mask data. The stride is rounded up to 4 byte multiples.
    int maskStride = ((width + 31) / 32) * 4;

    for (int y = 0; y < height; y++)
    {
        const uint8_t* row = andMask + (height - 1 - y) * maskStride;

        for (int x = 0; x < width; x++)
        {
            int byte = x >> 3;
            int bit = 7 - (x & 7);

            bool transparent = (row[byte] >> bit) & 1;
            rgba[(y * width + x) * 4 + 3] = transparent ? 0 : 255;
        }
    }
}

void ICOData::create_bitmaps_from_file(const FS::Path& icoPath, std::vector<Bitmap>& outBitmaps)
{
    LD_PROFILE_SCOPE;

    outBitmaps.clear();

    std::vector<byte> icoData;
    if (!FS::read_file_to_vector(icoPath, icoData))
        return;

    create_bitmaps_from_file_data(icoData.data(), icoData.size(), outBitmaps);
}

void ICOData::create_bitmaps_from_file_data(const void* icoData, size_t icoSize, std::vector<Bitmap>& outBitmaps)
{
    LD_PROFILE_SCOPE;

    outBitmaps.clear();

    const ICONDIR* iconDir = (const ICONDIR*)icoData;
    if (iconDir->idReserved != 0 || iconDir->idType != 1)
        return; // invalid ico header

    const ICONDIRENTRY* entries = (ICONDIRENTRY*)((byte*)icoData + sizeof(ICONDIR));

    for (uint16_t i = 0; i < iconDir->idCount; i++)
    {
        const ICONDIRENTRY& e = entries[i];

        if (e.dwImageOffset + e.dwBytesInRes > icoSize)
            continue; // out of bounds entry

        const void* bitmapData = (byte*)icoData + e.dwImageOffset;
        uint64_t bitmapSize = e.dwBytesInRes;

        if (PNGData::test_magic(bitmapData, bitmapSize))
        {
            Bitmap bitmap = Bitmap::create_from_file_data((uint32_t)bitmapSize, bitmapData);

            if (bitmap)
                outBitmaps.push_back(bitmap);

            continue;
        }

        // NOTE: Each ICO entry is either a full PNG image or a BMP image *without* the BITMAPFILEHEADER struct (thank you microsoft).
        //       For the latter case we have to reconstruct the BMP header in memory before passing to Bitmap::create_* APIs,
        //       which expect complete headers for image data. We also extract the AND mask that is used to reconstruct the alpha
        //       channel. Note that the AND mask is introduced by the ICO container format, not BMP.
        const uint8_t* icoANDMask = nullptr;
        bool bmpHasAlpha = false;
        std::vector<byte> tmpBmpData;
        if (!patch_ico_dib((const byte*)bitmapData, bitmapSize, tmpBmpData, &icoANDMask, &bmpHasAlpha))
            continue; // bad DIB data

        Bitmap bitmap = Bitmap::create_from_file_data((uint32_t)tmpBmpData.size(), tmpBmpData.data());
        if (!bitmap)
            continue;

        if (!bmpHasAlpha && icoANDMask)
        {
            patch_ico_AND_mask(bitmap.data(), (int)bitmap.width(), (int)bitmap.height(), icoANDMask);
        }

        outBitmaps.push_back(bitmap);
    }
}

} // namespace LD