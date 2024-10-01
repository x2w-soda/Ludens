#include <cstring>
#include <doctest.h>
#include "Core/DSA/Include/Buffer.h"

using namespace LD;

TEST_CASE("Buffer Size")
{
    Buffer buf;
    CHECK(buf.Data() == nullptr);
    CHECK(buf.Size() == 0);
    CHECK(buf.AllocSize() == 0);

    buf.Resize(1020);
    CHECK(buf.Data() != nullptr);
    CHECK(buf.Size() == 1020);
    CHECK(buf.AllocSize() == 1024); // assuming next power of 2 policy
}

TEST_CASE("Buffer Old Data")
{
    Buffer buf(8);
    char* str;
    int result;
    
    str = (char*)buf.Data();
    strncpy(str, "old data", buf.Size());

    result = strncmp(str, "old data", buf.Size());
    CHECK(result == 0);

    buf.Resize(1024);
    str = (char*)buf.Data();
    strncpy(str + 8, " is preserved", buf.Size() - 8);

    result = strncmp(str, "old data is preserved", buf.Size());
    CHECK(result == 0);
}