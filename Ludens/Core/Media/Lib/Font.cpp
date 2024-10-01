#include <cstdlib>
#include "Core/Header/Include/Error.h"
#include "Core/Media/Include/Font.h"
#include "Core/OS/Include/Memory.h"

namespace LD {

FontTTF::FontTTF(const FontTTFInfo& info)
{
    mName = info.Name;
    mPixelSize = info.PixelSize;
    mTTFSize = info.TTFSize;
    mTTFData = MemoryAlloc(mTTFSize);

    memcpy(mTTFData, info.TTFData, info.TTFSize);

    int fontOffset = stbtt_GetFontOffsetForIndex((unsigned char*)info.TTFData, 0);
    LD_DEBUG_ASSERT(fontOffset >= 0);

    int result = stbtt_InitFont(&mFontInfo, (const unsigned char*)info.TTFData, fontOffset);
    LD_DEBUG_ASSERT(result != 0);

    SetPixelSize(info.PixelSize);
}

FontTTF::~FontTTF()
{
    MemoryFree((void*)mTTFData);
    mTTFData = nullptr;
}

std::string FontTTF::GetName() const
{
    return mName;
}

float FontTTF::GetPixelSize() const
{
    return mPixelSize;
}

void FontTTF::SetPixelSize(float size)
{
    mPixelSize = size;

    float scale = stbtt_ScaleForPixelHeight(&mFontInfo, mPixelSize);
    stbtt_GetFontVMetrics(&mFontInfo, &mAscent, &mDescent, &mLineGap);

    mAscent = int(std::abs(mAscent) * scale);
    mDescent = int(std::abs(mDescent) * scale);
    mLineGap = int(std::abs(mLineGap) * scale);
    mLineSpace = mAscent + mDescent + mLineGap;
}

const void* FontTTF::GetData()
{
    return mTTFData;
}

void FontTTF::GetVerticalMetrics(int* ascent, int* descent, int* lineGap, int* lineSpace)
{
    if (ascent)
        *ascent = mAscent;

    if (descent)
        *descent = mDescent;

    if (lineGap)
        *lineGap = mLineGap;

    if (lineSpace)
        *lineSpace = mLineSpace;
}

} // namespace LD