#include <cstdio>

#include "vm.h"


#include "common.h"

#define UNIT_TESTS_ENABLED 0

int main(int argc, char **argv)
{
#if UNIT_TESTS_ENABLED
    printf(">>>>>> Unit tests\n");
    unit_tests::common::run();
    unit_tests::scanner::run();
    printf("<<<<<< Unit tests\n");
#endif // #if 0

    VirtualMachine VM;
    VM.init();

    //test//VM.interpret("marcos=13;");

    if (argc == 1)
    {
        auto result = VM.repl();
        if (!result.isOk())
        {
            LOG_ERROR("'%s'\n", result.error().message().c_str());
        }
    }
    else if (argc == 2)
    {
        auto result = VM.runFile(argv[1]);
        if (!result.isOk())
        {
            LOG_ERROR(result.error().message);
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
    }

    VM.finish();
    return 0;
}
