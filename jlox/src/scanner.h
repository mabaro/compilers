#pragma once

#include "common.h"

enum class TokenType
{
	LeftParen, RightParen,
	LeftBrace, RightBrace,
	Semicolon, Comma, Dot,
	Minus, Plus, Slash, Star,
	Bang, BangEqual,
	Equal, EqualEqual,
	Less, LessEqual,
	Greater, GreaterEqual,

	String, Identifier,
	Number, NumberFloat,
	Comment,
	Error,
	Eof = 0xff
};
struct Token
{
	int line = -1;
	int length = 0;
	const char* start = nullptr;
	TokenType type = TokenType::Eof;
};

struct Scanner
{
	const char* start = 0;
	const char* current = 0;
	int line = -1;

	Result<void> initScanner(const char* source)
	{
		start = source;
		current = source;
		line = 0;

		return makeResult();
	}

	Result<Token> scanToken()
	{
		Token token;
		start = current;

		skipWhitespace();

		if (isAtEnd())
		{
			return makeToken(TokenType::Eof);
		}

		const char c = advance();
		switch (c) {
		case '(': return makeToken(TokenType::LeftParen);
		case ')': return makeToken(TokenType::RightParen);
		case '{': return makeToken(TokenType::LeftBrace);
		case '}': return makeToken(TokenType::RightBrace);
		case '-': return makeToken(TokenType::Minus);
		case '+': return makeToken(TokenType::Plus);
		case '/':
			if (match('/')) {// comment
				while (peek() != '\n' && !isAtEnd()) { advance(); }
				return makeToken(TokenType::Comment);
			}
			else if (match('*')) {// comment
				while (!(match('*') && match('/')) && !isAtEnd()) { advance(); }
				return makeToken(TokenType::Comment);
			}
			else {
				return makeToken(TokenType::Slash);
			}
		case '*': return makeToken(TokenType::Star);
		case '!': return makeToken(match('=') ? TokenType::BangEqual : TokenType::Bang);
		case '=': return makeToken(match('=') ? TokenType::EqualEqual : TokenType::Equal);
		case '<': return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
		case '>': return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
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

		return makeResult<Token>(Error("Unexpected character."));
	}

protected:
	Token makeToken(TokenType type) const
	{
		Token token;
		token.type = type;
		token.start = start;
		token.length = static_cast<int>(current - start);
		token.line = line;
		return token;
	}
	Token errorToken(const char* message) {
		Token token;
		token.type = TokenType::Error;
		token.start = message;
		token.length = (int)strlen(message);
		token.line = line;
		return token;
	}

	Token string() {
		while (peek() != '"' && !isAtEnd()) {
			if (peek() == '\n') {
				++line;
			}
			advance();
		}
		if (isAtEnd()) {
			return errorToken("Unterminated string");
		}

		advance(); // ending "
		return makeToken(TokenType::String);
	}
	Token number() {
		while (isDigit(peek())) {
			advance();
		}
		if (peek() == '.' && isDigit(peekNext()))
		{
			advance();
			while (isDigit(peek())) {
				advance();
			}
			return makeToken(TokenType::NumberFloat);
		}
		return makeToken(TokenType::Number);
	}
	Token identifier() {
		while (isAlpha(peek()) || isDigit(peek())) {
			advance();
		}
		return makeToken(identifierType());
	}
	TokenType identifierType() {
		return TokenType::Identifier;
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
