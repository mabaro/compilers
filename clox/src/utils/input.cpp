#include "input.h"
#include "config.h"

#if defined(LINUX_OS)
#include <cstdlib>
#include <cstdio>
#elif defined(WINDOWS_OS)
#include <conio.h> //getch()
#endif

char read_char()
{
#if defined(LINUX_OS)
    system("stty raw");
    const char c = getchar();
    system("stty cooked");
#elif defined(WINDOWS_OS)
    const char c = static_cast<char>(_getch());
#endif
    return c;
}
