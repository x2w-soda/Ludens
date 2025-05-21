#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <cstring>
#include <filesystem>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace LD {

using BitmapFlags = uint32_t;
enum BitmapFlagBit : BitmapFlags
{
    BITMAP_FLAG_USE_STB_FREE = LD_BIT(0),
    BITMAP_FLAG_USE_HEAP_FREE = LD_BIT(1),
};

struct BitmapObj
{
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    BitmapChannel channel;
    byte* data;
};

Bitmap Bitmap::create_from_data(uint32_t width, uint32_t height, BitmapChannel channel, const void* data)
{
    LD_PROFILE_SCOPE;

    uint64_t dataSize = width * height * channel;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj) + dataSize, MEMORY_USAGE_MEDIA);
    obj->flags = 0;
    obj->width = width;
    obj->height = height;
    obj->channel = channel;
    obj->data = (byte*)(obj + 1);

    memcpy(obj->data, data, dataSize);

    return {obj};
}

Bitmap Bitmap::create_from_path(const char* path)
{
    LD_PROFILE_SCOPE;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);

    int x, y, ch;
    obj->data = (byte*)stbi_load(path, &x, &y, &ch, STBI_rgb_alpha);

    if (!obj->data)
        return {};

    obj->flags = BITMAP_FLAG_USE_STB_FREE;
    obj->width = (uint32_t)x;
    obj->height = (uint32_t)y;
    obj->channel = BITMAP_CHANNEL_RGBA;

    return {obj};
}

Bitmap Bitmap::create_cubemap_from_paths(const char** paths)
{
    LD_PROFILE_SCOPE;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);
    uint32_t size = 0;
    uint32_t layerSize = 0;
    byte* dst = nullptr;
    byte* tmp = nullptr;

    for (int i = 0; i < 6; i++)
    {
        int x, y, ch;
        tmp = (byte*)stbi_load(paths[i], &x, &y, &ch, STBI_rgb_alpha);

        if (!tmp)
        {
            printf("cubemap face not found: %s\n", paths[i]);
            goto failure;
        }

        if (x != y)
        {
            printf("cubemap face %d is not a cube: %dx%d\n", i, x, y);
            goto failure;
        }

        if (i == 0)
        {
            size = (uint32_t)x;
            layerSize = size * size * 4;
            dst = obj->data = (byte*)heap_malloc(6 * layerSize, MEMORY_USAGE_MEDIA);
        }
        else if (x != size || y != size)
        {
            printf("cubemap faces vary in size, expected %dx%d for face %d, found %dx%d\n", (int)size, (int)size, i, x, y);
            goto failure;
        }

        // copy into contiguous memory
        memcpy(dst, tmp, layerSize);
        stbi_image_free(tmp);

        dst += layerSize;
    }

    obj->flags = BITMAP_FLAG_USE_HEAP_FREE;
    obj->width = size;
    obj->height = size;
    obj->channel = BITMAP_CHANNEL_RGBA;

    return {obj};

failure:

    if (tmp)
        stbi_image_free(tmp);

    if (obj)
    {
        if (obj->data)
            heap_free(obj->data);
        heap_free(obj);
    }

    return {};
}

void Bitmap::destroy(Bitmap bitmap)
{
    LD_PROFILE_SCOPE;

    BitmapObj* obj = (BitmapObj*)bitmap;

    if (obj->data && (obj->flags & BITMAP_FLAG_USE_STB_FREE))
        stbi_image_free(obj->data);
    else if (obj->data && (obj->flags & BITMAP_FLAG_USE_HEAP_FREE))
        heap_free(obj->data);

    heap_free(obj);
}

void Bitmap::serialize(Serializer& serializer, const Bitmap& bitmap)
{
    LD_PROFILE_SCOPE;

    const BitmapObj* obj = bitmap;

    serializer.write_u32(obj->width);
    serializer.write_u32(obj->height);
    serializer.write_u32((uint32_t)obj->channel);

    size_t dataSize = obj->width * obj->height * obj->channel;
    serializer.write(obj->data, dataSize);
}

void Bitmap::deserialize(Serializer& serializer, Bitmap& bitmap)
{
    LD_PROFILE_SCOPE;

    uint32_t width, height;
    BitmapChannel channel;
    serializer.read_u32(width);
    serializer.read_u32(height);
    serializer.read_u32((uint32_t&)channel);

    size_t dataSize = width * height * channel;
    std::vector<byte> pixels(dataSize);
    serializer.read(pixels.data(), dataSize);
    bitmap = Bitmap::create_from_data(width, height, channel, pixels.data());
}

void Bitmap::flipy()
{
    LD_PROFILE_SCOPE;

    byte* data = mObj->data;
    byte tmp[2048];
    const uint32_t width = mObj->width;
    const uint32_t height = mObj->height;
    const uint32_t texelSize = mObj->channel; // currently only supports one byte per pixel channel
    const uint32_t bytesPerRow = texelSize * width;

    for (uint32_t row = 0; row < height / 2; row++)
    {
        uint8_t* row0 = data + row * bytesPerRow;
        uint8_t* row1 = data + (height - row - 1) * bytesPerRow;
        uint32_t bytesLeft = bytesPerRow;

        while (bytesLeft)
        {
            size_t bytesCopied = (bytesLeft < sizeof(tmp)) ? bytesLeft : sizeof(tmp);
            memcpy(tmp, row0, bytesCopied);
            memcpy(row0, row1, bytesCopied);
            memcpy(row1, tmp, bytesCopied);
            row0 += bytesCopied;
            row1 += bytesCopied;
            bytesLeft -= bytesCopied;
        }
    }
}

BitmapView Bitmap::view() const
{
    return {
        .width = mObj->width,
        .height = mObj->height,
        .channel = mObj->channel,
        .data = (const char*)mObj->data,
    };
}

uint32_t Bitmap::width() const
{
    return mObj->width;
}

uint32_t Bitmap::height() const
{
    return mObj->height;
}

BitmapChannel Bitmap::channel() const
{
    return mObj->channel;
}

byte* Bitmap::data()
{
    return mObj->data;
}

const byte* Bitmap::data() const
{
    return (const byte*)mObj->data;
}

bool save_bitmap_to_disk(const BitmapView& view, const char* c_path)
{
    LD_PROFILE_SCOPE;

    namespace fs = std::filesystem;

    fs::path path(c_path);
    std::string ext = path.extension().string();

    if (ext == ".png")
    {
        int success = stbi_write_png(c_path, view.width, view.height, view.channel, view.data, 0);
        if (!success)
        {
            printf("save_bitmap_to_disk: stbi_write_png error\n");
            return false;
        }
        printf("save_bitmap_to_disk: %s\n", c_path);
        return true;
    }
    else
    {
        printf("save_bitmap_to_disk: unsupported extension: %s\n", ext.c_str());
        return false;
    }

    return false;
}

} // namespace LD