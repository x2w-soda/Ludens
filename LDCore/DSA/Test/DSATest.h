#pragma once

namespace {

/// @brief A copyable, movable type for testing
struct Foo
{
    int value;

    static int sCtor;
    static int sDtor;
    static int sCopyCtor;
    static int sCopyAssign;
    static int sMoveCtor;
    static int sMoveAssign;

    Foo() : value(0)
    {
        sCtor++;
    }

    explicit Foo(int value) : value(value)
    {
        sCtor++;
    }

    Foo(const Foo& other) : value(other.value)
    {
        sCopyCtor++;
    }

    Foo(Foo&& other) : value(other.value)
    {
        sMoveCtor++;
    }

    ~Foo()
    {
        sDtor++;
    }

    Foo& operator=(const Foo& other)
    {
        value = other.value;
        sCopyAssign++;
        return *this;
    }

    Foo& operator=(Foo&& other)
    {
        value = other.value;
        sMoveAssign++;
        return *this;
    }

    Foo& operator=(int v)
    {
        value = v;
        return *this;
    }

    bool operator==(int v) const
    {
        return value == v;
    }

    static void reset()
    {
        sCtor = 0;
        sDtor = 0;
        sCopyCtor = 0;
        sCopyAssign = 0;
        sMoveCtor = 0;
        sMoveAssign = 0;
    }
};

} // namespace