#pragma once

namespace LD {

/// @brief Raw pointer to object guarded by unique ID. This requires user to provide
///        address stability for TObject and a unique TID distributor.
template <typename TObject, typename TID>
class IDHandle
{
public:
    IDHandle() = default;
    IDHandle(TObject* obj, TID id) : mObj(obj), mID(id) {}

    /// @brief Always check for ID handle validity before accessing raw pointer.
    /// @warning This requires the user to provide address stability for mObj.
    ///          For example this could be the address of a static TObject, or a memory block from the
    ///          PoolAllocator since the block address is always "valid" until allocator is destroyed.
    inline operator bool() const noexcept
    {
        return mObj && mObj->id == mID;
    }

    inline TObject* unwrap() noexcept
    {
        if (*this)
            return mObj;

        mID = 0;
        mObj = nullptr;
        return nullptr;
    }

protected:
    TObject* mObj;
    TID mID;
};

} // namespace LD