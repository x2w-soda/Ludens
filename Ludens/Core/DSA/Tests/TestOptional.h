#pragma once

#include <doctest.h>
#include "Core/DSA/Include/Optional.h"

using namespace LD;

namespace {

	struct Box
	{
		Box() : x(0) { sCounter++; }
		Box(int x_) : x(x_) { sCounter++; }
		Box(const Box& other) : x(other.x) { sCounter++; }
		~Box() { sCounter--; }

		Box& operator=(const Box& other) { x = other.x; return *this; }

		int x;

		static int sCounter;
	};

	class Cargo
	{
	public:
		inline void SetBox(const Box& box) { mBox = box; }
		inline const Optional<Box>& GetBox() const { return mBox; }
		inline void Reset() { mBox.Reset(); }
	private:
		Optional<Box> mBox;
	};

	int Box::sCounter = 0;
}

TEST_CASE("Optional Class")
{
	{
		Optional<int> opt;
		CHECK(!opt.HasValue());
		CHECK(opt.ValueOr(4) == 4);

		opt = 10;
		CHECK(opt.HasValue());
		CHECK(opt.Value() == 10);
		CHECK(opt.ValueOr(4) == 10);
	}

	{
		Optional<Box> opt;
		CHECK(Box::sCounter == 0);

		opt = Box{ 2 };
		CHECK(Box::sCounter == 1);
		CHECK(opt.HasValue());

		opt = Box{ 3 };
		opt = Box{ 4 };
		opt = Box{ 5 };
		CHECK(Box::sCounter == 1);
		CHECK(opt.Value().x == 5);
		
		const Box& b1 = opt.Value();
		CHECK(b1.x == 5);
		
		Box& b2 = opt.Value();
		CHECK(b2.x == 5);

		b2.x = 20;
		CHECK(opt.Value().x == 20);
		CHECK(b1.x == 20);
	}

	{
		Optional<Box> opt;
		CHECK(Box::sCounter == 0);

		Box box(3);
		opt = box;
		CHECK(Box::sCounter == 2);
		CHECK(opt.Value().x == 3);

		opt.Reset();
		CHECK(Box::sCounter == 1);
		CHECK(opt.ValueOr({ -1 }).x == -1);
	}

	CHECK(Box::sCounter == 0);
}

TEST_CASE("Optional Aggregate")
{
	{
		Cargo c1;
		CHECK(Box::sCounter == 0);
		CHECK(!c1.GetBox().HasValue());

		c1.SetBox({ 30 });
		CHECK(Box::sCounter == 1);
		CHECK(c1.GetBox().Value().x == 30);

		Cargo c2(c1);
		CHECK(Box::sCounter == 2);
		CHECK(c2.GetBox().Value().x == 30);

		Cargo c3 = c2;
		CHECK(Box::sCounter == 3);
		CHECK(c3.GetBox().Value().x == 30);

		c1.Reset();
		c2.Reset();
		CHECK(Box::sCounter == 1);
	}

	CHECK(Box::sCounter == 0);
}