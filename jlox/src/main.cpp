#include <cstdio>

#include "vm.h"

int main(int argc, char **argv)
{
    common::unit_tests::run();

    sVM.init();

    if (argc == 1)
    {
        auto result = sVM.repl();
        if (!result.isOk())
        {
            LOG_ERROR("'%s'\n", result.error().message.c_str());
        }
    }
    else if (argc == 2)
    {
        auto result = sVM.runFile(argv[1]);
        if (!result.isOk())
        {
            LOG_ERROR(result.error().message);
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
    }

    sVM.finish();
    return 0;
}
