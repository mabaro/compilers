#pragma once

#include "scanner.h"
#include "vm.h"
#include "chunk.h"
#include "common.h"
#include <vector>
#include <functional>
#include <unordered_map>

using Number = double;

enum class OperationType : uint8_t {
	Return,

	Constant8,

	Undefined = 255
};

struct Parser
{
	using error_t = Error<>;

	Token current;
	Token previous;

	Optional<error_t> optError;
	bool panicMode = false;
};

struct Compiler
{
	enum class ErrorCode {
		ScannerError,
		Undefined
	};
	using error_t = Error<ErrorCode>;
	using result_t = Result<Chunk*, error_t>;

	result_t compile(const char* source) {
		populateExpressions();

		_scanner.init(source);
		_compilingChunk = new Chunk();

		_parser.optError = Optional<Parser::error_t>();
		_parser.panicMode = false;

		token_result_t tokenResult = advance();
		if (!tokenResult.isOk())
		{
			return makeResultError<result_t>();
		}
		expression();

		consume(TokenType::Eof, "Expected end of expression");
		if (_parser.optError.hasValue()) {
			return makeResultError<result_t>(result_t::error_t::code_t::Undefined, _parser.optError.extract().message());
		}

		finishCompilation();
		return makeResult<result_t>(std::move(currentChunk()));
	}

protected:
	void finishCompilation()
	{
		emitReturn();
	}

	int makeConstant(Number value) {
		assert(currentChunk());
		Chunk& chunk = *currentChunk();

		const int constantId = chunk.addConstant(value);
		if (constantId > UINT8_MAX) {
			error(buildMessage("Max constants per chunk exceeded: %s", UINT8_MAX).c_str());
		}
		return constantId;
	}
	void emitConstant(Number value)
	{
		emitBytes(OpCode::Constant, makeConstant(value));
	}

	int makeVariable() {
		assert(currentChunk());
		Chunk& chunk = *currentChunk();

		const int id = chunk.addVariable(0);
		if (id > UINT8_MAX) {
			error(buildMessage("Max variables per chunk exceeded: %s", UINT8_MAX).c_str());
		}
		return id;
	}
	void emitVariable(const std::string& name)
	{
		int varIndex = -1;
		auto it = _variables.find(name);
		if (it == _variables.end())
		{
			varIndex = makeVariable();
			_variables.insert(std::make_pair(name, varIndex));
		}
		else {
			varIndex = it->second;
		}

		//emitBytes(OpCode::Variable, varIndex);
	}

	void emitReturn() {
		emitBytes((uint8_t)OperationType::Return);
	}

	void emitBytes(uint8_t byte)
	{
		Chunk* chunk = currentChunk();
		assert(chunk);
		chunk->write(byte, _parser.current.line);
	}
	void emitBytes(OpCode code)
	{
		emitBytes((uint8_t)code);
	}
	void emitBytes(int constantId)
	{
		assert(constantId < UINT8_MAX);
		emitBytes((uint8_t)constantId);
	}
	template<typename T, typename... Args>
	void emitBytes(T byte, Args... args)
	{
		emitBytes(byte);
		emitBytes(args...);
	}
protected:
	Parser::error_t errorAtCurrent(const char* errorMsg)
	{
		return errorAt(_parser.current, errorMsg);
	}
	Parser::error_t error(const char* errorMsg)
	{
		return errorAt(_parser.previous, errorMsg);
	}
	Parser::error_t errorAt(const Token& token, const char* errorMsg)
	{
		if (_parser.panicMode)
		{
			return _parser.optError.value();
		}
		_parser.panicMode = true;

		char message[1024];
		if (token.type == TokenType::Eof)
		{
			sprintf_s(message, " at end");
		}
		else if (token.type == TokenType::Error)
		{
			sprintf_s(message, " Error token!!!");
		}
		else {
			sprintf_s(message, " at '%.*s'", token.length, token.start);
		}

		_parser.optError = Parser::error_t(Parser::error_t::code_t::Undefined, buildMessage("[line %d] Error %s: %s\n", _parser.current.line, message, errorMsg));
		return _parser.optError.value();
	}

protected:
	using token_result_t = Result<Token, error_t>;
	using expression_handler_t = std::function<void()>;

	void addExpressionHandler(TokenType type, expression_handler_t&& handler)
	{
		_expressionCallbacks[(int)type] = std::move(handler);
	}
	expression_handler_t getExpressionHandler(TokenType type)
	{
		assert(_expressionCallbacks[(int)type] != nullptr);
		return _expressionCallbacks[(int)type];
	}
	void populateExpressions() {
		addExpressionHandler(TokenType::Number, [&]() {
			double value = strtod(_parser.previous.start, nullptr);
			emitConstant(value);
			});
		addExpressionHandler(TokenType::Identifier, [&]() {
			const std::string name(_parser.current.start, _parser.current.start + _parser.current.length);
			emitVariable(name);
			});
	}

	void expression()
	{
		TokenType token = _parser.current.type;
		switch (token)
		{
		case TokenType::Number:
		case TokenType::Identifier:
			getExpressionHandler(token)();
			break;
		default:
			printf("Unhandled token: %d\n", token);
		}
	}

	void consume(TokenType type, const char* message)
	{
		if (_parser.current.type == type)
		{
			advance();
			return;
		}
		errorAtCurrent(message);
	}

	token_result_t advance() {
		_parser.previous = _parser.current;

		for (;;)
		{
			auto tokenResult = _scanner.scanToken();
			if (!tokenResult.isOk())
			{
				return makeResultError<token_result_t>(buildMessage("Invalid token: %s\n", tokenResult.error().message().c_str()));
			}
			_parser.current = tokenResult.value();
			if (tokenResult.value().type != TokenType::Eof) {
				return makeResult<token_result_t>(tokenResult.extract());
			}

			return makeResultError<token_result_t>(errorAt(_parser.current, "").message());
		}
	}

protected:
	Scanner _scanner;
	Parser _parser;

	Chunk* _compilingChunk = nullptr;
	Chunk* currentChunk() {
		return _compilingChunk;
	}

	expression_handler_t _expressionCallbacks[static_cast<size_t>(TokenType::COUNT)];
	std::unordered_map<std::string, uint16_t> _variables;

};
