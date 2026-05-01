#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Header/View.h>

using namespace LD;

static_assert(LD::IsTrivial<View>);

TEST_CASE("String ctor")
{
    String s;
    CHECK(s.size() == 0);
    CHECK(s.empty());

    const char* cstr = "string";
    s = cstr;
    CHECK(s.size() == 6);

    s.clear();
    CHECK(s.empty());

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("String copy")
{
    const char* cstr = "hello, world";

    String s1(cstr);
    CHECK(s1.size() == 12);

    // copy construct
    String s2(s1);
    CHECK(s2.size() == s1.size());
    CHECK(s2 == s1);

    // copy assign
    String s3;
    s3 = s1;
    CHECK(s3.size() == s1.size());
    CHECK(s3 == s2);

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("String move")
{
    const char* cstr = "string move";
    size_t len = strlen(cstr);

    {
        String s1(cstr);
        CHECK(s1.size() == len);

        // move construct local string
        String s2(std::move(s1));
        CHECK(s2.size() == len);
        CHECK(s2 == "string move");

        // move assign local string
        String s3;
        s3 = std::move(s2);
        CHECK(s3.size() == len);
        CHECK(s3 == "string move");

        s1 = String(cstr);
        s1.resize(STRING_DEFAULT_LOCAL_SIZE * 2); // move from local to heap storage
        s1.resize(len);                           // does not revert to local storage
        CHECK(s1 == "string move");               // content should not be truncated

        s1.resize(STRING_DEFAULT_LOCAL_SIZE * 3); // grow heap storage
        s1.resize(len);                           // does not revert to local storage
        CHECK(s1 == "string move");               // content should not be truncated

        // move construct heap string
        String s4(std::move(s1));
        CHECK(s4.size() == len);
        CHECK(s4 == "string move");

        // move assign heap string
        s3 = std::move(s4);
        CHECK(s3.size() == len);
        CHECK(s3 == "string move");
    }

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("String methods")
{
    {
        String s;

        CHECK(s.capacity() == STRING_DEFAULT_LOCAL_SIZE);
        CHECK(s.empty());

        s = "foo";
        CHECK(s == "foo");

        s.resize(s.capacity() + 1);
        s.resize(3);
        CHECK(s == "foo");
        CHECK(s.data()[0] == 'f');
        CHECK(s.size() == 3);

        s.clear();
        CHECK(s.size() == 0);
    }

    CHECK_FALSE(get_memory_leaks(nullptr));
}

TEST_CASE("String replace")
{
    {
        String s = "foo";

        // nop
        s.replace(0, 0, nullptr, 0);
        CHECK(s == "foo");

        // replacement retains same size
        s.replace(1, 2, "ar", 2);
        CHECK(s == "far");

        // replacement grows string
        s.replace(1, 2, "bar", 3);
        CHECK(s == "fbar");

        // replacement shrinks string
        s.replace(1, 2, nullptr, 0);
        CHECK(s == "fr");
    }

    {
        // replace front
        String s = "abcdef";
        s.replace(0, 2, "XY", 2);
        CHECK(s == "XYcdef");

        // replace end
        s = "abcdef";
        s.replace(4, 2, "XY", 2);
        CHECK(s == "abcdXY");

        // replace middle
        s = "abcdef";
        s.replace(2, 2, "XY", 2);
        CHECK(s == "abXYef");

        // replace with longer string
        s = "abc";
        s.replace(1, 1, "XYZ", 3);
        CHECK(s == "aXYZc");

        // replace with shorter string
        s = "abcdef";
        s.replace(2, 3, "X", 1);
        CHECK(s == "abXf");

        // replace with nothing
        s = "abcdef";
        s.replace(2, 3, nullptr, 0);
        CHECK(s == "abf");

        // append
        s = "abc";
        s.replace(3, 0, "XYZ", 3);
        CHECK(s == "abcXYZ");

        // insert
        s = "abc";
        s.replace(1, 0, "XYZ", 3);
        CHECK(s == "aXYZbc");

        // insert front
        s = ", world";
        s.replace(0, 0, "Hello", 5);
        CHECK(s == "Hello, world");

        // full replace
        s = "abc";
        s.replace(0, 3, "XYZ", 3);
        CHECK(s == "XYZ");

        // nop
        s = "";
        s.replace(0, 0, nullptr, 0);
        CHECK(s == "");
    }

    CHECK_FALSE(get_memory_leaks(nullptr));
}