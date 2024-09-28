/*
https://craftinginterpreters.com/a-virtual-machine.html
*/
#pragma once

#include <cstdlib>
#include <cstring>
#include <memory>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "environment.h"

struct VirtualMachine
{
    enum class ErrorCode
    {
        CompileError,
        RuntimeError,
        FileSystemError,
        Undefined = 0x255
    };
    using error_t = Error<ErrorCode>;

    enum class InterpretResult
    {
        Ok,
        Error
    };
    using result_t = Result<InterpretResult, error_t>;

    static const int kGlobalEnvironmentIndex = 0;

    ///////////////////////////////////////////////////////////////////////////////////

    VirtualMachine() {}
    ~VirtualMachine() {}

    result_t init()
    {
        _environments.push_back(std::make_unique<Environment>());
        return makeResult<result_t>(InterpretResult::Ok);
    }
    result_t finish()
    {
        ASSERT(_environments.size() <= 1);
        for (auto &env : _environments)
        {
            env->Reset();
        }
        _environments.clear();

        Object::FreeObjects();
        return makeResult<result_t>(InterpretResult::Ok);
    }

    result_t run()
    {
#define READ_BYTE() (*_ip++)
#define READ_CONSTANT() (_chunk->getConstants()[READ_BYTE()])
#define READ_STRING() (READ_CONSTANT().as.object->asString()->chars)
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
            PrintVariables();
            printf("          ");
            _chunk->printConstants();
            disassembleInstruction(*_chunk, static_cast<uint16_t>(_ip - _chunk->getCode()));
#endif  // #if DEBUG_TRACE_EXECUTION

            const OpCode instruction = OpCode(READ_BYTE());
            switch (instruction)
            {
                case OpCode::Return:
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
                    const Value b = stackPop();
                    const Value a = stackPop();
                    stackPush(Value::Create(a == b));
                }
                break;
                case OpCode::Greater:
                {
                    const Value b = stackPop();
                    const Value a = stackPop();
                    stackPush(Value::Create(a > b));
                }
                break;
                case OpCode::Less:
                {
                    const Value b = stackPop();
                    const Value a = stackPop();
                    stackPush(Value::Create(a < b));
                }
                break;

                case OpCode::Add:
                {
                    const Value b = stackPop();
                    const Value a = stackPop();
                    if (a.type == Value::Type::Object || b.type == Value::Type::Object)
                    {
                        const Object *aObj = a.type == Value::Type::Object ? a.as.object : nullptr;
                        const Object *bObj = b.type == Value::Type::Object ? b.as.object : nullptr;
                        if (aObj && bObj)
                        {
                            Result<Value> result = *aObj + *bObj;
                            if (result.isOk())
                            {
                                stackPush(result.extract());
                            }
                            else
                            {
                                return runtimeError("Cannot add types %s + %s: %s",
                                                    aObj ? Object::getTypeName(aObj->type) : Value::getTypeName(a.type),
                                                    bObj ? Object::getTypeName(bObj->type) : Value::getTypeName(b.type),
                                                    result.error().message().c_str());
                            }
                        }
                        else
                        {
                            return runtimeError("Cannot add different types: %s + %s",
                                                aObj ? Object::getTypeName(aObj->type) : Value::getTypeName(a.type),
                                                bObj ? Object::getTypeName(bObj->type) : Value::getTypeName(b.type));
                        }
                    }
                    else
                    {
                        stackPush(a + b);
                    }
                }
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
                case OpCode::Print:
                {
                    printValue(stackPop());
                    break;
                }
                case OpCode::Pop:
                {
                    stackPop();
                    break;
                }
                case OpCode::GlobalVarDef:
                {
                    const char *varName = READ_STRING();
                    Value *value = AddVariable(varName, kGlobalEnvironmentIndex);
                    *value = stackPop();  // null or expression :)
                    break;
                }
                case OpCode::GlobalVarGet:
                {
                    const char *varName = READ_STRING();
                    Value *value = FindVariable(varName, kGlobalEnvironmentIndex);
                    if (value == nullptr)
                    {
                        return runtimeError("Trying to read undefined variable '%s'.",
                                            value->as.object->asString()->chars);
                    }
                    stackPush(*value);
                    break;
                }
                case OpCode::GlobalVarSet:
                {
                    const char *varName = READ_STRING();
                    Value *value = FindVariable(varName, kGlobalEnvironmentIndex);
                    if (value == nullptr)
                    {
                        return runtimeError("Trying to write to undefined variable '%s'.", varName);
                    }
                    *value = peek(0);
                    break;
                }
                case OpCode::Assignment:
                {
                    const Value rvalue = stackPop();
                    const char *varName = READ_STRING();
                    Value *varValue = nullptr;
                    // check local variables
                    // ...
                    if (varValue == nullptr)
                    {
                        varValue = FindVariable(varName, kGlobalEnvironmentIndex);
                    }
                    if (varValue == nullptr)
                    {  // allow dynamic creation ?
                        varValue = AddVariable(varName, kGlobalEnvironmentIndex);
                        DEBUGPRINT_EX("Dynamic var(%s) created\n", varName);
                    }
                    if (varValue != nullptr)
                    {
                        *varValue = rvalue;
#if USING(DEBUG_BUILD)
                        std::string tempStr;
                        printValue(tempStr, rvalue);
                        DEBUGPRINT_EX("var(%s)=%s\n", varName, tempStr.c_str());
#endif  // #if USING(DEBUG_BUILD)
                    }
                    break;
                }
                default:
                    break;
            }
        }

        return makeResult<result_t>(InterpretResult::Error);
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
    }

    result_t interpret(const char *source, const char *sourcePath)
    {
        Compiler::result_t result = _compiler.compile(source, sourcePath);
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
        using result_t = Result<char *>;
        
        FILE *file = nullptr;
        fopen_s(&file, path, "rb");
        if (file == nullptr)
        {
            return makeResultError<result_t>(result_t::error_t::code_t::Undefined,
                                                   format("Couldn't open file '%s'\n", path));
        }
        ScopedCallback closeFile([&file] { fclose(file); });

        fseek(file, 0L, SEEK_END);
        const size_t fileSize = ftell(file);
        rewind(file);

        char *buffer = (char *)malloc(fileSize+1);
        if (buffer == nullptr)
        {
            char message[1024];
            snprintf(message, sizeof(message),
                     "Couldn't allocate memory for reading the file %s with size %zu byte(s)\n", path, fileSize);
            LOG_ERROR(message);
            return makeResultError<result_t>(result_t::error_t::code_t::Undefined, message);
        }
        const size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
        if (bytesRead < fileSize)
        {
            LOG_ERROR("Couldn't read the file '%s'\n", path);
            return makeResultError<result_t>(result_t::error_t::code_t::Undefined,
                                                   format("Couldn't read the file '%s'\n", path));
        }
        buffer[bytesRead] = '\0';

        return makeResult<result_t>(buffer);
    }

    result_t runFromSource(const char *sourceCode)
    {
        LOG_INFO("Running from source...\n");
        result_t result = interpret(sourceCode, "SOURCE");
        return result;
    }

    result_t runFromFile(const char *path)
    {
        LOG_INFO("Running from file(%s)...\n", path);
        Result<char *> source = readFile(path);
        ScopedCallback freeSource([&source] { free(source.extract()); });
        if (!source.isOk())
        {
            return makeResultError<result_t>(source.error().message());
        }
        char *buffer = source.value();

        result_t result = interpret(buffer, path);
        if (!result.isOk())
        {
            LOG_ERROR("%s", result.error().message().c_str());
        }

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
                    printf("\tdebugbreak <enable/disable>\n");
                    printf("\tquit\n");
                    printf("--------------------------------\n");
                    continue;
                }
                else if (strstr(line, "quit"))
                {
                    printf("--------------------------------\n");
                    printf("Exiting...\n");
                    printf("--------------------------------\n");
                    break;
                }
#if USING(DEBUG_BUILD)
                else if (strstr(line, "debugbreak"))
                {
                    const bool enable = strstr(line, "enable");
                    util::SetDebugBreakEnabled(enable);
                    printf("--------------------------------\n");
                    printf("DebugBreak %s\n", enable ? "enabled" : "disabled");
                    printf("--------------------------------\n");
                    continue;
                }
#endif  // #if !USING(DEBUG_BUILD)
            }

            result_t result = interpret(line, "REPL");
            if (!result.isOk())
            {
                LOG_ERROR("[INTERPRETER] %s\n", result.error().message().c_str());
                LOG_ERROR("                   %s", line);
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

        const Chunk *chunk = this->_chunk;
        const uint16_t instruction = static_cast<uint16_t>(this->_ip - chunk->getCode() - 1);
        const uint16_t line = chunk->getLine(instruction);
        char outputMessage[512];
        snprintf(outputMessage, sizeof(outputMessage), "[%s:%d] Runtime error: %s\n", chunk->getSourcePath(), line,
                 message);
        stackReset();
        return makeResultError<result_t>(result_t::error_t::code_t::RuntimeError, outputMessage);
    }

   protected:  // Stack
    static constexpr size_t STACK_SIZE = 1024;
    Value _stack[STACK_SIZE];
    Value *_stackTop = &_stack[0];

    void stackReset() { _stackTop = &_stack[0]; }
    void stackPush(const Value &value)
    {
        *_stackTop = value;
        ++_stackTop;
    }
    void stackPush(Value &&value)
    {
        *_stackTop = std::move(value);
        ++_stackTop;
    }
    Value &stackPop()
    {
        --_stackTop;
        return *_stackTop;
    }
    Value &stackPeek(size_t offset)
    {
        ASSERT(offset < stackSize());
        return *(_stackTop - offset);
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

    void PrintVariables() const
    {
        printf(" Global variables: ");
        _environments.front()->Print();
    }
#endif  // #if DEBUG_TRACE_EXECUTION

   protected:  // STATE
    const Chunk *_chunk = nullptr;
    const uint8_t *_ip = nullptr;

    Value *AddVariable(const char *name, int environmentIndex)
    {
        ASSERT(environmentIndex < _environments.size());
        return _environments[environmentIndex]->AddVariable(name);
    }
    bool RemoveVariable(const char *name, int environmentIndex)
    {
        ASSERT(environmentIndex < _environments.size());
        return _environments[environmentIndex]->RemoveVariable(name);
    }
    Value *FindVariable(const char *name, int environmentIndex)
    {
        ASSERT(environmentIndex < _environments.size());
        return _environments[environmentIndex]->FindVariable(name);
    }
    std::vector<std::unique_ptr<Environment>> _environments;

    Compiler _compiler;
};
