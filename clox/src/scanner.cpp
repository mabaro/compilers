#include "scanner.h"

#include <sstream>

Scanner::result_t Scanner::init(const char* source)
{
    ASSERT_MSG(_line == -1, "Need to call finish() before init()");
    _start   = source;
    _current = source;
    _line    = 0;
    _linePtr = source;

    return makeResultError<result_t>();
}
Scanner::result_t Scanner::finish()
{
    _line = uint32_t(-1);
    _escapedStrings.clear();

    return makeResultError<result_t>();
}

Scanner::TokenResult_t Scanner::scanToken()
{
    Token token;

    skipWhitespace();

    _start = _current;

    if (isAtEnd())
    {
        return makeToken(TokenType::Eof);
    }

    const char c = advance();
    switch (c)
    {
        case '(': return makeToken(TokenType::LeftParen);
        case ')': return makeToken(TokenType::RightParen);
        case '{': return makeToken(TokenType::LeftBrace);
        case '}': return makeToken(TokenType::RightBrace);
        case '-': return makeToken(TokenType::Minus);
        case '+': return makeToken(TokenType::Plus);
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
        case '*': return makeToken(TokenType::Star);
        case '!': return makeToken(match('=') ? TokenType::BangEqual : TokenType::Bang);
        case '=': return makeToken(match('=') ? TokenType::EqualEqual : TokenType::Equal);
        case '<': return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
        case '>': return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
        case '&':
            if (match('&'))
            {
                return makeToken(TokenType::And);
            }
            break;
        case '\"': return string();
        case ';': return makeToken(TokenType::Semicolon);
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

Token Scanner::makeToken(TokenType type, int ltrim, int rtrim) const
{
    Token token;
    token.type   = type;
    token.start  = _start + ltrim;
    token.length = static_cast<int>(_current - token.start) - rtrim;
    token.line   = _line;
    return token;
}
Token Scanner::makeToken(TokenType type, const std::string& escapedString) const
{
    Token token;
    token.type   = type;
    token.start  = escapedString.data();
    token.length = static_cast<int>(escapedString.size());
    token.line   = _line;
    return token;
}
Scanner::TokenResult_t Scanner::makeTokenError(TokenType /*type*/, const char* msg, int64_t tokenLength)
{
    char      message[1024];
    const int pos = (int)(_current - _start);
    if (tokenLength <= 0)
    {
        tokenLength       = 0;
        const char* found = strchr(_start, '\n');
        if (found != nullptr)
        {
            tokenLength = found - _start;
        }
    }
    snprintf(message, sizeof(message), "%s at '%.*s' pos:%d in line %d\n", msg, (int)tokenLength, _start, pos, _line);
    return makeResultError<TokenResult_t>(TokenResult_t::error_t::code_t::SyntaxError, message);
}

Scanner::TokenResult_t Scanner::string()
{
    bool hasEscapedChars = false;
    bool prevWasEscape   = false;
    while ((prevWasEscape || peek() != '\"') && !isAtEnd())
    {
        const char c    = peek();
        prevWasEscape   = c == '\\';
        hasEscapedChars = hasEscapedChars || c == '\\';
        if (c == '\n')
        {
            ++_line;
            _linePtr = this->_current + 1;
        }
        advance();
    }
    if (isAtEnd())
    {
        return makeTokenError(TokenType::String, "Unterminated string");
    }
    advance();

    if (hasEscapedChars)
    {
        std::ostringstream ostr;
        const char*        ptr    = _start + 1;
        const char*        endPtr = _current - 1;
        while (ptr != endPtr)
        {
            if (*ptr != '\\')
            {
                ostr << *ptr;
            }
            else
            {
                ++ptr;
                switch (*ptr)
                {
                    case '\\': ostr << '\\'; break;
                    case '0': ostr << '\0'; break;
                    case '\"': ostr << '\"'; break;
                    case 'b': ostr << '\b'; break;
                    case 'f': ostr << '\f'; break;
                    case 'n': ostr << '\n'; break;
                    case 'r': ostr << '\r'; break;
                    case 't': ostr << '\t'; break;
                    default: FAIL_MSG("Unsupported escaped character '%c'\n", *ptr);
                }
            }
            ++ptr;
        }
        _escapedStrings.push_back(ostr.str());
        return makeToken(TokenType::String, _escapedStrings.back());
    }

    return makeToken(TokenType::String, 1, 1);
}
Token Scanner::number()
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
Token Scanner::identifier()
{
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}
bool Scanner::checkKeyword(int start, int length, const char* rest)
{
    const ptrdiff_t startToCurr  = this->_current - this->_start;
    const ptrdiff_t expectedSize = start + length;
    if ((startToCurr == expectedSize) && (0 == memcmp(this->_start + start, rest, length)))
    {
        return true;
    }

    return false;
}
TokenType Scanner::checkKeyword(int start, int length, const char* rest, TokenType type)
{
    if (checkKeyword(start, length, rest))
    {
        // current += length;
        return type;
    }

    return TokenType::Identifier;
}
TokenType Scanner::identifierType()
{
    struct Keyword
    {
        const char* str;
        int         len;
        TokenType   type;
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
        const char*  currentTokenStr = this->_start;
        const size_t currentTokenLen = this->_current - this->_start;
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
void Scanner::skipWhitespace()
{
    while (1)
    {
        const char c = peek();
        switch (c)
        {
            case ' ':
            case '\t':
            case '\r': advance(); break;
            case '\n':
                ++_line;
                _linePtr = this->_current + 1;
                advance();
                break;
            default: return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

namespace unit_tests
{
namespace scanner
{
void run()
{
    Token t1{};
    t1.line = 15;
    Token t2;
    auto  tokenResult = makeResult<Token>(t1);
    ASSERT(tokenResult.value().line == t1.line);
}
}  // namespace scanner
}  // namespace unit_tests