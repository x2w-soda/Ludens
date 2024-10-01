#pragma once

#include <doctest.h>
#include "Core/OS/Include/Memory.h"

using namespace LD;

struct Blob
{
	Blob() : x(0) { sObjectCtr++; sDefaultConstructCtr++; }
	Blob(const Blob& other) : x(other.x) { sObjectCtr++; sCopyConstructCtr++; }
	~Blob() { sObjectCtr--; }

	Blob& operator=(const Blob& other) { x = other.x; sCopyAssignCtr++; return *this; }

	int x;

	static void Reset()
	{
		sObjectCtr = 0;
		sDefaultConstructCtr = 0;
		sCopyAssignCtr = 0;
		sCopyConstructCtr = 0;
	}

	static int sObjectCtr;
	static int sDefaultConstructCtr;
	static int sCopyAssignCtr;
	static int sCopyConstructCtr;
};

int Blob::sObjectCtr = 0;
int Blob::sDefaultConstructCtr = 0;
int Blob::sCopyAssignCtr = 0;
int Blob::sCopyConstructCtr = 0;


TEST_CASE("Memory Alloc")
{
	{
		size_t n = 5;
		int* a = (int*)MemoryAlloc(sizeof(int) * n);

		for (size_t i = 0; i < n; i++)
			a[i] = i;

		n = 10;
		a = (int*)MemoryRealloc(a, sizeof(int) * n);

		for (size_t i = 0; i < 5; i++)
		{
			CHECK(a[i] == i);
			a[i + 5] = i + 5;
		}

		for (size_t i = 0; i < 5; i++)
			CHECK(a[i + 5] == i + 5);

		MemoryFree((void*)a);
	}

	{
		size_t n = 10;
		int* a = (int*)MemoryAlloc(sizeof(int) * n);

		for (size_t i = 0; i < n; i++)
			a[i] = i;

		n /= 2;
		a = (int*)MemoryRealloc(a, sizeof(int) * n);

		for (size_t i = 0; i < n; i++)
			CHECK(a[i] == i);

		MemoryFree((void*)a);
	}
}

TEST_CASE("Memory Placement Alloc")
{
	{
		Blob::Reset();

		Blob* a = MemoryPlacementAlloc<Blob>(5);

		CHECK(Blob::sObjectCtr == 5);
		CHECK(Blob::sDefaultConstructCtr == 5);

		for (int i = 0; i < 5; i++)
		{
			CHECK(a[i].x == 0);
			a[i].x = i;
		}

		a = MemoryPlacementRealloc<Blob>(a, 10);

		CHECK(Blob::sObjectCtr == 10);
		CHECK(Blob::sDefaultConstructCtr == 15);
		CHECK(Blob::sCopyAssignCtr == 5);

		for (int i = 0; i < 5; i++)
			CHECK(a[i].x == i);

		MemoryPlacementFree<Blob>(a);
		CHECK(Blob::sObjectCtr == 0);
	}

	{
		Blob::Reset();

		Blob* a = MemoryPlacementAlloc<Blob>(10);

		CHECK(Blob::sObjectCtr == 10);
		CHECK(Blob::sDefaultConstructCtr == 10);

		for (int i = 0; i < 10; i++)
		{
			CHECK(a[i].x == 0);
			a[i].x = i;
		}

		a = MemoryPlacementRealloc<Blob>(a, 5);

		CHECK(Blob::sObjectCtr == 5);
		CHECK(Blob::sDefaultConstructCtr == 15);
		CHECK(Blob::sCopyAssignCtr == 5);

		for (int i = 0; i < 5; i++)
			CHECK(a[i].x == i);

		MemoryPlacementFree<Blob>(a);
		CHECK(Blob::sObjectCtr == 0);
	}
}