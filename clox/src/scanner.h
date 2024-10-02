#pragma once

#include <stdio.h>

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

    True,
    False,

    And,
    Not,
    Or,

    Class,
    Super,
    This,

    Null,
    Var,
#if USING(LANG_EXT_MUT)
    Mut,  // extension ( default vars are const )
#endif    // #if USING(LANG_EXT_MUT)

    Else,
    If,
    Print,
    Return,

    While,
    For,

    Func,

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
    const char* linePtr = 0;
    int line = -1;

    result_t init(const char* source)
    {
        start = source;
        current = source;
        line = 0;
        linePtr = source;

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
            case '\"':
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

        return makeResultError<TokenResult_t>(buildMessage("Unexpected character: '%c'", c));
    }

   protected:
    Token makeToken(TokenType type, int ltrim=0, int rtrim=0) const
    {
        Token token;
        token.type = type;
        token.start = start + ltrim;
        token.length = static_cast<int>(current - token.start) - rtrim;
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
        snprintf(message, sizeof(message), "%s at '%.*s' pos:%d in line %d\n", msg, (int)tokenLength, start, pos, line);
        return makeResultError<TokenResult_t>(TokenResult_t::error_t::code_t::SyntaxError, message);
    }

    TokenResult_t string()
    {
        while (peek() != '\"' && !isAtEnd())
        {
            if (peek() == '\n')
            {
                ++line;
                linePtr = this->current + 1;
            }
            advance();
        }
        if (isAtEnd())
        {
            return makeTokenError(TokenType::String, "Unterminated string");
        }

        advance();
        return makeToken(TokenType::String, 1, 1);
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
        const ptrdiff_t startToCurr = this->current - this->start;
        const ptrdiff_t expectedSize = start + length;
        if ((startToCurr == expectedSize) && (0 == memcmp(this->start + start, rest, length)))
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
        struct Keyword
        {
            const char* str;
            int len;
            TokenType type;
        };
#define ADD_KEYWORD(STR, TYPE) {#STR, (int)strlen(#STR), TokenType::TYPE}
        static const Keyword keywords[] = {
            ADD_KEYWORD(class, Class),
            ADD_KEYWORD(else, Else),
            ADD_KEYWORD(exit, Exit),
            ADD_KEYWORD(false, False),
            ADD_KEYWORD(for, For),
            ADD_KEYWORD(if, If),
#if USING(LANG_EXT_MUT)
            ADD_KEYWORD(mut, Mut ),
#endif  // #if USING(LANG_EXT_MUT)
            ADD_KEYWORD(null, Null ),
            ADD_KEYWORD(print, Print ),
            ADD_KEYWORD(return, Return ),
            ADD_KEYWORD(super, Super ),
            ADD_KEYWORD(true, True),
            ADD_KEYWORD(var, Var),
            ADD_KEYWORD(while, While),
            };
#undef ADD_KEYWORD
        ASSERT(utils::is_sorted_if<Keyword>(&keywords[0], &keywords[0] + ARRAY_SIZE(keywords),
                                            [](const Keyword& a, const Keyword& b)
                                            { return memcmp(a.str, b.str, a.len) >= 0; }));
        const char* currentTokenStr = this->start;
        const size_t currentTokenLen = this->current - this->start;
        for (const auto& keyword : keywords)
        {
            if ((keyword.len == currentTokenLen) && 0 == memcmp(keyword.str, currentTokenStr, currentTokenLen))
            {
                return keyword.type;
            }
            if (keyword.str[0] > currentTokenStr[0])
            {  // requires keywords being sorted
                break;
            }
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
                    linePtr = this->current + 1;
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