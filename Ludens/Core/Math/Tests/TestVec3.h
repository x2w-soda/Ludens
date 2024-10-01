#pragma once

#include <doctest.h>
#include "Core/Math/Include/Vec3.h"

using namespace LD;

TEST_CASE("TVec3 Data")
{
	{
		IVec3 v(1, 2, 3);
		CHECK(v.Data[0] == 1);
		CHECK(v.Data[1] == 2);
		CHECK(v.Data[2] == 3);
		CHECK(v.x == 1);
		CHECK(v.y == 2);
		CHECK(v.z == 3);
	}
	{
		IVec2 v1(1, 2);
		IVec3 v2(v1, 3);
		CHECK(v2.x == 1);
		CHECK(v2.y == 2);
		CHECK(v2.z == 3);

		v2 = { 3, v1 };
		CHECK(v2.x == 3);
		CHECK(v2.y == 1);
		CHECK(v2.z == 2);
	}
}

TEST_CASE("TVec3 Operators")
{
	// TODO: far from complete

	{
		IVec3 v1(4, 12, 8);
		IVec3 v2;

		v2 = v1 + 3;
		CHECK(v2 == IVec3(7, 15, 11));

		v2 = v1 - 3;
		CHECK(v2 == IVec3(1, 9, 5));

		v2 = v1 * 3;
		CHECK(v2 == IVec3(12, 36, 24));

		v2 = v1 / 2;
		CHECK(v2 == IVec3(2, 6, 4));
	}

	{
		IVec3 v1(2, 3, 6);
		IVec3 v2(4, 6, 18);
		IVec3 v;

		v = v1 + v2;
		CHECK(v == IVec3(6, 9, 24));

		v = v1 - v2;
		CHECK(v == IVec3(-2, -3, -12));

		v = v1 * v2;
		CHECK(v == IVec3(8, 18, 108));

		v = v2 / v1;
		CHECK(v == IVec3(2, 2, 3));
	}
}

TEST_CASE("TVec3 Methods")
{
	{
		IVec3 v(3, 4, 5);
		CHECK(v.LengthSquared() == 50);

		v = { 7, 24, 1 };
		CHECK(v.LengthSquared() == 626);
	}

	{
		IVec3 v1(3, 4, 5);
		IVec3 v2(-4, 3, 2);
		IVec3 z = IVec3::Zero;

		CHECK(IVec3::Dot(v1, v2) == 10);
		CHECK(IVec3::Dot(v2, v1) == 10);

		v1 = { 4, 5, 3 };
		v2 = { -2, 2, 5 };
		CHECK(IVec3::Dot(v1, v2) == 17);
		CHECK(IVec3::Dot(v2, v1) == 17);

		CHECK(IVec3::Dot(z, v1) == 0);
		CHECK(IVec3::Dot(z, v2) == 0);
	}

	{
		IVec3 v1(3, 0, 0);
		IVec3 v2(0, 4, 0);
		IVec3 v = IVec3::Cross(v1, v2);
		CHECK(v.x == 0);
		CHECK(v.y == 0);
		CHECK(v.z == 12);

		v1 = { 0, 3, 0 };
		v2 = { 2, 0, 0 };

		v = IVec3::Cross(v1, v2);
		CHECK(v.x == 0);
		CHECK(v.y == 0);
		CHECK(v.z == -6);
	}
}

TEST_CASE("TVec3 Swizzle")
{
	{
		float x = 1.23f, y = 3.21f, z = 2.71f;
		Vec3 v(x, y, z);
		Vec3 xxx = v.xxx();
		CHECK(xxx.x == x);
		CHECK(xxx.y == x);
		CHECK(xxx.z == x);
	}
}