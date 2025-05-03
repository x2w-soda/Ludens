#include <Ludens/Header/Bitwise.h>
#include <Ludens/Media/Bitmap.h>
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
    char* data;
};

Bitmap Bitmap::create_from_data(uint32_t width, uint32_t height, BitmapChannel channel, const void* data)
{
    uint64_t dataSize = width * height * channel;

    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj) + dataSize, MEMORY_USAGE_MEDIA);
    obj->flags = 0;
    obj->width = width;
    obj->height = height;
    obj->channel = channel;
    obj->data = (char*)(obj + 1);

    memcpy(obj->data, data, dataSize);

    return {obj};
}

Bitmap Bitmap::create_from_path(const char* path)
{
    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);

    int x, y, ch;
    obj->data = (char*)stbi_load(path, &x, &y, &ch, STBI_rgb_alpha);

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
    BitmapObj* obj = (BitmapObj*)heap_malloc(sizeof(BitmapObj), MEMORY_USAGE_MEDIA);
    uint32_t size = 0;
    uint32_t layerSize = 0;
    char* dst = nullptr;
    char* tmp = nullptr;

    for (int i = 0; i < 6; i++)
    {
        int x, y, ch;
        tmp = (char*)stbi_load(paths[i], &x, &y, &ch, STBI_rgb_alpha);

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
            dst = obj->data = (char*)heap_malloc(6 * layerSize, MEMORY_USAGE_MEDIA);
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
    BitmapObj* obj = (BitmapObj*)bitmap;

    if (obj->data && (obj->flags & BITMAP_FLAG_USE_STB_FREE))
        stbi_image_free(obj->data);
    else if (obj->data && (obj->flags & BITMAP_FLAG_USE_HEAP_FREE))
        heap_free(obj->data);

    heap_free(obj);
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

char* Bitmap::data()
{
    return mObj->data;
}

const char* Bitmap::data() const
{
    return (const char*)mObj->data;
}

bool save_bitmap_to_disk(const BitmapView& view, const char* c_path)
{
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