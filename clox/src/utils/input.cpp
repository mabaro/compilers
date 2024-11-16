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
    [[maybe_unused]] int result = system("stty raw");
    const char c = getchar();
    result =  system("stty cooked");
#elif defined(WINDOWS_OS)
    const char c = static_cast<char>(_getch());
#endif
    return c;
}
