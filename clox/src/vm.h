/*
https://craftinginterpreters.com/a-virtual-machine.html
*/
#pragma once

#include <cstdlib>
#include <cstring>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"

struct VirtualMachine
{
    enum class ErrorCode
    {
        CompileError,
        RuntimeError,
        Undefined = 0x255
    };
    using error_t = Error<ErrorCode>;

    enum class InterpretResult
    {
        Ok,
        Error
    };
    using result_t = Result<InterpretResult, error_t>;

    VirtualMachine() {}
    ~VirtualMachine() {}

    result_t init() { return makeResult<result_t>(InterpretResult::Ok); }
    result_t finish() { return makeResult<result_t>(InterpretResult::Ok); }

    result_t run()
    {
#define READ_BYTE() (*_ip++)
#define READ_CONSTANT() (_chunk->getConstants()[READ_BYTE()])
#define BINARY_OP(op)               \
    do                              \
    {                               \
        const Value b = stackPop(); \
        const Value a = stackPop(); \
        stackPush(a op b);          \
    } while (false)

        for (;;)
        {
#if DEBUG_TRACE_EXECUTION
            printf("          ");
            stackPrint();
            printf("          ");
            _chunk->printConstants();
            disassembleInstruction(*_chunk, static_cast<uint16_t>(_ip - _chunk->getCode()));
#endif  // #if DEBUG_TRACE_EXECUTION

            const OpCode instruction = OpCode(READ_BYTE());
            switch (instruction)
            {
                case OpCode::Return:
                    printf("RESULT: ");
                    if (stackSize() > 0)
                    {
                        printValue(stackPop());
                    }
                    printf("\n");
                    return InterpretResult::Ok;
                case OpCode::Constant:
                {
                    const Value constant = READ_CONSTANT();
                    stackPush(constant);
                }
                break;

                case OpCode::Null:
                    stackPush(Value::Create(Value::Null));
                    break;
                case OpCode::True:
                    stackPush(Value::Create(true));
                    break;
                case OpCode::False:
                    stackPush(Value::Create(false));
                    break;

                case OpCode::Equal:
                {
                    const Value a = stackPop();
                    const Value b = stackPop();
                    stackPush(Value::Create(a == b));
                }
                break;
                case OpCode::Greater:
                {
                    const Value a = stackPop();
                    const Value b = stackPop();
                    stackPush(Value::Create(a > b));
                }
                break;
                case OpCode::Less:
                {
                    const Value a = stackPop();
                    const Value b = stackPop();
                    stackPush(Value::Create(a < b));
                }
                break;

                case OpCode::Add:
                    BINARY_OP(+);
                    break;
                case OpCode::Divide:
                    BINARY_OP(/);
                    break;
                case OpCode::Multiply:
                    BINARY_OP(*);
                    break;
                case OpCode::Subtract:
                    BINARY_OP(-);
                    break;
                case OpCode::Negate:
                {
                    const Value &value = peek(0);
                    if (value.type == Value::Type::Number || value.type == Value::Type::Integer)
                    {
                        stackPop();
                        stackPush(-value);
                    }
                    else
                    {
                        return runtimeError("Operand must be a number");
                    }
                }
                break;
                case OpCode::Not:
                    stackPush(Value::Create(stackPop().isFalsey()));
                    break;
                default:
                    break;
            }
        }

        return makeResult<result_t>(InterpretResult::Error);
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
    }

    result_t interpret(const char *source)
    {
        Compiler::result_t result = _compiler.compile(source);
        if (!result.isOk())
        {
            return makeResultError<result_t>(ErrorCode::CompileError, result.error().message());
        }
        const Chunk *currentChunk = result.value();
        _chunk = currentChunk;
        _ip = currentChunk->getCode();

        printf("--------------------------------------\n");
        printf("* Executing code...\n");
        result_t runResult = run();

        return runResult;
    }

    Result<char *> readFile(const char *path)
    {
        FILE *file = nullptr;
        errno_t error = fopen_s(&file, path, "rb");
        if (file == nullptr)
        {
            LOG_ERROR("Couldn't open file '%s'\n", path);
            return makeResultError<Result<char *>>(Result<char *>::error_t::code_t::Undefined);
        }

        fseek(file, 0L, SEEK_END);
        const size_t fileSize = ftell(file);
        rewind(file);

        char *buffer = (char *)malloc(fileSize) + 1;
        if (buffer == nullptr)
        {
            char message[1024];
            sprintf_s(message, "Couldn't allocate memory for reading the file %s with size %zu byte(s)\n", path,
                      fileSize);
            LOG_ERROR(message);
            fclose(file);
            return makeResultError<Result<char *>>(Result<char *>::error_t::code_t::Undefined, message);
        }
        const size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
        if (bytesRead < fileSize)
        {
            LOG_ERROR("Couldn't read the file '%s'\n", path);
            fclose(file);
            return makeResultError<Result<char *>>(Result<char *>::error_t::code_t::Undefined);
        }
        buffer[bytesRead] = '\0';

        fclose(file);
        return makeResult<Result<char *>>(buffer);
    }

    result_t runFile(const char *path)
    {
        Result<char *> source = readFile(path);
        if (!source.isOk())
        {
            return makeResultError<result_t>(source.error().message());
        }
        result_t result = interpret(source.value());
        free(source.value());

        return result;
    }

    result_t repl()
    {
        char line[1024];
        for (;;)
        {
            printf("> ");

            if (!fgets(line, sizeof(line), stdin))
            {
                printf("\n");
                break;
            }

            if (line[0] == '!')
            {
                if (strstr(line, "help"))
                {
                    printf("--------------------------------\n");
                    printf("Commands(preceded with '!'):\n");
                    printf("\t!debugbreak true/false\n");
                    printf("\tquit\n");
                    printf("--------------------------------\n");
                }
                else if (strstr(line, "quit"))
                {
                    printf("--------------------------------\n");
                    printf("Exiting...\n");
                    printf("--------------------------------\n");
                    break;
                }
                else if (strstr(line, "debugbreak"))
                {
                    const bool enable = strstr(line, "enable");
                    util::SetDebugBreakEnabled(enable);
                    printf("--------------------------------\n");
                    printf("DebugBreak %s\n", enable ? "enabled" : "disabled");
                    printf("--------------------------------\n");
                    continue;
                }
            }

            result_t result = interpret(line);
            if (!result.isOk())
            {
                char message[1024];
                sprintf_s(message, "INTERPRETER: %s", result.error().message().c_str());
                LOG_ERROR(message);
            }
        }

        return makeResult<result_t>(InterpretResult::Ok);
    }

   protected:  // Interpreter
    const Value &peek(int distance) const
    {
        ASSERT(distance < stackSize());
        return _stackTop[-1 + distance];
    }

    result_t runtimeError(const char *format, ...)
    {
        char message[256];
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        sprintf_s(message, "%s\n", message);

        const uint16_t instruction = static_cast<uint16_t>(this->_ip - this->_chunk->getCode() - 1);
        const uint16_t line = this->_chunk->getLine(instruction);
        fprintf(stderr, "[line %d] in script\n", line);
        stackReset();
        return makeResultError<result_t>(result_t::error_t::code_t::RuntimeError, message);
    }

   protected:  // Stack
    static constexpr size_t STACK_SIZE = 1024;
    Value _stack[STACK_SIZE];
    Value *_stackTop = &_stack[0];

    void stackReset() { _stackTop = &_stack[0]; }
    void stackPush(Value value)
    {
        *_stackTop = value;
        ++_stackTop;
    }
    Value stackPop()
    {
        --_stackTop;
        return *_stackTop;
    }
    size_t stackSize() const { return _stackTop - _stack; }
#if DEBUG_TRACE_EXECUTION
    void stackPrint() const
    {
        printf(" Stack: ");
        for (const Value *slot = _stack; slot < _stackTop; ++slot)
        {
            printf("[");
            printValue(*slot);
            printf("]");
        }
        printf("\n");
    }
#endif  // #if DEBUG_TRACE_EXECUTION

   protected:
    const Chunk *_chunk = nullptr;
    const uint8_t *_ip = nullptr;

    Compiler _compiler;
};
