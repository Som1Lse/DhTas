#ifndef WINDOW_H_INCLUDED
    #define WINDOW_H_INCLUDED

#include <vector>

#include <cstdint>

#include <windows.h>
#include <xinput.h>

struct key_event {
    int Key;
    bool Down;
};

struct move_event {
    int x, y;
};

extern std::vector<key_event> KeyEvents;
extern std::vector<move_event> MoveEvents;
extern std::vector<int> ScrollEvents;

extern std::uint32_t KeyState[8];
extern POINT CursorPos;
extern XINPUT_STATE XInputState;

extern RECT ClipCursorRect;

HWND WINAPI CreateWindowExW_Hook(
    DWORD ExStyle, const wchar_t* Class, const wchar_t* Title, DWORD Style,
    int x, int y, int Width, int Height, HWND Parent, HMENU Menu, HINSTANCE Instance, void* Param
);

BOOL WINAPI AdjustWindowRect_Hook(RECT* Rect, DWORD Style, BOOL Menu);
BOOL WINAPI AdjustWindowRectEx_Hook(RECT* Rect, DWORD Style, BOOL Menu, DWORD ExStyle);
long WINAPI GetWindowLongW_Hook(HWND Window, int Index);
long WINAPI SetWindowLongW_Hook(HWND Window, int Index, long Value);
long WINAPI GetWindowLongA_Hook(HWND Window, int Index);
long WINAPI SetWindowLongA_Hook(HWND Window, int Index, long Value);

BOOL WINAPI SystemParametersInfoA_Hook(UINT Action, UINT Param, void* Value, UINT WinIni);
BOOL WINAPI SystemParametersInfoW_Hook(UINT Action, UINT Param, void* Value, UINT WinIni);
BOOL WINAPI ClipCursor_Hook(const RECT* Rect);
BOOL WINAPI GetClipCursor_Hook(RECT* Rect);
HWND WINAPI SetCapture_Hook(HWND Window);
HWND WINAPI GetCapture_Hook();
BOOL WINAPI ReleaseCapture_Hook();

BOOL WINAPI SetCursorPos_Hook(int x, int y);
BOOL WINAPI GetCursorPos_Hook(POINT* Point);
HWND WINAPI GetFocus_Hook();
HWND WINAPI GetForegroundWindow_Hook();
SHORT WINAPI GetKeyState_Hook(int Key);
HRESULT WINAPI DirectInput8Create_Hook(HINSTANCE Instance, DWORD Version, const IID& Iid, void **Out, IUnknown* Outer);

void WINAPI XInputEnable_Hook(BOOL Enable) noexcept;
DWORD WINAPI XInputGetBatteryInformation_Hook(DWORD UserIndex, BYTE DevType, XINPUT_BATTERY_INFORMATION* Info) noexcept;
DWORD WINAPI XInputGetCapabilities_Hook(DWORD UserIndex, DWORD Flags, XINPUT_CAPABILITIES* Capabilities) noexcept;
DWORD WINAPI XInputGetDSoundAudioDeviceGuids_Hook(DWORD UserIndex, GUID* RenderGuid, GUID* CaptureGuid);
DWORD WINAPI XInputGetKeystroke_Hook(DWORD UserIndex, DWORD Reserved, XINPUT_KEYSTROKE* Keystroke) noexcept;
DWORD WINAPI XInputGetState_Hook(DWORD UserIndex, XINPUT_STATE* State) noexcept;
DWORD WINAPI XInputSetState_Hook(DWORD UserIndex, XINPUT_VIBRATION* Vibration) noexcept;

void flush_events();
bool set_key(int Index, bool Down);
void move_mouse(int x, int y);
void scroll_wheel(short Delta);

void clip_cursor(bool Clip);

#endif
