#pragma once

#include <cstdio>
#include <cstring>

#include "utils/common.h"

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

    Do,
    While,
    For,
    Break,
    Continue,

    Func,

    Exit,

    Comment,

    Error,
    Eof,

    COUNT
};
struct Token
{
    uint32_t    line   = uint32_t(-1);
    uint32_t    length = 0;
    const char* start  = nullptr;
    TokenType   type   = TokenType::Eof;

    static bool equalString(const Token& a, const Token& b);
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
    using result_t      = result_base_t<void>;
    using TokenResult_t = result_base_t<Token>;

    const char*              _start   = 0;
    const char*              _current = 0;
    const char*              _linePtr = 0;
    uint32_t                 _line    = uint32_t(-1);
    std::vector<std::string> _escapedStrings;

    result_t init(const char* source);
    result_t finish();

    TokenResult_t scanToken();

   protected:
    Token         makeToken(TokenType type, int ltrim = 0, int rtrim = 0) const;
    Token         makeToken(TokenType type, const std::string& escapedString) const;
    TokenResult_t makeTokenError(TokenType type, const char* msg, int64_t tokenLength = -1);

    TokenResult_t string();
    Token         number();
    Token         identifier();
    bool          checkKeyword(int start, int length, const char* rest);
    TokenType     checkKeyword(int start, int length, const char* rest, TokenType type);
    TokenType     identifierType();
    inline bool   isDigit(const char c) const { return c >= '0' && c <= '9'; }
    inline bool   isAlpha(const char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'); }
    void          skipWhitespace();
    inline char   peek() { return *_current; }
    inline char   peekNext() { return isAtEnd() ? '\0' : _current[1]; }
    inline char   advance()
    {
        ++_current;
        return _current[-1];
    }
    inline bool match(char expected)
    {
        if (isAtEnd() || *_current != expected)
        {
            return false;
        }
        ++_current;
        return true;
    }
    inline bool isAtEnd() const { return *_current == '\0'; }
};
