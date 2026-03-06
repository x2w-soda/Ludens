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

} // namespace LD
