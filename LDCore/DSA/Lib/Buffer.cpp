#include <Ludens/DSA/Buffer.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace LD {

Buffer::Buffer() : mSize(0), mCap(0), mData(nullptr)
{
}

Buffer::Buffer(const Buffer& other)
    : mSize(other.mSize), mCap(other.mCap), mData(nullptr)
{
    if (mSize > 0)
    {
        mData = (byte*)heap_malloc(mCap, MEMORY_USAGE_MISC);
        memcpy(mData, other.mData, mSize);
    }
}

Buffer::~Buffer()
{
    if (mData)
        heap_free(mData);
}

void Buffer::reserve(size_t cap)
{
    if (cap <= mCap)
        return;

    size_t nextCap = std::max<size_t>(mCap, BUFSIZ);

    while (cap > nextCap)
        nextCap *= 2;

    if (nextCap > mCap)
    {
        byte* newData = (byte*)heap_malloc(nextCap, MEMORY_USAGE_MISC);

        if (mData)
        {
            memcpy(newData, mData, mSize);
            heap_free(mData);
        }

        mData = newData;
        mCap = nextCap;
    }
}

void Buffer::resize(size_t size)
{
    reserve(size);
    mSize = size;
}

void Buffer::write(const byte* bytes, size_t size)
{
    reserve(mSize + size);

    memcpy(mData + mSize, bytes, size);
    mSize += size;
}

void* Buffer::read(size_t pos)
{
    LD_ASSERT(mData && pos < mSize);

    return mData + pos;
}

Buffer& Buffer::operator=(const Buffer& other)
{
    if (mSize < other.mSize)
    {
        heap_free(mData);
        mData = (byte*)heap_malloc(other.mCap, MEMORY_USAGE_MISC);
    }

    mCap = other.mCap;
    mSize = other.mSize;
    memcpy(mData, other.mData, mSize);

    return *this;
}

} // namespace LD
