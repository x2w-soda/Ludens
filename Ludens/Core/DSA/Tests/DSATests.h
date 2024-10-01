#pragma once

// utility class to test element lifetime in containers
class Foo
{
public:
    Foo()
    {
        Value = 0;
        CtorCounter++;
    }

    Foo(int value) : Value(value)
    {
        CtorCounter++;
    }

    Foo(const Foo& other)
    {
        Value = other.Value;
    }

    ~Foo()
    {
        DtorCounter++;
    }

    Foo& operator=(const Foo& other)
    {
        Value = other.Value;
        return *this;
    }

    int Value;

    static void Reset()
    {
        CtorCounter = 0;
        DtorCounter = 0;
    }

    static int CtorCount()
    {
        return CtorCounter;
    }

    static int DtorCount()
    {
        return DtorCounter;
    }

private:
    static int CtorCounter;
    static int DtorCounter;
};