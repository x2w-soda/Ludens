#pragma once

#include <Ludens/DSA/Array.h>
#include <atomic>

namespace LD {

template <typename T>
class TripleBuffer
{
public:
    /// @brief Update latest buffer data.
    void store(const T& data)
    {
        mBuffers[mWriteIdx] = data;

        mWriteIdx = mLastIdx.exchange(mWriteIdx, std::memory_order_release);
        mIsDirty.store(true, std::memory_order_release);
    }

    /// @brief Load latest buffer data.
    const T& load()
    {
        if (mIsDirty.exchange(false, std::memory_order_acquire))
            mReadIdx = mLastIdx.exchange(mReadIdx, std::memory_order_acquire);

        return mBuffers[mReadIdx];
    }

private:
    Array<T, 3> mBuffers;
    int mWriteIdx = 0;
    int mReadIdx = 1;
    std::atomic<int> mLastIdx = 2;
    std::atomic<bool> mIsDirty = {};
};

} // namespace LD