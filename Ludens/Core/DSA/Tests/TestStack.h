#pragma once

#include <doctest.h>
#include "Core/DSA/Include/Stack.h"
#include "Core/DSA/Tests/DSATests.h"

using namespace LD;

TEST_CASE("Stack Ctor")
{
    Stack<int> s1;
    CHECK(s1.Size() == 0);
    CHECK(s1.IsEmpty());

    s1.Push(20);
    s1.Push(30);

    Stack<int> s2(s1);

    CHECK(s2.Size() == 2);
    CHECK(s2.Top() == 30);
}

TEST_CASE("Stack Element Lifetime")
{
    Foo::Reset();

    {
        Stack<Foo> s1;

        for (int i = 0; i < 100; i++)
        {
            s1.Push({i});
        }

        const Foo& foo = s1.Top();
        CHECK(s1.Size() == 100);
        CHECK(foo.Value == 99);
        CHECK(Foo::CtorCount() == 200);
        CHECK(Foo::DtorCount() == 100);

        Stack<Foo> s2 = s1;
        CHECK(s2.Size() == 100);
        CHECK(Foo::CtorCount() == 300);
        CHECK(Foo::DtorCount() == 100);

        while (!s1.IsEmpty())
            s1.Pop();
        CHECK(Foo::CtorCount() == 300);
        CHECK(Foo::DtorCount() == 200);
    }

    CHECK(Foo::CtorCount() == 300);
    CHECK(Foo::DtorCount() == 300);
}