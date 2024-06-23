#include "assert.h"

namespace util {
namespace detail {
static bool s_debugBreakEnabled = true;
}  // namespace detail
void SetDebugBreakEnabled(bool enabled) { detail::s_debugBreakEnabled = enabled; }
bool IsDebugBreakEnabled() { return detail::s_debugBreakEnabled; }
}  // namespace util
