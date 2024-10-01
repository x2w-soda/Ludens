#pragma once

#include <doctest.h>
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Tests/DSATests.h"

using namespace LD;

TEST_CASE("Vector ctor")
{
    {
        Vector<float> v;
        CHECK(v.Size() == 0);
        CHECK(v.ByteSize() == 0);
        CHECK(v.IsEmpty());
    }

    {
        Vector<char> v(5);
        CHECK(v.Size() == 5);
        CHECK(v.ByteSize() == 5);
        CHECK(!v.IsEmpty());

        v.Front() = 1;
        v.Back() = 5;

        CHECK(v.Front() == 1);
        CHECK(v.Back() == 5);
    }
}

template <typename TVectorInt>
static void TestVectorMutate()
{
    {
        TVectorInt v;
        for (int i = 0; i < 10; i++)
            v.PushBack(i);

        CHECK(v.Size() == 10);
        CHECK(v.ByteSize() == sizeof(int) * 10);
        CHECK(v.Front() == 0);
        CHECK(v.Back() == 9);

        for (int i = 0; i < v.Size(); i++)
        {
            CHECK(v[i] == i);
            ++v[i];
            CHECK(v[i] == i + 1);
        }
    }
}

TEST_CASE("Vector Mutate")
{
    TestVectorMutate<Vector<int>>();
    TestVectorMutate<SmallVector<int, 5>>();
}

template <typename TVectorInt>
static void TestVectorCopy()
{
    {
       TVectorInt v1 = {1, 2, 3, 4, 5};
       TVectorInt v2(v1);

        CHECK(v2.Size() == 5);
        CHECK(v2[1] == 2);
        CHECK(v2[4] == 5);
        CHECK(v2.Front() == 1);
        CHECK(v2.Back() == 5);

        for (int i = 0; i < v2.Size(); i++)
        {
            v2[i]++;
            CHECK(v2[i] - v1[i] == 1);
        }
    }

    {
       TVectorInt v1 = {1, 2, 3, 4};
       TVectorInt v2 = v1;

        v2.PushBack(5);
        v2.PushBack(6);
        v2.PushBack(7);
        v2.PushBack(8);

        CHECK(v2.Back() == 8);
        CHECK(v2.Size() == 8);
        CHECK(v1.Size() == 4);

        for (int i = 0; i < v1.Size(); i++)
        {
            CHECK(v1[i] == v2[i]);
            v1[i] += 4;
            CHECK(v1[i] == v2[i + 4]);
        }
    }
}

TEST_CASE("Vector Copy")
{
    TestVectorCopy<Vector<int>>();
    TestVectorCopy<SmallVector<int, 1>>();
}

template <typename TVectorFoo>
static void TestVectorElementLifetime()
{
    Foo::Reset();

    {
        TVectorFoo v;
        CHECK(Foo::CtorCount() == 0);
        CHECK(Foo::DtorCount() == 0);

        // growing should default construct elements
        v.Resize(27);
        CHECK(Foo::CtorCount() == 27);
        CHECK(Foo::DtorCount() == 0);

        // shrinking should destruct the unused elements
        v.Resize(20);
        CHECK(Foo::CtorCount() == 27);
        CHECK(Foo::DtorCount() == 7);
    }

    CHECK(Foo::CtorCount() == 27);
    CHECK(Foo::DtorCount() == 27);

    Foo::Reset();

    {
        TVectorFoo v1(10);
        CHECK(Foo::CtorCount() == 10);
        CHECK(Foo::DtorCount() == 0);

        v1.PushBack({});
        v1.PushBack({});
        CHECK(v1.Size() == 12);
        CHECK(Foo::CtorCount() == 14);
        CHECK(Foo::DtorCount() == 2);

        TVectorFoo v2 = v1;
        CHECK(v2.Size() == 12);
        CHECK(Foo::CtorCount() == 26);
        CHECK(Foo::DtorCount() == 2);

        v2.Clear();
        CHECK(Foo::CtorCount() == 26);
        CHECK(Foo::DtorCount() == 14);
    }

    CHECK(Foo::CtorCount() == 26);
    CHECK(Foo::DtorCount() == 26);
}

TEST_CASE("Vector Element Lifetime")
{
    TestVectorElementLifetime<Vector<Foo>>();
    TestVectorElementLifetime<SmallVector<Foo, 8>>();
}