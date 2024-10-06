/*
https://craftinginterpreters.com/a-virtual-machine.html
*/
#pragma once

#include <cstdlib>
#include <cstring>
#include <memory>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "environment.h"
#include "utils/common.h"

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

    VirtualMachine(const VirtualMachine &) = delete;
    VirtualMachine(VirtualMachine &&) = delete;
    VirtualMachine& operator=(const VirtualMachine&) = delete;
    VirtualMachine& operator=(VirtualMachine&&) = delete;

    result_t init()
    {
        _environments.push_back(std::make_unique<Environment>());
        for (auto &env : _environments)
        {
            env->Init();
        }

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

#if DEBUG_TRACE_EXECUTION
        const bool linesAvailable = _chunk->getLineCount() > 0;
        for (;;)
        {
            if (_compiler.getConfiguration().disassemble)
            {
                printf("          ");
                stackPrint();
                printf("          ");
                printVariables();
                printf("          ");
                _chunk->printConstants();
                disassembleInstruction(*_chunk, static_cast<uint16_t>(_ip - _chunk->getCode()), linesAvailable);
            }
#else   // #if DEBUG_TRACE_EXECUTION
        for (;;)
        {
#endif  // #else // #if DEBUG_TRACE_EXECUTION
            const OpCode instruction = OpCode(READ_BYTE());
            switch (instruction)
            {
                case OpCode::Return: return InterpretResult::Ok;
                case OpCode::Constant:
                {
                    const Value constant = READ_CONSTANT();
                    stackPush(constant);
                }
                break;

                case OpCode::Null: stackPush(Value::Create(Value::Null)); break;
                case OpCode::True: stackPush(Value::Create(true)); break;
                case OpCode::False: stackPush(Value::Create(false)); break;

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
                case OpCode::Divide: BINARY_OP(/); break;
                case OpCode::Multiply: BINARY_OP(*); break;
                case OpCode::Subtract: BINARY_OP(-); break;
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
                case OpCode::Not: stackPush(Value::Create(stackPop().isFalsey())); break;
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
                    Value      *value   = addVariable(varName, kGlobalEnvironmentIndex);
                    *value              = stackPop();  // null or expression :)
                    break;
                }
                case OpCode::GlobalVarGet:
                {
                    const char *varName = READ_STRING();
                    Value      *value   = findVariable(varName, kGlobalEnvironmentIndex);
                    if (value == nullptr)
                    {
                        return runtimeError("Trying to read undeclared variable '%s'.", varName);
                    }
                    else if (value->is(Value::Type::Null))
                    {
                        return runtimeError("Trying to read undefined variable '%s'.", varName);
                    }
                    stackPush(*value);
                    break;
                }
                case OpCode::GlobalVarSet:
                {
                    const char *varName = READ_STRING();
                    Value      *value   = findVariable(varName, kGlobalEnvironmentIndex);
                    if (value == nullptr)
                    {
                        if (_compiler.getConfiguration().allowDynamicVariables)
                        {
                            value = addVariable(varName, kGlobalEnvironmentIndex);
                        }
                        else
                        {
                            return runtimeError("Trying to write to undeclared variable '%s'.", varName);
                        }
                    }
                    *value = peek(0);
                    break;
                }
                case OpCode::Assignment:
                {
                    const Value rvalue   = stackPop();
                    const char *varName  = READ_STRING();
                    Value      *varValue = nullptr;
                    // check local variables
                    // ...
                    if (varValue == nullptr)
                    {
                        varValue = findVariable(varName, kGlobalEnvironmentIndex);
                    }
                    if (varValue == nullptr)
                    {  // allow dynamic creation ?
                        varValue = addVariable(varName, kGlobalEnvironmentIndex);
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
                default: break;
            }
        }

        return makeResult<result_t>(InterpretResult::Error);
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
    }

    result_t interpret(const char *source, const char *sourcePath,
                       Optional<Compiler::Configuration> optConfiguration = none_t)
    {
        if (optConfiguration.hasValue())
        {
            _compiler.setConfiguration(optConfiguration.value());
        }

        Compiler::result_t result = _compiler.compile(source, sourcePath);
        if (!result.isOk())
        {
            return makeResultError<result_t>(ErrorCode::CompileError, result.error().message());
        }
        const Chunk *currentChunk = result.value().get();
        _chunk                    = currentChunk;
        _ip                       = currentChunk->getCode();

        result_t runResult = run();

        return runResult;
    }

    result_t runFromSource(const char *sourceCode, Optional<Compiler::Configuration> optConfiguration = none_t)
    {
        result_t result = interpret(sourceCode, "SOURCE", optConfiguration);
        return result;
    }

    result_t runFromFile(const char *path, Optional<Compiler::Configuration> optConfiguration = none_t)
    {
        Result<std::unique_ptr<char[]>> source = utils::readFile(path);
        if (!source.isOk())
        {
            return makeResultError<result_t>(source.error().message());
        }
        char *buffer = source.value().get();

        result_t result = interpret(buffer, path, optConfiguration);
        if (!result.isOk())
        {
            LOG_ERROR("%s", result.error().message().c_str());
        }

        return result;
    }

    result_t runFromByteCode(const Chunk &bytecode)
    {
        _chunk = &bytecode;
        _ip    = bytecode.getCode();
        return run();
    }

    result_t repl(Optional<Compiler::Configuration> optConfiguration = none_t)
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
#if USING(DEBUG_PRINT_CODE)
                    printf("\tsetdebugprintlevel <N>\n");
                    printf("\tgetdebugprintlevel\n");
#endif  // #if USING(DEBUG_PRINT_CODE)
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
                    printf("DebugBreak <- %s\n", enable ? "enabled" : "disabled");
                    printf("--------------------------------\n");
                    continue;
                }
#if USING(DEBUG_PRINT_CODE)
                else if (strstr(line, "setdebugprintlevel"))
                {
                    char *secondArg = strstr(line, " ");
                    if (secondArg != nullptr)
                    {
                        const int level = atoi(secondArg + 1);
                        debug_print::SetLevel(level);
                        printf("--------------------------------\n");
                        printf("SetDebugPrintLevel <- %d\n", level);
                        printf("--------------------------------\n");
                    }
                    continue;
                }
                else if (strstr(line, "getdebugprintlevel"))
                {
                    const int level = debug_print::GetLevel();
                    printf("--------------------------------\n");
                    printf("GetDebugPrintLevel -> %d\n", level);
                    printf("--------------------------------\n");
                    continue;
                }
#endif  // #if USING(DEBUG_PRINT_CODE)
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
        char    message[256];
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        va_end(args);

        const Chunk   *chunk       = this->_chunk;
        const uint16_t instruction = static_cast<uint16_t>(this->_ip - chunk->getCode() - 1);
        const uint16_t line        = chunk->getLine(instruction);
        char           outputMessage[512];
        snprintf(outputMessage, sizeof(outputMessage), "[%s:%d] Runtime error: %s\n", chunk->getSourcePath(), line,
                 message);
        stackReset();
        return makeResultError<result_t>(result_t::error_t::code_t::RuntimeError, outputMessage);
    }

   protected:  // Stack
    static constexpr size_t STACK_SIZE = 1024;
    Value                   _stack[STACK_SIZE];
    Value                  *_stackTop = &_stack[0];

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
    size_t stackSize() const
    {
        ASSERT(_stackTop >= _stack);
        return static_cast<size_t>(_stackTop - _stack);
    }
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

    void printVariables() const
    {
        printf(" Global variables: ");
        _environments.front()->print();
    }
#endif  // #if DEBUG_TRACE_EXECUTION

   protected:  // STATE
    const Chunk   *_chunk = nullptr;
    const uint8_t *_ip    = nullptr;

    Value *addVariable(const char *name, int environmentIndex)
    {
        ASSERT(environmentIndex < _environments.size());
        return _environments[environmentIndex]->addVariable(name);
    }
    bool removeVariable(const char *name, int environmentIndex)
    {
        ASSERT(environmentIndex < _environments.size());
        return _environments[environmentIndex]->removeVariable(name);
    }
    Value *findVariable(const char *name, int environmentIndex)
    {
        ASSERT(environmentIndex < static_cast<int>(_environments.size()));
        return _environments[environmentIndex]->findVariable(name);
    }
    std::vector<std::unique_ptr<Environment>> _environments;

    Compiler _compiler;
};
