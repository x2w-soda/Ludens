#pragma once

#include <Ludens/Header/Types.h>
#include <cstdlib>

namespace LD {

class Buffer
{
public:
    Buffer();
    Buffer(const Buffer&);
    ~Buffer();

    Buffer& operator=(const Buffer&);

    void write(const byte* bytes, size_t size);
    void* read(size_t pos);

    inline size_t size() const { return mSize; }
    inline const byte* data() const { return mData; }

private:
    size_t mSize;
    size_t mCap;
    byte* mData = nullptr;
};

} // namespace LD
