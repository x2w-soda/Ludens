#pragma once

// clang-format off

// detect OS platform
#ifdef WIN32
# define LD_PLATFORM_WIN32
#elif defined(__linux__)
# define LD_PLATFORM_LINUX
#endif

// detect compiler
#ifdef LD_PLATFORM_WIN32
# ifdef _MSC_VER
#  define LD_COMPILER_MSVC
# endif
#endif

// detect CPU architecture
#if defined(__x86_64__) || defined(_M_X64)
# define LD_ARCH_X64
#endif

// clang-format on
