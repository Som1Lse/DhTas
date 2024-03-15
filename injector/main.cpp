#include <cwchar>
#include <cstdlib>

#include <string>
#include <filesystem>
#include <system_error>

#define NOMINMAX

#include <windows.h>
#include <shlwapi.h>

#include <hookargs.h>

#include "inject_dll.h"

namespace {

// This must be global to support `console_ctrl_handler`.
HANDLE ChildProcess = nullptr;

BOOL WINAPI console_ctrl_handler(DWORD) noexcept {
    TerminateProcess(ChildProcess, 1);
    return TRUE;
}

std::filesystem::path get_exe_path(){
    auto Size = GetModuleFileNameW(nullptr, nullptr, 0);

    if(Size == 0){
        Size = MAX_PATH;
    }

    std::wstring r(Size, '\0');

    Size = GetModuleFileNameW(nullptr, r.data(), Size);
    if(Size == 0){
        throw std::system_error(GetLastError(),std::system_category());
    }

    r.resize(Size);

    return r;
}

std::filesystem::path get_dishonored_path(){
    #define STEAMAPP_PATH LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App )"

    static constexpr const wchar_t* RegistryPaths[] = {
        STEAMAPP_PATH L"205100",
        STEAMAPP_PATH L"217980",
    };

    wchar_t DishonoredPath[MAX_PATH] = {};

    static constexpr auto RegFlags = RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ|RRF_SUBKEY_WOW6464KEY;

    for(auto& Path:RegistryPaths){
        DWORD Size = sizeof(DishonoredPath);

        auto r = RegGetValueW(
            HKEY_LOCAL_MACHINE, Path, L"InstallLocation", RegFlags, nullptr, DishonoredPath, &Size
        );

        if(r == ERROR_SUCCESS){
            return DishonoredPath;
        }
    }

    throw std::runtime_error("Unable to find dishonored. Specify it via `--exe`.");
}

std::filesystem::path get_dishonored_exe(int* Argc, wchar_t** Argv){
    const wchar_t* r = nullptr;;

    int j = 1;
    for(int i = 1, End = *Argc; i < End; ++i){
        if(std::wcscmp(Argv[i], L"--exe") == 0){
            if(r){
                throw std::runtime_error("`--exe` encountered twice");
            }else if(i+1 >= End){
                throw std::runtime_error("`--exe` without a value");
            }else{
                r = Argv[++i];
            }
        }else{
            Argv[j++] = Argv[i];
        }
    }

    *Argc = j;
    Argv[j] = nullptr;

    if(r){
        return r;
    }else{
        return get_dishonored_path()/"Binaries/Win32/Dishonored.exe";
    }
}

}

int wmain(int Argc, wchar_t** Argv){
    try {
        auto Module = GetModuleHandleW(L"kernel32.dll");
        if(!Module){
            return __LINE__;
        }

        auto DishonoredExe = get_dishonored_exe(&Argc, Argv);

        STARTUPINFOW StartupInfo = {};
        StartupInfo.cb = sizeof(StartupInfo);

        PROCESS_INFORMATION ProcInfo;

        auto CmdLine = L"\""+DishonoredExe.native()+L"\" -nostartupmovies -windowed";

        std::wstring EnvPath(GetEnvironmentVariableW(L"PATH", nullptr, 0), L'\0');
        auto r = GetEnvironmentVariableW(L"PATH", EnvPath.data(), static_cast<DWORD>(EnvPath.size()));
        if(r == 0 || r > EnvPath.size()){
            return __LINE__;
        }

        auto DllPath = get_exe_path();

        EnvPath.insert(0, DllPath.parent_path().native() + L';');
        if(!SetEnvironmentVariableW(L"PATH", EnvPath.c_str())){
            return __LINE__;
        }

        auto Flags = GetPriorityClass(nullptr)|CREATE_SUSPENDED;
        if(!CreateProcessW(DishonoredExe.c_str(), CmdLine.data(), nullptr, nullptr, FALSE,
                        Flags, nullptr, nullptr, &StartupInfo, &ProcInfo)){
            return __LINE__;
        }

        ChildProcess = ProcInfo.hProcess;
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

        DllPath.replace_filename(L"dhtashook.dll");

        hookargs HookArgs;
        if(!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), ProcInfo.hProcess,
                            &HookArgs.Parent, 0, FALSE, DUPLICATE_SAME_ACCESS)){
            TerminateProcess(ProcInfo.hProcess,1);
            return __LINE__;
        }

        HookArgs.Argc = Argc;

        auto ArgsSize = sizeof(HookArgs);

        for(int i = 0; i < Argc; ++i){
            ArgsSize += sizeof(wchar_t*)+sizeof(wchar_t)*(std::wcslen(Argv[i])+1);
        }

        ArgsSize += sizeof(wchar_t*);

        auto HookArgsPtr = inject_dll(ProcInfo, DllPath.c_str(), "init", ArgsSize);

        auto ArgvPtr = reinterpret_cast<wchar_t**>(HookArgsPtr+sizeof(HookArgs));
        auto ArgsPtr = reinterpret_cast<wchar_t*>(ArgvPtr+Argc+1);

        HookArgs.Argc = Argc;
        HookArgs.Argv = ArgvPtr;

        write_or_crash(ProcInfo.hProcess, HookArgsPtr, &HookArgs, sizeof(HookArgs));

        for(int i = 0; i < Argc; ++i){
            auto Length = std::wcslen(Argv[i])+1;

            write_or_crash(ProcInfo.hProcess, ArgvPtr, &ArgsPtr, sizeof(ArgsPtr));
            write_or_crash(ProcInfo.hProcess, ArgsPtr, Argv[i], sizeof(wchar_t)*Length);

            ++ArgvPtr;
            ArgsPtr += Length;
        }

        ArgsPtr = nullptr;

        write_or_crash(ProcInfo.hProcess, ArgvPtr, &ArgsPtr, sizeof(ArgsPtr));

        if(!ResumeThread(ProcInfo.hThread)){
            TerminateProcess(ProcInfo.hProcess, 1);
            return __LINE__;
        }

        CloseHandle(ProcInfo.hThread);

        if(WaitForSingleObject(ProcInfo.hProcess, INFINITE) != WAIT_OBJECT_0){
            TerminateProcess(ProcInfo.hProcess, 1);
            return __LINE__;
        }

        SetConsoleCtrlHandler(console_ctrl_handler, FALSE);

        DWORD ExitCode;
        if(!GetExitCodeProcess(ProcInfo.hProcess, &ExitCode)){
            return __LINE__;
        }

        CloseHandle(ProcInfo.hProcess);

        return static_cast<int>(ExitCode);
    }catch(std::exception& e){
        std::fprintf(stderr, "Error: %s\n", e.what());
        return -1;
    }
}
