// This is the base for the assembly code used for `payload` in "dllinject.cpp".
// You should NOT link with this file instead compile it and dump the assembly and extract it
// (manually) into "dllinject.cpp".

#include "inject_dll.h"

#ifdef __GNUC__
    asm(
        #if IS_32BIT
            "payload:\n"
            "    pushal\n"
            "    movl $0xCDCDCDCD, %ecx\n"
            "    call @load_dll@4\n"
            "    popal\n"
            "    jmp start"
        #elif IS_64BIT
            "payload:\n"
            "    pushq %rax\n" // 10
            "    pushq %rcx\n" // 18
            "    pushq %rdx\n" // 20
            "    pushq %r8\n" // 28
            "    pushq %r9\n" // 30
            "    pushq %r10\n" // 38
            "    pushq %r11\n" // 40
            "    movq $0xCDCDCDCDCDCDCDCD, %rcx\n"
            "    subq $32, %rsp\n" // 60
            "    call load_dll\n"
            "    addq $32, %rsp\n"
            "    popq %r11\n"
            "    popq %r10\n"
            "    popq %r9\n"
            "    popq %r8\n"
            "    popq %rdx\n"
            "    popq %rcx\n"
            "    popq %rax\n"
            "    jmp *jmpaddr(%rip)\n"
            ".align 8\n"
            "jmpaddr:\n"
            "    .quad $0xCDCDCDCDCDCDCDCD"
        #endif
    );
#endif

extern "C" void __fastcall load_dll(inject_info* Info){
    auto Module = Info->LoadLibraryW(Info->DllName);
    auto Function = reinterpret_cast<void(*)(void*)>(
        Info->GetProcAddress(Module,Info->FunctionName));
    Function(Info->FunctionArg);
}
