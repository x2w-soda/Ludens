#pragma once

#include <Ludens/Memory/Memory.h>
#include <cstring>
#include <type_traits>

#define HEAP_STORAGE_DEFAULT_CAP 8

namespace LD {

template <typename T, MemoryUsage TUsage = MEMORY_USAGE_MISC>
struct THeapStorage
{
    size_t cap;
    size_t size;
    T* data;

    THeapStorage()
        : data(nullptr), cap(0), size(0)
    {
    }

    THeapStorage(size_t cap)
        : cap(cap), size(0)
    {
        data = (T*)heap_malloc(sizeof(T) * cap, TUsage);
    }

    THeapStorage(const THeapStorage& other)
        : data(nullptr), cap(0), size(other.size)
    {
        grow(other.cap);

        if (cap > 0 && size > 0)
        {
            if constexpr (std::is_trivially_constructible<T>::value)
            {
                memcpy(data, other.data, sizeof(T) * size);
            }
            else
            {
                for (size_t i = 0; i < size; i++)
                    new (data + i) T(other.data[i]);
            }
        }
    }

    THeapStorage(THeapStorage&& other)
        : cap(other.cap), size(other.size), data(other.data)
    {
        other.data = nullptr;
        other.size = 0;
        other.cap = 0;
    }

    ~THeapStorage()
    {
        release();
    }

    THeapStorage& operator=(const THeapStorage& other)
    {
        if (cap != other.cap)
        {
            release();
            grow(other.cap);
        }

        size = other.size;

        if (cap > 0 && size > 0)
        {
            if constexpr (std::is_trivially_constructible<T>::value)
            {
                memcpy(data, other.data, sizeof(T) * size);
            }
            else
            {
                for (size_t i = 0; i < size; i++)
                    data[i] = other.data[i];
            }
        }

        return *this;
    }

    THeapStorage& operator=(THeapStorage&& other)
    {
        data = other.data;
        size = other.size;
        cap = other.cap;
        other.data = nullptr;
        other.size = 0;
        other.cap = 0;

        return *this;
    }

    /// @brief resize storage, grows capacity if necessary
    /// @param nsize new size, reallocates if greater than current capacity
    void resize(size_t nsize)
    {
        if (nsize <= size)
        {
            if constexpr (!std::is_trivially_destructible<T>::value)
            {
                for (size_t i = nsize; i < size; i++)
                    data[i].~T();
            }

            size = nsize;
            return;
        }

        size_t ncap = cap;

        if (ncap == 0)
            ncap = HEAP_STORAGE_DEFAULT_CAP;

        while (nsize > ncap)
            ncap *= 2;

        if (ncap > cap)
            grow(ncap);

        if constexpr (!std::is_trivially_constructible<T>::value)
        {
            for (size_t i = size; i < nsize; i++)
                new (data + i) T();
        }

        size = nsize;
    }

    /// @brief increase storage capacity, moves old data over but does not effect size
    /// @param ncap new capacity
    void grow(size_t ncap)
    {
        if (ncap <= cap)
            return;

        T* ndata = (T*)heap_malloc(sizeof(T) * ncap, TUsage);

        if (data)
        {
            if constexpr (std::is_trivially_constructible<T>::value)
            {
                memcpy(ndata, data, sizeof(T) * size);
            }
            else
            {
                for (size_t i = 0; i < size; i++)
                {
                    new (ndata + i) T(std::move(data[i]));
                    data[i].~T();
                }
            }

            heap_free(data);
        }

        data = ndata;
        cap = ncap;
    }

    /// @brief release heap allocation and resets capacity to zero.
    void release()
    {
        resize(0);

        if (data)
        {
            heap_free(data);
            data = nullptr;
        }

        size = 0;
        cap = 0;
    }
};

} // namespace LD