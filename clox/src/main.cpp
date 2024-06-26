#include <cstdio>

#include "common.h"
#include "vm.h"

#ifdef _DEBUG
#define UNIT_TESTS_ENABLED 1
#else  // #ifdef _DEBUG
#define UNIT_TESTS_ENABLED 0
#endif  // #else // #ifdef _DEBUG

int main(int argc, char **argv)
{
#if UNIT_TESTS_ENABLED
    printf(">>>>>> Unit tests\n");
    unit_tests::common::run();
    unit_tests::scanner::run();
    printf("<<<<<< Unit tests\n");
#endif  // #if 0

    VirtualMachine VM;
    VM.init();

    const bool interactiveRepl = true;
    if (!interactiveRepl)  // quick tests
    {
        // const char codeStr[] = "(-1 + 2) - 4 * 3 / ( -5 - 6 + 35)";
        // const char codeStr[] = "print !true";
        const char codeStr[] = "!(5 - 4 > 0 * 2 == !false)";
        // const char codeStr[] = "print true";
        LOG_INFO("> Quick test: '%s'...\n", codeStr);
        auto result = VM.interpret(codeStr, "QUICK_TESTS");
        if (!result.isOk())
        {
            LOG_ERROR("%s\n", result.error().message().c_str());
        }
        LOG_INFO("< Quick test\n");
    }
    else if (argc == 1)
    {
        auto result = VM.repl();
        if (!result.isOk())
        {
            LOG_ERROR("%s\n", result.error().message().c_str());
        }
    }
    else if (argc == 2)
    {
        auto result = VM.runFile(argv[1]);
        if (!result.isOk())
        {
            LOG_ERROR(result.error().message().c_str());
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
    }

    VM.finish();
    return 0;
}
