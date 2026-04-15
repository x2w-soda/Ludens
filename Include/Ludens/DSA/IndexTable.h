#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Memory.h>

namespace LD {

template <typename T, MemoryUsage TUsage>
class IndexTable
{
public:
    IndexTable() = default;
    IndexTable(const IndexTable&) = delete;
    ~IndexTable()
    {
        for (T* entry : mEntries)
            heap_delete<T>(entry);
    }

    IndexTable& operator=(const IndexTable&) = delete;

    T* get(size_t index)
    {
        if (index >= mEntries.size())
        {
            mEntries.resize(index + 1);
            for (size_t i = index; i < mEntries.size(); i++)
                mEntries[i] = heap_new<T>(TUsage);
        }

        return mEntries[index];
    }

    inline size_t size() const noexcept
    {
        return mEntries.size();
    }

    inline T* operator[](int index)
    {
        return get(index);
    }

private:
    Vector<T*> mEntries;
};

} // namespace LD