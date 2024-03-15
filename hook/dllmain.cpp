static_assert(sizeof(void*) == 4, "Only 32-bit builds are supported.");

#include "defines.h"

#include <filesystem>

#include <algorithm>

#include <intrin.h>

#include <sumhook.h>

#include <hookargs.h>

#include "debug.h"
#include "hooks.h"
#include "pytas.h"
#include "state.h"
#include "steam.h"
#include "window.h"

constinit MODULEINFO GameModule = {};

constinit game_version GameVersion;

cmdargs CmdArgs;

// Timer
constinit LARGE_INTEGER QpcFrequency_Orig;

constinit std::atomic<std::int64_t> Qpc = 0;
constinit std::uint64_t SystemFileTime = 0;

constinit int FrameTime = QpcFrequency/60;
constinit bool FrameWait = true;

// Game
thread_local bool IsBinkThread = false;

constinit bool IsInLoadScreen = false;
constinit bool IsInMovie = false;

inline FARPROC get_proc_address(HMODULE Module, const char* Name, bool Follow = true){
    auto r = GetProcAddress(Module, Name);

    std::size_t i = 0;

    while(i < 5){
        auto Code = reinterpret_cast<const unsigned char*>(r)+i;
        switch(Code[0]){
            case 0xE9: {
                if(Follow && i == 0){
                    std::uint32_t Jmp;
                    std::memcpy(&Jmp, Code+1, sizeof(Jmp));
                    r = reinterpret_cast<FARPROC>(reinterpret_cast<std::uint32_t>(Code)+5+Jmp);
                    DLOG("%s: 0x%p -> 0x%p\n", Name, Code, r);
                    continue;
                }
                DLOG("%s+%zX: jmp rel32\n", Name, i);
                break;
            }
            case 0xFF: {
                switch((Code[1] >> 3)&7){
                    case 4: {
                        if(Follow && i == 0 && Code[1] == 0x25){
                            std::uint32_t Jmp;
                            std::memcpy(&Jmp, Code+2, sizeof(Jmp));
                            r = *reinterpret_cast<FARPROC*>(Jmp);
                            DLOG("%s: 0x%p -> 0x%p\n", Name, Code, r);
                            continue;
                        }
                        DLOG("%s+%zX: jmp r/m32\n", Name, i);
                        break;
                    }
                    case 5: {
                        DLOG("%s+%zX: jmp m16:32\n", Name, i);
                        break;
                    }
                }
            }
        }

        i += smhk::instruction_info(Code).Size;
    }

    return r;
}

#define GET_PROC_ADDRESS(m, f) reinterpret_cast<decltype(f)*>(get_proc_address(m, #f))

#define DEFINE_HOOKS(m, f) constinit smhk::unique_hook<decltype(&f)> f##_Orig = nullptr;
#define PREPARE_HOOKS(m, f) f##_Orig.prepare(GET_PROC_ADDRESS(m, f##), f##_Hook),

KERNEL32_HOOKS(DEFINE_HOOKS)
SHELL32_HOOKS(DEFINE_HOOKS)
WINDOW_HOOKS(DEFINE_HOOKS)
STEAMAPI_HOOKS(DEFINE_HOOKS)
TIMING_HOOKS(DEFINE_HOOKS)
INPUT_HOOKS(DEFINE_HOOKS)
BINKW32_HOOKS(DEFINE_HOOKS)

constinit smhk::unique_buffer HookBuffer = nullptr;

HANDLE WINAPI CreateMutexA_Hook(
    SECURITY_ATTRIBUTES* Security, BOOL InitialOwner, const char* Name
){
    auto Name2 = (Name && std::strcmp(Name, "Local\\Dishonored-PC-Session-Mutex") == 0)?nullptr:Name;
    auto r = CreateMutexA_Orig(Security, InitialOwner, Name2);

    return r;
}

HANDLE WINAPI CreateMutexW_Hook(
    SECURITY_ATTRIBUTES* Security, BOOL InitialOwner, const wchar_t* Name
){
    auto Name2 = (Name && std::wcscmp(Name, L"Local\\Dishonored-PC-Session-Mutex") == 0)?nullptr:Name;

    auto r = CreateMutexW_Orig(Security, InitialOwner, Name2);

    return r;
}

void WINAPI GetSystemInfo_Hook(SYSTEM_INFO* Info){
    DLOG("GetSystemInfo(0x%p)\n", Info);
    GetSystemInfo_Orig(Info);
    Info->dwNumberOfProcessors = 1;
}

bool in_module(const void* Ptr, const MODULEINFO& ModInfo){
    auto Base = static_cast<char*>(ModInfo.lpBaseOfDll);
    return Base <= Ptr && Ptr < Base+ModInfo.SizeOfImage;
}

BOOL WINAPI QueryPerformanceCounter_Hook(LARGE_INTEGER* Counter){
    auto Ret = _ReturnAddress();

    BOOL r = TRUE;

    if(in_module(Ret, GameModule) && !IsBinkThread){
        Counter->QuadPart = Qpc;
    }else{
        r = QueryPerformanceCounter_Orig(Counter);
    }

    return r;
}

BOOL WINAPI QueryPerformanceFrequency_Hook(LARGE_INTEGER* Frequency){
    auto Ret = _ReturnAddress();

    BOOL r = TRUE;

    if(in_module(Ret, GameModule) && !IsBinkThread){
        Frequency->QuadPart = QpcFrequency;
    }else{
        r = QueryPerformanceFrequency_Orig(Frequency);
    }

    return r;
}

void WINAPI GetSystemTimeAsFileTime_Hook(FILETIME* Time){
    ULARGE_INTEGER r;
    r.QuadPart = SystemFileTime+static_cast<std::uint64_t>(Qpc);
    DLOG("GetSystemTimeAsFileTime(): %llu\n", r.QuadPart);

    Time->dwLowDateTime = r.LowPart;
    Time->dwHighDateTime = r.HighPart;
}

DWORD WINAPI GetTickCount_Hook(){
    if(in_module(_ReturnAddress(), GameModule) && !IsBinkThread){
        return static_cast<DWORD>(Qpc/TickConversion);
    }else{
        return GetTickCount_Orig();
    }
}

ULONGLONG WINAPI GetTickCount64_Hook(){
    if(in_module(_ReturnAddress(), GameModule) && !IsBinkThread){
        return Qpc/TickConversion;
    }else{
        return GetTickCount64_Orig();
    }
}

DWORD WINAPI timeGetTime_Hook(){
    if(in_module(_ReturnAddress(), GameModule) && !IsBinkThread){
        return static_cast<DWORD>(Qpc/TickConversion);
    }else{
        return timeGetTime_Orig();
    }
}

HRESULT WINAPI SHGetFolderPathA_Hook(
    HWND Window, int Csidl, HANDLE Token, DWORD Flags, char* Path
){
    auto r = S_OK;

    if(Csidl == CSIDL_PERSONAL){
        std::snprintf(Path, MAX_PATH, "%ls", CmdArgs.Documents.c_str());
    }else{
        r = SHGetFolderPathA_Orig(Window, Csidl, Token, Flags, Path);
    }

    DLOG("SHGetFolderPathA(0x%p, 0x%04X, 0x%p, %08X, \"%s\"): %d\n",
        Window, Csidl, Token, Flags, Path, r);

    return r;
}

HRESULT WINAPI SHGetFolderPathW_Hook(
    HWND Window, int Csidl, HANDLE Token, DWORD Flags, wchar_t* Path
){
    auto r = S_OK;

    if(Csidl == CSIDL_PERSONAL){
        std::swprintf(Path, MAX_PATH, L"%ls", CmdArgs.Documents.c_str());
    }else{
        r = SHGetFolderPathW_Orig(Window, Csidl, Token, Flags, Path);
    }

    DLOG("SHGetFolderPathW(0x%p, 0x%04X, 0x%p, %08X, \"%ls\"): %d\n",
        Window, Csidl, Token, Flags, Path, r);

    return r;
}

void* __stdcall BinkOpen_Hook(HANDLE File, std::uint32_t Flags){
    auto r = BinkOpen_Orig(File, Flags);

    IsBinkThread = true;

    return r;
}

struct game {
    void do_frame_hook();
};

void message_loop_hook();
void init_window_hook();

struct unknown1 {
    void movie_loop_hook();
};

// Something to do with the message loop in a load screen.
void load_loop_hook();

smhk::unique_hook<decltype(&game::do_frame_hook)> DoFrame_Orig = nullptr;
smhk::unique_hook<decltype(&message_loop_hook)> MessageLoop_Orig = nullptr;
smhk::unique_hook<decltype(&init_window_hook)> InitWindow_Orig = nullptr;
smhk::unique_hook<decltype(&unknown1::movie_loop_hook)> MovieLoop_Orig = nullptr;
smhk::unique_hook<decltype(&load_loop_hook)> LoadLoop_Orig = nullptr;

constinit LARGE_INTEGER PrevTime = {};

void game::do_frame_hook(){
    Qpc += FrameTime;

    (this->*DoFrame_Orig)();
}

void message_loop_hook(){
    MessageLoop_Orig();

    if(!IsInLoadScreen){
        pytas_next();

        if(FrameWait && PrevTime.QuadPart != 0){
            LARGE_INTEGER CurrentTime;
            QueryPerformanceCounter_Orig(&CurrentTime);

            std::int64_t EndTime = PrevTime.QuadPart+FrameTime*QpcFrequency_Orig.QuadPart/QpcFrequency;
            if(EndTime <= CurrentTime.QuadPart){
                EndTime = CurrentTime.QuadPart;
            }

            while(CurrentTime.QuadPart < EndTime){
                QueryPerformanceCounter_Orig(&CurrentTime);
            }

            PrevTime.QuadPart = EndTime;
        }else{
            QueryPerformanceCounter_Orig(&PrevTime);
        }

        ++XInputState.dwPacketNumber;

        flush_events();
    }
}

void init_window_hook(){
    IsInLoadScreen = true;

    InitWindow_Orig();

    IsInLoadScreen = false;
}

void unknown1::movie_loop_hook(){
    IsInMovie = true;

    (this->*MovieLoop_Orig)();

    IsInMovie = false;
}

void load_loop_hook(){
    IsInLoadScreen = true;

    LoadLoop_Orig();

    IsInLoadScreen = false;
}

cmdargs parse_cmdline(int* Argc, wchar_t** Argv){
    cmdargs r = {
        .Documents =
            (GameVersion == game_version::V12)?"datafiles/documents12":"datafiles/documents14",
        .Userdata = "datafiles/userdata",
        .Scripts = "datafiles/scripts",
        .Main = L"main",
    };

    bool Documents = false;
    bool Userdata = false;
    bool Scripts = false;
    bool Main = false;

    int j = 1;
    for(int i = 1, End = *Argc; i < End; ++i){
        if(std::wcscmp(Argv[i], L"--documents") == 0){
            if(Documents){
                throw std::runtime_error("`--documents` encountered twice");
            }else if(i+1 >= End){
                throw std::runtime_error("`--documents` without a path");
            }else{
                Documents = true;

                r.Documents = Argv[++i];
            }
        }else if(std::wcscmp(Argv[i], L"--userdata") == 0){
            if(Userdata){
                throw std::runtime_error("`--userdata` encountered twice");
            }else if(i+1 >= End){
                throw std::runtime_error("`--userdata` without a value");
            }else{
                Userdata = true;

                r.Userdata = Argv[++i];
            }
        }else if(std::wcscmp(Argv[i], L"--scripts") == 0){
            if(Scripts){
                throw std::runtime_error("`--scripts` encountered twice");
            }else if(i+1 >= End){
                throw std::runtime_error("`--scripts` without a value");
            }else{
                Scripts = true;

                r.Scripts = Argv[++i];
            }
        }else if(std::wcscmp(Argv[i], L"--main") == 0){
            if(Main){
                throw std::runtime_error("`--main` encountered twice");
            }else if(i+1 >= End){
                throw std::runtime_error("`--main` without a value");
            }else{
                Main = true;

                r.Main = Argv[++i];
            }
        }else{
            Argv[j++] = Argv[i];
        }
    }

    *Argc = j;
    Argv[j] = nullptr;

    r.Documents = fs::canonical(r.Documents);
    r.Userdata = fs::canonical(r.Userdata);
    r.Scripts = fs::canonical(r.Scripts);

    return r;
}

extern "C" void __declspec(dllexport) init(hookargs* HookArgs){
    try {
        (void)CreateThread(nullptr, 0, [](HANDLE Parent) -> DWORD {
            WaitForSingleObject(Parent, INFINITE);
            TerminateProcess(GetCurrentProcess(), __LINE__);
            return 0;
        }, HookArgs->Parent, 0, nullptr);

        AttachConsole(ATTACH_PARENT_PROCESS);

        std::freopen("CONIN$", "rb", stdin);
        std::freopen("CONOUT$", "wb", stdout);
        std::freopen("CONOUT$", "wb", stderr);

        // std::system("pause");

        auto Game = GetModuleHandleW(nullptr);
        if(!Game){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        if(!GetModuleInformation(GetCurrentProcess(), Game, &GameModule, sizeof(GameModule))){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        switch(GameModule.SizeOfImage){
            case 18219008: {
                GameVersion = game_version::V12;
                break;
            }
            case 18862080:
            case 19427328: {
                GameVersion = game_version::V14;
                break;
            }
            default: {
                std::abort();
            }
        }

        CmdArgs = parse_cmdline(&HookArgs->Argc, HookArgs->Argv);

        pytas_init(HookArgs->Argc, HookArgs->Argv);

        if(!QueryPerformanceFrequency(&QpcFrequency_Orig)){
            std::abort();
        }

        void* DoFrameAddress;
        void* InitWindowAddress;
        void* MessageLoopAddress;
        void* MovieLoopAddress;
        void* LoadLoopAddress;

        auto Base = static_cast<char*>(GameModule.lpBaseOfDll);

        switch(GameVersion){
            case game_version::V12: {
                DoFrameAddress = Base+0x5E0010;
                InitWindowAddress = Base+0x16C90;
                MessageLoopAddress = Base+0x16C10;
                MovieLoopAddress = Base+0xDB570;
                LoadLoopAddress = Base+0x14AE00;
                break;
            }
            case game_version::V14: {
                DoFrameAddress = Base+0x5E0CB0;
                InitWindowAddress = Base+0x16D40;
                MessageLoopAddress = Base+0x16CC0;
                MovieLoopAddress = Base+0xDB460;
                LoadLoopAddress = Base+0x14A5F0;
                break;
            }
            default: {
                std::abort();
            }
        }

        // 2022-02-22 00:00:00
        std::uint64_t StartTime = 19045*24*3600;

        SystemFileTime = 116444736000000000+10000000*StartTime;

        load_steam_cloud();

        auto Kernel32 = GetModuleHandleW(L"kernel32.dll");
        if(!Kernel32){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto Binkw32 = GetModuleHandleW(L"binkw32.dll");
        if(!Binkw32){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto Winmm = GetModuleHandleW(L"winmm.dll");
        if(!Winmm){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto Shell32 = GetModuleHandleW(L"shell32.dll");
        if(!Shell32){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto User32 = GetModuleHandleW(L"user32.dll");
        if(!User32){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto SteamApi = GetModuleHandleW(L"steam_api.dll");
        if(!SteamApi){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto DInput8 = GetModuleHandleW(L"dinput8.dll");
        if(!DInput8){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        auto XInput13 = GetModuleHandleW(L"xinput1_3.dll");
        if(!XInput13){
            TerminateProcess(GetCurrentProcess(), __LINE__);
        }

        HookBuffer = smhk::create_hooks({
            KERNEL32_HOOKS(PREPARE_HOOKS)
            SHELL32_HOOKS(PREPARE_HOOKS)
            WINDOW_HOOKS(PREPARE_HOOKS)
            STEAMAPI_HOOKS(PREPARE_HOOKS)
            TIMING_HOOKS(PREPARE_HOOKS)
            INPUT_HOOKS(PREPARE_HOOKS)
            BinkOpen_Orig.prepare(
                reinterpret_cast<decltype(&BinkOpen)>(get_proc_address(Binkw32, "_BinkOpen@8")),
                BinkOpen_Hook
            ),
            DoFrame_Orig.prepare(
                smhk::fun_cast<decltype(&game::do_frame_hook)>(DoFrameAddress),
                &game::do_frame_hook
            ),
            MessageLoop_Orig.prepare(
                smhk::fun_cast<decltype(&message_loop_hook)>(MessageLoopAddress),
                &message_loop_hook
            ),
            InitWindow_Orig.prepare(
                smhk::fun_cast<decltype(&init_window_hook)>(InitWindowAddress),
                &init_window_hook
            ),
            MovieLoop_Orig.prepare(
                smhk::fun_cast<decltype(&unknown1::movie_loop_hook)>(MovieLoopAddress),
                &unknown1::movie_loop_hook
            ),
            LoadLoop_Orig.prepare(
                smhk::fun_cast<decltype(&load_loop_hook)>(LoadLoopAddress),
                &load_loop_hook
            ),
        });
    }catch(std::exception& e){
        std::fprintf(stderr, "Error: %s\n", e.what());
        TerminateProcess(GetCurrentProcess(), __LINE__);
    }
}
