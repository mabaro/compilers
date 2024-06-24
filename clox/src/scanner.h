#pragma once

#include <cstring>

#include "common.h"

enum class TokenType
{
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Semicolon,
    Comma,
    Dot,
    Minus,
    Plus,
    Slash,
    Star,
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    Identifier,
    String,
    Number,
    NumberFloat,

    Not,
    And,
    Or,
    Class,
    Super,
    Null,
    Var,
    This,
    Else,
    If,
    Print,
    Return,
    While,
    For,
    Func,
    True,
    False,
    Exit,

    Comment,

    Error,
    Eof,

    COUNT
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
    enum class ErrorCode
    {
        SyntaxError,
        Undefined
    };
    using error_t = Error<ErrorCode>;
    template <typename T = void>
    using result_base_t = Result<T, error_t>;
    using result_t = result_base_t<void>;
    using TokenResult_t = result_base_t<Token>;

    const char* start = 0;
    const char* current = 0;
    int line = -1;

    result_t init(const char* source)
    {
        start = source;
        current = source;
        line = 0;

        return makeResultError<result_t>();
    }

    TokenResult_t scanToken()
    {
        Token token;

        skipWhitespace();

        start = current;

        if (isAtEnd())
        {
            return makeToken(TokenType::Eof);
        }

        const char c = advance();
        switch (c)
        {
            case '(':
                return makeToken(TokenType::LeftParen);
            case ')':
                return makeToken(TokenType::RightParen);
            case '{':
                return makeToken(TokenType::LeftBrace);
            case '}':
                return makeToken(TokenType::RightBrace);
            case '-':
                return makeToken(TokenType::Minus);
            case '+':
                return makeToken(TokenType::Plus);
            case '/':
                if (match('/'))
                {  // comment
                    while (peek() != '\n' && !isAtEnd())
                    {
                        advance();
                    }
                    return makeToken(TokenType::Comment);
                }
                else if (match('*'))
                {  // comment
                    while (!(match('*') && match('/')) && !isAtEnd())
                    {
                        advance();
                    }
                    return makeToken(TokenType::Comment);
                }
                else
                {
                    return makeToken(TokenType::Slash);
                }
            case '*':
                return makeToken(TokenType::Star);
            case '!':
                return makeToken(match('=') ? TokenType::BangEqual : TokenType::Bang);
            case '=':
                return makeToken(match('=') ? TokenType::EqualEqual : TokenType::Equal);
            case '<':
                return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
            case '>':
                return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
            case '&':
                if (match('&'))
                {
                    return makeToken(TokenType::And);
                }
                break;
            case '"':
                return string();
            case ';':
                return makeToken(TokenType::Semicolon);
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

        return makeResultError<TokenResult_t>("Unexpected character.");
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
    TokenResult_t makeTokenError(TokenType type, const char* msg, int64_t tokenLength = -1)
    {
        char message[1024];
        const int pos = (int)(current - start);
        if (tokenLength <= 0)
        {
            tokenLength = 0;
            const char* found = strchr(start, '\n');
            if (found != nullptr)
            {
                tokenLength = found - start;
            }
        }
        sprintf_s(message, "%s at '%.*s' pos:%d in line %d\n", msg, (int)tokenLength, start, pos, line);
        return makeResultError<TokenResult_t>(TokenResult_t::error_t::code_t::SyntaxError, message);
    }

    TokenResult_t string()
    {
        while (peek() != '"' && !isAtEnd())
        {
            if (peek() == '\n')
            {
                ++line;
            }
            advance();
        }
        if (isAtEnd())
        {
            return makeTokenError(TokenType::String, "Unterminated string");
        }

        advance();
        return makeToken(TokenType::String);
    }
    Token number()
    {
        while (isDigit(peek()))
        {
            advance();
        }
        if (peek() == '.' && isDigit(peekNext()))
        {
            advance();
            while (isDigit(peek()))
            {
                advance();
            }
            return makeToken(TokenType::NumberFloat);
        }
        return makeToken(TokenType::Number);
    }
    Token identifier()
    {
        while (isAlpha(peek()) || isDigit(peek())) advance();
        return makeToken(identifierType());
    }
    bool checkKeyword(int start, int length, const char* rest)
    {
        const ptrdiff_t curToStart = this->current - this->start;
        const ptrdiff_t expectedSize = start + length;
        if (((ptrdiff_t)(this->current - this->start) == (start + length)) &&
            memcmp(this->start + start, rest, length) == 0)
        {
            return true;
        }

        return false;
    }
    TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
    {
        if (checkKeyword(start, length, rest))
        {
            // current += length;
            return type;
        }

        return TokenType::Identifier;
    }
    TokenType identifierType()
    {
        switch (start[0])
        {
            // case 'a': return checkKeyword(1, 2, "nd", TokenType::And);
            // case 'o': return checkKeyword(1, 1, "r", TokenType::Or);
            // case 'n': return checkKeyword(1, 2, "ot", TokenType::Not;
            case 'c':
                return checkKeyword(1, 4, "lass", TokenType::Class);
            case 'e':
                if (checkKeyword(1, 3, "lse"))
                    return TokenType::Else;
                else if (checkKeyword(1, 3, "xit"))
                    return TokenType::Exit;
                break;
            case 'f':
                if (checkKeyword(1, 2, "or"))
                    return TokenType::For;
                else if (checkKeyword(1, 4, "alse"))
                    return TokenType::False;
                break;
            case 'i':
                return checkKeyword(1, 1, "f", TokenType::If);
            case 'n':
                return checkKeyword(1, 3, "ull", TokenType::Null);
            case 'p':
                return checkKeyword(1, 4, "rint", TokenType::Print);
            case 'r':
                return checkKeyword(1, 5, "eturn", TokenType::Return);
            case 't':
                return checkKeyword(1, 3, "rue", TokenType::True);
            case 'w':
                return checkKeyword(1, 4, "hile", TokenType::While);
        }
        return TokenType::Identifier;
    }
    bool isDigit(const char c) const { return c >= '0' && c <= '9'; }
    bool isAlpha(const char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'); }
    void skipWhitespace()
    {
        while (1)
        {
            const char c = peek();
            switch (c)
            {
                case ' ':
                case '\t':
                case '\r':
                    advance();
                    break;
                case '\n':
                    ++line;
                    advance();
                    break;
                default:
                    return;
            }
        }
    }
    char peek() { return *current; }
    char peekNext() { return isAtEnd() ? '\0' : current[1]; }
    char advance()
    {
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
    bool isAtEnd() const { return *current == '\0'; }
};

namespace unit_tests
{
namespace scanner
{
void run()
{
    Token t1{};
    t1.line = 15;
    Token t2;
    auto tokenResult = makeResult<Token>(t1);
    ASSERT(tokenResult.value().line == t1.line);
}
}  // namespace scanner
}  // namespace unit_tests