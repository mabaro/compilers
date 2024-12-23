
#include <fstream>
#include <iostream>
#include <sstream>

#include "header.h"
#include "utils/byte_buffer.h"
#include "utils/common.h"
#include "vm.h"

#ifdef _DEBUG
#define UNIT_TESTS_ENABLED 0
#else  // #ifdef _DEBUG
#define UNIT_TESTS_ENABLED 0
#endif  // #else // #ifdef _DEBUG

int main(int argc, const char* argv[])
{
    int resultCode = 0;

#if UNIT_TESTS_ENABLED
    printf(">>>>>> Unit tests\n");
    unit_tests::common::run();
    unit_tests::scanner::run();
    printf("<<<<<< Unit tests\n");
#endif  // #if 0

    Compiler                      compiler;
    Compiler::Configuration       compilerConfiguration;
    VirtualMachine::Configuration virtualMachineConfiguration;

    auto errorReportFunc =
        [&resultCode](const char* errorMessage, int errorCode = -1, std::function<void()> postErrorCallback = nullptr)
    {
        resultCode = errorCode;
        LOG_ERROR("(CODE: %d) %s\n", errorCode, errorMessage);
        if (postErrorCallback)
        {
            postErrorCallback();
        }
        return resultCode;
    };

#if USING(DEBUG_BUILD)
    if (0)
    {
        VirtualMachine VM;
        VM.init(virtualMachineConfiguration);
        ScopedCallback vmFinish([&VM] { VM.finish(); });

        // const char codeStr[] = "(-1 + 2) - 4 * 3 / ( -5 - 6 + 35)";
        // const char codeStr[] = "print !true";
        // const char codeStr[] = "!(5 - 4 > 0 * 2 == !false)";
        // const char codeStr[] = "print (\"true is: \"); print(true); print(\"\n\")";
        // const char codeStr[] = "var a = 1; print (\"var a: \"); print(a); print(\"\n\")";
        // const char codeStr[] = "a = 1; print (\"Testing variables functionality: 'var a: \"); print(a);
        // print(\"'\n\")"; const char codeStr[] = "var a; var b; var c; a = 1; b=2; c=a+b; print (\"var a: \");
        // print(a); print (\"var b: \"); print(b); print (\"var c: \"); print(c); print(\"\n\")";
        // const char codeStr[] = "var a=1; var b; var c; b=2; c=a + b; print (\"var c: \"); print(c); print(\"\n\");";
        const char codeStr[] = "var a;var b; var c; var d; a*b=c+d;";
        // const char codeStr[] = "print true";
        LOG_INFO("> Quick test: [%s]\n", codeStr);
        auto result = VM.interpret(codeStr, "QUICK_TESTS", compilerConfiguration);
        if (!result.isOk())
        {
            return errorReportFunc(result.error().message().c_str());
        }
    }
    else
#endif  // #else  // #if USING(DEBUG_BUILD)
    {
        struct Param
        {
            const char* arg;
            const char* desc;
            enum class Type
            {
                help,
                allow_dynamic_variables,
                default_const_variables,
#if USING(EXTENDED_ERROR_REPORT)
                extended_errors,
#endif  // #if USING(EXTENDED_ERROR_REPORT)
                code,
                disassemble,
                step_debugging,
                repl,
                compile,
                run,
                output,
            };
            Type        type;
            const char* params = nullptr;
        };

#define ADD_PARAM(TYPE, DESC) {#TYPE, DESC, Param::Type::TYPE}
#define ADD_PARAM_WITH_PARAMS(TYPE, DESC, PARAMS) {#TYPE, DESC, Param::Type::TYPE, PARAMS}
        const Param params[] = {
            ADD_PARAM(help, "Shows this help"),
            ADD_PARAM(allow_dynamic_variables,
                      "Allows dynamic creation of variables on use (i.e., variable declaration not required)"),
            ADD_PARAM(default_const_variables,
                      "Variables are const by default, requiring <mut> modifier to be writable"),
#if USING(EXTENDED_ERROR_REPORT)
            ADD_PARAM_WITH_PARAMS(extended_errors, "Show extended error reporting", "<0 / 1>"),
#endif  // #if USING(EXTENDED_ERROR_REPORT)
            ADD_PARAM(disassemble, "Show disassembled code"),
            ADD_PARAM(step_debugging, "Step-by-step execution"),
            ADD_PARAM(repl, "Enters interactive mode(i.e. REPL)"),
            ADD_PARAM(compile, "Compiles into bytecode and outputs the result to console or the output_file defined"),
            ADD_PARAM_WITH_PARAMS(output, "Allows defining the output file for -compile", "<output_file>"),
            ADD_PARAM(run, "Runs the input code through the VM"),
            ADD_PARAM_WITH_PARAMS(code, "Allows passing <source_code> as a character string", "<source_code>"),
        };
#undef ADD_PARAM
        auto showHelpFunc = [&](std::ostream& ostr)
        {
            ostr << "CLOX-variant tools (compiler / interpreter / REPL / VM) version " << VERSION << std::endl;
            ostr << format("Usage: %s [arguments] [filepath]\n", argv[0]);
            size_t longerArg = 0;
            for (const Param& param : params)
            {
                longerArg = std::max<size_t>(longerArg, strlen(param.arg));
            }
            for (const Param& param : params)
            {
                const size_t spaceCount =
                    longerArg - (strlen(param.arg) + (param.params ? strlen(param.params) + 1 : 0));
                ostr << format("\t-%s%s%s", param.arg, param.params ? " " : "", param.params ? param.params : "");
                ostr << format("%*s\t%s\n", (int)spaceCount, " ", param.desc);
            }
        };
        auto errorReportWithHelpFunc = [&](const char* msg, int32_t errCode = -1)
        {
            auto result = errorReportFunc(msg, errCode);
            showHelpFunc(std::cerr);
            return result;
        };

        /////////////////////////////////////////////////////////////////////////////////

        enum class ExecutionMode
        {
            Compile,
            Run,
            Interpret,
            REPL,
        };
        enum class OutputMode
        {
            File,
            Console
        };

        struct
        {
            bool          hasToShowHelp     = false;
            const char*   srcCodeOrFile     = nullptr;
            bool          isCodeOrFile      = false;
            ExecutionMode mode              = ExecutionMode::Interpret;
            const char*   compileOutputPath = nullptr;
        } config;

        auto        isArgFunc = [](const char* arg) { return (arg[0] == '-'); };
        const char* lastArg   = argv[argc - 1];
        for (const char** argvPtr = &argv[1]; *argvPtr != argv[argc]; ++argvPtr)
        {
            const char* curArg = *argvPtr;
            if (isArgFunc(curArg))
            {
                bool validParam = false;
                for (const Param& param : params)
                {
                    if (0 == strcmp(&curArg[1], param.arg))
                    {
                        validParam = true;

                        switch (param.type)
                        {
                            case Param::Type::code:
                            {
                                config.isCodeOrFile = true;
                                if (*argvPtr == lastArg)
                                {
                                    return errorReportWithHelpFunc(
                                        format("Mising parameter for %s <code_string>", curArg).c_str());
                                }
                                if (!isArgFunc(*(argvPtr + 1)))
                                {
                                    config.srcCodeOrFile = *(++argvPtr);
                                }
                                break;
                            }
                            case Param::Type::output:
                            {
                                if (config.mode != ExecutionMode::Compile)
                                {
                                    return errorReportWithHelpFunc(
                                        format("Unexpected output file. Only used with -compile: %s", curArg).c_str());
                                }
                                if (!isArgFunc(*(argvPtr + 1)))
                                {
                                    config.compileOutputPath = *(++argvPtr);
                                }
                                break;
                            }
                            case Param::Type::allow_dynamic_variables:
                                compilerConfiguration.allowDynamicVariables = true;
                                break;
                            case Param::Type::default_const_variables:
                                compilerConfiguration.defaultConstVariables = true;
                                break;
#if USING(EXTENDED_ERROR_REPORT)
                            case Param::Type::extended_errors:
                                if (!isArgFunc(*(argvPtr + 1)))
                                {
                                    compilerConfiguration.extendedErrorReport = **(++argvPtr) == '1';
                                }
                                break;
#endif  // #if USING(EXTENDED_ERROR_REPORT)
                            case Param::Type::repl: config.mode = ExecutionMode::REPL; break;
                            case Param::Type::help: config.hasToShowHelp = true; break;
                            case Param::Type::compile: config.mode = ExecutionMode::Compile; break;
                            case Param::Type::run:
                                config.mode         = ExecutionMode::Run;
                                config.isCodeOrFile = false;
                                break;
                            case Param::Type::disassemble: compilerConfiguration.disassemble = true; break;
                            case Param::Type::step_debugging: virtualMachineConfiguration.stepByStep = true; break;
                            default: validParam = false; break;
                        }
                    }
                }
                if (!validParam)
                {
                    return errorReportWithHelpFunc(format("Invalid parameter: %s", curArg).c_str());
                }
            }
            else
            {
                if (config.srcCodeOrFile != nullptr)
                {
                    return errorReportWithHelpFunc(format("Parameter '%s' is unexpected, <%s> is already defined: '%s'",
                                                          curArg, config.isCodeOrFile ? "source_code" : "source_path",
                                                          config.srcCodeOrFile)
                                                       .c_str());
                }
                config.srcCodeOrFile = *argvPtr;
            }
        }

        if (config.hasToShowHelp)
        {
            showHelpFunc(std::cout);
        }
        else if (config.mode == ExecutionMode::REPL)
        {
            VirtualMachine VM;
            VM.init(virtualMachineConfiguration);
            ScopedCallback vmFinish([&VM] { VM.finish(); });

#if USING(DEBUG_BUILD)
            util::SetDebugBreakEnabled(false);
#endif  // #if USING(DEBUG_BUILD)
            auto result = VM.repl(compilerConfiguration);
            if (!result.isOk())
            {
                return errorReportWithHelpFunc(format("%s", result.error().message().c_str()).c_str());
            }
        }
        else
        {
            if (config.srcCodeOrFile == nullptr)
            {
                return errorReportWithHelpFunc(
                    format("Missing %s", config.isCodeOrFile ? "source_code parameter" : "source_path").c_str());
            }
            if (config.mode == ExecutionMode::Compile)
            {
                ObjectFunction* function = nullptr;
                if (config.isCodeOrFile)
                {
                    auto result = compiler.compileFromSource(config.srcCodeOrFile, compilerConfiguration);
                    if (!result.isOk())
                    {
                        return errorReportFunc(result.error().message().c_str());
                    }
                    function = result.extract();
                }
                else
                {
                    auto result = compiler.compileFromFile(config.srcCodeOrFile, compilerConfiguration);
                    if (!result.isOk())
                    {
                        return errorReportFunc(result.error().message().c_str());
                    }
                    function = result.extract();
                }
                ASSERT(function != nullptr);

                if (config.compileOutputPath != nullptr)
                {
                    std::ofstream ofs(config.compileOutputPath,
                                      std::ofstream::binary | std::ofstream::trunc | std::ofstream::out);
                    if (ofs.bad())
                    {
                        return errorReportFunc(
                            format("Failed to open file '%s' for writing", config.compileOutputPath).c_str());
                    }
                    auto serializeResult = function->serialize(ofs);
                    if (!serializeResult.isOk())
                    {
                        return errorReportFunc(format("Failed serializing to file '%s': %s", config.compileOutputPath,
                                                      serializeResult.error().message().c_str())
                                                   .c_str());
                    }
                }
                else
                {
                    auto serializeResult = function->serialize(std::cout);
                    if (!serializeResult.isOk())
                    {
                        return errorReportFunc(
                            format("Failed serializing: %s", serializeResult.error().message().c_str()).c_str());
                    }
                }
            }
            else
            {
                VirtualMachine VM;
                VM.init(virtualMachineConfiguration);
                ScopedCallback vmFinish([&VM] { VM.finish(); });

                if (config.mode == ExecutionMode::Run)
                {
                    ASSERT(config.isCodeOrFile == false);

                    ObjectFunction* function = nullptr;
                    function                 = ObjectFunction::Create(config.srcCodeOrFile);
                    std::ifstream ifs(config.srcCodeOrFile, std::ifstream::binary);
                    if (!ifs.is_open() || !ifs.good())
                    {
                        return errorReportFunc(
                            format("Failed to open file '%s' for reading", config.srcCodeOrFile).c_str());
                    }
                    auto deserializeResult = function->deserialize(ifs);
                    if (!deserializeResult.isOk())
                    {
                        return errorReportFunc(
                            format("Failed loading bytecode: %s", deserializeResult.error().message().c_str()).c_str());
                    }

                    if (compilerConfiguration.disassemble)
                    {
                        disassemble(function->chunk, config.srcCodeOrFile);
                    }

                    auto result = VM.runFromByteCode(function->chunk);
                    if (!result.isOk())
                    {
                        resultCode = -1;
                        return errorReportFunc(result.error().message().c_str());
                    }
                }
                else if (config.mode == ExecutionMode::Interpret)
                {
                    auto result = config.isCodeOrFile ? VM.runFromSource(config.srcCodeOrFile, compilerConfiguration)
                                                      : VM.runFromFile(config.srcCodeOrFile, compilerConfiguration);
                    if (!result.isOk())
                    {
                        resultCode = -1;
                        return errorReportFunc(result.error().message().c_str());
                    }
                }
            }
        }
    }
    return resultCode;
}