#ifndef INJECT_DLL_H_INCLUDED
    #define INJECT_DLL_H_INCLUDED 1

#include <type_traits>

#include <windows.h>

#include <cstdint>

#define IS_32BIT (SIZE_MAX == 0xFFFFFFFF)
#define IS_64BIT (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)

static_assert(IS_32BIT || IS_64BIT);

struct inject_info {
    wchar_t* DllName;
    char* FunctionName;
    void* FunctionArg;

    decltype(LoadLibraryW)* LoadLibraryW;
    decltype(GetProcAddress)* GetProcAddress;
};

void write_or_crash(HANDLE Process, void* Dest, const void* Source, size_t Size);

char* inject_dll(const PROCESS_INFORMATION& ProcInfo, const wchar_t* DllName,
                 const char* FunctionName, std::size_t FunctionArgSize);

#endif
