#pragma once

#include <cstdio>
#include <cstdarg>

#define BUILDER_PREAMBLE "LudensBuilder> "
#define PrintLn(...)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        printf(BUILDER_PREAMBLE);                                                                                      \
        printf(__VA_ARGS__);                                                                                           \
        putchar('\n');                                                                                                 \
    } while (0)