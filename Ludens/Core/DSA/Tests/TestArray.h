#pragma once

#include <doctest.h>
#include <array>
#include "Core/DSA/Include/Array.h"

using namespace LD;

TEST_CASE("Array ctor")
{
	{
		Array<int, 3> a;
		CHECK(a.Size() == 3);
		CHECK(a.ByteSize() == sizeof(int) * 3);
	}

	{
		Array<int, 4> a = { 1, 2, 3, 4 };
		CHECK(a.Size() == 4);
		CHECK(a.ByteSize() == sizeof(int) * 4);
		CHECK(a[0] == 1);
		CHECK(a[1] == 2);
		CHECK(a[2] == 3);
		CHECK(a[3] == 4);
	}

	{
		Array<int, 5> a = {{ 1, 2, 3, 4, 5 }};
		CHECK(a.Size() == 5);
		CHECK(a.ByteSize() == sizeof(int) * 5);
		CHECK(a[0] == 1);
		CHECK(a[1] == 2);
		CHECK(a[2] == 3);
		CHECK(a[3] == 4);
		CHECK(a[4] == 5);
	}
}

TEST_CASE("Array assign")
{
	Array<int, 3> a;
	a[0] = 1;
	a[1] = 2;
	a[2] = 3;
	CHECK(a[0] == 1);
	CHECK(a[1] == 2);
	CHECK(a[2] == 3);

	a = { 3, 2, 1 };
	CHECK(a[0] == 3);
	CHECK(a[1] == 2);
	CHECK(a[2] == 1);
}