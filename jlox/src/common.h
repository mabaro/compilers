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
template<typename ErrCodeT = ErrorCode>
struct Error
{
	using code_t = ErrCodeT;

	Error(const char* msg) : message(msg) {}
	Error(const std::string& msg) : message(msg) {}
	Error(code_t e) : code(e) {}
	Error(code_t e, const std::string& msg) : code(e), message(msg) {}
	
	Error(const Error& e) : code(e.code), message(e.message) {}
	Error(Error&& e) : code(e.code), message(std::move(e.message)) {}
	Error& operator=(Error&& e) = delete;
	Error& operator=(const Error& e) = delete;

	const code_t code = code_t::Undefined;
	const std::string message = "Undefined";
};
template<typename T>
struct Optional
{
	Optional() {}
	Optional(const std::remove_const_t<T>& t) : _value(new T(t)), _hasValue(true) {}
	Optional(T&& t) : _value(new T(std::move(t))), _hasValue(true) {}
	Optional(const Optional& o) : _value(new T(*o._value)), _hasValue(o._hasValue) { assert(!hasValue() || _value); }
	Optional& operator=(const Optional& o) = delete;
	Optional& operator=(Optional&& o) = delete;
	~Optional() {
		delete _value;
		_value = nullptr;
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
			Optional<int> opt(1);
			Optional<int> opt2(opt);
			assert(opt.hasValue());
			//assert(opt2.hasValue());

		}
	}
}
template <typename T, typename ErrorT = Error<>>
struct Result
{
	using value_t = T;
	using error_t = ErrorT;

	Result(T&& t) : _value(std::move(t)) {}
	Result(const T& t) : _value(t) {}
	Result(const ErrorT& e) : _error(e) {}
	Result(typename ErrorT::code_t e) : _error(error_t(e)) {}
	Result(const Result& r) : _value(r._value), _error(r._error) {}
	Result(Result&& r) : _value(std::move(r._value)), _error(std::move(r._error)) {}
	Result& operator=(const Result&) = delete;
	Result& operator=(Result&&) = delete;

	bool isOk() const { return !_error.hasValue(); }
	ErrorT error() const { assert(_error.hasValue()); return _error.value(); }
	const T& value() const { assert(_value.hasValue()); return _value.value(); }
	T& value() { assert(_value.hasValue()); return _value.value(); }

protected:
	Optional<T> _value;
	Optional<ErrorT> _error;
};

template <typename ErrorT>
struct Result<void, ErrorT>
{
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
	const error_t& error() const { assert(_error.hasValue()); return _error.value(); }

protected:
	Optional<error_t> _error;
};

namespace detail
{
	template <typename T, typename E = Error<>>
	struct ResultHelper
	{
		static Result<T, E> make(const T& t) { return Result<T, E>(t); }
		static Result<T, E> make(T&& t) { return Result<T, E>(std::move(t)); }
	};
	template<typename E>
	struct ResultHelper<void, typename E>
	{
		static Result<void, E> make(void) { return Result<void, E>(); }
	};
}

template <typename R> R makeResult(const typename R::value_t& t) {
	return detail::ResultHelper<R::value_t, R::error_t>::make(t);
}
template <typename R> R makeResult() {
	return detail::ResultHelper<void, R::error_t>::make();
}

template <typename ResultT> ResultT makeError() { return ResultT(e); }
template <typename ResultT> ResultT makeError(typename ResultT::error_t::code_t e, const char* msg) { return ResultT(ResultT::error_t(e, msg)); }
template <typename ResultT> ResultT makeError(typename ResultT::error_t::code_t e, const std::string& msg) { return ResultT(ResultT::error_t(e, msg)); }
template <typename ResultT> ResultT makeError(typename ResultT::error_t e) { return ResultT(e); }
template <typename ResultT> ResultT makeError(const char* msg) { return ResultT(ResultT::error_t(msg)); }
template <typename ResultT> ResultT makeError(const std::string& msg) { return ResultT(ResultT::error_t(msg)); }
