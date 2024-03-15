#ifndef HOOKS_H_INCLUDED
    #define HOOKS_H_INCLUDED 1

#include <sumhook.h>

#include <windows.h>
#include <shlobj_core.h>
#include <dinput.h>
#include <xinput.h>

#include "steam_api.h"

#define KERNEL32_HOOKS(xx)                          \
    xx(Kernel32, CreateMutexA)                      \
    xx(Kernel32, CreateMutexW)                      \
    xx(Kernel32, GetSystemInfo)                     \
    xx(Kernel32, GetSystemTimeAsFileTime)           \

#define TIMING_HOOKS(xx)                    \
    xx(Kernel32, QueryPerformanceFrequency) \
    xx(Kernel32, QueryPerformanceCounter)   \
    xx(Kernel32, GetTickCount)              \
    xx(Kernel32, GetTickCount64)            \
    xx(Winmm, timeGetTime)                  \

#define SHELL32_HOOKS(xx)           \
    xx(Shell32, SHGetFolderPathA)   \
    xx(Shell32, SHGetFolderPathW)   \

#define WINDOW_HOOKS(xx)                \
    xx(User32, CreateWindowExW)         \
    xx(User32, AdjustWindowRect)        \
    xx(User32, AdjustWindowRectEx)      \
    xx(User32, GetWindowLongW)          \
    xx(User32, SetWindowLongW)          \
    xx(User32, GetWindowLongA)          \
    xx(User32, SetWindowLongA)          \
    xx(User32, SystemParametersInfoA)   \
    xx(User32, SystemParametersInfoW)   \

#define STEAMAPI_HOOKS(xx)                      \
    xx(SteamApi, SteamAPI_Init)                 \
    xx(SteamApi, SteamAPI_RegisterCallResult)   \
    xx(SteamApi, SteamAPI_RegisterCallback)     \
    xx(SteamApi, SteamAPI_RunCallbacks)         \
    xx(SteamApi, SteamAPI_Shutdown)             \
    xx(SteamApi, SteamAPI_UnregisterCallResult) \
    xx(SteamApi, SteamAPI_UnregisterCallback)   \
    xx(SteamApi, SteamApps)                     \
    xx(SteamApi, SteamFriends)                  \
    xx(SteamApi, SteamGameServer)               \
    xx(SteamApi, SteamGameServer_Shutdown)      \
    xx(SteamApi, SteamMatchmaking)              \
    xx(SteamApi, SteamMatchmakingServers)       \
    xx(SteamApi, SteamNetworking)               \
    xx(SteamApi, SteamRemoteStorage)            \
    xx(SteamApi, SteamUser)                     \
    xx(SteamApi, SteamUserStats)                \
    xx(SteamApi, SteamUtils)                    \

extern "C" DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD UserIndex, GUID* RenderGuid, GUID* CaptureGuid);

#define INPUT_HOOKS(xx)                             \
    xx(User32, ClipCursor)                          \
    xx(User32, GetClipCursor)                       \
    xx(User32, SetCapture)                          \
    xx(User32, ReleaseCapture)                      \
    xx(User32, GetCapture)                          \
    xx(User32, SetCursorPos)                        \
    xx(User32, GetCursorPos)                        \
    xx(User32, GetFocus)                            \
    xx(User32, GetForegroundWindow)                 \
    xx(User32, GetKeyState)                         \
    xx(DInput8, DirectInput8Create)                 \
    xx(XInput13, XInputEnable)                      \
    xx(XInput13, XInputGetBatteryInformation)       \
    xx(XInput13, XInputGetCapabilities)             \
    xx(XInput13, XInputGetDSoundAudioDeviceGuids)   \
    xx(XInput13, XInputGetKeystroke)                \
    xx(XInput13, XInputGetState)                    \
    xx(XInput13, XInputSetState)                    \

// https://wiki.multimedia.cx/index.php/RAD_Game_Tools_Bink_API
extern "C" void* __stdcall BinkOpen(HANDLE File, std::uint32_t Flags);

#define BINKW32_HOOKS(xx)   \
    xx(Binkw32, BinkOpen)   \

#define DECLARE_HOOKS(m, f) extern smhk::unique_hook<decltype(&f)> f##_Orig;

KERNEL32_HOOKS(DECLARE_HOOKS)
SHELL32_HOOKS(DECLARE_HOOKS)
WINDOW_HOOKS(DECLARE_HOOKS)
STEAMAPI_HOOKS(DECLARE_HOOKS)
TIMING_HOOKS(DECLARE_HOOKS)
INPUT_HOOKS(DECLARE_HOOKS)
BINKW32_HOOKS(DECLARE_HOOKS)

#endif
