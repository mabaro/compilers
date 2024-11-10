#include "input.h"

#include "config.h"

#if defined(LINUX_OS)
#include <cstdio>
#include <cstdlib>
#elif defined(WINDOWS_OS)
#include <conio.h>  //getch()
#endif

char read_char()
{
#if defined(LINUX_OS)
    [[maybe_unused]] system("stty raw");
    const char c = getchar();
    [[maybe_unused]] system("stty cooked");
#elif defined(WINDOWS_OS)
    const char c = static_cast<char>(_getch());
#endif
    return c;
}
