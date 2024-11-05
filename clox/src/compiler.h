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
#define CMP_DEBUGPRINT_PARSE(LEVEL)                                                                                   \
    do                                                                                                                \
    {                                                                                                                 \
        const int  lineLen       = (int)(strchr(_scanner._linePtr, '\n') - _scanner._linePtr - 1);                    \
        const char paddingBuff[] = "                                                                               "; \
        const int  padding       = 40 - (int)strlen(__FUNCTION__) - _parser.previous.length - _parser.current.length; \
        CMP_DEBUGPRINT(LEVEL, "PRV[%.*s] CUR[%.*s] %.*s LINE[%.*s]", _parser.previous.length, _parser.previous.start, \
                       _parser.current.length, _parser.current.start, padding >= 0 ? padding : 0, paddingBuff,                           \
                       lineLen >= 0 ? lineLen : 0, _scanner._linePtr)                                                 \
        break;                                                                                                        \
    } while (1)
#else  // #if USING(DEBUG_PRINT_CODE)
#define CMP_DEBUGPRINT(...)
#define CMP_DEBUGPRINT_PARSE(...)
#endif  // #else // #if USING(DEBUG_PRINT_CODE)

#include <functional>
#include <unordered_map>
#include <vector>

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

enum class Precedence : uint8_t
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
        bool isREPL                = false;
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

    inline void setConfiguration(const Configuration &config) { _configuration = config; }

    result_t compileFromSource(const char *sourceCode, Optional<Compiler::Configuration> optConfiguration = none_t);
    result_t compileFromFile(const char *path, Optional<Compiler::Configuration> optConfiguration = none_t);

    result_t compile(const char *source, const char *sourcePath,
                     const Optional<Configuration> &optConfiguration = none_t);

   protected:  // high level stuff
    void declaration();
    void statement();
    void printStatement();
    void blockStatement();
    void ifStatement();
    void whileStatement();
    void dowhileStatement();
    void expressionStatement();

    /////////////////////////////////////////////////////////////////////////////////
   protected:
    void expression();
    void skip();
    void grouping();
    void variable(bool canAssign);
    void namedVariable(const Token &name, bool canAssign);
    void literal();
    void number();
    void string();
    void unary();
    void binary();
    void variableDeclaration();

    const ParseRule &getParseRule(TokenType type) const { return _parseRules[(size_t)type]; }

    void    parsePrecedence(Precedence precedence);
    uint8_t parseVariable(const char *errorMessage);
    void    declareVariable();
    void    defineVariable(uint8_t id);
    uint8_t identifierConstant(const Token &token);
    void    beginScope();
    void    endScope();

   protected:
    void finishCompilation();

    uint8_t makeConstant(const Value &value);

    void     emitConstant(const Value &value);
    void     emitReturn();
    uint16_t emitJump(OpCode op, codepos_t jumpOffset);
    uint16_t emitJump(OpCode op);
    void     patchJump(codepos_t codePos);
    void     emitBytes(uint8_t byte);
    void     emitBytes(uint16_t word);
    void     emitBytes(OpCode code);

    template <typename T, typename... Args>
    void emitBytes(T byte, Args... args)
    {
        emitBytes(byte);
        emitBytes(args...);
    }

   protected:  // local variables
    void addLocalVariable(const Token &name);
    void initializeLocalVariable();
    int  resolveLocalVariable(const Token &name);

   protected:
    Parser::ErrorInfo errorAtCurrent(const char *errorMsg) { return errorAt(_parser.current, errorMsg); }

    Parser::ErrorInfo error(const char *errorMsg) { return errorAt(_parser.previous, errorMsg); }

    Parser::ErrorInfo errorAt(const Token &token, const char *errorMsg);

    void synchronize();

   protected:
    using token_result_t       = Result<Token, error_t>;
    using expression_handler_t = std::function<void()>;

    void populateParseRules();

    const Optional<Parser::ErrorInfo> &getCurrentError() const { return _parser.optError; }

    const Token &getCurrentToken() const { return _parser.current; }

    bool isAtEnd();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void consume(TokenType type, const char *message);

    void advance();

   protected:
    Chunk *currentChunk() { return _compilingChunk.get(); }

    std::unique_ptr<Chunk> extractChunk() { return std::move(_compilingChunk); }

   protected:
    Configuration _configuration;

    Scanner  _scanner;
    Parser   _parser;
    uint32_t _lastExpressionLine = uint32_t(-1);

    std::unique_ptr<Chunk> _compilingChunk = nullptr;

    ParseRule _parseRules[(size_t)TokenType::COUNT];

    struct LocalState
    {
        struct Local
        {
            Token name;
            int   declarationDepth = 0;
        };

        Local locals[MAX_U8_COUNT];
        int   localCount = 0;
        int   scopeDepth = 0;
    };

    LocalState _localState;
};
