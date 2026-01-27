#pragma once

#include <cstdio>
#include <cstdlib>
#include <Ludens/Header/Platform.h>

#ifndef LD_DEBUG_BREAK
# ifdef LD_PLATFORM_WIN32
#  define LD_DEBUG_BREAK __debugbreak()
# elif defined(LD_PLATFORM_LINUX)
#  include <csignal>
#  define LD_DEBUG_BREAK raise(SIGTRAP)
# endif
#endif

#ifndef NDEBUG
#define LD_ASSERT(EXPR)                                                        \
    do                                                                         \
        if (!(EXPR))                                                           \
        {                                                                      \
            puts("Assertion Failed: " #EXPR);                                  \
            LD_DEBUG_BREAK;                                                    \
        }                                                                      \
    while (0)
#else
#define LD_ASSERT(EXPR) ((void)0)
#endif

#ifndef LD_UNREACHABLE
# ifdef LD_COMPILER_MSVC
#  define LD_UNREACHABLE __assume(0)
# else
#  define LD_UNREACHABLE __builtin_unreachable()
# endif
#endif
