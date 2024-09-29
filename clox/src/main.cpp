#include <cstdio>

#include "common.h"
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

    VirtualMachine VM;
    VM.init();
    ScopedCallback vmFinish([&VM] { VM.finish(); });

    Compiler::Configuration compilerConfiguration;

#if USING(DEBUG_BUILD)
    if (0)
    {
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
            LOG_ERROR("%s\n", result.error().message().c_str());
        }
        LOG_INFO("< Quick test\n");
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
                code,
                default_const_variables,
                allow_dynamic_variables,
                repl,
                help,
            };
            Type type;
            const char* params = nullptr;
        };
#define ADD_PARAM(TYPE, DESC) {#TYPE, DESC, Param::Type::TYPE}
#define ADD_PARAM_WITH_PARAMS(TYPE, DESC, PARAMS) {#TYPE, DESC, Param::Type::TYPE, PARAMS}
        const Param params[] = {
            ADD_PARAM_WITH_PARAMS(code, "Allows passing <source_code> as a character string", "<source_code>"),
            ADD_PARAM(allow_dynamic_variables,
                      "Allows dynamic creation of variables on use (i.e., variable declaration not required)"),
            ADD_PARAM(default_const_variables,
                      "Variables are const by default, requiring <mut> modifier to be writable"),
            ADD_PARAM(repl, "Enters interactive mode(i.e. REPL)"),
            ADD_PARAM(help, "Shows this help"),
        };
#undef ADD_PARAM
        auto showHelpFunc = [&]()
        {
            fprintf(stderr, "Usage: %s [source_path]\n", argv[0]);
            size_t longerArg = 0;
            for (const Param& param : params)
            {
                longerArg = std::max<size_t>(longerArg, strlen(param.arg));
            }
            for (const Param& param : params)
            {
                const size_t spaceCount =
                    longerArg - (strlen(param.arg) + (param.params ? strlen(param.params) + 1 : 0));
                fprintf(stderr, "\t-%s%s%s", param.arg, param.params ? " " : "", param.params ? param.params : "");
                fprintf(stderr, "%*s\t%s\n", (int)spaceCount, " ", param.desc);
            }
        };

        /////////////////////////////////////////////////////////////////////////////////

        bool hasToShowHelp = false;
        const char* srcCodeOrFile = nullptr;
        bool isCodeOrFile = false;
        bool isREPL = false;

        for (const char** argvPtr = &argv[1]; argvPtr != &argv[argc]; ++argvPtr)
        {
            const char* argv = *argvPtr;
            if (argv[0] == '-')
            {
                bool validParam = false;
                for (const Param& param : params)
                {
                    if (0 == strcmp(&argv[1], param.arg))
                    {
                        validParam = true;

                        switch (param.type)
                        {
                            case Param::Type::code:
                                isCodeOrFile = true;
                                break;
                            case Param::Type::allow_dynamic_variables:
                                compilerConfiguration.allowDynamicVariables = true;
                                break;
                            case Param::Type::default_const_variables:
                                compilerConfiguration.defaultConstVariables = true;
                                break;
                            case Param::Type::repl:
                                isREPL = true;
                                break;
                            case Param::Type::help:
                                hasToShowHelp = true;
                                break;
                            default:
                                LOG_ERROR("Invalid parameter: %s", argv);
                                validParam = false;
                                break;
                        }
                    }
                }
                if (!validParam)
                {
                    LOG_ERROR("Invalid parameter: %s\n", argv);
                    hasToShowHelp = true;
                    resultCode = -1;
                    break;
                }
            }
            else
            {
                if (srcCodeOrFile != nullptr)
                {
                    LOG_ERROR("Parameter '%s' is unexpected, <%s> is already defined: '%s'\n", argv,
                              isCodeOrFile ? "source_code" : "source_path", srcCodeOrFile);
                    hasToShowHelp = true;
                    resultCode = -1;
                    break;
                }
                srcCodeOrFile = *argvPtr;
            }
        }

        if (hasToShowHelp)
        {
            showHelpFunc();
        }
        else if (isREPL)
        {
#if USING(DEBUG_BUILD)
            util::SetDebugBreakEnabled(false);
#endif  // #if USING(DEBUG_BUILD)
            auto result = VM.repl(compilerConfiguration);
            if (!result.isOk())
            {
                resultCode = -1;
                LOG_ERROR("%s\n", result.error().message().c_str());
            }
        }
        else
        {
            if (!srcCodeOrFile)
            {
                resultCode = -1;
                LOG_ERROR("Missing %s\n", isCodeOrFile ? "source_code parameter" : "source_path");
                showHelpFunc();
            }
            else if (isCodeOrFile)
            {
                auto result = VM.runFromSource(srcCodeOrFile, compilerConfiguration);
                if (!result.isOk())
                {
                    resultCode = -1;
                    LOG_ERROR(result.error().message().c_str());
                }
            }
            else
            {
                auto result = VM.runFromFile(srcCodeOrFile, compilerConfiguration);
                if (!result.isOk())
                {
                    resultCode = -1;
                    LOG_ERROR(result.error().message().c_str());
                }
            }
        }
    }

    return resultCode;
}
