#pragma once


#ifdef _WIN32
# define LD_PLATFORM_WIN32
# ifdef _MSC_VER
#  define LD_PLATFORM_WIN32_MSVC
# endif
#endif


#ifdef __linux__
# define LD_PLATFORM_LINUX
#endif