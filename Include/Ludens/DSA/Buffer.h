#pragma once

#include <Ludens/DSA/View.h>
#include <Ludens/Header/Types.h>
#include <cstdlib>

namespace LD {

class Buffer
{
public:
    Buffer();
    Buffer(const char* cstr);
    Buffer(const Buffer&);
    Buffer(Buffer&&) = delete;
    ~Buffer();

    Buffer& operator=(const Buffer&);
    Buffer& operator=(Buffer&&) = delete;

    /// @brief Reserve for capacity, does not effect size.
    void reserve(size_t cap);

    /// @brief Set buffer to byte size, reserving memory as necessary.
    void resize(size_t size);

    /// @brief Append bytes to buffer.
    void write(const byte* bytes, size_t size);

    /// @brief Append bytes in view to buffer.
    inline void write(const View& view) { write((const byte*)view.data, view.size); };

    /// @brief View bytes at position.
    void* read(size_t pos);

    /// @brief Empty buffer contents.
    inline void clear() { resize(0); }

    /// @brief Get buffer size in bytes.
    inline size_t size() const { return mSize; }

    /// @brief Get read only buffer data.
    inline const byte* data() const { return mData; }

    /// @brief Get writable buffer data
    inline byte* data() { return mData; }

    /// @brief Create a view into buffer data, the view is invalidated the moment buffer size changes.
    inline View view() const { return {(const char*)mData, mSize}; }

private:
    size_t mSize;
    size_t mCap;
    byte* mData = nullptr;
};

} // namespace LD
