#include "common.h"

////////////////////////////////////////////////////////////////////////////////
#if USING(DEBUG_PRINT_CODE)
namespace debug_print
{
static int sDebugPrintLevel = 0;
int GetLevel() { return sDebugPrintLevel; }
void SetLevel(int level) { sDebugPrintLevel = level; }
}  // namespace debug_print
#endif // #if USING(DEBUG_PRINT_CODE)

////////////////////////////////////////////////////////////////////////////////
namespace Logger
{
namespace detail
{
static LogLevel sLogLevel = LogLevel::All;
}  // namespace detail
void SetLogLevel(LogLevel level) { detail::sLogLevel = level; }
LogLevel GetLogLevel() { return detail::sLogLevel; }
}  // namespace Logger

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

namespace unit_tests
{
namespace common
{
struct Dummy
{
    int a = -1;
    bool b = false;
};
static bool test_result()
{
    int a = 0;
    Result<int> res = makeResult<int>(a);
    ASSERT(res.value() == a);
    Result<Dummy> res2 = makeResult(Dummy{});
    ASSERT(res2.value().a == -1);
    Result<Dummy> res3 = makeResult(res2.value());
    ASSERT(res3.value().a == res2.value().a);
    Result<Dummy> res4 = makeResult(res3.extract());
    ASSERT(res4.value().a == res2.value().a && !res3.isOk());
    Result<Dummy> res5 = makeResultError<Result<Dummy>>();
    ASSERT(!res5.isOk() && res5.error() == Error<>{});
    char* buffer = (char*)malloc(512);
    Result<char*> res6 = makeResult<Result<char*>>(buffer);
    free(buffer);

    return true;
}
static bool test_optional()
{
    {
        Optional<int> opt(1);
        Optional<int> opt2(opt);
        ASSERT(opt.hasValue());
        opt = opt2.extract();
        ASSERT(!opt2.hasValue());
        ASSERT(opt.hasValue());
    }
    {
        using T = Dummy;
        Optional<Dummy> opt(Dummy{});
        Optional<Dummy> opt2(opt);
        ASSERT(opt.hasValue());
        opt = opt2.extract();
        ASSERT(!opt2.hasValue());
        ASSERT(opt.hasValue());
    }
    return true;
}
void run()
{
    bool success = true;
    success &= test_optional();
    success &= test_result();

    const char* message = "Unit tests finished";
    const char* result = success ? "Succeeded" : "Failed";
    auto msg = buildMessage("[line %d] %s. Result: %s\n", 3, message, result);
    printf("%s\n", msg.c_str());
}
}  // namespace common
}  // namespace unit_tests
