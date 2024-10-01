#pragma once

#include <doctest.h>
#include "Core/OS/Include/UID.h"
#include "Core/DSA/Include/Vector.h"

using namespace LD;

namespace {

	struct Bar;

	struct Foo
	{
		Foo()
			: ID(CUID<Foo>::Get())
		{
		}
		Foo(const Foo&) = delete;
		Foo(Foo&& other)
			: ID(std::move(other.ID))
		{
		}
		~Foo() = default;

		Foo& operator=(const Foo&) = delete;
		Foo& operator=(Foo&& other) noexcept
		{
			ID = std::move(other.ID);
			return *this;
		}

		CUID<Foo> ID;
	};

}

TEST_CASE("CUID Move")
{
	{
		UID id;
		CUID<Bar> id1;
		CHECK((UID)id1 == 0);

		id1 = CUID<Bar>::Get();
		CHECK((UID)id1 != 0);

		CUID<Bar> id2 = std::move(id1);
		CHECK((UID)id1 == 0);
		CHECK((UID)id2 != 0);
	}

	{
		Foo f1, f2;
		UID id1 = f1.ID;
		UID id2 = f2.ID;

		CHECK(id1 != 0);
		CHECK(id2 != 0);

		f1 = std::move(f2);
		CHECK(f1.ID == id2);
		CHECK(f2.ID == 0);
	}
}