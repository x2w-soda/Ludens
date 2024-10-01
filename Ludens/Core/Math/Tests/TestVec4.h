#include <doctest.h>
#include "Core/Math/Include/Vec4.h"

using namespace LD;

TEST_CASE("TVec4 Data")
{
	{
		IVec4 v(1, 2, 3, 4);
		CHECK(v.Data[0] == 1);
		CHECK(v.Data[1] == 2);
		CHECK(v.Data[2] == 3);
		CHECK(v.Data[3] == 4);
		CHECK(v.x == 1);
		CHECK(v.y == 2);
		CHECK(v.z == 3);
		CHECK(v.w == 4);
	}

	{
		IVec4 v;
		CHECK(v == IVec4::Zero);
	}

	{
		IVec2 v0(1, 2);
		IVec2 v1(3, 4);
		IVec3 v2(5, 6, 7);
		IVec4 v3(v0, 3, 4);
		CHECK(v3 == IVec4(1, 2, 3, 4));

		v3 = { 3, v0, 4 };
		CHECK(v3 == IVec4(3, 1, 2, 4));

		v3 = { 3, 4, v0 };
		CHECK(v3 == IVec4(3, 4, 1, 2));

		v3 = { v0, v1 };
		CHECK(v3 == IVec4(1, 2, 3, 4));

		v3 = { v2, 1 };
		CHECK(v3 == IVec4(5, 6, 7, 1));

		v3 = { 1, v2 };
		CHECK(v3 == IVec4(1, 5, 6, 7));
	}
}

TEST_CASE("TVec4 Operators")
{
	// TODO: far from complete

	{
		IVec4 v1(4, 12, 8, -4);
		IVec4 v2;

		v2 = v1 + 3;
		CHECK(v2 == IVec4(7, 15, 11, -1));

		v2 = v1 - 3;
		CHECK(v2 == IVec4(1, 9, 5, -7));

		v2 = v1 * 3;
		CHECK(v2 == IVec4(12, 36, 24, -12));

		v2 = v1 / 2;
		CHECK(v2 == IVec4(2, 6, 4, -2));
	}

	{
		IVec4 v1(2, 3, 6, 8);
		IVec4 v2(4, 6, 18, -16);
		IVec4 v;

		v = v1 + v2;
		CHECK(v == IVec4(6, 9, 24, -8));

		v = v1 - v2;
		CHECK(v == IVec4(-2, -3, -12, 24));

		v = v1 * v2;
		CHECK(v == IVec4(8, 18, 108, -128));

		v = v2 / v1;
		CHECK(v == IVec4(2, 2, 3, -2));
	}
}

TEST_CASE("TVec4 Methods")
{
	{
		IVec4 v(3, 4, 5, 1);
		CHECK(v.LengthSquared() == 51);

		v = { 7, 24, 1, 2 };
		CHECK(v.LengthSquared() == 630);
	}

	{
		IVec4 v1(3, 4, 5, 4);
		IVec4 v2(-4, 3, 2, -3);
		IVec4 z = IVec4::Zero;

		CHECK(IVec4::Dot(v1, v2) == -2);
		CHECK(IVec4::Dot(v2, v1) == -2);

		v1 = { 4, 5, 3, -2 };
		v2 = { -2, 2, 5, 7 };
		CHECK(IVec4::Dot(v1, v2) == 3);
		CHECK(IVec4::Dot(v2, v1) == 3);

		CHECK(IVec4::Dot(z, v1) == 0);
		CHECK(IVec4::Dot(z, v2) == 0);
	}
}