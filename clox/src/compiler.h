#pragma once

#include "chunk.h"
#include "scanner.h"
#include "utils/common.h"

#if USING(DEBUG_PRINT_CODE)
#include "debug.h"
#define CMP_DEBUGPRINT(LEVEL, X, ...)                        \
    if (debug_print::GetLevel() >= LEVEL)                    \
    {                                                        \
        printf("[%s] " X "\n", __FUNCTION__, ##__VA_ARGS__); \
    }
#define CMP_DEBUGPRINT_PARSE(LEVEL)                                                               \
    CMP_DEBUGPRINT(LEVEL, "CUR[%.*s] NXT[%.*s]", _parser.previous.length, _parser.previous.start, \
                   _parser.current.length, _parser.current.start)
#else  // #if USING(DEBUG_PRINT_CODE)
#define CMP_DEBUGPRINT(...)
#define CMP_DEBUGPRINT_PARSE(...)
#endif  // #else // #if USING(DEBUG_PRINT_CODE)

#include <functional>
#include <unordered_map>
#include <vector>

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

    struct ErrorInfo
    {
        error_t     error;
        Token       token;
        const char *linePtr;
    };
    Optional<ErrorInfo> optError;
    bool                panicMode = false;
    bool                hadError  = false;
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
    using parse_func_t = std::function<void(bool canAssign)>;
    parse_func_t prefix;
    parse_func_t infix;
    Precedence   precedence;
};

struct Compiler
{
    enum class ErrorCode
    {
        ScannerError,
        Undefined
    };
    using error_t  = Error<ErrorCode>;
    using result_t = Result<std::unique_ptr<Chunk>, error_t>;

    struct Configuration
    {
        bool allowDynamicVariables = false;  // no need to declare with <var>
        bool defaultConstVariables = false;  // <mut> allows modifying variables
        bool disassemble           = false;
#if USING(DEBUG_TRACE_EXECUTION)
        bool debugPrintConstants = false;  // print constants on every new one
        bool debugPrintVariables = false;  // print variables on every new one
#endif                                     // #if USING(DEBUG_TRACE_EXECUTION)
#if USING(EXTENDED_ERROR_REPORT)
        bool extendedErrorReport = true;
#endif  // #if USING(EXTENDED_ERROR_REPORT)
    };

   public:
    inline const Configuration &getConfiguration() const { return _configuration; }
    inline void                 setConfiguration(const Configuration &config) { _configuration = config; }

    result_t compileFromSource(const char *sourceCode, Optional<Compiler::Configuration> optConfiguration = none_t);
    result_t compileFromFile(const char *path, Optional<Compiler::Configuration> optConfiguration = none_t);

    result_t compile(const char *source, const char *sourcePath,
                     const Optional<Configuration> &optConfiguration = none_t)
    {
        if (optConfiguration.hasValue())
        {
            setConfiguration(optConfiguration.value());
        }

        populateParseRules();
        _scanner.init(source);
        ScopedCallback onExit([&] { _scanner.finish(); });
        _compilingChunk = std::make_unique<Chunk>(sourcePath);

        _parser.optError.reset();
        _parser.panicMode = false;
        _parser.hadError  = false;

        advance();
        while (!isAtEnd())
        {
            declaration();
        }

        consume(TokenType::Eof, "Expected end of expression");

        finishCompilation();

        if (_parser.hadError)
        {
            if (getCurrentError().hasValue())
            {
                const Parser::ErrorInfo errorInfo    = _parser.optError.extract();
                return makeResultError<result_t>(result_t::error_t::code_t::Undefined, errorInfo.error.message());
            }
            return makeResultError<result_t>(result_t::error_t::code_t::Undefined, "Compilation failed.");
        }
        return extractChunk();
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
        else if (match(TokenType::LeftBrace))
        {
            blockStatement();
        }
        else if (match(TokenType::If))
        {
            ifStatement();
        }
        else if (match(TokenType::While))
        {
            whileStatement();
        }
        else if (match(TokenType::Do))
        {
            dowhileStatement();
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
        CMP_DEBUGPRINT_PARSE(3);
        emitBytes(OpCode::Print);
    }
    void blockStatement()
    {
        emitBytes(OpCode::ScopeBegin);
        while (!check(TokenType::RightBrace) && !isAtEnd())
        {
            declaration();
        }
        consume(TokenType::RightBrace, "Expect '}' after value");
        CMP_DEBUGPRINT_PARSE(3);
        emitBytes(OpCode::ScopeEnd);
    }
    void ifStatement()
    {
        consume(TokenType::LeftParen, "Expected '(' before expression.");
        expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");

        const codepos_t thenJump = emitJump(OpCode::JumpIfFalse);
        emitBytes(OpCode::Pop);  // pop condition

        statement();
        const codepos_t elseJump = emitJump(OpCode::Jump);

        patchJump(thenJump);
        emitBytes(OpCode::Pop);  // pop condition

        if (match(TokenType::Else))
        {
            statement();
        }
        patchJump(elseJump);
    }
    void whileStatement()
    {
        const codepos_t loopStart = _compilingChunk->getCodeSize();

        consume(TokenType::LeftParen, "Expected '(' after 'while'.");
        expression();
        consume(TokenType::RightParen, "Expected ')' after condition.");

        const codepos_t exitJump = emitJump(OpCode::JumpIfFalse);
        emitBytes(OpCode::Pop);  // pop condition

        statement();
        emitJump(OpCode::Jump, loopStart);

        patchJump(exitJump);
        emitBytes(OpCode::Pop);  // pop condition
    }
    void dowhileStatement()
    {
        const codepos_t doJump = emitJump(OpCode::Jump);

        const codepos_t loopStart = _compilingChunk->getCodeSize();
        emitBytes(OpCode::Pop);  // pop condition

        patchJump(doJump);

        statement();

        consume(TokenType::While, "Expected 'While'");
        consume(TokenType::LeftParen, "Expected '(' after 'while'.");
        expression();
        consume(TokenType::RightParen, "Expected ')' after condition.");
        consume(TokenType::Semicolon, "Expected ';' after expression.");

        emitJump(OpCode::JumpIfTrue, loopStart);
        emitBytes(OpCode::Pop);  // pop condition
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
        CMP_DEBUGPRINT_PARSE(3);
        _lastExpressionLine = _parser.current.line;
        parsePrecedence(Precedence::ASSIGNMENT);
    }
    void skip()
    {
        CMP_DEBUGPRINT_PARSE(3);
        // nothing to do here
    }
    void grouping()
    {
        CMP_DEBUGPRINT_PARSE(3);
        expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");
    }
    void variable(bool canAssign)
    {
        CMP_DEBUGPRINT_PARSE(3);
        namedVariable(_parser.previous, canAssign);
    }
    void namedVariable(const Token &name, bool canAssign)
    {
        CMP_DEBUGPRINT_PARSE(3);
        const uint8_t varId = identifierConstant(name);
        if (canAssign && match(TokenType::Equal))
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
        CMP_DEBUGPRINT_PARSE(3);

        switch (_parser.previous.type)
        {
            case TokenType::Null: emitBytes(OpCode::Null); break;
            case TokenType::False: emitBytes(OpCode::False); break;
            case TokenType::True: emitBytes(OpCode::True); break;
            default: ASSERT(false); return;
        }
    }
    void number()
    {
        CMP_DEBUGPRINT_PARSE(3);

        Value value = Value::Create(strtod(_parser.previous.start, nullptr));
        emitConstant(value);
    }
    void string() { emitConstant(Value::CreateByCopy(_parser.previous.start, _parser.previous.length)); }
    void unary()
    {
        CMP_DEBUGPRINT_PARSE(3);
        const TokenType operatorType = _parser.previous.type;

        // parse expression
        parsePrecedence(Precedence::UNARY);

        // Emit the operator instruction.
        switch (operatorType)
        {
            case TokenType::Minus: emitBytes(OpCode::Negate); break;
            case TokenType::Bang: emitBytes(OpCode::Not); break;
            default:
                FAIL_MSG("Unreachable symbol while parsing [%.*s]", _parser.previous.length, _parser.previous.start);
                return;  // Unreachable.
        }
    }
    void binary()
    {
        CMP_DEBUGPRINT_PARSE(3);

        const TokenType operatorType = _parser.previous.type;
        const ParseRule parseRule    = getParseRule(operatorType);
        // left-associative: 1+2+3+4 = ((1 + 2) + 3) + 4
        // right-associative: a=b=c=d -> a = (b = (c = d))
        parsePrecedence(Precedence((int)parseRule.precedence + 1));

        switch (operatorType)
        {
            case TokenType::Plus: emitBytes(OpCode::Add); break;
            case TokenType::Minus: emitBytes(OpCode::Subtract); break;
            case TokenType::Star: emitBytes(OpCode::Multiply); break;
            case TokenType::Slash: emitBytes(OpCode::Divide); break;

            case TokenType::BangEqual: emitBytes(OpCode::Equal, OpCode::Not); break;
            case TokenType::EqualEqual: emitBytes(OpCode::Equal); break;
            case TokenType::Greater: emitBytes(OpCode::Greater); break;
            case TokenType::GreaterEqual: emitBytes(OpCode::Equal, OpCode::Not); break;
            case TokenType::Less: emitBytes(OpCode::Less); break;
            case TokenType::LessEqual: emitBytes(OpCode::Greater, OpCode::Not); break;
            case TokenType::Equal: emitBytes(OpCode::Assignment); break;
            default:
                FAIL_MSG("Unreachable symbol while parsing [%.*s]", _parser.previous.length, _parser.previous.start);
                return;  // Unreachable.
        }
    }
    void variableDeclaration()
    {
        CMP_DEBUGPRINT_PARSE(2);

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
    void             parsePrecedence(Precedence precedence)
    {
        advance();

        const ParseRule &parseRule = getParseRule(_parser.previous.type);
        if (parseRule.prefix == NULL)
        {
            error("Expect expression.");
            return;
        }
        const bool canAssign = precedence <= Precedence::ASSIGNMENT;
        parseRule.prefix(canAssign);

        while (precedence <= getParseRule(_parser.current.type).precedence)
        {
            advance();
            ParseRule::parse_func_t infixRule = getParseRule(_parser.previous.type).infix;
            infixRule(canAssign);
        }

        if (!canAssign && match(TokenType::Equal))
        {
            error("Invalid assignment target");
        }
    }

    uint8_t parseVariable(const char *errorMessage)
    {
        consume(TokenType::Identifier, errorMessage);
        return identifierConstant(_parser.previous);
    }
    void    defineVariable(uint8_t id) { emitBytes(OpCode::GlobalVarDef, id); }
    uint8_t identifierConstant(const Token &token)
    {
        CMP_DEBUGPRINT_PARSE(2);
        return makeConstant(Value::CreateByCopy(token.start, token.length));
    }

   protected:
    void finishCompilation()
    {
        emitReturn();
#if USING(DEBUG_PRINT_CODE)
        if (_configuration.disassemble && !_parser.hadError)
        {
            ASSERT(currentChunk());
            disassemble(*currentChunk(), "code");
        }
#endif  // #if USING(DEBUG_PRINT_CODE)
    }

    uint8_t makeConstant(const Value &value)
    {
        ASSERT(currentChunk());
        Chunk &chunk = *currentChunk();

        const int constantId = chunk.addConstant(value);
        ASSERT(constantId < UINT8_MAX);
        if (constantId > UINT8_MAX)
        {
            error(buildMessage("Max constants per chunk exceeded: %s", UINT8_MAX).c_str());
        }
#if USING(DEBUG_TRACE_EXECUTION)
        if (_configuration.debugPrintConstants)
        {
            chunk.printConstants();
        }
#endif  // #if USING(DEBUG_TRACE_EXECUTION)
        return static_cast<uint8_t>(constantId);
    }
    void emitConstant(const Value &value) { emitBytes(OpCode::Constant, makeConstant(value)); }

    void emitReturn() { emitBytes((uint8_t)OperationType::Return); }

    uint16_t emitJump(OpCode op, codepos_t jumpOffset)
    {
        constexpr size_t jumpBytes = sizeof(jump_t);
        emitBytes(op);
        Chunk *chunk = currentChunk();
        const codepos_t codeOffset = chunk->getCodeSize();
        ASSERT(static_cast<size_t>(chunk->getCodeSize() - jumpOffset) < limits::kMaxJumpLength);

        const jump_t jump = jumpOffset - codeOffset - jumpBytes;

        emitBytes(static_cast<uint16_t>(jump));
        return codeOffset;
    }
    uint16_t emitJump(OpCode op)
    {
        emitBytes(op);
        const codepos_t codeOffset = currentChunk()->getCodeSize();
        // placeholder
        emitBytes((uint8_t)0xff);
        emitBytes((uint8_t)0xff);
        return codeOffset;
    }
    void patchJump(codepos_t codePos)
    {
        constexpr size_t jumpBytes = sizeof(jump_t);
        Chunk           *chunk     = currentChunk();
        ASSERT(static_cast<size_t>(chunk->getCodeSize() - codePos) < limits::kMaxJumpLength);
        const jump_t jumpLen = chunk->getCodeSize() - codePos - jumpBytes;

        uint8_t *code     = chunk->getCodeMut();
        code[codePos]     = static_cast<uint8_t>((jumpLen >> 8) & 0xff);
        code[codePos + 1] = static_cast<uint8_t>(jumpLen & 0xff);
    }

    void emitBytes(uint8_t byte)
    {
        Chunk *chunk = currentChunk();
        ASSERT(chunk);
        chunk->write(byte, static_cast<uint16_t>(_lastExpressionLine));
    }
    void emitBytes(uint16_t word)
    {
        Chunk *chunk = currentChunk();
        ASSERT(chunk);
        uint8_t byte = static_cast<uint8_t>((word >> 8) & 0xFF);
        chunk->write(byte, _lastExpressionLine);
        byte = static_cast<uint8_t>(word & 0xFF);
        chunk->write(byte, _lastExpressionLine);
    }
    void emitBytes(OpCode code)
    {
        CMP_DEBUGPRINT(1, "emitOp: %s", named_enum::name(code));
        emitBytes((uint8_t)code);
    }
    // void emitBytes(int constantId)
    // {
    //     ASSERT(constantId < UINT8_MAX);
    //     emitBytes((uint8_t)constantId);
    // }
    template <typename T, typename... Args>
    void emitBytes(T byte, Args... args)
    {
        emitBytes(byte);
        emitBytes(args...);
    }

   protected:
    Parser::ErrorInfo errorAtCurrent(const char *errorMsg) { return errorAt(_parser.current, errorMsg); }
    Parser::ErrorInfo error(const char *errorMsg) { return errorAt(_parser.previous, errorMsg); }
    Parser::ErrorInfo errorAt(const Token &token, const char *errorMsg)
    {
        FAIL_MSG(errorMsg);
        if (_parser.panicMode)
        {
            return _parser.optError.value();
        }
        _parser.panicMode = true;
        _parser.hadError  = true;

        char innerMessage[1024];
        if (token.type == TokenType::Eof)
        {
            snprintf(innerMessage, sizeof(innerMessage), "at end");
        }
        else if (token.type == TokenType::Error)
        {
            snprintf(innerMessage, sizeof(innerMessage), "token found!!!");
        }
        else
        {
            snprintf(innerMessage, sizeof(innerMessage), "at '%.*s'", token.length, token.start);
        }

        char message[2048];
        snprintf(message, sizeof(message), "[line %d] Error %s: %s", _parser.current.line, innerMessage, errorMsg);
#if USING(EXTENDED_ERROR_REPORT)
        if (_configuration.extendedErrorReport)
        {
            const char *errorLinePtr = _scanner._linePtr;
            const char *lineEnd      = strchr(errorLinePtr, '\n');
            if (lineEnd == nullptr)
            {
                lineEnd = strchr(errorLinePtr, '\0');
            }
            if (lineEnd != nullptr)
            {
                const int lineLen       = static_cast<int>(lineEnd - errorLinePtr);

                const char *messagePtrEnd = message + sizeof(message);
                char     *messagePtr    = message + strlen(message);
                snprintf(messagePtr, messagePtrEnd - messagePtr, "\n\t'%.*s'", lineLen, errorLinePtr);
                messagePtr = message + strlen(message);

                const Token *errorToken = token.type != TokenType::COUNT ? &token : nullptr;
                const size_t lenToToken = errorToken ? errorToken->start - errorLinePtr : strlen(errorLinePtr);
                *messagePtr++           = '\n';
                *messagePtr++           = '\t';
                *messagePtr++           = ' ';  // extra <'>
                memset(messagePtr, ' ', lenToToken);
                messagePtr += lenToToken;
                *messagePtr++ = '^';
                *messagePtr++ = '\0';
            }
        }
#endif  // #if USING(EXTENDED_ERROR_REPORT)
        _parser.optError =
            Parser::ErrorInfo{Parser::error_t(Parser::error_t::code_t::Undefined, message), token, _scanner._linePtr};
        return _parser.optError.value();
    }

    void synchronize()
    {
        if (getCurrentError().hasValue())
        {
            const Parser::ErrorInfo errorInfo    = _parser.optError.extract();
            LOG_ERROR("%s\n", errorInfo.error.message().c_str());
        }

        // discard remaining part of current block (i.e., statement)
        _parser.panicMode = false;

        while (!isAtEnd())
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
                case TokenType::Return: return;
                default: break;
            }

            advance();
        }
    }

   protected:
    using token_result_t       = Result<Token, error_t>;
    using expression_handler_t = std::function<void()>;

    void populateParseRules()
    {
        auto binaryFunc     = [&](bool /*canAssign*/) { binary(); };
        auto groupingFunc   = [&](bool /*canAssign*/) { grouping(); };
        auto literalFunc    = [&](bool /*canAssign*/) { literal(); };
        auto numberFunc     = [&](bool /*canAssign*/) { number(); };
        auto stringFunc     = [&](bool /*canAssign*/) { string(); };
        auto skipFunc       = [&](bool /*canAssign*/) { skip(); };
        auto unaryFunc      = [&](bool /*canAssign*/) { unary(); };
        auto varFunc        = [&](bool /*canAssign*/) { variableDeclaration(); };
        auto identifierFunc = [&](bool canAssign) { variable(canAssign); };
        auto andFunc        = [&](bool /*canAssign*/)
        {
            const uint16_t endJump = emitJump(OpCode::JumpIfFalse);
            emitBytes(OpCode::Pop);
            parsePrecedence(Precedence::AND);
            patchJump(endJump);
        };
        auto orFunc = [&](bool /*canAssign*/)
        {
            const uint16_t endJump = emitJump(OpCode::JumpIfTrue);
            emitBytes(OpCode::Pop);
            parsePrecedence(Precedence::OR);
            patchJump(endJump);
        };

        _parseRules[(size_t)TokenType::LeftParen]    = {groupingFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::RightParen]   = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::LeftBrace]    = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::RightBrace]   = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Semicolon]    = {skipFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Comma]        = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Dot]          = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Minus]        = {unaryFunc, binaryFunc, Precedence::TERM};
        _parseRules[(size_t)TokenType::Plus]         = {NULL, binaryFunc, Precedence::TERM};
        _parseRules[(size_t)TokenType::Slash]        = {NULL, binaryFunc, Precedence::FACTOR};
        _parseRules[(size_t)TokenType::Star]         = {NULL, binaryFunc, Precedence::FACTOR};
        _parseRules[(size_t)TokenType::Bang]         = {unaryFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::BangEqual]    = {NULL, binaryFunc, Precedence::EQUALITY};
        _parseRules[(size_t)TokenType::Equal]        = {NULL, binaryFunc, Precedence::EQUALITY};
        _parseRules[(size_t)TokenType::EqualEqual]   = {NULL, binaryFunc, Precedence::COMPARISON};
        _parseRules[(size_t)TokenType::Greater]      = {NULL, binaryFunc, Precedence::COMPARISON};
        _parseRules[(size_t)TokenType::GreaterEqual] = {NULL, binaryFunc, Precedence::COMPARISON};
        _parseRules[(size_t)TokenType::Less]         = {NULL, binaryFunc, Precedence::COMPARISON};
        _parseRules[(size_t)TokenType::LessEqual]    = {NULL, binaryFunc, Precedence::COMPARISON};
        _parseRules[(size_t)TokenType::Identifier]   = {identifierFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::String]       = {stringFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Number]       = {numberFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::NumberFloat]  = {numberFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::And]          = {NULL, andFunc, Precedence::AND};
        _parseRules[(size_t)TokenType::Or]           = {NULL, orFunc, Precedence::OR};
        _parseRules[(size_t)TokenType::Class]        = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Super]        = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Null]         = {literalFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::True]         = {literalFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::False]        = {literalFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Var]          = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::This]         = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Else]         = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::If]           = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Print]        = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Var]          = {varFunc, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Return]       = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Do]           = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::While]        = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::For]          = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Func]         = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Error]        = {NULL, NULL, Precedence::NONE};
        _parseRules[(size_t)TokenType::Eof]          = {NULL, NULL, Precedence::NONE};
    }

    const Optional<Parser::ErrorInfo> &getCurrentError() const { return _parser.optError; }
    const Token                       &getCurrentToken() const { return _parser.current; }

    bool isAtEnd() { return getCurrentToken().type == TokenType::Eof; }
    bool check(TokenType type) const { return getCurrentToken().type == type; }
    bool match(TokenType type)
    {
        if (check(type))
        {
            advance();
            return true;
        }
        return false;
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
                errorAt(_parser.current, tokenResult.error().message().c_str());
            }
        }
    }

   protected:
    Configuration _configuration;

    Scanner  _scanner;
    Parser   _parser;
    uint32_t _lastExpressionLine = uint32_t(-1);

    std::unique_ptr<Chunk> _compilingChunk = nullptr;
    Chunk                 *currentChunk() { return _compilingChunk.get(); }
    std::unique_ptr<Chunk> extractChunk() { return std::move(_compilingChunk); }

    ParseRule _parseRules[(size_t)TokenType::COUNT];

    std::unordered_map<std::string, uint16_t> _variables;
};
