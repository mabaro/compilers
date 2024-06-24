#pragma once

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <type_traits>

#include "assert.h"

#define DEBUG_PRINT_CODE 1
#define DEBUG_TRACE_EXECUTION 1

////////////////////////////////////////////////////////////////////////////////

enum class LogLevel
{
    Error = 1 << 0,
    Warning = 1 << 1,
    Info = 1 << 2,
    Debug = 1 << 3,

    All = 0xFF
};
LogLevel sLogLevel = LogLevel::All;

static std::string buildMessage(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buff[1000];
    int len = vsnprintf(buff, 1000, fmt, args);
    va_end(args);
    return std::string(buff);
}

#define LOG_BASE(level, fmt, ...)                 \
    while (1)                                     \
    {                                             \
        FILE* outFile = nullptr;                  \
        switch (level)                            \
        {                                         \
            case LogLevel::Error:                 \
                outFile = stderr;                 \
                fprintf(outFile, "Error: ");      \
                break;                            \
            case LogLevel::Warning:               \
                outFile = stderr;                 \
                fprintf(outFile, "Warning: ");    \
                break;                            \
            case LogLevel::Info:                  \
                outFile = stdout;                 \
                fprintf(outFile, "Info: ");       \
                break;                            \
            case LogLevel::Debug:                 \
                outFile = stdout;                 \
                fprintf(outFile, "Warning: ");    \
                break;                            \
            default:                              \
                break;                            \
        }                                         \
        if (outFile)                              \
        {                                         \
            fprintf(outFile, fmt, ##__VA_ARGS__); \
        }                                         \
        break;                                    \
    }

#define LOG_ERROR(fmt, ...) LOG_BASE(LogLevel::Error, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG_BASE(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG_BASE(LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_BASE(LogLevel::Debug, fmt, ##__VA_ARGS__)

#define DEBUGPRINT(fmt, ...) LOG_BASE(LogLevel::Debug, "DEBUG(line_%d): " #fmt, __LINE__, ##__VA_ARGS__)

enum class ErrorCode
{
    Undefined = 0
};
template <typename ErrCodeT = ErrorCode>
struct Error
{
    using code_t = ErrCodeT;

    Error() {}
    Error(const char* msg) : _message(msg) {}
    Error(const std::string& msg) : _message(msg) {}
    Error(code_t e) : _code(e) {}
    Error(code_t e, const std::string& msg) : _code(e), _message(msg) {}
    Error(code_t e, const char* msg) : _code(e), _message(msg) {}

    Error(const Error& e) : _code(e._code), _message(e._message) {}
    Error(Error&& e) : _code(e._code), _message(std::move(e._message)) {}
    Error& operator=(const Error& e)
    {
        _code = e;
        _message = e.msg;
    }
    Error& operator=(Error&& e)
    {
        _code = std::move(e);
        _message = std::move(e.msg);
    }

    bool operator==(const Error& e) const { return e._code == _code && !e._message.compare(_message); }

    code_t code() const { return _code; }
    const std::string& message() const { return _message; }

   protected:
    const code_t _code = code_t::Undefined;
    const std::string _message = "Undefined";
};

struct ScopedCallback
{
    using callback_t = std::function<void()>;
    ScopedCallback(callback_t callback) : callbackFunc(callback) {}
    ~ScopedCallback() { callbackFunc(); }
    callback_t callbackFunc = nullptr;
};

template <typename T>
struct Optional
{
    Optional() {}
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
    T& value() { return *_value; }
    bool hasValue() const { return _value != nullptr; }
    T&& extract()
    {
        ASSERT(hasValue());
        T temp = std::move(*_value);
        reset();
        return std::move(temp);
    }
    void reset()
    {
        delete _value;
        _value = nullptr;
    }

   protected:
    T* _value = nullptr;
};

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
    Result& operator=(Result&&) = delete;

    bool isOk() const { return !_error.hasValue() && _value.hasValue(); }
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

    T&& extract()
    {
        ASSERT(isOk());
        _error.reset();
        return std::forward<T>(_value.extract());
    }

   protected:
    Optional<T> _value;
    Optional<ErrorT> _error;
};

template <typename ErrorT>
struct Result<void, ErrorT>
{
    using value_t = void;
    using error_t = ErrorT;
    using error_code_t = typename ErrorT::code_t;

    Result() {}
    Result(error_t e) : _error(e) {}
    Result(error_code_t e) : _error(error_t(e)) {}

    Result(const Result& r) : _error(r._error) {}
    Result(Result&& r) : _error(std::move(r._error)) {}
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) = delete;

    bool isOk() const { return !_error.hasValue(); }
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

namespace unit_tests
{
namespace common
{
struct Dummy
{
    int a = -1;
    bool b = false;
};
bool test_result()
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

    return true;
}
bool test_optional()
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
    printf(msg.c_str());
}
}  // namespace common
}  // namespace unit_tests
