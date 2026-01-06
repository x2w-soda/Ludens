#pragma once

#include <Ludens/DSA/HashSet.h>

#include <cstdint>
#include <type_traits>

namespace LD {

/// @brief ID type for monotonic counter, zero is invalid ID. Since signed integer overflow is UB, ID must be unsigned.
template <typename T>
concept MonotonicID = std::is_integral_v<T> && std::is_unsigned_v<T>;

/// @brief Monotonic ID counter.
template <MonotonicID T>
class IDCounter
{
public:
    IDCounter() = default;
    IDCounter(const IDCounter&) = delete;
    IDCounter(IDCounter&&) = delete;
    ~IDCounter() = default;

    IDCounter& operator=(const IDCounter&) = delete;
    IDCounter& operator=(IDCounter&&) = delete;

    /// @brief Acquire the next valid ID, or zero if the entire ID space is exhausted.
    T get_id()
    {
        if (mUsed.size() == (std::size_t)std::numeric_limits<T>::max())
            return (T)0;

        while (mCounter == (T)0 || mUsed.contains(mCounter))
            mCounter++;

        mUsed.insert(mCounter);
        return mCounter;
    }

    /// @brief User tries to acquire ID.
    /// @return True if suggested ID is not in use before, and has become registered from here on.
    bool try_get_id(T id)
    {
        if (id == 0 || mUsed.contains(id))
            return false;

        mUsed.insert(id);
        return true;
    }

private:
    HashSet<T> mUsed;
    T mCounter = 1;
};

} // namespace LD