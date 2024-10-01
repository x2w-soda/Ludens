#pragma once

#include <doctest.h>
#include "Core/OS/Include/Allocator.h"

using namespace LD;

TEST_CASE("StackAllocator")
{
	StackAllocator sa;

	char* base;
	
	sa.Startup(32);
	
	{
		// alloc full stack at once
		base = (char*)sa.Alloc(32);

		// out of memory
		void* mem = sa.Alloc(1);
		CHECK(mem == nullptr);
		sa.Free(base);
	}

	{
		void* mem = sa.Alloc(33);
		CHECK(mem == nullptr);
	}
	
	{
		// we should be able to partition the stack into chunks of varying size
		char* p0 = (char*)sa.Alloc(4);
		char* p4 = (char*)sa.Alloc(7);
		char* p11 = (char*)sa.Alloc(9);
		char* p20 = (char*)sa.Alloc(12);

		CHECK(p0 - base == 0);
		CHECK(p4 - base == 4);
		CHECK(p11 - base == 11);
		CHECK(p20 - base == 20);

		// out of memory
		void* mem = sa.Alloc(1);
		CHECK(mem == nullptr);
		sa.Free(base);
	}

	sa.Cleanup();
}