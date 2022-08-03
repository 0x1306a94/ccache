//
//  fmtmacros.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef fmtmacros_h
#define fmtmacros_h

#include <fmt/core.h>
#include <fmt/format.h>

// Convenience macro for calling `fmt::format` with `FMT_STRING` around the
// format string literal.
#define FMT(format_, ...) fmt::format(FMT_STRING(format_), __VA_ARGS__)

// Convenience macro for calling `fmt::print` with `FMT_STRING` around the
// format string literal.
#define PRINT(stream_, format_, ...) \
    fmt::print(stream_, FMT_STRING(format_), __VA_ARGS__)

// Convenience macro for calling `fmt::print` with a message that is not a
// format string.
#define PRINT_RAW(stream_, message_) fmt::print(stream_, "{}", message_)

#endif /* fmtmacros_h */

