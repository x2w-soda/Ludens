#pragma once

namespace LD
{

/// stack of pointers for logic ordering,
/// input events are passed from stack top to bottom
/// while items are rendered from stack bottom to top
template <typename T>
class UILogicStack
{
public:
    void PushTop(T* item)
    {
        mItems.PushBack(item);
    }

    void Remove(T* item)
    {
        size_t i;
        size_t count = mItems.Size();

        for (i = 0; i < count && mItems[i] != item; i++)
            ;

        if (i >= count)
            return;

        for (; i + 1 < count; i++)
            mItems[i] = mItems[i + 1];

        mItems.Resize(count - 1);
    }

    inline const T* operator[](int idx) const
    {
        LD_DEBUG_ASSERT(0 <= idx && idx < mItems.Size());
        return mItems[idx];
    }

    inline T* operator[](int idx)
    {
        LD_DEBUG_ASSERT(0 <= idx && idx < mItems.Size());
        return mItems[idx];
    }

    View<T*> GetView() const
    {
        return { mItems.Size(), mItems.Begin() };
    }

    size_t Size() const
    {
        return mItems.Size();
    }

private:
    Vector<T*> mItems;
};

} // namespace LD