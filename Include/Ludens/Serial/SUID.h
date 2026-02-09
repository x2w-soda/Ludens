#pragma once

#include <cstdint>

namespace LD {

/// @brief Serial unique ID in u32 space, zero is invalid ID.
///        This is intended for persistent IDs to be stored on disk.
typedef uint32_t SUID;

/// @brief Acquire next valid SUID, or zero if the ID space is exhausted simultaneously.
///        Not thread safe.
SUID get_suid();

/// @brief Try acquire a SUID, this is intended for deserialization code paths where the ID is known.
///        Failure suggests either the exhaustion of ID space (unlikely) or an invalid schema.
///        Not thread safe.
bool try_get_suid(SUID id);

/// @brief Release a registered SUID, the ID may be returned by get_suid() again.
///        Not thread safe.
void free_suid(SUID id);

} // namespace LD