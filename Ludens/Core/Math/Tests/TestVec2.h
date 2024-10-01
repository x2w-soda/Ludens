#pragma once

#include <doctest.h>
#include "Core/Math/Include/Vec2.h"

using namespace LD;


TEST_CASE("TVec2 Data")
{
	{
		IVec2 v;
		CHECK(v.Data[0] == 0);
		CHECK(v.Data[1] == 0);
	}

	{
		IVec2 v(1, 2);
		CHECK(v.Data[0] == 1);
		CHECK(v.Data[1] == 2);
		CHECK(v.x == 1);
		CHECK(v.r == 1);
		CHECK(v.y == 2);
		CHECK(v.g == 2);
	}

	{
		Vec2 v(-1.0f, 3.14f);
		CHECK(v.Data[0] == -1.0f);
		CHECK(v.Data[1] == 3.14f);
		CHECK(v.x == -1.0f);
		CHECK(v.r == -1.0f);
		CHECK(v.y == 3.14f);
		CHECK(v.g == 3.14f);
	}
}

TEST_CASE("TVec2 Operators")
{
	// TODO: far from complete

	{
		IVec2 v1(4, 12);
		IVec2 v2;
		
		v2 = v1 + 3;
		CHECK(v2 == IVec2(7, 15));

		v2 = v1 - 3;
		CHECK(v2 == IVec2(1, 9));

		v2 = v1 * 3;
		CHECK(v2 == IVec2(12, 36));

		v2 = v1 / 2;
		CHECK(v2 == IVec2(2, 6));
	}

	{
		IVec2 v1(2, 3);
		IVec2 v2(4, 6);
		IVec2 v;

		v = v1 + v2;
		CHECK(v == IVec2(6, 9));

		v = v1 - v2;
		CHECK(v == IVec2(-2, -3));

		v = v1 * v2;
		CHECK(v == IVec2(8, 18));

		v = v2 / v1;
		CHECK(v == IVec2(2, 2));
	}
}

TEST_CASE("TVec2 Methods")
{
	{
		IVec2 v(3, 4);
		CHECK(v.LengthSquared() == 25);
		
		v = { 7, 24 };
		CHECK(v.LengthSquared() == 625);
	}

	{
		IVec2 v1(3, 4);
		IVec2 v2(-4, 3);
		IVec2 z = IVec2::Zero;

		CHECK(IVec2::Dot(v1, v2) == 0);
		CHECK(IVec2::Dot(v2, v1) == 0);

		v1 = { 4, 5 };
		v2 = { -2, 2 };
		CHECK(IVec2::Dot(v1, v2) == 2);
		CHECK(IVec2::Dot(v2, v1) == 2);

		CHECK(IVec2::Dot(z, v1) == 0);
		CHECK(IVec2::Dot(z, v2) == 0);
	}
}

TEST_CASE("TVec2 Swizzle")
{
	{
		float x = 1.23f, y = 3.21f;
		Vec2 v1(x, y);
		Vec2 v2 = v1.xx();
		CHECK(v2.x == x);
		CHECK(v2.y == x);
	}
}

TEST_CASE("TVec2 Color Swizzle")
{
	{
		float r = 1.23f, g = 3.21f;
		Vec2 v1(r, g);
		Vec2 v2 = v1.gr();
		CHECK(v2.r == g);
		CHECK(v2.g == r);
	}
}