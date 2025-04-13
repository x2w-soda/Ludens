#pragma once

#ifdef WIN32
# define LD_PLATFORM_WIN32
#elif defined(__linux__)
# define LD_PLATFORM_LINUX
#endif

#ifdef LD_PLATFORM_WIN32
# ifdef _MSC_VER
#  define LD_COMPILER_MSVC
# endif
#endif
