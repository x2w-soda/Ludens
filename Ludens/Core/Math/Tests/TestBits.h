#include <doctest.h>
#include "Core/Math/Include/Bits.h"

using namespace LD;

TEST_CASE("CountTrailingZeroBits")
{
	CHECK(CountTrailingZeroBits(0) == 32);
	CHECK(CountTrailingZeroBits(1) == 0);
	CHECK(CountTrailingZeroBits(2) == 1);
	CHECK(CountTrailingZeroBits(4) == 2);
	CHECK(CountTrailingZeroBits(1024) == 10);
}

TEST_CASE("IsPowerOf2")
{
	CHECK(!IsPowerOf2(0));
	CHECK(IsPowerOf2(1));
	CHECK(IsPowerOf2(2));
	CHECK(!IsPowerOf2(3));
	CHECK(IsPowerOf2(4));
	CHECK(IsPowerOf2(16384));
}

TEST_CASE("NextPowerOf2")
{
	CHECK(NextPowerOf2(0) == 0);
	CHECK(NextPowerOf2(1) == 1);
	CHECK(NextPowerOf2(2) == 2);
	CHECK(NextPowerOf2(3) == 4);
	CHECK(NextPowerOf2(4) == 4);
	CHECK(NextPowerOf2(5) == 8);
	
	CHECK(NextPowerOf2(63) == 64);
	CHECK(NextPowerOf2(64) == 64);
	CHECK(NextPowerOf2(65) == 128);

	CHECK(NextPowerOf2(65535) == 65536);
	CHECK(NextPowerOf2(65536) == 65536);
	CHECK(NextPowerOf2(65537) == 131072);
}

TEST_CASE("NextPowerOf4")
{
	CHECK(NextPowerOf4(0) == 0);
	CHECK(NextPowerOf4(1) == 1);
	CHECK(NextPowerOf4(2) == 4);
	CHECK(NextPowerOf4(3) == 4);
	CHECK(NextPowerOf4(4) == 4);
	CHECK(NextPowerOf4(5) == 16);

	CHECK(NextPowerOf4(63) == 64);
	CHECK(NextPowerOf4(64) == 64);
	CHECK(NextPowerOf4(65) == 256);

	CHECK(NextPowerOf4(255) == 256);
	CHECK(NextPowerOf4(256) == 256);
	CHECK(NextPowerOf4(257) == 1024);

	CHECK(NextPowerOf4(511) == 1024);
	CHECK(NextPowerOf4(512) == 1024);
	CHECK(NextPowerOf4(513) == 1024);

	CHECK(NextPowerOf4(4095) == 4096);
	CHECK(NextPowerOf4(4096) == 4096);
	CHECK(NextPowerOf4(4097) == 16384);
}