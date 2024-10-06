#include "utils/common.h"

////////////////////////////////////////////////////////////////////////////////
#if USING(DEBUG_PRINT_CODE)
namespace debug_print
{
static int sDebugPrintLevel = 0;
int        GetLevel() { return sDebugPrintLevel; }
void       SetLevel(int level) { sDebugPrintLevel = level; }
}  // namespace debug_print
#endif  // #if USING(DEBUG_PRINT_CODE)

////////////////////////////////////////////////////////////////////////////////
namespace Logger
{
namespace detail
{
static LogLevel sLogLevel = LogLevel::All;
}  // namespace detail
void     SetLogLevel(LogLevel level) { detail::sLogLevel = level; }
LogLevel GetLogLevel() { return detail::sLogLevel; }
}  // namespace Logger

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

namespace utils
{

Result<CharBufferUPtr> readFile(const char* path, bool binaryMode)
{
    using result_t = Result<CharBufferUPtr>;

    FILE* file = nullptr;
    fopen_s(&file, path, binaryMode ? "rb" : "r");
    if (file == nullptr)
    {
        return makeResultError<result_t>(result_t::error_t::code_t::Undefined,
                                         format("Couldn't open file '%s'\n", path));
    }
    ScopedCallback closeFile([&file] { fclose(file); });

    fseek(file, 0L, SEEK_END);
    const size_t fileSize = static_cast<size_t>(ftell(file));
    rewind(file);

    CharBufferUPtr buffer = std::make_unique<char[]>(fileSize + 1);
    if (buffer == nullptr)
    {
        char message[1024];
        snprintf(message, sizeof(message), "Couldn't allocate memory for reading the file %s with size %zu byte(s)\n",
                 path, fileSize);
        LOG_ERROR(message);
        return makeResultError<result_t>(result_t::error_t::code_t::Undefined, message);
    }
    size_t bytesRead = 0;
    do
    {
        const size_t count = fread(buffer.get(), sizeof(char), fileSize, file);
        if (count == 0)
        {
            break;
        }
        bytesRead += count;
    } while (bytesRead < fileSize);
    if (bytesRead < fileSize)
    {
        LOG_ERROR("Couldn't read the file '%s'\n", path);
        return makeResultError<result_t>(result_t::error_t::code_t::Undefined,
                                         format("Couldn't read the file '%s'\n", path));
    }
    buffer[bytesRead] = '\0';

    return std::move(buffer);
}
}  // namespace utils

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

namespace unit_tests
{
namespace common
{
struct Dummy
{
    int  a = -1;
    bool b = false;
};
static bool test_result()
{
    int         a   = 0;
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
    char*         buffer = (char*)malloc(512);
    Result<char*> res6   = makeResult<Result<char*>>(buffer);
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
    const char* result  = success ? "Succeeded" : "Failed";
    auto        msg     = buildMessage("[LINE %d] %s. Result: %s\n", 3, message, result);
    printf("%s\n", msg.c_str());
}
}  // namespace common
}  // namespace unit_tests
