#include <windows.h>

#include <cstdio>
#include <cstdlib>

// The abort call is just there to prevent the compiler from complaining. It is never reached.
#define NYI(...)                                        \
    std::printf("NYI!" __VA_ARGS__);                    \
    std::system("pause");                               \
    TerminateProcess(GetCurrentProcess(), __LINE__);    \
    std::abort()                                        \

#if 0
    #define DLOG(...) std::printf(__VA_ARGS__)
#else
    constexpr void DLOG(const char*, ...){}
#endif
