//
//  assertions.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef assertions_h
#define assertions_h

#include <cstddef>

#ifdef _MSC_VER
#define CCACHE_FUNCTION __func__
#else
#define CCACHE_FUNCTION __PRETTY_FUNCTION__
#endif

// ASSERT is like the standard C `assert` macro but enabled in both debug and
// release builds.
#define ASSERT(condition)                                         \
    do {                                                          \
        if (!(condition)) {                                       \
            handle_failed_assertion(                              \
                __FILE__, __LINE__, CCACHE_FUNCTION, #condition); \
        }                                                         \
    } while (false)

// DEBUG_ASSERT is like the standard C `assert` macro, i.e. only enabled in
// debug builds.
#ifdef NDEBUG
#define DEBUG_ASSERT(condition) ((void)0)
#else
#define DEBUG_ASSERT(condition) ASSERT(condition)
#endif

[[noreturn]] void handle_failed_assertion(const char *file,
                                          size_t line,
                                          const char *function,
                                          const char *condition);

#endif /* assertions_h */

