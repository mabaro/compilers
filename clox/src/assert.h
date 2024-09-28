#pragma once

#include "config.h"

#if USING(DEBUG_BUILD)

#include <cstdio>
#include <utility>
#include <debugbreak/debugbreak.h>

namespace util
{

void SetDebugBreakEnabled(bool enabled);
bool IsDebugBreakEnabled();

namespace detail
{
template <typename... Args>
bool assert_handler(const char* condition, const char* file, int line, const char* fmt, Args... args)
{
    fprintf(stderr, "[ASSERTION FAILED: '%s' | %s:%d]", condition, file, line);
    if (fmt)
    {
        fprintf(stderr, ": ");
        fprintf(stderr, fmt, std::forward<Args>(args)...);
    }
    fprintf(stderr, "\n");

    return IsDebugBreakEnabled();
}

}  // namespace detail

}  // namespace util

#define ASSERT_MSG(X, fmt, ...)                                                         \
    do                                                                                  \
    {                                                                                   \
        if (!(X))                                                                       \
        {                                                                               \
            if (util::detail::assert_handler(#X, __FILE__, __LINE__, fmt, ##__VA_ARGS__)) \
            {                                                                           \
                debug_break();                                                          \
            }                                                                           \
        }                                                                               \
    } while (0)

#define ASSERT(X)                                                              \
    do                                                                         \
    {                                                                          \
        if (!(X))                                                              \
        {                                                                      \
            if (util::detail::assert_handler(#X, __FILE__, __LINE__, nullptr)) \
            {                                                                  \
                debug_break();                                                 \
            }                                                                  \
        }                                                                      \
    } while (0)

#define FAIL_MSG(fmt, ...) ASSERT_MSG(0, fmt, ##__VA_ARGS__)
#define FAIL() ASSERT(false)

#else // #if USING(DEBUG_BUILD)

#define ASSERT_MSG(X, fmt, ...)
#define ASSERT(X)
#define FAIL_MSG(fmt, ...)
#define FAIL()

#endif // #else // #if USING(DEBUG_BUILD)
