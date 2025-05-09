#pragma once

#include <Ludens/Header/Platform.h>

// clang-format off

// detect instruction sets, drags platform specific headers
#if defined(LD_ARCH_X64)
# define LD_SSE2 true
#else
# define LD_SSE2 false
#endif
#if defined(__SSE4_1__)
# define LD_SSE4_1
#endif

#if defined(LD_SSE2) && defined(LD_COMPILER_MSVC)
# include <emmintrin.h>
# include <xmmintrin.h>
#endif

// clang-format on