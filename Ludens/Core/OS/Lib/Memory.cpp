#include <cstdio>
#include <cstdlib>
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/Memory.h"

namespace LD {

	void* MemoryAlloc(size_t size)
	{
		void* mem = malloc(size);

		LD_DEBUG_ASSERT(mem != NULL);
		return mem;
	}

	void* MemoryRealloc(void* mem, size_t size)
	{
		if (mem == NULL)
			return MemoryAlloc(size);
		
		mem = realloc(mem, size);

		LD_DEBUG_ASSERT(mem != NULL);
		return mem;
	}

	void MemoryFree(void* mem)
	{
		LD_DEBUG_ASSERT(mem != NULL);

		free(mem);
	}

} // namespace LD