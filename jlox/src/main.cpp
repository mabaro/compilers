#include <cstdio>

#include "vm.h"

int main(int argc, char **argv)
{
    common::unit_tests::run();

    initVM();

    if (argc == 1)
    {
        repl();
    }
    else if (argc == 2)
    {
        runFile(argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
    }

    freeVM();
    return 0;
}
