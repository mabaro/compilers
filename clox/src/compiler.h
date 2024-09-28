#pragma once

#include "chunk.h"
#include "common.h"
#include "scanner.h"
#include "vm.h"

#if DEBUG_PRINT_CODE
#include "debug.h"
#define MYPRINT(X, ...) printf("[%s] " X "\n", __FUNCTION__, ##__VA_ARGS__)
#define MYPRINT_T()                                                                                         \
    MYPRINT("CUR[%.*s] NXT[%.*s]", _parser.previous.length, _parser.previous.start, _parser.current.length, \
            _parser.current.start);
#else
#define MYPRINT(X, ...)
#define MYPRINT_T()
#endif  // #if DEBUG_PRINT_CODE

#include <functional>
#include <unordered_map>
#include <vector>

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

    result_t compile(const char *source, const char *sourcePath)
    {
        populateParseRules();

        _scanner.init(source);
        _compilingChunk = std::make_unique<Chunk>(sourcePath);

        _parser.optError = Optional<Parser::error_t>();
        _parser.panicMode = false;

        advance();
        while (!getCurrentError().hasValue() && !match(TokenType::Eof))
        {
            declaration();
        }

        consume(TokenType::Eof, "Expected end of expression");
        if (getCurrentError().hasValue())
        {
            return makeResultError<result_t>(result_t::error_t::code_t::Undefined, getCurrentError().value().message());
        }

        finishCompilation();
        return makeResult<result_t>(std::move(currentChunk()));
    }

   protected:  // high level stuff
    void declaration()
    {
        if (match(TokenType::Var))
        {
            variableDeclaration();
        }
        else
        {
            statement();
        }

        if (_parser.panicMode)
        {
            synchronize();
        }
    }
    void statement()
    {
        if (match(TokenType::Print))
        {
            printStatement();
        }
        else
        {
            expressionStatement();
        }
    }
    void printStatement()
    {
        expression();
        consume(TokenType::Semicolon, "Expect ';' after value");
        MYPRINT_T();
        emitBytes(OpCode::Print);
    }
    void expressionStatement()
    {
        expression();
        consume(TokenType::Semicolon, "Expected ';' after expression.");
        emitBytes(OpCode::Pop);
    }

    /////////////////////////////////////////////////////////////////////////////////
   protected:
    void expression()
    {
        MYPRINT_T();
        _lastExpressionLine = _parser.current.line;
        parsePrecedence(Precedence::ASSIGNMENT);
    }
    void skip()
    {
        MYPRINT_T();
        // nothing to do here
    }
    void grouping()
    {
        MYPRINT_T();
        expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");
    }
    void variable()
    {
        MYPRINT_T();
        namedVariable(_parser.previous);
    }
    void namedVariable(const Token &name)
    {
        MYPRINT_T();
        const uint8_t varId = identifierConstant(name);
        if (match(TokenType::Equal))
        {
            expression();
            emitBytes(OpCode::GlobalVarSet, varId);
        }
        else
        {
            emitBytes(OpCode::GlobalVarGet, varId);
        }
    }
    void literal()
    {
        MYPRINT_T();

        switch (_parser.previous.type)
        {
            case TokenType::Null:
                emitBytes(OpCode::Null);
                break;
            case TokenType::False:
                emitBytes(OpCode::False);
                break;
            case TokenType::True:
                emitBytes(OpCode::True);
                break;
            default:
                ASSERT(false);
                return;
        }
    }
    void number()
    {
        MYPRINT_T();

        Value value = Value::Create(strtod(_parser.previous.start, nullptr));
        emitConstant(value);
    }
    void string() { emitConstant(Value::CreateByCopy(_parser.previous.start + 1, _parser.previous.length - 1)); }
    void unary()
    {
        MYPRINT_T();
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
        MYPRINT_T();

        const TokenType operatorType = _parser.previous.type;
        const ParseRule parseRule = getParseRule(operatorType);
        // left-associative: 1+2+3+4 = ((1 + 2) + 3) + 4
        // right-associative: a=b=c=d -> a = (b = (c = d))
        parsePrecedence(Precedence((int)parseRule.precedence + 1));

        switch (operatorType)
        {
            case TokenType::Plus:
                emitBytes(OpCode::Add);
                break;
            case TokenType::Minus:
                emitBytes(OpCode::Subtract);
                break;
            case TokenType::Star:
                emitBytes(OpCode::Multiply);
                break;
            case TokenType::Slash:
                emitBytes(OpCode::Divide);
                break;

            case TokenType::BangEqual:
                emitBytes(OpCode::Equal, OpCode::Not);
                break;
            case TokenType::EqualEqual:
                emitBytes(OpCode::Equal);
                break;
            case TokenType::Greater:
                emitBytes(OpCode::Greater);
                break;
            case TokenType::GreaterEqual:
                emitBytes(OpCode::Equal, OpCode::Not);
                break;
            case TokenType::Less:
                emitBytes(OpCode::Less);
                break;
            case TokenType::LessEqual:
                emitBytes(OpCode::Greater, OpCode::Not);
                break;
            case TokenType::Equal:
                emitBytes(OpCode::Assignment);
                break;
            default:
                FAIL_MSG("Unreachable symbol while parsing [%.*s]", _parser.previous.length, _parser.previous.start);
                return;  // Unreachable.
        }
    }
    void variableDeclaration()
    {
        MYPRINT_T();

        const uint8_t varId = parseVariable("Expected variable name.");
        if (match(TokenType::Equal))
        {
            expression();
        }
        else
        {
            emitBytes(OpCode::Null);
        }
        consume(TokenType::Semicolon, "Expected ';' after variable declaration.");

        defineVariable(varId);
    }
    const ParseRule &getParseRule(TokenType type) const { return _parseRules[(size_t)type]; }
    void parsePrecedence(Precedence precedence)
    {
        advance();

        const ParseRule &parseRule = getParseRule(_parser.previous.type);
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

    uint8_t parseVariable(const char *errorMessage)
    {
        consume(TokenType::Identifier, errorMessage);
        return identifierConstant(_parser.previous);
    }
    void defineVariable(uint8_t id) { emitBytes(OpCode::GlobalVarDef, id); }
    uint8_t identifierConstant(const Token &token)
    {
        MYPRINT_T();
        return makeConstant(Value::CreateByCopy(token.start, token.length));
    }

   protected:
    void finishCompilation()
    {
        emitReturn();
#if DEBUG_PRINT_CODE
        if (!_parser.optError.hasValue())
        {
            ASSERT(currentChunk());
            disassemble(*currentChunk(), "code");
        }
#endif  // #if DEBUG_PRINT_CODE
    }

    int makeConstant(const Value &value)
    {
        ASSERT(currentChunk());
        Chunk &chunk = *currentChunk();

        const int constantId = chunk.addConstant(value);
        if (constantId > UINT8_MAX)
        {
            error(buildMessage("Max constants per chunk exceeded: %s", UINT8_MAX).c_str());
        }
#if USING(DEBUG_TRACE_EXECUTION)
        chunk.printConstants();
#endif  // #if USING(DEBUG_PRINT_CODE)
        return constantId;
    }
    void emitConstant(const Value &value) { emitBytes(OpCode::Constant, makeConstant(value)); }

    void emitReturn() { emitBytes((uint8_t)OperationType::Return); }

    void emitBytes(uint8_t byte)
    {
        Chunk *chunk = currentChunk();
        ASSERT(chunk);
        chunk->write(byte, _lastExpressionLine);
    }
    void emitBytes(OpCode code)
    {
        MYPRINT_T();
        MYPRINT("emitOp: %s", named_enum::name(code));
        emitBytes((uint8_t)code);
    }
    void emitBytes(int constantId)
    {
        ASSERT(constantId < UINT8_MAX);
        emitBytes((uint8_t)constantId);
    }
    template <typename T, typename... Args>
    void emitBytes(T byte, Args... args)
    {
        emitBytes(byte);
        emitBytes(args...);
    }

   protected:
    Parser::error_t errorAtCurrent(const char *errorMsg) { return errorAt(_parser.current, errorMsg); }
    Parser::error_t error(const char *errorMsg) { return errorAt(_parser.previous, errorMsg); }
    Parser::error_t errorAt(const Token &token, const char *errorMsg)
    {
        FAIL_MSG(errorMsg);
        if (_parser.panicMode)
        {
            return _parser.optError.value();
        }
        _parser.panicMode = true;

        char message[1024];
        if (token.type == TokenType::Eof)
        {
            snprintf(message, sizeof(message), " at end");
        }
        else if (token.type == TokenType::Error)
        {
            snprintf(message, sizeof(message), " Error token!!!");
        }
        else
        {
            snprintf(message, sizeof(message), " at '%.*s'", token.length, token.start);
        }

        _parser.optError =
            Parser::error_t(Parser::error_t::code_t::Undefined,
                            buildMessage("[line %d] Error %s: %s", _parser.current.line, message, errorMsg));
        return _parser.optError.value();
    }

    void synchronize()
    {
        // discard remaining part of current block (i.e., statement)
        _parser.panicMode = false;

        while (_parser.current.type != TokenType::Eof)
        {
            if (_parser.previous.type == TokenType::Semicolon)
            {
                return;
            }
            switch (_parser.current.type)
            {
                case TokenType::Class:
                case TokenType::Func:
                case TokenType::Var:
                case TokenType::For:
                case TokenType::If:
                case TokenType::While:
                case TokenType::Print:
                case TokenType::Return:
                    return;
                default:
                    break;
            }

            advance();
        }
    }

   protected:
    using token_result_t = Result<Token, error_t>;
    using expression_handler_t = std::function<void()>;

    void populateParseRules()
    {
        auto binaryFunc = [&] { binary(); };
        auto groupingFunc = [&] { grouping(); };
        auto literalFunc = [&] { literal(); };
        auto numberFunc = [&] { number(); };
        auto stringFunc = [&] { string(); };
        auto skipFunc = [&] { skip(); };
        auto unaryFunc = [&] { unary(); };
        auto varFunc = [&] { variableDeclaration(); };
        auto identifierFunc = [&] { variable(); };

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
        _parseRules[(size_t)TokenType::Identifier] = {identifierFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::String] = {stringFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Number] = {numberFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::NumberFloat] = {numberFunc, NULL, Precedence::NONE};
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
        _parseRules[(size_t)TokenType::Var] = {varFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Return] = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::While] = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::For] = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Func] = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Error] = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Eof] = {NULL, NULL, Precedence::NONE};
    }

    const Optional<Parser::error_t> &getCurrentError() const { return _parser.optError; }
    const Token &getCurrentToken() const { return _parser.current; }

    bool check(TokenType type) const { return getCurrentToken().type == type; }
    bool match(TokenType type)
    {
        if (!check(type))
        {
            return false;
        }
        advance();
        return true;
    }
    void consume(TokenType type, const char *message)
    {
        if (check(type))
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
            if (tokenResult.isOk())
            {
                _parser.current = tokenResult.value();
                if (tokenResult.value().type != TokenType::Error)
                {
                    break;
                }
            }
            else
            {
                errorAt(_parser.current, tokenResult.error().message().c_str()).message();
            }
        }
    }

   protected:
    Scanner _scanner;
    Parser _parser;
    int _lastExpressionLine = -1;

    std::unique_ptr<Chunk> _compilingChunk = nullptr;
    Chunk *currentChunk() { return _compilingChunk.get(); }

    ParseRule _parseRules[(size_t)TokenType::COUNT];

    std::unordered_map<std::string, uint16_t> _variables;
};
