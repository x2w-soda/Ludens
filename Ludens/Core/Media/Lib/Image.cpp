#include <iostream>
#include <cstring>
#include <cstdio>
#include <stb/stb_image.h>
#include "Core/Header/Include/Error.h"
#include "Core/Media/Include/Image.h"

namespace LD
{

Image::Image() : mWidth(0), mHeight(0), mChannels(0), mByteSize(0), mPixels(nullptr)
{
}

Image::Image(int width, int height, int channels, const Byte* pixels)
    : mWidth(width), mHeight(height), mChannels(channels)
{
    mByteSize = width * height * channels; // assumes 8-bit depth
    mPixels = (Byte*)MemoryAlloc(mByteSize);

    // TODO: replace copy with some sort of move semantics
    memcpy(mPixels, pixels, mByteSize);

    LD_DEBUG_ASSERT(width > 0 && height > 0 && mPixels);
}

Image::~Image()
{
    MemoryFree(mPixels);
}

int Image::GetWidth() const
{
    return mWidth;
}

int Image::GetHeight() const
{
    return mHeight;
}

int Image::GetChannels() const
{
    return mChannels;
}

const Byte* Image::Pixels() const
{
    return mPixels;
}

int Image::ByteSize() const
{
    return mByteSize;
}

bool LoadImage(u8** pixels, const char* path, int* width, int* height, int* channels)
{

    stbi_uc* result = stbi_load(path, width, height, channels, STBI_rgb_alpha);
    LD_DEBUG_ASSERT(result);

    if (result == NULL)
        return false;

    *pixels = static_cast<u8*>(result);

    return true;
}

Ref<Image> ImageLoader::LoadImage(const Path& path)
{
    int width, height, ch;
    return LoadImage(path, width, height, ch);
}

Ref<Image> ImageLoader::LoadImage(const Path& path, int& width, int& height, int& ch)
{
    stbi_uc* pixels = stbi_load(path.ToString().c_str(), &width, &height, &ch, STBI_rgb_alpha);

    LD_DEBUG_ASSERT(pixels);
    if (!pixels)
        return nullptr;

    ch = 4; // STBI_rgb_alpha

    Ref<Image> image = MakeRef<Image>(width, height, ch, (Byte*)pixels);
    stbi_image_free((void*)pixels);

    printf("ImageLoader::LoadImage [%s] %dx%d\n", path.ToString().c_str(), width, height);

    return image;
}

} // namespace LD