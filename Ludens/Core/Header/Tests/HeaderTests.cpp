#include <functional>
#include <iostream>
#include <cstring>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include "Core/Header/Include/Types.h"
#include "Core/Header/Include/Error.h"
#include "Core/Header/Tests/TestSingleton.h"
#include "Core/Header/Tests/TestObserver.h"

// Sanity Check
LD_STATIC_ASSERT(sizeof(LD::u8) == 1);
LD_STATIC_ASSERT(sizeof(LD::u16) == 2);
LD_STATIC_ASSERT(sizeof(LD::u32) == 4);
LD_STATIC_ASSERT(sizeof(LD::u64) == 8);

LD_STATIC_ASSERT(sizeof(LD::f32) == 4);
LD_STATIC_ASSERT(sizeof(LD::f64) == 8);

static int sCounter;
static const char* sMessage;

static void function_canary(const char* msg)
{
    sCounter++;
    sMessage = msg; // msg is static string literal
}

struct struct_canary
{
    void operator()(const char* msg)
    {
        sCounter++;
        sMessage = msg;
    }
};

TEST_CASE("Debug Canaries")
{
    sCounter = 0;
    sMessage = nullptr;

    {
        std::function<void(const char*)> canary = [&](const char* msg)
        {
            sCounter++;
            sMessage = msg;
        };

        LD_DEBUG_CANARY(1 == 1, canary)
        CHECK(sCounter == 0);

        LD_DEBUG_CANARY(1 == 2, canary)
        CHECK(sCounter == 1);
        CHECK(strcmp(sMessage, "1 == 2") == 0);
    }

    {
        LD_DEBUG_CANARY(true, function_canary)
        CHECK(sCounter == 1);

        LD_DEBUG_CANARY(nullptr, function_canary)
        CHECK(sCounter == 2);
        CHECK(strcmp(sMessage, "nullptr") == 0);
    }

    {
        void (*canary)(const char*) = &function_canary;

        LD_DEBUG_CANARY(3.14f, canary)
        CHECK(sCounter == 2);

        LD_DEBUG_CANARY(false, canary)
        CHECK(sCounter == 3);
        CHECK(strcmp(sMessage, "false") == 0);
    }

    {
        struct_canary canary;

        LD_DEBUG_CANARY(!NULL, canary)
        CHECK(sCounter == 3);

        LD_DEBUG_CANARY(1 - 1, canary)
        CHECK(sCounter == 4);
        CHECK(strcmp(sMessage, "1 - 1") == 0);
    }

    {
        LD_DEBUG_CANARY(0.0f,
                        [&](const char* msg)
                        {
                            sCounter++;
                            sMessage = msg;
                        })

        CHECK(sCounter == 5);
        CHECK(strcmp(sMessage, "0.0f") == 0);
    }
}