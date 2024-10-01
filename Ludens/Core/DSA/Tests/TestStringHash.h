#pragma once

#include <doctest.h>
#include "Core/DSA/Include/String.h"
#include "Core/Media/Include/Mesh.h"

using namespace LD;

TEST_CASE("StringHash")
{
	// empty strings are considered equal
	CHECK(StringHash{} == StringHash{});
	
	StringHash h1("foo");
	StringHash h2("bar");
	CHECK(h1 != h2);

	// rehash
	h2 = "foo";
	CHECK(h1 == h2);
}