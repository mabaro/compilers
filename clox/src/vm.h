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

#if DEBUG_TRACE_EXECUTION
#include "utils/input.h"
#endif  // #if DEBUG_TRACE_EXECUTION

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

    struct Configuration
    {
        bool stepByStep = false;
    };

    Configuration _configuration;

    ///////////////////////////////////////////////////////////////////////////////////

    VirtualMachine() {}

    ~VirtualMachine() {}

    VirtualMachine(const VirtualMachine &)            = delete;
    VirtualMachine(VirtualMachine &&)                 = delete;
    VirtualMachine &operator=(const VirtualMachine &) = delete;
    VirtualMachine &operator=(VirtualMachine &&)      = delete;

    void setConfiguration(const Configuration &config) { _configuration = config; }

    const Configuration &getConfiguration() const { return _configuration; }

    result_t init(const Configuration &configuration)
    {
        _configuration = configuration;

        ASSERT(_environments.empty());
        _environments.push_back(std::make_unique<Environment>());
        _currrentEnvironment = _environments.back().get();
        _environments.back()->Init(nullptr);

        return makeResult<result_t>(InterpretResult::Ok);
    }

    void init() { init(_configuration); }

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
#define READ_U8() (*_ip++)
#define READ_OFFSET16() (_ip += 2, (int16_t)((_ip[-2] << 8) | _ip[-1]))
#define READ_CONSTANT() (_chunk->getConstants()[READ_U8()])
#define READ_STRING() (READ_CONSTANT().as.object->asString()->chars)
#define BINARY_OP(op)               \
    do                              \
    {                               \
        const Value b = stackPop(); \
        const Value a = stackPop(); \
        stackPush(a op b);          \
    } while (false)

#if DEBUG_TRACE_EXECUTION
        if (_compiler.getConfiguration().disassemble)
        {
            printf("== VM ==\n");
        }
        const bool linesAvailable = _chunk->getLineCount() > 0;
        uint16_t   scopeCount     = 0;
        bool       stepDebugging  = _configuration.stepByStep;
        for (;;)
        {
            if (_compiler.getConfiguration().disassemble)
            {
                static bool sWasPrint = false;
                if (sWasPrint)
                {
                    printf("\n");
                    sWasPrint = false;
                }
                const char *padding = "          ";
                printStack(padding);
                printVariables(padding);
                _chunk->printConstants(padding);
                OpCode instruction = OpCode::Undefined;
                disassembleInstruction(*_chunk, static_cast<uint16_t>(_ip - _chunk->getCode()), linesAvailable,
                                       &scopeCount, &instruction);
                if (instruction == OpCode::Print)
                {
                    sWasPrint = true;
                    printf("[output]");
                }
                if (stepDebugging)
                {
                    // todo : time machine (prev/next IP)
                    // static std::vector<codepos_t> sTimeMachine;
                    while (true)
                    {
                        const char c = read_char();
                        if (c == 'n')
                        {
                            // sTimeMachine.push_back(_ip - _chunk->getCode());
                            break;
                        }
                        // WIP - going back requires restoring previous state
                        // else if (c == 'p')
                        // {
                        //     _ip = sTimeMachine.back() + _chunk->getCode();
                        //     sTimeMachine.pop_back();
                        //     break;
                        // }
                        else if (c == 'q')
                        {
                            stepDebugging = false;
                            break;
                        }
                    }
                }
            }
#else   // #if DEBUG_TRACE_EXECUTION
        for (;;)
        {
#endif  // #else // #if DEBUG_TRACE_EXECUTION
            const OpCode instruction = OpCode(READ_U8());
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
                    Value      *value   = addVariable(varName);
                    *value              = stackPop();  // null or expression :)
                    break;
                }
                case OpCode::GlobalVarSet:
                {
                    const char *varName = READ_STRING();
                    Value      *value   = findVariable(varName);
                    if (value == nullptr)
                    {
                        if (_compiler.getConfiguration().allowDynamicVariables)
                        {
                            value = addVariable(varName);
                        }
                        else
                        {
                            return runtimeError("Trying to write to undeclared variable '%s'.", varName);
                        }
                    }
                    *value = peek(0);
                    break;
                }
                case OpCode::GlobalVarGet:
                {
                    const char *varName = READ_STRING();
                    Value      *value   = findVariable(varName);
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
                case OpCode::LocalVarSet:
                {
                    const uint8_t slot = READ_U8();
                    _stack[slot]       = peek(0);
                    break;
                }
                case OpCode::LocalVarGet:
                {
                    const uint8_t slot = READ_U8();
                    stackPush(_stack[slot]);
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
                        varValue = findVariable(varName);
                    }
                    if (varValue == nullptr)
                    {  // allow dynamic creation ?
                        varValue = addVariable(varName);
                        DEBUGPRINT_EX("Dynamic var(%s) created\n", varName);
                    }
                    if (varValue != nullptr)
                    {
                        *varValue = rvalue;
#if USING(DEBUG_BUILD)
                        DEBUGPRINT_EX("var(%s)=%s\n", varName);
                        printValueDebug(rvalue);
#endif  // #if USING(DEBUG_BUILD)
                    }
                    break;
                }
                case OpCode::Skip: break;

                case OpCode::Jump:
                {
                    const int16_t offset = READ_OFFSET16();
                    _ip += offset;
                    break;
                }
                case OpCode::JumpIfFalse:
                {
                    const int16_t offset = READ_OFFSET16();
                    if (peek(0).isFalsey())
                    {
                        _ip += offset;
                    }
                    break;
                }
                case OpCode::JumpIfTrue:
                {
                    const int16_t offset = READ_OFFSET16();
                    if (!peek(0).isFalsey())
                    {
                        _ip += offset;
                    }
                    break;
                }
                case OpCode::ScopeBegin:
                {
                    Environment *curEnv = _environments.back().get();
                    _environments.push_back(std::make_unique<Environment>());
                    _environments.back()->Init(curEnv);
                    _currrentEnvironment = _environments.back().get();
                    break;
                }
                case OpCode::ScopeEnd:
                {
                    _currrentEnvironment = _environments.back()->_parentEnvironment;
                    _environments.back()->Reset();
                    _environments.pop_back();
                    break;
                }
                case OpCode::Undefined:
                    FAIL();
                    return makeResultError<result_t>(ErrorCode::RuntimeError,
                                                     format("Undefined OpCode: %d", instruction));
            }
        }
#undef READ_U8
#undef READ_U16
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
        return interpret(sourceCode, "SOURCE", optConfiguration);
    }

    result_t runFromFile(const char *path, Optional<Compiler::Configuration> optConfiguration = none_t)
    {
        Result<std::unique_ptr<char[]>> source = utils::readFile(path);
        if (!source.isOk())
        {
            return makeResultError<result_t>(source.error().message());
        }
        char *buffer = source.value().get();

        return interpret(buffer, path, optConfiguration);
    }

    result_t runFromByteCode(const Chunk &bytecode)
    {
        _chunk = &bytecode;
        _ip    = bytecode.getCode();
        return run();
    }

    result_t repl(Optional<Compiler::Configuration> optConfiguration = none_t)
    {
        Compiler::Configuration compilerConfig =
            optConfiguration.hasValue() ? optConfiguration.value() : _compiler.getConfiguration();
        compilerConfig.isREPL = true;
        _compiler.setConfiguration(compilerConfig);

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
                if (nullptr != strstr(line, "help") || nullptr != strstr(line, "Help"))
                {
                    printf("--------------------------------\n");
                    printf("Commands(preceded with '!'):\n");
                    printf("\tAllowDynamicVar <0/1>\n");
#if DEBUG_TRACE_EXECUTION
                    printf("\tPrintConstants\n");
                    printf("\tPrintVariables\n");
#endif  // #if DEBUG_TRACE_EXECUTION
#if USING(DEBUG_BUILD)
                    printf("\tdebugbreak <enable/disable>\n");
#endif  // #if USING(DEBUG_BUILD)
#if USING(DEBUG_PRINT_CODE)
                    printf("\tDebugPrintLevel <N>\n");
#endif  // #if USING(DEBUG_PRINT_CODE)
                    printf("\tquit\n");
                    printf("--------------------------------\n");
                    continue;
                }
                else if (nullptr != strstr(line, "quit") || nullptr != strstr(line, "Quit"))
                {
                    printf("--------------------------------\n");
                    printf("Exiting...\n");
                    printf("--------------------------------\n");
                    break;
                }
                else if (nullptr != strstr(line, "allowdynamicvar") || nullptr != strstr(line, "AllowDynamicVar"))
                {
                    char                   *secondArg = strstr(line, " ");
                    Compiler::Configuration config    = _compiler.getConfiguration();
                    if (secondArg != nullptr && secondArg[1] != '\n')
                    {
                        const bool enable            = secondArg[1] == '1';
                        config.allowDynamicVariables = enable;
                        _compiler.setConfiguration(config);
                        printf("[CMD] AllowDynamicVar set to '%s'\n", enable ? "enabled" : "disabled");
                    }
                    else
                    {
                        printf("[CMD] AllowDynamicVar is '%s'\n",
                               config.allowDynamicVariables ? "enabled" : "disabled");
                    }
                    continue;
                }
#if DEBUG_TRACE_EXECUTION
                else if (nullptr != strstr(line, "printconstants") || nullptr != strstr(line, "PrintConstants"))
                {
                    printf("[CMD] Constants:\n");
                    if (_chunk)
                    {
                        _chunk->printConstants();
                    }
                    continue;
                }
                else if (nullptr != strstr(line, "printvariables") || nullptr != strstr(line, "PrintVariables"))
                {
                    printf("[CMD] Variables:\n");
                    this->printVariables();
                    continue;
                }
#endif  // #if DEBUG_TRACE_EXECUTION
#if USING(DEBUG_BUILD)
                else if (nullptr != strstr(line, "debugbreak") || nullptr != strstr(line, "DebugBreak"))
                {
                    char *secondArg = strstr(line, " ");
                    if (secondArg != nullptr && secondArg[1] != '\n')
                    {
                        const bool enable = secondArg[1] == '1';
                        util::SetDebugBreakEnabled(enable);
                        printf("[CMD] DebugBreak set to '%s'\n", enable ? "enabled" : "disabled");
                    }
                    else
                    {
                        printf("[CMD] DebugBreak is '%s'\n", util::IsDebugBreakEnabled() ? "enabled" : "disabled");
                    }
                    continue;
                }
#endif  // #if !USING(DEBUG_BUILD)
#if USING(DEBUG_PRINT_CODE)
                else if (nullptr != strstr(line, "debugprintlevel") || nullptr != strstr(line, "DebugPrintLevel"))
                {
                    char *secondArg = strstr(line, " ");
                    if (secondArg != nullptr)
                    {
                        const int level = atoi(secondArg + 1);
                        debug_print::SetLevel(level);
                        printf("[CMD] DebugPrintLevel set to '%d'\n", level);
                    }
                    else
                    {
                        const int level = debug_print::GetLevel();
                        printf("[CMD] DebugPrintLevel is '%d'\n", level);
                    }
                    continue;
                }
#endif  // #if USING(DEBUG_PRINT_CODE)
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
    const Value &peek(uint32_t distance) const
    {
        ASSERT(distance < stackSize());
        const Value &value = *(_stackTop - 1 + distance);
        return value;
    }

    result_t runtimeError(const char *format, ...)
    {
        char    message[256];
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        va_end(args);

        const Chunk *chunk       = this->_chunk;
        const auto   instruction = static_cast<codepos_t>(this->_ip - chunk->getCode() - 1);
        const size_t line        = chunk->getLine(instruction);
        char         outputMessage[512];
        snprintf(outputMessage, sizeof(outputMessage), "[%s:%zu] Runtime error: %s\n", chunk->getSourcePath(), line,
                 message);
        FAIL_MSG(outputMessage);
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
    void printStack(const char *padding = "") const
    {
        if (_stack == _stackTop)
        {
            return;
        }

        printf("%s", padding);
        printf("Stack");
        for (const Value *slot = _stack; slot < _stackTop; ++slot)
        {
            printf("[");
            printValueDebug(*slot);
            printf("]");
        }
        printf("\n");
    }

    void printVariables(const char *padding = "") const
    {
        ASSERT(_currrentEnvironment);
        if (_currrentEnvironment->getVariableCount() > 0)
        {
            printf("%s", padding);
            printf("Variables ");
            _currrentEnvironment->print();
        }
    }
#endif  // #if DEBUG_TRACE_EXECUTION

   protected:  // STATE
    const Chunk   *_chunk = nullptr;
    const uint8_t *_ip    = nullptr;

    Value *addVariable(const char *name)
    {
        ASSERT(_currrentEnvironment);
#if USING(DEBUG_TRACE_EXECUTION)
        if (_compiler.getConfiguration().debugPrintVariables)
        {
            _currrentEnvironment->print();
        }
#endif  // #if USING(DEBUG_TRACE_EXECUTION)

        return _currrentEnvironment->addVariable(name);
    }

    bool removeVariable(const char *name)
    {
        ASSERT(_currrentEnvironment);
        return _currrentEnvironment->removeVariable(name);
    }

    Value *findVariable(const char *name)
    {
        ASSERT(_currrentEnvironment);
        return _currrentEnvironment->findVariable(name);
    }

    std::vector<std::unique_ptr<Environment>> _environments;
    Environment                              *_currrentEnvironment = nullptr;

    Compiler _compiler;
};
