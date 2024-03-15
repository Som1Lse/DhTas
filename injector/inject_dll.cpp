#include "inject_dll.h"

#include <cwchar>
#include <cstdlib>
#include <cstring>

namespace {

#if IS_32BIT
    struct payload {
        unsigned char Insn00[1]   = {0x60                        }; // pusha
        unsigned char Insn01[1]   = {0xB9                        }; // mov    ecx, ARGUMENT-ADDRESS
        unsigned char Argument[4] = {};
        unsigned char Insn06[5]   = {0xE8, 0x06, 0x00, 0x00, 0x00}; // call   11 <@load_dll@4>
        unsigned char Insn0B[1]   = {0x61                        }; // popa
        unsigned char InsnRet[1]  = {0xE9                        }; // jmp    RETURN-ADDRESS
        unsigned char Return[4]   = {};
        unsigned char Insn11[1]   = {0x53                        }; // push   ebx
        unsigned char Insn12[2]   = {0x89, 0xCB                  }; // mov    ebx, ecx
        unsigned char Insn14[3]   = {0x83, 0xEC, 0x18            }; // sub    esp, 0x18
        unsigned char Insn17[2]   = {0x8B, 0x01                  }; // mov    eax, DWORD PTR [ecx]
        unsigned char Insn19[3]   = {0x89, 0x04, 0x24            }; // mov    DWORD PTR [esp], eax
        unsigned char Insn1C[3]   = {0xFF, 0x53, 0x0C            }; // call   DWORD PTR [ebx+0xc]
        unsigned char Insn1F[3]   = {0x8B, 0x53, 0x04            }; // mov    edx, DWORD PTR [ebx+0x4]
        unsigned char Insn22[3]   = {0x83, 0xEC, 0x04            }; // sub    esp, 0x4
        unsigned char Insn25[3]   = {0x89, 0x04, 0x24            }; // mov    DWORD PTR [esp], eax
        unsigned char Insn28[4]   = {0x89, 0x54, 0x24, 0x04      }; // mov    DWORD PTR [esp+0x4], edx
        unsigned char Insn2C[3]   = {0xFF, 0x53, 0x10            }; // call   DWORD PTR [ebx+0x10]
        unsigned char Insn2F[3]   = {0x8B, 0x53, 0x08            }; // mov    edx, DWORD PTR [ebx+0x8]
        unsigned char Insn32[3]   = {0x83, 0xEC, 0x08            }; // sub    esp, 0x8
        unsigned char Insn35[3]   = {0x89, 0x14, 0x24            }; // mov    DWORD PTR [esp], edx
        unsigned char Insn38[2]   = {0xFF, 0xD0                  }; // call   eax
        unsigned char Insn3A[3]   = {0x83, 0xC4, 0x18            }; // add    esp, 0x18
        unsigned char Insn3D[1]   = {0x5B                        }; // pop    ebx
        unsigned char Insn3E[1]   = {0xC3                        }; // ret
    };

    static_assert(sizeof(payload) == 0x3F);
#else
    struct payload {
        unsigned char Insn00[1] = {0x50                              }; // push   rax
        unsigned char Insn01[1] = {0x51                              }; // push   rcx
        unsigned char Insn02[1] = {0x52                              }; // push   rdx
        unsigned char Insn03[2] = {0x41, 0x50                        }; // push   r8
        unsigned char Insn05[2] = {0x41, 0x51                        }; // push   r9
        unsigned char Insn07[2] = {0x41, 0x52                        }; // push   r10
        unsigned char Insn09[2] = {0x41, 0x53                        }; // push   r11
        unsigned char Insn0B[2] = {0x48, 0xB9                        }; // movabs rcx, ARGUMENT-ADDRESS
        unsigned char Argument[8] = {};
        unsigned char Insn15[4] = {0x48, 0x83, 0xEC, 0x20,           }; // sub    rsp,0x20
        unsigned char Insn19[5] = {0xE8, 0x22, 0x00, 0x00, 0x00      }; // call   40 <load_dll>
        unsigned char Insn1E[4] = {0x48, 0x83, 0xC4, 0x20,           }; // add    rsp,0x20
        unsigned char Insn22[2] = {0x41, 0x5B                        }; // pop    r11
        unsigned char Insn24[2] = {0x41, 0x5A                        }; // pop    r10
        unsigned char Insn26[2] = {0x41, 0x59                        }; // pop    r9
        unsigned char Insn28[2] = {0x41, 0x58                        }; // pop    r8
        unsigned char Insn2A[1] = {0x5A                              }; // pop    rdx
        unsigned char Insn2B[1] = {0x59                              }; // pop    rcx
        unsigned char Insn2C[1] = {0x58                              }; // pop    rax
        unsigned char Insn2D[6] = {0xFF, 0x25, 0x05, 0x00, 0x00, 0x00}; // jmp    QWORD PTR [rip+0x5]
        unsigned char Insn33[5] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC      }; // int3 * 5
        unsigned char Return[8] = {};                                   // RETURN-ADDRESS
        unsigned char Insn40[1] = {0x53                              }; // push   rbx
        unsigned char Insn41[4] = {0x48, 0x83, 0xEC, 0x20            }; // sub    rsp,0x20
        unsigned char Insn45[3] = {0x48, 0x89, 0xCB                  }; // mov    rbx,rcx
        unsigned char Insn48[3] = {0x48, 0x8B, 0x09                  }; // mov    rcx,QWORD PTR [rcx]
        unsigned char Insn4B[3] = {0xFF, 0x53, 0x18                  }; // call   QWORD PTR [rbx+0x18]
        unsigned char Insn4E[4] = {0x48, 0x8B, 0x53, 0x08            }; // mov    rdx,QWORD PTR [rbx+0x8]
        unsigned char Insn52[3] = {0x48, 0x89, 0xC1                  }; // mov    rcx,rax
        unsigned char Insn55[3] = {0xFF, 0x53, 0x20                  }; // call   QWORD PTR [rbx+0x20]
        unsigned char Insn58[4] = {0x48, 0x8B, 0x4B, 0x10            }; // mov    rcx,QWORD PTR [rbx+0x10]
        unsigned char Insn5C[4] = {0x48, 0x83, 0xC4, 0x20            }; // add    rsp,0x20
        unsigned char Insn60[1] = {0x5B                              }; // pop    rbx
        unsigned char Insn61[3] = {0x48, 0xFF, 0xE0                  }; // rex.W jmp rax
    };

    static_assert(sizeof(payload) == 0x64);
#endif

static_assert(alignof(payload) == 1);

static_assert(std::is_standard_layout<payload>::value);

void write_payload_or_crash(HANDLE Process, void* PayloadAddress, const void* InjectInfo,
                         std::uintptr_t ReturnAddress){
    payload Payload = {};

    (void)InjectInfo;
    std::memcpy(Payload.Argument, &InjectInfo, sizeof(InjectInfo));

    #if IS_32BIT
        ReturnAddress -= reinterpret_cast<std::uintptr_t>(PayloadAddress);
        ReturnAddress -= offsetof(payload, InsnRet);
        ReturnAddress -= sizeof(payload::InsnRet)+sizeof(payload::Return);
    #endif

    std::memcpy(Payload.Return, &ReturnAddress, sizeof(ReturnAddress));

    write_or_crash(Process, PayloadAddress, &Payload, sizeof(Payload));
}

}

void write_or_crash(HANDLE Process, void* Dest, const void* Source, size_t Size){
    SIZE_T BytesWritten;
    if(!WriteProcessMemory(Process, Dest, Source, Size, &BytesWritten) || BytesWritten != Size){
        TerminateProcess(Process, 1);
        std::abort();
    }
}

char* inject_dll(const PROCESS_INFORMATION& ProcInfo, const wchar_t* DllName,
                 const char* FunctionName, size_t FunctionArgSize){
    auto DllNameLength = std::wcslen(DllName);
    auto FunctionNameLength = std::strlen(FunctionName);

    auto DllNameBytes = (DllNameLength+1)*sizeof(wchar_t);
    auto FunctionNameBytes = (FunctionNameLength+1)*sizeof(char);

    inject_info InjectInfo = {};

    static constexpr auto AlignMask = alignof(inject_info)-1;
    FunctionArgSize = (FunctionArgSize+AlignMask)&~AlignMask;
    auto TotalSize = FunctionArgSize+sizeof(InjectInfo)+DllNameBytes+FunctionNameBytes;

    auto InfoPtr = reinterpret_cast<char*>(
        VirtualAllocEx(ProcInfo.hProcess, nullptr, TotalSize,
                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE));
    if(!InfoPtr){
        TerminateProcess(ProcInfo.hProcess, 1);
        std::abort();
    }

    auto CodePtr = VirtualAllocEx(ProcInfo.hProcess, nullptr, sizeof(payload),
                                 MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READ);
    if(!CodePtr){
        TerminateProcess(ProcInfo.hProcess, 1);
        std::abort();
    }

    CONTEXT Context;
    Context.ContextFlags = CONTEXT_CONTROL;

    if(!GetThreadContext(ProcInfo.hThread, &Context)){
        TerminateProcess(ProcInfo.hProcess, 1);
        std::abort();
    }

    auto Kernel32 = GetModuleHandleW(L"kernel32.dll");

    InjectInfo.LoadLibraryW = reinterpret_cast<decltype(LoadLibraryW)*>(
        GetProcAddress(Kernel32, "LoadLibraryW"));
    InjectInfo.GetProcAddress = reinterpret_cast<decltype(GetProcAddress)*>(
        GetProcAddress(Kernel32, "GetProcAddress"));

    auto InjectInfoPtr = reinterpret_cast<inject_info*>(InfoPtr+FunctionArgSize);
    InjectInfo.DllName = reinterpret_cast<wchar_t*>(InjectInfoPtr+1);
    InjectInfo.FunctionName = reinterpret_cast<char*>(InjectInfo.DllName+DllNameLength+1);

    #if IS_32BIT
        auto& InsnPtr = Context.Eip;
    #else
        auto& InsnPtr = Context.Rip;
    #endif

    InjectInfo.FunctionArg = InfoPtr;

    write_or_crash(ProcInfo.hProcess, InjectInfoPtr, &InjectInfo, sizeof(InjectInfo));
    write_or_crash(ProcInfo.hProcess, InjectInfo.DllName, DllName, DllNameBytes);
    write_or_crash(ProcInfo.hProcess, InjectInfo.FunctionName, FunctionName, FunctionNameBytes);

    write_payload_or_crash(ProcInfo.hProcess, CodePtr, InjectInfoPtr, InsnPtr);

    InsnPtr = reinterpret_cast<std::uintptr_t>(CodePtr);

    if(!SetThreadContext(ProcInfo.hThread, &Context)){
        TerminateProcess(ProcInfo.hProcess, 1);
        std::abort();
    }

    return InfoPtr;
}
