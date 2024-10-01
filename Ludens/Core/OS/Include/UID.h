#pragma once

#include <atomic>
#include "Core/Header/Include/Types.h"

namespace LD {


	// unique identifiers are 64-bit throughout the codebase.
	// zero is the only invalid UID, any positive integer is valid.
	using UID = u64;


	// Class Unique Identifier, for object identification
	template <typename T>
	class CUID
	{
	public:
		CUID() : mUID(0) {}
		CUID(const CUID&) = delete;
		CUID(CUID&& other) : mUID(other.mUID) { other.mUID = 0; }
		~CUID() {}

		CUID& operator=(const CUID&) = delete;
		CUID& operator=(CUID&& other) { mUID = other.mUID; other.mUID = 0; return *this; }

		static CUID Get()
		{
			static std::atomic<UID> sCounter{ 0 };
			return CUID { sCounter.fetch_add(1) + 1 };
		}

		inline operator UID() const { return mUID; }
		inline void Reset() { mUID = 0; }

	private:
		CUID(UID uid) : mUID(uid) {}

		UID mUID = 0;
	};

	class GUIDClass;

	// Globally Unique Identifier
	using GUID = CUID<GUIDClass>;


} // namespace LD