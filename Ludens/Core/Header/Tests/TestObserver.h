#pragma once

#include <doctest.h>
#include "Core/Header/Include/Observer.h"

using namespace LD;

// designed by user to express what has been changed, also
// distinguishes between Observer<EventA> and Observer<EventB> classes
using FooInvalidation = int;

class FooObservable : public Observable<FooInvalidation>
{
public:
    void SetValue(int value)
    {
        mValue = value;
        NotifyObservers({ mValue });
    }

private:
    int mValue = 0;
};

// observe class Foo state changes
class BarObserver : public Observer<FooInvalidation>
{
public:
    int LatestValue = -1;

protected:
    virtual void OnObserverNotify(Observable<FooInvalidation>* observable, const FooInvalidation& event) override
    {
        // query and do stuff with class public interface
        (void)static_cast<FooObservable*>(observable);

        // or access event struct
        LatestValue = event;
    }
};

TEST_CASE("Single Observer")
{
    BarObserver b1;

    FooObservable f1, f2, f3;
    f1.AddObserver(&b1);
    f2.AddObserver(&b1);
    f3.AddObserver(&b1);

    CHECK(b1.LatestValue == -1);

    f1.SetValue(2);
    CHECK(b1.LatestValue == 2);

    f2.SetValue(4);
    CHECK(b1.LatestValue == 4);

    f3.SetValue(27);
    CHECK(b1.LatestValue == 27);
}

TEST_CASE("Single Observable")
{
    BarObserver b1, b2, b3;

    FooObservable f1;
    f1.AddObserver(&b1);
    f1.AddObserver(&b2);
    f1.AddObserver(&b3);

    f1.SetValue(20);
    CHECK(b1.LatestValue == 20);
    CHECK(b2.LatestValue == 20);
    CHECK(b3.LatestValue == 20);
}

TEST_CASE("Many-to-Many")
{
    BarObserver b1;
    FooObservable f1;

    f1.AddObserver(&b1);

    {
        BarObserver b2;
        f1.AddObserver(&b2);

        f1.SetValue(33);
        CHECK(b1.LatestValue == 33);
        CHECK(b2.LatestValue == 33);
    }

    f1.SetValue(20);
    CHECK(b1.LatestValue == 20);

    {
        FooObservable f2;
        f2.AddObserver(&b1);

        f2.SetValue(44);
        CHECK(b1.LatestValue == 44);
    }

    f1.SetValue(99);
    CHECK(b1.LatestValue == 99);
}