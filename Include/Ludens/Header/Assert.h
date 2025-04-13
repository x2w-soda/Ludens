#pragma once

#include <cstdio>
#include <cstdlib>
#include <Ludens/Header/Platform.h>

#ifndef LD_DEBUG_BREAK
# ifdef LD_PLATFORM_WIN32
#  define LD_DEBUG_BREAK __debugbreak()
# else
#  define LD_DEBUG_BREAK
# endif
#endif

#ifndef LD_ASSERT
#define LD_ASSERT(EXPR)                                                        \
    do                                                                         \
        if (!(EXPR))                                                           \
        {                                                                      \
            puts("Assertion Failed: " #EXPR);                                  \
            LD_DEBUG_BREAK;                                                    \
        }                                                                      \
    while (0)
#endif

#ifndef LD_UNREACHABLE
#define LD_UNREACHABLE LD_ASSERT(0 && "Unreachable")
#endif
