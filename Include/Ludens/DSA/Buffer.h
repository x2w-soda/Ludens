#pragma once

#include <cstdlib>

namespace LD {

class Buffer
{
public:
    Buffer();
    Buffer(const Buffer&);
    ~Buffer();

    Buffer& operator=(const Buffer&);

    void write(char* bytes, size_t size);
    void* read(size_t pos);

    inline size_t size() const { return mSize; }

private:
    size_t mSize;
    size_t mCap;
    char* mData = nullptr;
};

} // namespace LD
