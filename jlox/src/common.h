#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <cassert>

enum class LogLevel
{
	Error = 1 << 0,
	Warning = 1 << 1,
	Info = 1 << 2,
	Debug = 1 << 3,

	All = 0xFF
};
LogLevel sLogLevel = LogLevel::All;

#define LOG_BASE(level, fmt, ...)             \
    while (1)            \
    {                                         \
        FILE *outFile = nullptr;               \
        switch (level)                        \
        {                                     \
        case LogLevel::Error:                 \
            outFile = stderr;                 \
            break;                            \
        case LogLevel::Warning:               \
            outFile = stderr;                 \
            break;                            \
        case LogLevel::Info:                  \
            outFile = stdout;                 \
            break;                            \
        case LogLevel::Debug:                 \
            outFile = stdout;                 \
            break;                            \
        default:                              \
            break;                            \
        }                                     \
        if (outFile){fprintf(outFile, fmt, ##__VA_ARGS__);} \
        break;                                \
    }

#define LOG_ERROR(fmt, ...) LOG_BASE(LogLevel::Error, "ERROR: " #fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG_BASE(LogLevel::Warning, "WARNING: " #fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG_BASE(LogLevel::Info, "INFO: " #fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_BASE(LogLevel::Debug, "DEBUG: " #fmt,  ##__VA_ARGS__)

enum class ErrorCode
{
	Undefined = 0
};
struct Error
{
	Error(const std::string& msg) : message(msg) {}
	Error(ErrorCode e) : code(e) {}
	Error(ErrorCode e, const std::string& msg) : code(e), message(msg) {}

	const ErrorCode code = ErrorCode::Undefined;
	const std::string message = "Undefined";
};
template<typename T>
struct Optional
{
	Optional() {}
	Optional(const std::remove_const_t<T>& t) : _value(new T(t)), _hasValue(true) {}
	Optional(T&& t) : _value(new T(std::move(t))), _hasValue(true) {}
	~Optional() {
		if (hasValue())
		{
			delete _value;
			_value= nullptr;
		}
	}

	const T& value() const { assert(hasValue()); return *_value; }
	T& value() { return *_value; }
	bool hasValue() const { return _hasValue; }
	T&& extract() { auto result = std::move(_value); _value = nullptr; return std::move(result); }

protected:
	T* _value = nullptr;
	bool _hasValue = false;
};
namespace common
{
	namespace unit_tests
	{
		void run()
		{
			Optional<int> opt;
		}
	}
}
template <class T>
struct Result
{
	Result(Error e) : _error(e) {}
	Result(ErrorCode e) : _error(Error(e)) {}
	Result(T&& t) : _result(std::move(t)) {}
	Result(const T& t) : _result(t) {}

	bool isOk() const { return !_error.hasValue(); }
	Error error() const { assert(_error.hasValue()); return _error.value(); }
	const T& value() const { assert(_result.hasValue()); return _result.value(); }
	T& value() { assert(_result.hasValue()); return _result.value(); }

protected:
	Optional<T> _result;
	Optional<Error> _error;
};

template <>
struct Result<void>
{
	Result() {}
	Result(Error e) : _error(e) {}
	Result(ErrorCode e) : _error(Error(e)) {}

	bool isOk() const { return !_error.hasValue(); }
	const Error& error() const { assert(_error.hasValue()); return _error.value(); }

protected:
	Optional<Error> _error;
};

template <typename T>
Result<T> makeResult(T&& t) { return Result<T>(std::move(t)); }
template <typename T>
Result<T> makeResult(const T& t) { return Result<T>(t); }
Result<void> makeResult() { return Result<void>(); }
template <typename T>
Result<T> makeResult(Error e) { return Result<T>(e); }
