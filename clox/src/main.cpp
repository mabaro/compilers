#include <cstdio>

#include "common.h"
#include "vm.h"

#ifdef _DEBUG
#define UNIT_TESTS_ENABLED 1
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
        const char codeStr[] = "var a=1; var b; var c; b=2; c=a + b; print (\"var c: \"); print(c); print(\"\n\");";
        // const char codeStr[] = "print true";
        LOG_INFO("> Quick test: [%s]\n", codeStr);
        auto result = VM.interpret(codeStr, "QUICK_TESTS");
        if (!result.isOk())
        {
            LOG_ERROR("%s\n", result.error().message().c_str());
        }
        LOG_INFO("< Quick test\n");
    }
    else
#endif  // #else  // #if USING(DEBUG_BUILD)
        if (argc > 1)
        {
            if (0 == strcmp(argv[1], "-repl"))
            {
                auto result = VM.repl();
                if (!result.isOk())
                {
                    LOG_ERROR("%s\n", result.error().message().c_str());
                }
            }
            else
            {
                const char* prevArg = "";
                const char* srcCodeOrFile = nullptr;
                bool isCodeOrFile = false;
                for (const char** argvPtr = &argv[1]; argvPtr != &argv[argc]; ++argvPtr)
                {
                    if (0 == strcmp(*argvPtr, "-code"))
                    {
                        isCodeOrFile = true;
                    }
                    else
                    {
                        srcCodeOrFile = *argvPtr;
                    }

                    prevArg = *argvPtr;
                }

                if (isCodeOrFile)
                {
                    auto result = VM.runFromSource(srcCodeOrFile);
                    if (!result.isOk())
                    {
                        resultCode = -1;
                        LOG_ERROR(result.error().message().c_str());
                    }
                }
                else
                {
                    auto result = VM.runFromFile(srcCodeOrFile);
                    if (!result.isOk())
                    {
                        resultCode = -1;
                        LOG_ERROR(result.error().message().c_str());
                    }
                }
            }
        }
        else
        {
            fprintf(stderr, "Usage: %s [source_path] [-code <code_source>] [-repl]\n", argv[0]);
        }

    VM.finish();
    return resultCode;
}
