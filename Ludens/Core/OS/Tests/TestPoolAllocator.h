#pragma once

#include <set>
#include <doctest.h>
#include "Core/OS/Include/Allocator.h"
#include "Core/Math/Include/Vec2.h"
#include "Core/Math/Include/Mat4.h"

using namespace LD;


TEST_CASE("PoolAllocator")
{
	int numChunks = 128;
	std::set<u8*> offsets;
	PoolAllocator<sizeof(Vec2)> pa;

	pa.Startup(numChunks);
	{
		for (int i = 0; i < numChunks; i++)
		{
			CHECK(pa.CountUsedChunks() == i);
			CHECK(pa.CountFreeChunks() == numChunks - i);
			offsets.insert((u8*)pa.Alloc(sizeof(Vec2)));
		}

		CHECK(offsets.size() == numChunks);

		int i = 0;
		u8* min = *offsets.begin();

		for (auto it = offsets.begin(); it != offsets.end(); it++, i++)
		{
			CHECK((*it - min) % sizeof(Vec2) == 0);
			CHECK((*it - min) / sizeof(Vec2) == i);

			CHECK(pa.CountFreeChunks() == i);
			CHECK(pa.CountUsedChunks() == numChunks - i);
			pa.Free(*it);
		}
	}
	pa.Cleanup();
}

TEST_CASE("PoolAllocator Contains")
{
    int numChunks = 8;
    PoolAllocator<sizeof(Mat4)> pa;
    std::set<u8*> offsets;

    pa.Startup(numChunks);
    {
		for (int i = 0; i < 8; i++)
		{
            u8* offset = (u8*)pa.Alloc(sizeof(Mat4));
            offsets.insert(offset);
		}

        u8* min = *offsets.begin();

        CHECK(pa.Contains(min));
        CHECK(pa.Contains(min + sizeof(Mat4)));

		// invalid chunk addresses
        CHECK(!pa.Contains(min + sizeof(Mat4) + 1));
        CHECK(!pa.Contains(min + sizeof(Mat4) - 1));

		for (u8* offset : offsets)
            pa.Free(offset);

		CHECK(pa.CountUsedChunks() == 0);

		// Even if a chunk is not in use, we still return true since it is a valid chunk address.
        CHECK(pa.Contains(min));
        CHECK(pa.Contains(min) + sizeof(Mat4) * 1);
        CHECK(pa.Contains(min) + sizeof(Mat4) * 7);
	}
	pa.Cleanup();
}