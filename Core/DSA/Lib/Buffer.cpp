#include <Ludens/Header/Assert.h>
#include <Ludens/DSA/Buffer.h>
#include <Ludens/System/Memory.h>
#include <cstring>

namespace LD {

Buffer::Buffer() : mSize(0), mCap(128)
{
    mData = (char*)heap_malloc(mCap, MEMORY_USAGE_MISC);
}

Buffer::Buffer(const Buffer& other)
    : mSize(other.mSize), mCap(other.mCap), mData(nullptr)
{
    if (mSize > 0)
    {
        mData = (char*)heap_malloc(mCap, MEMORY_USAGE_MISC);
        memcpy(mData, other.mData, mSize);
    }
}

Buffer::~Buffer()
{
    if (mData)
        heap_free(mData);
}

void Buffer::write(char* bytes, size_t size)
{
    if (mSize + size > mCap)
    {
        mCap *= 2;
        char* newData = (char*)heap_malloc(mCap, MEMORY_USAGE_MISC);
        memcpy(newData, mData, mSize);
        heap_free(mData);
        mData = newData;
    }

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
        mData = (char*)heap_malloc(other.mCap, MEMORY_USAGE_MISC);
    }

    mCap = other.mCap;
    mSize = other.mSize;
    memcpy(mData, other.mData, mSize);

    return *this;
}

} // namespace LD