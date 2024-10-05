#pragma once

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>  // unique_ptr
#include <string>
#include <type_traits>

#include "assert.h"
#include "config.h"

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

////////////////////////////////////////////////////////////////////////////////

#define DEBUG_PRINT_CODE NOT_INT_USE  // USING(DEBUG_BUILD)
#define DEBUG_TRACE_EXECUTION USING(DEBUG_BUILD)

#if USING(DEBUG_PRINT_CODE)
namespace debug_print
{
int  GetLevel();
void SetLevel(int level);
}  // namespace debug_print
#endif  // #if USING(DEBUG_PRINT_CODE)

////////////////////////////////////////////////////////////////////////////////
// Portability hacks

#ifdef __unix
#define fopen_s(pFile, filename, mode) ((*(pFile)) = fopen((filename), (mode))) == NULL
#endif

////////////////////////////////////////////////////////////////////////////////
// Language features

#define LANG_EXT_MUT IN_USE  // variables are const by default

////////////////////////////////////////////////////////////////////////////////

enum class LogLevel
{
    Error   = 1 << 0,
    Warning = 1 << 1,
    Info    = 1 << 2,
    Debug   = 1 << 3,

    All = 0xFF
};
namespace Logger
{
void     SetLogLevel(LogLevel level);
LogLevel GetLogLevel();

template <typename... Args>
void Log(LogLevel level, const char* fmt, Args... args)
{
    FILE* outFile = nullptr;
    switch (level)
    {
        case LogLevel::Error:
            outFile = stderr;
            fprintf(outFile, "Error: ");
            break;
        case LogLevel::Warning:
            outFile = stderr;
            fprintf(outFile, "Warning: ");
            break;
        case LogLevel::Info:
            outFile = stdout;
            fprintf(outFile, "Info: ");
            break;
        case LogLevel::Debug:
            outFile = stdout;
            fprintf(outFile, "Debug: ");
            break;
        default: break;
    }
    if (outFile)
    {
        std::fprintf(outFile, fmt, std::forward<Args>(args)...);
    }
}
}  // namespace Logger

static std::string buildMessage(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buff[1000];
    int  len = vsnprintf(buff, 1000, fmt, args);
    va_end(args);
    return std::string(buff);
}

#define LOG_BASE(level, fmt, ...)               \
    do                                          \
    {                                           \
        Logger::Log(level, fmt, ##__VA_ARGS__); \
    } while (0)

#define LOG_ERROR(fmt, ...) LOG_BASE(LogLevel::Error, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG_BASE(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG_BASE(LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_BASE(LogLevel::Debug, fmt, ##__VA_ARGS__)

#define DEBUGPRINT(fmt, ...) LOG_BASE(LogLevel::Debug, "DEBUG(line_%d): " #fmt, __LINE__, ##__VA_ARGS__)
#define DEBUGPRINT_EX(fmt, ...) LOG_BASE(LogLevel::Debug, "[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////

static std::string format(const char* format, ...)
{
    char    message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    return message;
}

////////////////////////////////////////////////////////////////////////////////////////////////

struct ScopedCallback
{
    using callback_t = std::function<void()>;
    ScopedCallback(callback_t callback) : callbackFunc(callback) {}
    ~ScopedCallback() { callbackFunc(); }
    callback_t callbackFunc = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////

enum class ErrorCode
{
    Undefined = 0
};
template <typename ErrCodeT = ErrorCode>
struct Error
{
    using code_t = ErrCodeT;

    Error() {}
    explicit Error(const char* msg) : _message(msg) {}
    explicit Error(const std::string& msg) : _message(msg) {}
    explicit Error(code_t e) : _code(e) {}
    Error(code_t e, const std::string& msg) : _code(e), _message(msg) {}
    Error(code_t e, const char* msg) : _code(e), _message(msg) {}

    Error(const Error& e) : _code(e._code), _message(e._message) {}
    Error(Error&& e) : _code(e._code), _message(std::move(e._message)) {}
    Error& operator=(const Error& e)
    {
        _code    = e;
        _message = e.msg;
    }
    Error& operator=(Error&& e)
    {
        _code    = std::move(e);
        _message = std::move(e.msg);
    }

    bool operator==(const Error& e) const { return e._code == _code && !e._message.compare(_message); }

    code_t             code() const { return _code; }
    const std::string& message() const { return _message; }

   protected:
    const code_t      _code    = code_t::Undefined;
    const std::string _message = "Undefined";
};
using Error_t = Error<ErrorCode>;

////////////////////////////////////////////////////////////////////////////////////////////////

struct none_type
{
};
constexpr none_type none_t = none_type{};

template <typename T>
struct Optional
{
    Optional() {}
    Optional(none_type) {}
    Optional(const T& t) : _value(new T(t)) {}
    Optional(T&& t) : _value(new T(std::move(t))) {}
    Optional(const Optional& o) : _value(o._value ? new T(*o._value) : nullptr) { ASSERT(!hasValue() || _value); }
    Optional& operator=(const Optional& o)
    {
        reset();
        _value = o._value ? new T(*o._value) : nullptr;
        return *this;
    }
    Optional& operator=(Optional&& o)
    {
        reset();
        std::swap(_value, o._value);
        return *this;
    }
    ~Optional() { reset(); }

    const T& value() const
    {
        ASSERT(hasValue());
        return *_value;
    }
    T&   value() { return *_value; }
    bool hasValue() const { return _value != nullptr; }
    T    extract()
    {
        ASSERT(hasValue());
        T temp = std::move(*_value);
        reset();
        return temp;
    }
    void reset()
    {
        delete _value;
        _value = nullptr;
    }

   protected:
    T* _value = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename ErrorT = Error<>>
struct Result
{
    using value_t = T;
    using error_t = ErrorT;

    Result(const T& t) : _value(t) {}
    Result(T&& t) : _value(std::move(t)) {}
    Result(const ErrorT& e) : _error(e) {}
    Result(typename ErrorT::code_t e) : _error(error_t(e)) {}
    Result(const Result& r) : _value(r._value), _error(r._error) {}
    Result(Result&& r) : _value(std::move(r._value)), _error(std::move(r._error)) {}
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&)      = delete;

    bool   isOk() const { return !_error.hasValue() && _value.hasValue(); }
    ErrorT error() const
    {
        ASSERT(!isOk());
        ASSERT(_error.hasValue());
        return _error.value();
    }

    const T& value() const
    {
        ASSERT(_value.hasValue());
        return _value.value();
    }
    T& value()
    {
        ASSERT(_value.hasValue());
        return _value.value();
    }

    T extract()
    {
        ASSERT(isOk());
        _error.reset();
        return std::forward<T>(_value.extract());
    }

   protected:
    Optional<T>      _value;
    Optional<ErrorT> _error;
};

template <typename ErrorT>
struct Result<void, ErrorT>
{
    using value_t      = void;
    using error_t      = ErrorT;
    using error_code_t = typename ErrorT::code_t;

    Result() {}
    Result(error_t e) : _error(e) {}
    Result(error_code_t e) : _error(error_t(e)) {}

    Result(const Result& r) : _error(r._error) {}
    Result(Result&& r) : _error(std::move(r._error)) {}
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&)      = delete;

    bool           isOk() const { return !_error.hasValue(); }
    const error_t& error() const
    {
        ASSERT(_error.hasValue());
        return _error.value();
    }

   protected:
    Optional<error_t> _error;
};

namespace detail
{
template <typename T, typename E = Error<>>
struct ResultHelper
{
    static Result<T, E> make(const T& t) { return Result<T, E>(t); }
    static Result<T, E> make(T&& t) { return Result<T, E>(std::forward<T>(t)); }
};
template <typename E>
struct ResultHelper<void, E>
{
    static Result<void, E> make() { return Result<void, E>(); }
};
}  // namespace detail

template <typename T, typename E>
static Result<T, E> makeResult(T t)
{
    return detail::ResultHelper<T, E>::make(t);
}
template <typename T>
static Result<T, Error<>> makeResult(T t)
{
    return detail::ResultHelper<T, Error<>>::make(std::forward<T>(t));
}

template <typename R>
static R makeResult(typename R::value_t t)
{
    return detail::ResultHelper<typename R::value_t, typename R::error_t>::make(std::forward<typename R::value_t>(t));
}
template <typename R>
static R makeResult()
{
    return detail::ResultHelper<typename R::value_t, typename R::error_t>::make();
}

template <typename ResultT>
static ResultT makeResultError()
{
    return ResultT(typename ResultT::error_t());
}
template <typename ResultT>
static ResultT makeResultError(typename ResultT::error_t::code_t e, const char* msg)
{
    return ResultT(typename ResultT::error_t(e, msg));
}
template <typename ResultT>
static ResultT makeResultError(typename ResultT::error_t::code_t e, const std::string& msg)
{
    return ResultT(typename ResultT::error_t(e, msg));
}
template <typename ResultT>
static ResultT makeResultError(typename ResultT::error_t e)
{
    return ResultT(e);
}
template <typename ResultT>
static ResultT makeResultError(const char* msg)
{
    return ResultT(typename ResultT::error_t(msg));
}
template <typename ResultT>
static ResultT makeResultError(const std::string& msg)
{
    return ResultT(typename ResultT::error_t(msg));
}
template <typename ResultT>
static ResultT makeResultError(std::string&& msg)
{
    return ResultT(typename ResultT::error_t(std::move(msg)));
}

////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RandomAccessContainer
{
    using value_t = T;

    void     resize(size_t size) { _values.resize(size); }
    bool     empty() const { return _values.empty(); }
    size_t   size() const { return _values.size(); }
    const T* data() const { return _values.data(); }
    const T* getValues() const { return _values.data(); }
    const T& getValue(size_t index) const
    {
        ASSERT(index < _values.size());
        return _values[index];
    }

    const T* cbegin() const { return _values.data(); }
    const T* cend() const { return _values.data() + _values.size(); }
    T*       begin() { return _values.data(); }
    T*       end() { return _values.data() + _values.size(); }

    const T& operator[](size_t index) const { return getValue(index); }

    void write(T value) { _values.push_back(value); }

   protected:
    std::vector<T> _values;
};

////////////////////////////////////////////////////////////////////////////////////////////////

namespace utils
{
template <typename T>
bool is_sorted(const T* begin, const T* end)
{
    const T* lastValue = begin;
    for (const T* it = begin + 1; it != end; ++it)
    {
        if (!(*lastValue < *it))
        {
            return false;
        }
    }
    return true;
}
template <typename T>
bool is_sorted_if(const T* begin, const T* end, std::function<bool(const T& a, const T& b)> compareOp)
{
    const T* lastValue = begin;
    for (const T* it = begin + 1; it != end; ++it)
    {
        if (compareOp(*lastValue, *it))
        {
            return false;
        }
    }
    return true;
}

using CharBufferUPtr = std::unique_ptr<char[]>;
Result<CharBufferUPtr> readFile(const char* path, bool binaryMode = true);

inline bool isLittleEndian()
{
    constexpr uint32_t test = 0x01020304;
    return *((uint8_t*)&test) == 0x04;
}

}  // namespace utils

////////////////////////////////////////////////////////////////////////////////
namespace unit_tests
{
namespace common
{
void run();
}  // namespace common
}  // namespace unit_tests
