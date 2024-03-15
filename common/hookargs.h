#ifndef HOOKARGS_H_INCLUDED
    #define HOOKARGS_H_INCLUDED 1

#include <windows.h>

struct hookargs {
    HANDLE Parent;

    int Argc;
    wchar_t** Argv;
};

#endif
