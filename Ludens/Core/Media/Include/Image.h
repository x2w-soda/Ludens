#pragma once

#include "Core/OS/Include/Memory.h"
#include "Core/IO/Include/FileSystem.h"
#include "Core/Header/Include/Types.h"

namespace LD
{

class Image
{
public:
    Image();
    Image(const Image&) = delete;
    Image(Image&&) = delete;
    Image(int width, int height, int channels, const Byte* pixels);
    ~Image();

    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    /// number of pixels in a row
    int GetWidth() const;

    /// number of pixels in a column
    int GetHeight() const;

    /// number of channels per pixel
    int GetChannels() const;

    const Byte* Pixels() const;
    int ByteSize() const;

private:
    int mWidth;
    int mHeight;
    int mChannels;
    Byte* mPixels;
    size_t mByteSize;
};

struct ImageLoader
{
    Ref<Image> LoadImage(const Path& path);
    Ref<Image> LoadImage(const Path& path, int& width, int& height, int& ch);
};

} // namespace LD