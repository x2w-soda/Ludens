#include "Core/RenderService/Lib/TextureResources.h"

namespace LD
{

void TextureResources::Startup(RDevice device)
{
    LD_DEBUG_ASSERT(device);
    mDevice = device;
}

void TextureResources::Cleanup()
{
    if (mWhitePixel)
        mDevice.DeleteTexture(mWhitePixel);

    mDevice.ResetHandle();
}

RTexture TextureResources::GetWhitePixel()
{
    if (!mWhitePixel)
    {
        u32 pixel = 0xFFFFFFFF;

        RTextureInfo info;
        info.Type = RTextureType::Texture2D;
        info.Format = RTextureFormat::RGBA8;
        info.Data = (const u8*)&pixel;
        info.Size = sizeof(pixel);
        info.Width = 1;
        info.Height = 1;
        
        mDevice.CreateTexture(mWhitePixel, info);
        LD_DEBUG_ASSERT(mWhitePixel);
    }

    return mWhitePixel;
}

} // namespace LD