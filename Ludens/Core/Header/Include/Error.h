#pragma once


// Debug Assertions
// - the asserted expression will be stripped in release builds
// - mostly used to assert prerequisites and crash early in debug builds
#ifndef LD_DEBUG_ASSERT
# include <cassert>
# define LD_DEBUG_ASSERT(EXPR)             assert(EXPR)
#endif


// Debug Canaries
// - the asserted expression and canary will be stripped in release builds
// - the canary is anything that is compliant with the function call syntax
#ifndef LD_DEBUG_CANARY
# define LD_DEBUG_CANARY(EXPR, CANARY)     if (!static_cast<bool>(EXPR)) { CANARY( #EXPR ); }
#endif


// Debug Unreachable
// - prohibit code paths in debug builds
#ifndef LD_DEBUG_UNREACHABLE
# define LD_DEBUG_UNREACHABLE              LD_DEBUG_ASSERT(false && "unreachable")
#endif


// Static Assertions
// - the asserted expression is checked by the compiler at compile time
// - does not need to be in function body
#ifndef LD_STATIC_ASSERT
# define LD_STATIC_ASSERT(EXPR)            static_assert(EXPR, #EXPR)
#endif