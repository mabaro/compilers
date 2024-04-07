#pragma once

#include "common.h"

enum class Token
{
	LeftParen, RightParen,
	LeftBrace, RightBrace,
	Semicolon, Comma, Dot,
	Minus, Plus, Slash, Star,
	Bang, BangEqual,
	Equal, EqualEqual,
	Less, LessEqual,
	Greater, GreaterEqual,

    Not, And, Or,
    Class, Else, If, Nil,
    Print, Return,
    While, For,
	True, False,
	Exit,

	String, Identifier,
	Number, NumberFloat,

	Comment,

	Error,
	Eof = 0xff
};
struct TokenHolder
{
	int line = -1;
	int length = 0;
	const char* start = nullptr;
	Token type = Token::Eof;
};

struct Scanner
{
	enum class ErrorCode {
		SyntaxError,
		Undefined
	};
	using error_t = Error<ErrorCode>;
	template<typename T = void>
	using result_base_t = Result<T, error_t>;
	using result_t = result_base_t<void>;
	using TokenResult_t = result_base_t<TokenHolder>;

	const char* start = 0;
	const char* current = 0;
	int line = -1;

	result_t initScanner(const char* source)
	{
		start = source;
		current = source;
		line = 1;

		return makeResult<result_t>();
	}

	TokenResult_t scanToken()
	{
		TokenHolder token;
		
		skipWhitespace();
		
		start = current;

		if (isAtEnd())
		{
			return makeToken(Token::Eof);
		}

		const char c = advance();
		switch (c) {
		case '(': return makeToken(Token::LeftParen);
		case ')': return makeToken(Token::RightParen);
		case '{': return makeToken(Token::LeftBrace);
		case '}': return makeToken(Token::RightBrace);
		case '-': return makeToken(Token::Minus);
		case '+': return makeToken(Token::Plus);
		case '/':
			if (match('/')) {// comment
				while (peek() != '\n' && !isAtEnd()) { advance(); }
				return makeToken(Token::Comment);
			}
			else if (match('*')) {// comment
				while (!(match('*') && match('/')) && !isAtEnd()) { advance(); }
				return makeToken(Token::Comment);
			}
			else {
				return makeToken(Token::Slash);
			}
		case '*': return makeToken(Token::Star);
		case '!': return makeToken(match('=') ? Token::BangEqual : Token::Bang);
		case '=': return makeToken(match('=') ? Token::EqualEqual : Token::Equal);
		case '<': return makeToken(match('=') ? Token::LessEqual : Token::Less);
		case '>': return makeToken(match('=') ? Token::GreaterEqual : Token::Greater);
		case '"': return string();
		default:
			if (isDigit(c))
			{
				return number();
			}
			if (isAlpha(c))
			{
				return identifier();
			}
			break;
		}

		return makeError<TokenResult_t>("Unexpected character.");
	}

protected:
	TokenHolder makeToken(Token type) const
	{
		TokenHolder token;
		token.type = type;
		token.start = start;
		token.length = static_cast<int>(current - start);
		token.line = line;
		return token;
	}
	TokenResult_t makeTokenError(Token type, const char* msg, int tokenLength = -1) {
		char message[1024];
		const size_t pos = current - start;
		if (tokenLength > 0)
		{
			sprintf(message, "%s at '%.*s' pos:%d in line %d\n", msg, tokenLength, start, pos, line);
		}
		else {
			tokenLength = 0;
			const char* found = strchr(start, '\n');
			if (found != nullptr)
			{
				tokenLength = found - start - 1;
			}
			sprintf(message, "%s at '%.*s' pos:%d in line %d\n", msg, tokenLength, start, pos, line);
		}
		return makeError<TokenResult_t>(TokenResult_t::error_t::code_t::SyntaxError, message);
	}

	TokenResult_t string() {
		while (peek() != '"' && !isAtEnd()) {
			if (peek() == '\n') {
				++line;
			}
			advance();
		}
		if (isAtEnd())
		{
			return makeTokenError(Token::String, "Unterminated string");
		}

		advance();
		return makeToken(Token::String);
	}
	TokenHolder number() {
		while (isDigit(peek())) {
			advance();
		}
		if (peek() == '.' && isDigit(peekNext()))
		{
			advance();
			while (isDigit(peek())) {
				advance();
			}
			return makeToken(Token::NumberFloat);
		}
		return makeToken(Token::Number);
	}
	TokenHolder identifier() {
		while (isAlpha(peek()) || isDigit(peek())) advance();
		return makeToken(identifierType());
	}
	bool checkKeyword(int start, int length,
		const char* rest) {
		if (((int)(this->current - this->start) == (start + length)) &&
			memcmp(this->start + start, rest, length) == 0) {
			return true;
		}

		return false;
	}
	Token checkKeyword(int start, int length,
		const char* rest, Token type) {
		if (checkKeyword(start, length, rest))
		{
			current += length;
			return type;
		}

		return Token::Identifier;
	}
	Token identifierType() {
		switch (start[0]) {
		case '&': return checkKeyword(1, 1, "&", Token::And);
		case '|': return checkKeyword(1, 1, "|", Token::Or);
		case '~': return Token::Not;
		case 'c': return checkKeyword(1, 4, "lass", Token::Class);
		case 'e':
			if (checkKeyword(1, 3, "lse")) return Token::Else;
			else if (checkKeyword(1, 3, "xit")) return Token::Exit;
			break;
		case 'f':
			if (checkKeyword(1, 2, "or")) return Token::For;
			else if (checkKeyword(1, 4, "alse")) return Token::False;
				break;
		case 'i': return checkKeyword(1, 1, "f", Token::If);
		case 'n': return checkKeyword(1, 2, "il", Token::Nil);
		case 'p': return checkKeyword(1, 4, "rint", Token::Print);
		case 'r': return checkKeyword(1, 5, "eturn", Token::Return);
		case 't': return checkKeyword(1, 3, "rue", Token::True);
		case 'w': return checkKeyword(1, 4, "hile", Token::While);
		}
		return Token::Identifier;
	}
	bool isDigit(const char c) const {
		return c >= '0' && c <= '9';
	}
	bool isAlpha(const char c) const {
		return (c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c == '_');
	}
	void skipWhitespace() {
		while (1)
		{
			const char c = peek();
			switch (c) {
			case ' ':
			case '\t':
			case '\r':
				advance();
				break;
			case  '\n':
				++line;
				advance();
				break;
			default: return;
			}
		}
	}
	char peek() { return *current; }
	char peekNext() { return isAtEnd() ? '\0' : current[1]; }
	char advance() {
		++current;
		return current[-1];
	}
	bool match(char expected)
	{
		if (isAtEnd() || *current != expected)
		{
			return false;
		}
		++current;
		return true;
	}
	bool isAtEnd() const {
		return *current == '\0';
	}
};
Scanner sScanner;
