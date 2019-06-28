#include <windows.h>

extern "C"
{
    const char *foo()
    {
        Beep(330, 500);
        return 0;
    }

    const char *bar()
    {
        return "Произвольное сообщение об ошибке.";
    }
}
