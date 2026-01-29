#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/View.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

namespace LD {

template <typename TChar>
class GapBuffer
{
public:
    /// @brief Set cursor such that gap starts at position.
    void set_cursor(size_t pos)
    {
        if (pos == mGapStart)
            return;

        if (pos < mGapStart)
        {
            const size_t moveCount = mGapStart - pos;

            for (size_t i = 0; i < moveCount; i++)
                mBuffer[mGapEnd - 1 - i] = mBuffer[mGapStart - 1 - i];

            mGapStart = pos;
            mGapEnd -= moveCount;
        }
        else // pos > mGapStart
        {
            const size_t moveCount = pos - mGapStart;

            for (size_t i = 0; i < moveCount; i++)
                mBuffer[mGapStart + i] = mBuffer[mGapEnd + i];

            mGapStart = pos;
            mGapEnd += moveCount;
        }
    }

    /// @brief Get number of characters in buffer.
    size_t size() const
    {
        return mBuffer.size() - gap_size();
    }

    /// @brief Cast to STL string.
    std::basic_string<TChar> to_string() const
    {
        std::basic_string<TChar> str;
        str.resize(size());
        std::copy(mBuffer.begin(), mBuffer.begin() + mGapStart, str.begin());
        std::copy(mBuffer.begin() + mGapEnd, mBuffer.end(), str.begin() + mGapStart);

        return str;
    }

    /// @brief Get character at position.
    TChar at(size_t pos) const
    {
        if (pos < mGapStart)
            return mBuffer[pos];

        return mBuffer[pos + gap_size()];
    }

    /// @brief Get character reference at position.
    TChar& at(size_t pos)
    {
        if (pos < mGapStart)
            return mBuffer[pos];

        return mBuffer[pos + gap_size()];
    }

    /// @brief Inserts a single char at position.
    void insert(size_t pos, TChar c)
    {
        set_cursor(pos);
        reserve(1);

        mBuffer[mGapStart++] = c;
    }

    /// @brief Inserts view content at position.
    void insert(size_t pos, View view)
    {
        if (!view.data || view.size == 0)
            return;

        set_cursor(pos);
        reserve(view.size);

        for (size_t i = 0; i < view.size; i++)
            mBuffer[mGapStart++] = static_cast<TChar>(view.data[i]);
    }

    /// @brief Inserts a C string at position, does not insert '\0'.
    void insert(size_t pos, const char* cstr)
    {
        if (!cstr)
            return;

        insert(pos, View(cstr, strlen(cstr)));
    }

    /// @brief Inserts a STL string at position.
    void insert(size_t pos, const std::basic_string<TChar>& str)
    {
        set_cursor(pos);
        reserve(str.size());

        for (size_t i = 0; i < str.size(); i++)
            mBuffer[mGapStart++] = str[i];
    }

    /// @brief Erase n characters at position.
    void erase(size_t pos, size_t n)
    {
        if (n == 0)
            return;

        n = std::min<size_t>(n, size() - pos);

        set_cursor(pos);
        mGapEnd += n;
    }

    /// @brief Erase all characters in buffer.
    inline void clear()
    {
        erase(0, size());
    }

private:
    inline size_t gap_size() const
    {
        return mGapEnd - mGapStart;
    }

    void reserve(size_t gapSize)
    {
        if (gap_size() >= gapSize)
            return;

        uint32_t newGapSize = next_power_of_two((uint32_t)gapSize);
        size_t oldGapSize = gap_size();
        const size_t moveCount = mBuffer.size() - mGapEnd;
        mBuffer.resize(mBuffer.size() + newGapSize - oldGapSize);

        size_t newSize = mBuffer.size();
        for (size_t i = 0; i < moveCount; i++)
            mBuffer[newSize - 1 - i] = mBuffer[mGapEnd + moveCount - 1 - i];

        mGapEnd = mGapStart + newGapSize;
    }

private:
    Vector<TChar> mBuffer;
    size_t mGapStart = 0;
    size_t mGapEnd = 0;
};

} // namespace LD