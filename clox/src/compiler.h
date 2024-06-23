#pragma once

#include "scanner.h"
#include "vm.h"
#include "chunk.h"
#include "common.h"

#if DEBUG_PRINT_CODE
#include "debug.h"
#if 0 // assert on error
#define COMPILER_ASSERT(X) assert(X)
#else
#define COMPILER_ASSERT(X) 
#endif
#if 1 // enable debug print
#define MYPRINT(X,...) printf(X"\n",##__VA_ARGS__)
#else
#define MYPRINT(X,...)
#endif
#endif // #if DEBUG_PRINT_CODE

#include <vector>
#include <functional>
#include <unordered_map>

using Number = double;

enum class OperationType : uint8_t
{
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

enum class Precedence
{
	NONE,
	ASSIGNMENT, // =
	OR,			// or
	AND,		// and
	EQUALITY,	// == !=
	COMPARISON, // < > <= >=
	TERM,		// + -
	FACTOR,		// * /
	UNARY,		// ! -
	CALL,		// . ()
	PRIMARY
};

struct ParseRule
{
	using parse_func_t = std::function<void()>;
	parse_func_t prefix;
	parse_func_t infix;
	Precedence precedence;
};

struct Compiler
{
	enum class ErrorCode
	{
		ScannerError,
		Undefined
	};
	using error_t = Error<ErrorCode>;
	using result_t = Result<Chunk *, error_t>;

	result_t compile(const char *source)
	{
		populateExpressions();

		_scanner.init(source);
		_compilingChunk = new Chunk();

		_parser.optError = Optional<Parser::error_t>();
		_parser.panicMode = false;

		advance();
		while (!getCurrentError().hasValue() && !match(TokenType::Eof))
		{
			expression();
		}

		consume(TokenType::Eof, "Expected end of expression");
		if (getCurrentError().hasValue())
		{
			return makeResultError<result_t>(result_t::error_t::code_t::Undefined, getCurrentError().value().message());
		}

		finishCompilation();
		return makeResult<result_t>(std::move(currentChunk()));
	}

protected: // high level stuff
	void expression()
	{
		MYPRINT("expression: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);
		_lastExpressionLine = _parser.current.line;
		parsePrecedence(Precedence::ASSIGNMENT);
	}
	void skip()
	{
		MYPRINT("skip: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);
		// nothing to do here
	}
	void grouping()
	{
		MYPRINT("grouping: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);
		expression();
		consume(TokenType::RightParen, "Expected ')' after expression.");
	}
	void literal()
	{
		MYPRINT("literal: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);

		switch (_parser.previous.type)
		{
			case TokenType::Null:  emitBytes(OpCode::Null); break;
			case TokenType::False: emitBytes(OpCode::False); break;
			case TokenType::True:  emitBytes(OpCode::True); break;
			default: assert(false); return;
		}
	}
	void number()
	{
		MYPRINT("number: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);

		Value value = Value::CreateValue(strtod(_parser.previous.start, nullptr));
		emitConstant(value);
	}
	void unary()
	{
		MYPRINT("unary: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);
		const TokenType operatorType = _parser.previous.type;

		// parse expression
		parsePrecedence(Precedence::UNARY);

		// Emit the operator instruction.
		switch (operatorType)
		{
		case TokenType::Minus:
			emitBytes(OpCode::Negate);
			break;
		case TokenType::Bang:
			emitBytes(OpCode::Not);
			break;
		default:
			return;
		}
	}
	void binary()
	{
		MYPRINT("binary: prev[%s] cur[%s]", _parser.previous.start, _parser.current.start);

		const TokenType operatorType = _parser.previous.type;
		const ParseRule parseRule = getParseRule(operatorType);
		// left-associative: 1+2+3+4 = ((1 + 2) + 3) + 4
		// right-associative: a=b=c=d -> a = (b = (c = d))
		parsePrecedence(Precedence((int)parseRule.precedence + 1));

		switch (operatorType)
		{
		case TokenType::Plus:  emitBytes(OpCode::Add); break;
		case TokenType::Minus: emitBytes(OpCode::Subtract); break;
		case TokenType::Star:  emitBytes(OpCode::Multiply); break;
		case TokenType::Slash: emitBytes(OpCode::Divide); break;

		case TokenType::BangEqual:    emitBytes(OpCode::Equal, OpCode::Not); break;
		case TokenType::EqualEqual:   emitBytes(OpCode::Equal); break;
		case TokenType::Greater:      emitBytes(OpCode::Greater); break;
		case TokenType::GreaterEqual: emitBytes(OpCode::Equal, OpCode::Not); break;
		case TokenType::Less:         emitBytes(OpCode::Less); break;
		case TokenType::LessEqual:    emitBytes(OpCode::Greater, OpCode::Not); break;
		default:
			return; // Unreachable.
		}
	}

	const ParseRule &getParseRule(TokenType type) const
	{
		return _parseRules[(size_t)type];
	}
	void parsePrecedence(Precedence precedence)
	{
		advance();

		const ParseRule& parseRule = getParseRule(_parser.previous.type);
		if (parseRule.prefix == NULL)
		{
			error("Expect expression.");
			return;
		}
		parseRule.prefix();

		while (precedence <= getParseRule(_parser.current.type).precedence)
		{
			advance();
			ParseRule::parse_func_t infixRule = getParseRule(_parser.previous.type).infix;
			infixRule();
		}
	}

protected:
	void finishCompilation()
	{
		emitReturn();
#if DEBUG_PRINT_CODE
		if (!_parser.optError.hasValue())
		{
			assert(currentChunk());
			disassemble(*currentChunk(), "code");
		}
#endif // #if DEBUG_PRINT_CODE
	}

	int makeConstant(const Value & value)
	{
		assert(currentChunk());
		Chunk& chunk = *currentChunk();

		const int constantId = chunk.addConstant(value);
		if (constantId > UINT8_MAX)
		{
			error(buildMessage("Max constants per chunk exceeded: %s", UINT8_MAX).c_str());
		}
		return constantId;
	}
	void emitConstant(const Value& value)
	{
		emitBytes(OpCode::Constant, makeConstant(value));
	}

	int makeVariable()
	{
		assert(currentChunk());
		Chunk &chunk = *currentChunk();

		const int id = chunk.addVariable(Value::CreateValue());
		if (id > UINT8_MAX)
		{
			error(buildMessage("Max variables per chunk exceeded: %s", UINT8_MAX).c_str());
		}
		return id;
	}
	void emitVariable(const std::string &name)
	{
		int varIndex = -1;
		auto it = _variables.find(name);
		if (it == _variables.end())
		{
			varIndex = makeVariable();
			_variables.insert(std::make_pair(name, varIndex));
		}
		else
		{
			varIndex = it->second;
		}

		// emitBytes(OpCode::Variable, varIndex);
	}

	void emitReturn()
	{
		emitBytes((uint8_t)OperationType::Return);
	}

	void emitBytes(uint8_t byte)
	{
		Chunk *chunk = currentChunk();
		assert(chunk);
		chunk->write(byte, _lastExpressionLine);
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
	template <typename T, typename... Args>
	void emitBytes(T byte, Args... args)
	{
		emitBytes(byte);
		emitBytes(args...);
	}

protected:
	Parser::error_t errorAtCurrent(const char *errorMsg)
	{
		return errorAt(_parser.current, errorMsg);
	}
	Parser::error_t error(const char *errorMsg)
	{
		return errorAt(_parser.previous, errorMsg);
	}
	Parser::error_t errorAt(const Token &token, const char *errorMsg)
	{
		COMPILER_ASSERT(false);
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
		else
		{
			sprintf_s(message, " at '%.*s'", token.length, token.start);
		}

		_parser.optError = Parser::error_t(Parser::error_t::code_t::Undefined, buildMessage("[line %d] Error %s: %s", _parser.current.line, message, errorMsg));
		return _parser.optError.value();
	}

protected:
	using token_result_t = Result<Token, error_t>;
	using expression_handler_t = std::function<void()>;

	void populateExpressions()
	{
		auto binaryFunc = [&]{ binary(); };
		auto groupingFunc = [&]{ grouping(); };
		auto literalFunc = [&]{ literal(); };
		auto numberFunc = [&]{ number(); };
		auto skipFunc = [&]{ skip(); };
		auto unaryFunc = [&]{ unary(); };

		_parseRules[(size_t)TokenType::LeftParen] = {groupingFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::RightParen] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::LeftBrace] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::RightBrace] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Semicolon] = {skipFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Comma] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Dot] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Minus] = {unaryFunc, binaryFunc, Precedence::TERM};
		_parseRules[(size_t)TokenType::Plus] = {NULL, binaryFunc, Precedence::TERM};
		_parseRules[(size_t)TokenType::Slash] = {NULL, binaryFunc, Precedence::FACTOR};
		_parseRules[(size_t)TokenType::Star] = {NULL, binaryFunc, Precedence::FACTOR};
		_parseRules[(size_t)TokenType::Bang] = {unaryFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::BangEqual] = {NULL, binaryFunc, Precedence::EQUALITY};
		_parseRules[(size_t)TokenType::Equal] = {NULL, binaryFunc, Precedence::EQUALITY};
		_parseRules[(size_t)TokenType::EqualEqual] = {NULL, binaryFunc, Precedence::COMPARISON};
		_parseRules[(size_t)TokenType::Greater] = {NULL, binaryFunc, Precedence::COMPARISON};
		_parseRules[(size_t)TokenType::GreaterEqual] = {NULL, binaryFunc, Precedence::COMPARISON};
		_parseRules[(size_t)TokenType::Less] = {NULL, binaryFunc, Precedence::COMPARISON};
		_parseRules[(size_t)TokenType::LessEqual] = {NULL, binaryFunc, Precedence::COMPARISON};
		_parseRules[(size_t)TokenType::Identifier] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::String] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Number] = ParseRule{numberFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::And] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Or] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Class] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Super] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Null] = {literalFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::True] = {literalFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::False] = {literalFunc, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Var] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::This] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Else] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::If] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Print] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Return] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::While] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::For] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Func] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Error] = {NULL, NULL, Precedence::NONE};
		_parseRules[(size_t)TokenType::Eof] = {NULL, NULL, Precedence::NONE};
	}

	const Optional<Parser::error_t> &getCurrentError() const
	{
		return _parser.optError;
	}
	const Token &getCurrentToken() const
	{
		return _parser.current;
	}

	bool match(TokenType type) const
	{
		return getCurrentToken().type == type;
	}

	void consume(TokenType type, const char *message)
	{
		if (_parser.current.type == type)
		{
			advance();
			return;
		}
		errorAtCurrent(message);
	}

	void advance()
	{
		_parser.previous = _parser.current;

		for (;;)
		{
			auto tokenResult = _scanner.scanToken();
			assert(tokenResult.isOk());

			_parser.current = tokenResult.value();
			if (tokenResult.value().type != TokenType::Error)
			{
				break;
			}

			errorAt(_parser.current, "").message();
		}
	}

protected:
	Scanner _scanner;
	Parser _parser;
	int _lastExpressionLine = -1;

	Chunk *_compilingChunk = nullptr;
	Chunk *currentChunk()
	{
		return _compilingChunk;
	}

	ParseRule _parseRules[(size_t)TokenType::COUNT];

	std::unordered_map<std::string, uint16_t> _variables;
};
