#pragma once

#include <Ludens/DSA/HashSet.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace LD {

/// @brief ID type for monotonic counter, zero is invalid ID. Since signed integer overflow is UB, ID must be unsigned.
template <typename T>
concept MonotonicID = std::is_integral_v<T> && std::is_unsigned_v<T>;

/// @brief Monotonic ID counter, increments counter for every ID acquired. Not thread safe.
/// @tparam T
template <typename T>
class IDCounter
{
public:
    IDCounter() = default;
    IDCounter(const IDCounter&) = delete;
    IDCounter(IDCounter&&) = delete;
    ~IDCounter() = default;

    IDCounter& operator=(const IDCounter&) = delete;
    IDCounter& operator=(IDCounter&&) = delete;

    /// @brief Acquire the next valid ID and increment the counter.
    T get_id()
    {
        if (mCounter == 0)
            mCounter = 1;

        return mCounter++;
    }

private:
    T mCounter = 1;
};

/// @brief ID registry, keeps track of all IDs already distributed. Not thread safe.
template <MonotonicID T>
class IDRegistry
{
public:
    IDRegistry() = default;
    IDRegistry(const IDRegistry&) = delete;
    IDRegistry(IDRegistry&&) = delete;
    ~IDRegistry() = default;

    IDRegistry& operator=(const IDRegistry&) = delete;
    IDRegistry& operator=(IDRegistry&&) = delete;

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

    /// @brief Try to acquire ID.
    /// @return True if suggested ID is not in use before, and has become registered from here on.
    bool try_get_id(T id)
    {
        if (id == 0 || mUsed.contains(id))
            return false;

        mUsed.insert(id);
        return true;
    }

    /// @brief Release a registered ID, the registry is free to distribute this ID later.
    void free(T id)
    {
        mUsed.erase(id);
    }

private:
    HashSet<T> mUsed;
    T mCounter = 1;
};

} // namespace LD
