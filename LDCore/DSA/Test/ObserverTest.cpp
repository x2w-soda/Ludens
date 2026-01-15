#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Observer.h>

using namespace LD;

static void increment(int inc, void* user)
{
    *(int*)user += inc;
}

TEST_CASE("ObserverList")
{
    ObserverList<int> list;
    int value = 0;
    list.add_observer(&increment, &value);

    list.notify(1);
    CHECK(value == 1);

    list.notify(2);
    CHECK(value == 3);

    list.notify(3);
    CHECK(value == 6);

    list.remove_observer(&increment, &value);
    list.notify(4);
    CHECK(value == 6);
}

TEST_CASE("ObserverList Membership")
{
    ObserverList<int> list;
    int value = 0;
    list.add_observer(&increment, &value);
    list.remove_observer(&increment, nullptr);
    list.remove_observer(nullptr, &value);

    list.notify(0xCAFE);
    CHECK(value == 0xCAFE);

    list.remove_observer(&increment, &value);
    list.notify(1);
    CHECK(value == 0xCAFE);
}