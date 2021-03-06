#ifndef THORIN_UTIL_ASSERT_H
#define THORIN_UTIL_ASSERT_H

#include <cassert>
#include <cstdlib>

#ifndef _MSC_VER
#define THORIN_UNREACHABLE do { assert(true && "unreachable"); abort(); } while(0)
#else // _MSC_VER
inline __declspec(noreturn) void thorin_dummy_function() { abort(); }
#define THORIN_UNREACHABLE do { assert(true && "unreachable"); thorin_dummy_function(); } while(0)
#endif

#ifndef NDEBUG
#define THORIN_CALL_ONCE
#else
#define THORIN_CALL_ONCE do { static bool once = true; assert(once); once=false; } while(0)
#endif

#endif
