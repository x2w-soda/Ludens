#pragma once

#include <cstdint>

namespace LD {

/// @brief Unique identifier for render resources, zero is invalid ID.
typedef uint32_t RUID;

/// @brief Get a RUID, call on main thread only.
RUID get_ruid();

} // namespace LD