#pragma once

#include "scanner.h"
#include "vm.h"
#include "chunk.h"
#include "common.h"
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

enum class Precedence {
	NONE,
	ASSIGNMENT,  // =
	OR,          // or
	AND,         // and
	EQUALITY,    // == !=
	COMPARISON,  // < > <= >=
	TERM,        // + -
	FACTOR,      // * /
	UNARY,       // ! -
	CALL,        // . ()
	PRIMARY
};

struct ParseRule {
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
		while ( !getCurrentError().hasValue() && !match(TokenType::Eof))
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
		parsePrecedence(Precedence::ASSIGNMENT);

		//const TokenType token = _parser.current.type;
		//const ParseRule& parseRule = _parseRules[(size_t)token];
		//switch (token)
		//{
		//case TokenType::LeftParen:
		//	grouping();
		//	break;
		//case TokenType::Minus:
		//case TokenType::Bang:
		//	unary();
		//	break;
		//case TokenType::Number:
		//case TokenType::Identifier:
		//	break;
		//default:
		//	printf("Unhandled token: %d\n", token);
		//}
	}
	void skip() {
		// nothing to do here
	}
	void grouping(){
		expression();
		consume(TokenType::RightParen, "Expected ')' after expression.");
	}
	void number() {
		double value = strtod(_parser.previous.start, nullptr);
		emitConstant(value);
	}
	void unary() {
		const TokenType operatorType = _parser.previous.type;

		parsePrecedence(Precedence::UNARY);

		// Compile the operand.
		expression();

		// Emit the operator instruction.
		switch (operatorType) {
		case TokenType::Minus:
			emitBytes(OpCode::Negate);
			break;
		case TokenType::Bang:
			emitBytes(OpCode::NegateBool);
			break;
		default:
			return; 
		}
	}
	void binary() {
		TokenType operatorType = _parser.previous.type;
		ParseRule parseRule = getParseRule(operatorType);
		parsePrecedence(Precedence((int)parseRule.precedence + 1));

		switch (operatorType) {
		case TokenType::Plus:    emitBytes(OpCode::Add); break;
		case TokenType::Minus: emitBytes(OpCode::Subtract); break;
		case TokenType::Star:  emitBytes(OpCode::Multiply); break;
		case TokenType::Slash: emitBytes(OpCode::Divide); break;
		default: return; // Unreachable.
		}
	}

	ParseRule getParseRule(TokenType type) const {
		return _parseRules[(size_t)type];
	}
	void parsePrecedence(Precedence precedence) {
		advance();

		ParseRule::parse_func_t prefixRule = getParseRule(_parser.previous.type).prefix;
		if (prefixRule == NULL) {
			error("Expect expression.");
			return;
		}

		prefixRule();

		while (precedence <= getParseRule(_parser.current.type).precedence) {
			advance();
			ParseRule::parse_func_t infixRule = getParseRule(_parser.previous.type).infix;
			infixRule();
		}
	}

protected:
	void finishCompilation()
	{
		emitReturn();
	}

	int makeConstant(Number value)
	{
		assert(currentChunk());
		Chunk &chunk = *currentChunk();

		const int constantId = chunk.addConstant(value);
		if (constantId > UINT8_MAX)
		{
			error(buildMessage("Max constants per chunk exceeded: %s", UINT8_MAX).c_str());
		}
		return constantId;
	}
	void emitConstant(Number value)
	{
		emitBytes(OpCode::Constant, makeConstant(value));
	}

	int makeVariable()
	{
		assert(currentChunk());
		Chunk &chunk = *currentChunk();

		const int id = chunk.addVariable(0);
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

		_parser.optError = Parser::error_t(Parser::error_t::code_t::Undefined, buildMessage("[line %d] Error %s: %s\n", _parser.current.line, message, errorMsg));
		return _parser.optError.value();
	}

protected:
	using token_result_t = Result<Token, error_t>;
	using expression_handler_t = std::function<void()>;

	void populateExpressions()
	{
		{
			_parseRules[(size_t)TokenType::LeftParen] = { [&] { grouping(); }, NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::RightParen] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::LeftBrace] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::RightBrace] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Comma] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Dot] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Minus] = { [&] {unary(); },    [&] {binary(); }, Precedence::TERM };
			_parseRules[(size_t)TokenType::Plus] = { NULL,     [&] {binary(); }, Precedence::TERM };
			_parseRules[(size_t)TokenType::Semicolon] = { [&] {skip(); } ,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Slash] = { NULL,     [&] {binary(); }, Precedence::FACTOR };
			_parseRules[(size_t)TokenType::Star] = { NULL,     [&] {binary(); }, Precedence::FACTOR };
			_parseRules[(size_t)TokenType::Bang] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::BangEqual] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Equal] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::EqualEqual] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Greater] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::GreaterEqual] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Less] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::LessEqual] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Identifier] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::String] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Number] = ParseRule{ [&] {number(); },   NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::And] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Class] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Else] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::False] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::For] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Func] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::If] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Nil] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Or] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Print] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Return] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Super] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::This] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::True] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Var] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::While] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Error] = { NULL,     NULL,   Precedence::NONE };
			_parseRules[(size_t)TokenType::Eof] = { NULL,     NULL,   Precedence::NONE };
		}
	}

	const Optional<Parser::error_t>& getCurrentError() const {
		return _parser.optError;
	}
	const Token& getCurrentToken() const {
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

	Chunk *_compilingChunk = nullptr;
	Chunk *currentChunk()
	{
		return _compilingChunk;
	}

	ParseRule _parseRules[(size_t)TokenType::COUNT];
	
	std::unordered_map<std::string, uint16_t> _variables;
};
