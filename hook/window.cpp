#include "defines.h"

#include "window.h"

#include "debug.h"

#include "state.h"
#include "hooks.h"

constexpr int RepeatDelay = QpcFrequency/2;
constexpr int RepeatFrequency = QpcFrequency/30;

std::vector<key_event> KeyEvents = {};
std::vector<move_event> MoveEvents = {};
std::vector<int> ScrollEvents = {};

constinit POINT CursorPos = {};
constinit std::uint32_t KeyState[8] = {};
constinit XINPUT_STATE XInputState = {};

constinit bool ShouldClip = false;
constinit bool IsWindowActive = false;

constinit RECT FullScreen = {0, 0, 1920, 1080};
constinit RECT ClipCursorRect = {0, 0, 1920, 1080};

constinit HWND MainWindow;

constinit WNDPROC WndProc_Orig;

constinit HWND WindowCapture = nullptr;

namespace {

struct sys_mouse:IDirectInputDevice8W {
    std::vector<DIDEVICEOBJECTDATA> ObjectData = {};
    std::size_t DataIndex = 0;

    DWORD Sequence = 0;

    void add_object_data(DWORD Offset, int Value){
        if(DataIndex != 0){
            ObjectData.erase(ObjectData.begin(), ObjectData.begin()+DataIndex);
            DataIndex = 0;
        }

        ObjectData.push_back({
            .dwOfs = Offset,
            .dwData = static_cast<DWORD>(Value),
            .dwTimeStamp = static_cast<DWORD>(Qpc/TickConversion),
            .dwSequence = Sequence++,
            .uAppData = 0xFFFFFFFF,
        });
    }

    HRESULT __stdcall QueryInterface(const IID&, void**){ NYI("sys_mouse::QueryInterface()\n"); }
    ULONG __stdcall AddRef(){ NYI("sys_mouse::AddRef()\n"); }
    HRESULT __stdcall GetCapabilities(DIDEVCAPS*){ NYI("sys_mouse::GetCapabilities()\n"); }
    HRESULT __stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKW, void*, DWORD){ NYI("sys_mouse::EnumObjects()\n"); }
    HRESULT __stdcall GetProperty(const GUID&, DIPROPHEADER*){ NYI("sys_mouse::GetProperty()\n"); }
    HRESULT __stdcall GetDeviceState(DWORD, void*){ NYI("sys_mouse::GetDeviceState()\n"); }
    HRESULT __stdcall SetEventNotification(HANDLE){ NYI("sys_mouse::SetEventNotification()\n"); }
    HRESULT __stdcall GetObjectInfo(DIDEVICEOBJECTINSTANCEW*, DWORD, DWORD){ NYI("sys_mouse::GetObjectInfo()\n"); }
    HRESULT __stdcall GetDeviceInfo(DIDEVICEINSTANCEW*){ NYI("sys_mouse::GetDeviceInfo()\n"); }
    HRESULT __stdcall RunControlPanel(HWND, DWORD){ NYI("sys_mouse::RunControlPanel()\n"); }
    HRESULT __stdcall Initialize(HINSTANCE, DWORD, const GUID &){ NYI("sys_mouse::Initialize()\n"); }
    HRESULT __stdcall CreateEffect(const GUID&, const DIEFFECT*, IDirectInputEffect**, LPUNKNOWN){ NYI("sys_mouse::CreateEffect()\n"); }
    HRESULT __stdcall EnumEffects(LPDIENUMEFFECTSCALLBACKW, void*, DWORD){ NYI("sys_mouse::EnumEffects()\n"); }
    HRESULT __stdcall GetEffectInfo(DIEFFECTINFOW*, const GUID &){ NYI("sys_mouse::GetEffectInfo()\n"); }
    HRESULT __stdcall GetForceFeedbackState(LPDWORD){ NYI("sys_mouse::GetForceFeedbackState()\n"); }
    HRESULT __stdcall SendForceFeedbackCommand(DWORD){ NYI("sys_mouse::SendForceFeedbackCommand()\n"); }
    HRESULT __stdcall EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK, void*, DWORD){ NYI("sys_mouse::EnumCreatedEffectObjects()\n"); }
    HRESULT __stdcall Escape(LPDIEFFESCAPE){ NYI("sys_mouse::Escape()\n"); }
    HRESULT __stdcall SendDeviceData(DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD){ NYI("sys_mouse::SendDeviceData()\n"); }
    HRESULT __stdcall EnumEffectsInFile(LPCWSTR, LPDIENUMEFFECTSINFILECALLBACK, void*, DWORD){ NYI("sys_mouse::EnumEffectsInFile()\n"); }
    HRESULT __stdcall WriteEffectToFile(LPCWSTR, DWORD, LPDIFILEEFFECT, DWORD){ NYI("sys_mouse::WriteEffectToFile()\n"); }
    HRESULT __stdcall BuildActionMap(LPDIACTIONFORMATW, LPCWSTR, DWORD){ NYI("sys_mouse::BuildActionMap()\n"); }
    HRESULT __stdcall SetActionMap(LPDIACTIONFORMATW, LPCWSTR, DWORD){ NYI("sys_mouse::SetActionMap()\n"); }
    HRESULT __stdcall GetImageInfo(LPDIDEVICEIMAGEINFOHEADERW){ NYI("sys_mouse::GetImageInfo()\n"); }

    ULONG __stdcall Release(){
        ULONG r = 0;
        DLOG("sys_mouse::Release(): %u\n", r);
        return r;
    }

    HRESULT __stdcall SetDataFormat(const DIDATAFORMAT* Format){
        HRESULT r = S_OK;
        DLOG("sys_mouse::SetDataFormat(0x%p): %d\n", Format, r);

        return r;
    }

    HRESULT __stdcall SetProperty(const GUID& Guid, const DIPROPHEADER* Header){
        HRESULT r = S_OK;
        DLOG("sys_mouse::SetProperty(0x%p, 0x%p): %d\n", &Guid, Header, r);

        return r;
    }

    HRESULT __stdcall Poll(){
        HRESULT r = S_FALSE;
        DLOG("sys_mouse::Poll(): %d\n", r);

        return r;
    }

    HRESULT __stdcall GetDeviceData(
        DWORD NumData, DIDEVICEOBJECTDATA* Data, DWORD* InOut, DWORD Flags
    ){
        assert(Flags == 0);
        assert(NumData == sizeof(*Data));

        auto PrevInOut = *InOut;

        auto Count = *InOut = std::min(*InOut, static_cast<DWORD>(ObjectData.size()-DataIndex));

        HRESULT r = S_OK;
        DLOG("sys_mouse::GetDeviceData(%u, 0x%p, *%p = %u->%u, 0x%08X): %d\n",
            NumData, Data, InOut, PrevInOut, *InOut, Flags, r
        );

        if(Data){
            for(std::size_t i = 0; i < Count; ++i){
                Data[i] = ObjectData[DataIndex++];
            }
        }

        return r;
    }

    HRESULT __stdcall Acquire(){
        HRESULT r = S_OK;
        DLOG("sys_mouse::Acquire(): %d\n", r);
        return r;
    }

    HRESULT __stdcall Unacquire(){
        HRESULT r = S_FALSE;
        DLOG("sys_mouse::Unacquire(): %d\n", r);
        return r;
    }

    HRESULT __stdcall SetCooperativeLevel(HWND Window, DWORD Flags){
        HRESULT r = S_OK;
        DLOG("sys_mouse::SetCooperativeLevel(0x%p, 0x%X): %d\n", Window, Flags, r);
        return r;
    }
};

struct direct_input:IDirectInput8W {
    sys_mouse SysMouse;

    HRESULT __stdcall QueryInterface(const IID&, void**) override {
        NYI("direct_input::QueryInterface()\n");
    }

    ULONG __stdcall AddRef() override {
        NYI("direct_input::AddRef()\n");
    }

    ULONG __stdcall Release() override {
        ULONG r = 0;
        DLOG("direct_input::Release(): %u\n", r);
        return 0;
    }

    HRESULT __stdcall CreateDevice(const GUID& Guid, IDirectInputDevice8W** Out, IUnknown* Outer) override {
        assert(Outer == nullptr);

        if(Guid == GUID_SysMouse){
            HRESULT r = S_OK;

            *Out = &SysMouse;

            DLOG("direct_input::CreateDevice(GUID_SysMouse, *0x%p = 0x%p, 0x%p): %d\n",
                Out, *Out, Outer, r);

            return r;
        }else{
            NYI("direct_input::CreateDevice({%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}, 0x%p, 0x%p)\n",
                Guid.Data1, Guid.Data2, Guid.Data3,
                Guid.Data4[0], Guid.Data4[1], Guid.Data4[2], Guid.Data4[3],
                Guid.Data4[4], Guid.Data4[5], Guid.Data4[6], Guid.Data4[7],
                Out, Outer
            );
        }
    }

    HRESULT __stdcall EnumDevices(DWORD, LPDIENUMDEVICESCALLBACKW, void*, DWORD) override {
        NYI("direct_input::EnumDevices()\n");
    }

    HRESULT __stdcall GetDeviceStatus(const GUID&) override {
        NYI("direct_input::GetDeviceStatus()\n");
    }

    HRESULT __stdcall RunControlPanel(HWND, DWORD) override {
        NYI("direct_input::RunControlPanel()\n");
    }

    HRESULT __stdcall Initialize(HINSTANCE, DWORD) override {
        NYI("direct_input::Initialize()\n");
    }

    HRESULT __stdcall FindDevice(const GUID&, const wchar_t*, GUID*) override {
        NYI("direct_input::FindDevice()\n");
    }

    HRESULT __stdcall EnumDevicesBySemantics(const wchar_t*, DIACTIONFORMATW*, LPDIENUMDEVICESBYSEMANTICSCBW, void*, DWORD) override {
        NYI("direct_input::EnumDevicesBySemantics()\n");
    }

    HRESULT __stdcall ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK, LPDICONFIGUREDEVICESPARAMSW, DWORD, void*) override {
        NYI("direct_input::ConfigureDevices()\n");
    }
};

direct_input DirectInput;

// User Input
constinit int RepeatKey = -1;
constinit std::int64_t RepeatTime;

constinit std::uint32_t KeyEventState[8] = {};

// Events
struct window_event {
    UINT Message;
    WPARAM WParam;
    LPARAM LParam;
};

std::vector<window_event> WindowEvents = {};

void push_key_event(int Key, bool Down){
    assert(0 < Key);
    assert(Key < 256);

    auto& State = KeyEventState[Key >> 5];
    auto Mask = (1u << (Key&31));
    auto IsDown = ((State&Mask) != 0);

    if(Down != IsDown){
        KeyEvents.push_back({Key, Down});
        if(Down){
            State |= Mask;
        }else{
            State &= ~Mask;
        }
    }
}

int translate_rawinput(const RAWKEYBOARD& Keyboard){
    UINT r = Keyboard.VKey;

    auto IsE0 = ((Keyboard.Flags&RI_KEY_E0) != 0);

    switch(r){
        case VK_CONTROL: {
            return VK_LCONTROL+IsE0;
        }
        case VK_MENU: {
            return VK_LMENU+IsE0;
        }
        case VK_SHIFT: {
            r = MapVirtualKeyW(Keyboard.MakeCode, MAPVK_VSC_TO_VK_EX);
            if(r != VK_LSHIFT && r != VK_RSHIFT){
                return -1;
            }
            break;
        }
        case VK_CANCEL: {
            return VK_PAUSE;
        }
        default: {
            if(IsE0){
                if(r == VK_RETURN){
                    return VK_SEPARATOR;
                }
            }else{
                switch(r){
                    case VK_INSERT: {
                        r = VK_NUMPAD0;
                        break;
                    }
                    case VK_END: {
                        r = VK_NUMPAD1;
                        break;
                    }
                    case VK_DOWN: {
                        r = VK_NUMPAD2;
                        break;
                    }
                    case VK_NEXT: {
                        r = VK_NUMPAD3;
                        break;
                    }
                    case VK_LEFT: {
                        r = VK_NUMPAD4;
                        break;
                    }
                    case VK_CLEAR: {
                        r = VK_NUMPAD5;
                        break;
                    }
                    case VK_RIGHT: {
                        r = VK_NUMPAD6;
                        break;
                    }
                    case VK_HOME: {
                        r = VK_NUMPAD7;
                        break;
                    }
                    case VK_UP: {
                        r = VK_NUMPAD8;
                        break;
                    }
                    case VK_PRIOR: {
                        r = VK_NUMPAD9;
                        break;
                    }
                    case VK_DELETE: {
                        r = VK_DECIMAL;
                        break;
                    }
                }
            }
        }
    }

    return static_cast<int>(r);
}

int translate_key(int Key, bool Shift){
    switch(Key){
        case VK_BACK: return '\b';
        case VK_TAB: return '\t';
        case VK_RETURN: return '\r';
        case VK_SPACE: return ' ';
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            static constexpr char Specials[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
            if(Shift){
                return Specials[Key-'0'];
            }else{
                return Key;
            }

            break;
        }
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
        case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': {
            if(Shift){
                return Key;
            }else{
                return Key|32;
            }
        }
        case VK_NUMPAD0: case VK_NUMPAD1: case VK_NUMPAD2: case VK_NUMPAD3: case VK_NUMPAD4:
        case VK_NUMPAD5: case VK_NUMPAD6: case VK_NUMPAD7: case VK_NUMPAD8: case VK_NUMPAD9: {
            if(!Shift){
                return Key-0x30;
            }
        }
        case VK_MULTIPLY: case VK_ADD: case VK_SUBTRACT: case VK_DECIMAL: case VK_DIVIDE: {
            return Key-0x40;
        }
        case VK_OEM_1: return Shift?':':';';
        case VK_OEM_PLUS: return Shift?'+':'=';
        case VK_OEM_COMMA: return Shift?'<':',';
        case VK_OEM_MINUS: return Shift?'_':'-';
        case VK_OEM_PERIOD: return Shift?'>':'.';
        case VK_OEM_2: return Shift?'?':'/';
        case VK_OEM_3: return Shift?'~':'`';
        case VK_OEM_4: return Shift?'{':'[';
        case VK_OEM_5:
        case VK_OEM_102: return Shift?'|':'\\';
        case VK_OEM_6: return Shift?'}':']';
        case VK_OEM_7: return Shift?'"':'\'';
    }

    return -1;
}

bool set_key_state(int Index, bool Down){
    std::uint32_t Mask = (1u << (Index&31));

    auto& State = KeyState[Index >> 5];
    auto PrevState = State;

    if(Down){
        State |= Mask;
    }else{
        State &= ~Mask;
    }

    return PrevState != State;
}

inline bool get_key(int Index){
    std::uint32_t Mask = (1u << (Index&31));
    return (KeyState[Index >> 5]&Mask) != 0;
}

LPARAM mouse_lparam(){
    return (static_cast<DWORD>(CursorPos.y) << 16)|static_cast<WORD>(CursorPos.x);
}

WPARAM mouse_wparam(){
    return
        get_key(VK_LBUTTON)*MK_LBUTTON |
        get_key(VK_RBUTTON)*MK_RBUTTON |
        (get_key(VK_LSHIFT) || get_key(VK_RSHIFT))*MK_SHIFT |
        (get_key(VK_LCONTROL) || get_key(VK_RCONTROL))*MK_CONTROL |
        get_key(VK_MBUTTON)*MK_MBUTTON |
        get_key(VK_XBUTTON1)*MK_XBUTTON1 |
        get_key(VK_XBUTTON2)*MK_XBUTTON2;
}

unsigned get_scancode(int Key){
    switch(Key){
        case VK_CANCEL:              return 0x146;
        case VK_BACK:                return 0x0E;
        case VK_TAB:                 return 0x0F;
        case VK_CLEAR:               return 0x4C;
        case VK_RETURN:              return 0x1C;
        case VK_SHIFT:               return 0x2A;
        case VK_CONTROL:             return 0x1D;
        case VK_MENU:                return 0x38;
        case VK_PAUSE:               return 0x1D; // E1
        case VK_CAPITAL:             return 0x3A;
        case VK_ESCAPE:              return 0x01;
        case VK_SPACE:               return 0x39;
        case VK_PRIOR:               return 0x49;
        case VK_NEXT:                return 0x51;
        case VK_END:                 return 0x4F;
        case VK_HOME:                return 0x47;
        case VK_LEFT:                return 0x4B;
        case VK_UP:                  return 0x48;
        case VK_RIGHT:               return 0x4D;
        case VK_DOWN:                return 0x50;
        case VK_SNAPSHOT:            return 0x54;
        case VK_INSERT:              return 0x52;
        case VK_DELETE:              return 0x53;
        case VK_HELP:                return 0x63;
        case '0':                    return 0x0B;
        case '1':                    return 0x02;
        case '2':                    return 0x03;
        case '3':                    return 0x04;
        case '4':                    return 0x05;
        case '5':                    return 0x06;
        case '6':                    return 0x07;
        case '7':                    return 0x08;
        case '8':                    return 0x09;
        case '9':                    return 0x0A;
        case 'A':                    return 0x1E;
        case 'B':                    return 0x30;
        case 'C':                    return 0x2E;
        case 'D':                    return 0x20;
        case 'E':                    return 0x12;
        case 'F':                    return 0x21;
        case 'G':                    return 0x22;
        case 'H':                    return 0x23;
        case 'I':                    return 0x17;
        case 'J':                    return 0x24;
        case 'K':                    return 0x25;
        case 'L':                    return 0x26;
        case 'M':                    return 0x32;
        case 'N':                    return 0x31;
        case 'O':                    return 0x18;
        case 'P':                    return 0x19;
        case 'Q':                    return 0x10;
        case 'R':                    return 0x13;
        case 'S':                    return 0x1F;
        case 'T':                    return 0x14;
        case 'U':                    return 0x16;
        case 'V':                    return 0x2F;
        case 'W':                    return 0x11;
        case 'X':                    return 0x2D;
        case 'Y':                    return 0x15;
        case 'Z':                    return 0x2C;
        case VK_LWIN:                return 0x15B;
        case VK_RWIN:                return 0x15C;
        case VK_APPS:                return 0x15D;
        case VK_SLEEP:               return 0x15F;
        case VK_NUMPAD0:             return 0x52;
        case VK_NUMPAD1:             return 0x4F;
        case VK_NUMPAD2:             return 0x50;
        case VK_NUMPAD3:             return 0x51;
        case VK_NUMPAD4:             return 0x4B;
        case VK_NUMPAD5:             return 0x4C;
        case VK_NUMPAD6:             return 0x4D;
        case VK_NUMPAD7:             return 0x47;
        case VK_NUMPAD8:             return 0x48;
        case VK_NUMPAD9:             return 0x49;
        case VK_MULTIPLY:            return 0x37;
        case VK_ADD:                 return 0x4E;
        case VK_SUBTRACT:            return 0x4A;
        case VK_DECIMAL:             return 0x53;
        case VK_DIVIDE:              return 0x135;
        case VK_F1:                  return 0x3B;
        case VK_F2:                  return 0x3C;
        case VK_F3:                  return 0x3D;
        case VK_F4:                  return 0x3E;
        case VK_F5:                  return 0x3F;
        case VK_F6:                  return 0x40;
        case VK_F7:                  return 0x41;
        case VK_F8:                  return 0x42;
        case VK_F9:                  return 0x43;
        case VK_F10:                 return 0x44;
        case VK_F11:                 return 0x57;
        case VK_F12:                 return 0x58;
        case VK_F13:                 return 0x64;
        case VK_F14:                 return 0x65;
        case VK_F15:                 return 0x66;
        case VK_F16:                 return 0x67;
        case VK_F17:                 return 0x68;
        case VK_F18:                 return 0x69;
        case VK_F19:                 return 0x6A;
        case VK_F20:                 return 0x6B;
        case VK_F21:                 return 0x6C;
        case VK_F22:                 return 0x6D;
        case VK_F23:                 return 0x6E;
        case VK_F24:                 return 0x76;
        case VK_NUMLOCK:             return 0x45;
        case VK_SCROLL:              return 0x46;
        case VK_LSHIFT:              return 0x2A;
        case VK_RSHIFT:              return 0x36;
        case VK_LCONTROL:            return 0x1D;
        case VK_RCONTROL:            return 0x11D;
        case VK_LMENU:               return 0x38;
        case VK_RMENU:               return 0x138;
        case VK_BROWSER_BACK:        return 0x16A;
        case VK_BROWSER_FORWARD:     return 0x169;
        case VK_BROWSER_REFRESH:     return 0x167;
        case VK_BROWSER_STOP:        return 0x168;
        case VK_BROWSER_SEARCH:      return 0x165;
        case VK_BROWSER_FAVORITES:   return 0x166;
        case VK_BROWSER_HOME:        return 0x132;
        case VK_VOLUME_MUTE:         return 0x120;
        case VK_VOLUME_DOWN:         return 0x12E;
        case VK_VOLUME_UP:           return 0x130;
        case VK_MEDIA_NEXT_TRACK:    return 0x119;
        case VK_MEDIA_PREV_TRACK:    return 0x110;
        case VK_MEDIA_STOP:          return 0x124;
        case VK_MEDIA_PLAY_PAUSE:    return 0x122;
        case VK_LAUNCH_MAIL:         return 0x16C;
        case VK_LAUNCH_MEDIA_SELECT: return 0x16D;
        case VK_LAUNCH_APP1:         return 0x16B;
        case VK_LAUNCH_APP2:         return 0x121;
        case VK_OEM_1:               return 0x27;
        case VK_OEM_PLUS:            return 0x0D;
        case VK_OEM_COMMA:           return 0x33;
        case VK_OEM_MINUS:           return 0x0C;
        case VK_OEM_PERIOD:          return 0x34;
        case VK_OEM_2:               return 0x35;
        case VK_OEM_3:               return 0x29;
        case VK_OEM_4:               return 0x1A;
        case VK_OEM_5:               return 0x2B;
        case VK_OEM_6:               return 0x1B;
        case VK_OEM_7:               return 0x28;
        case VK_OEM_102:             return 0x56;
        default: return 0;
    }
}

LPARAM keyboard_lparam(int Key, bool Down){
    return (Down?1:0xC0000001)|(get_scancode(Key) << 16);
}

}

void move_mouse(int x, int y){
    CursorPos.x = std::clamp(CursorPos.x+x, ClipCursorRect.left, ClipCursorRect.right);
    CursorPos.y = std::clamp(CursorPos.y+y, ClipCursorRect.top, ClipCursorRect.bottom);

    if(x != 0){
        DirectInput.SysMouse.add_object_data(0, x);
    }

    if(y != 0){
        DirectInput.SysMouse.add_object_data(4, y);
    }

    if(x != 0 || y != 0){
        WindowEvents.push_back({
            .Message = WM_MOUSEMOVE,
            .WParam = mouse_wparam(),
            .LParam = mouse_lparam(),
        });
    }
}

void scroll_wheel(short Delta){
    if(Delta != 0){
        DirectInput.SysMouse.add_object_data(8, Delta);

        WindowEvents.push_back({
            .Message = WM_MOUSEWHEEL,
            .WParam = (static_cast<DWORD>(static_cast<std::uint16_t>(Delta)) << 16)|mouse_wparam(),
            .LParam = mouse_lparam(),
        });
    }
}

bool set_key(int Key, bool Down){
    switch(Key){
        case VK_CONTROL: Key = VK_LCONTROL; break;
        case VK_SHIFT:   Key = VK_LSHIFT; break;
        case VK_MENU:    Key = VK_LMENU; break;
    }

    assert(0 <= Key);
    assert(Key < 256);

    if(set_key_state(Key, Down != 0)){
        UINT Message;
        WPARAM WParam = 0;
        switch(Key){
            case VK_LBUTTON: {
                Message = WM_LBUTTONDOWN;
                DirectInput.SysMouse.add_object_data(12, Down << 7);
                break;
            }
            case VK_RBUTTON: {
                Message = WM_RBUTTONDOWN;
                DirectInput.SysMouse.add_object_data(13, Down << 7);
                break;
            }
            case VK_MBUTTON: {
                Message = WM_MBUTTONDOWN;
                DirectInput.SysMouse.add_object_data(14, Down << 7);
                break;
            }
            case VK_XBUTTON1: {
                Message = WM_XBUTTONDOWN;
                DirectInput.SysMouse.add_object_data(15, Down << 7);
                WParam = XBUTTON1 << 16;
                break;
            }
            case VK_XBUTTON2: {
                Message = WM_XBUTTONDOWN;
                WParam = XBUTTON2 << 16;
                break;
            }
            default: {
                WParam = static_cast<WPARAM>(Key);
                switch(Key){
                    case VK_LCONTROL: WParam = VK_CONTROL; break;
                    case VK_RCONTROL: WParam = VK_CONTROL; break;
                    case VK_LSHIFT:   WParam = VK_SHIFT; break;
                    case VK_RSHIFT:   WParam = VK_SHIFT; break;
                    case VK_LMENU:    WParam = VK_MENU; break;
                    case VK_RMENU:    WParam = VK_MENU; break;
                }

                Message = WM_KEYDOWN;

                break;
            }
        }

        if(Message == WM_KEYDOWN){
            auto LParam = keyboard_lparam(Key, Down);

            WindowEvents.push_back({
                .Message = Message+(!Down),
                .WParam = WParam,
                .LParam = keyboard_lparam(Key, Down),
            });

            if(Down){
                RepeatKey = Key;
            }else if(RepeatKey == Key){
                RepeatKey = -1;
            }

            RepeatTime = Qpc.load()+RepeatDelay;

            if(Down && !get_key(VK_LCONTROL) && !get_key(VK_RCONTROL)){
                auto Char = translate_key(Key, get_key(VK_LSHIFT) || get_key(VK_RSHIFT));
                if(Char != -1){
                    WindowEvents.push_back({
                        .Message = WM_CHAR,
                        .WParam = static_cast<WPARAM>(Char),
                        .LParam = LParam|((get_key(VK_LMENU) || get_key(VK_RMENU)) << 29),
                    });
                }
            }
        }else{
            WindowEvents.push_back({
                .Message = Message+(!Down),
                .WParam = mouse_wparam()|WParam,
                .LParam = mouse_lparam(),
            });
        }

        return true;
    }else{
        return false;
    }
}

void flush_events(){
    for(auto& Event:WindowEvents){
        WndProc_Orig(MainWindow, Event.Message, Event.WParam, Event.LParam);
    }

    if(RepeatKey != -1){
        assert(get_key(RepeatKey));
        auto WParam = static_cast<WPARAM>(RepeatKey);
        for(auto End = Qpc.load(); RepeatTime <= End; RepeatTime += RepeatFrequency){
            auto LParam = keyboard_lparam(RepeatKey, true)|0x40000000;

            WndProc_Orig(MainWindow, WM_KEYDOWN, WParam, LParam);

            if(!get_key(VK_LCONTROL) && !get_key(VK_RCONTROL)){
                auto Char = translate_key(
                    RepeatKey, get_key(VK_LSHIFT) || get_key(VK_RSHIFT)
                );

                if(Char != -1){
                    WndProc_Orig(
                        MainWindow, WM_KEYDOWN, static_cast<WPARAM>(Char),
                        LParam|((get_key(VK_LMENU) || get_key(VK_RMENU)) << 29)
                    );
                }
            }
        }
    }

    WindowEvents.clear();

    KeyEvents.clear();
    MoveEvents.clear();
    ScrollEvents.clear();
}

void clip_cursor(bool Clip){
    ShouldClip = Clip;

    if(ShouldClip){
        ClipCursor_Orig(IsWindowActive?&ClipCursorRect:nullptr);
    }
}

LRESULT WINAPI WndProc_Hook(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){
    switch(Message){
        case WM_ACTIVATEAPP:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_NCHITTEST: {
            return DefWindowProcW(Window, Message, WParam, LParam);
        }

        case WM_ACTIVATE: {
            IsWindowActive = (LOWORD(WParam) != WA_INACTIVE);

            if(ShouldClip){
                ClipCursor_Orig(IsWindowActive?&ClipCursorRect:nullptr);
            }

            if(!IsWindowActive){
                for(int i = 0; auto& State:KeyEventState){
                    for(std::uint32_t m = 1; m != 0; m <<= 1, ++i){
                        if((State&m) != 0){
                            KeyEvents.push_back({i, false});
                        }
                    }
                    State = 0;
                }
            }

            return DefWindowProcW(Window, Message, WParam, LParam);
        }
        case WM_SETCURSOR: {
            if(LOWORD(LParam) == HTCLIENT && GetForegroundWindow_Orig() == Window){
                SetCursor(nullptr);
            }else{
                SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            }

            return TRUE;
        }

        case WM_MOUSEMOVE:

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:

        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:

        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:

        case WM_CHAR:
        case WM_UNICHAR: {
            return 0;
        }

        case WM_INPUT: {
            RAWINPUT RawInput;
            UINT Size = sizeof(RawInput);
            auto Handle = reinterpret_cast<HRAWINPUT>(LParam);
            auto r = GetRawInputData(Handle, RID_INPUT, &RawInput, &Size, sizeof(RawInput.header));
            if(r != static_cast<UINT>(-1)){
                switch(RawInput.header.dwType){
                    case RIM_TYPEMOUSE: {
                        auto& Mouse = RawInput.data.mouse;

                        if((Mouse.usFlags&MOUSE_MOVE_ABSOLUTE) == 0){
                            if(Mouse.lLastX != 0 || Mouse.lLastY != 0){
                                MoveEvents.push_back({Mouse.lLastX, Mouse.lLastY});
                            }
                        }

                        auto Buttons = Mouse.ulButtons;

                        if(Buttons&RI_MOUSE_WHEEL){
                            auto Scroll = static_cast<short>(Mouse.usButtonData);
                            if(Scroll != 0){
                                ScrollEvents.push_back(Scroll);
                            }
                        }

                        static constexpr int Keys[] = {
                            VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2
                        };

                        for(auto& Key:Keys){
                            switch(Buttons&3){
                                case RI_MOUSE_LEFT_BUTTON_DOWN: push_key_event(Key, true); break;
                                case RI_MOUSE_LEFT_BUTTON_UP: push_key_event(Key, false); break;
                            }

                            Buttons >>= 2;
                        }

                        break;
                    }
                    case RIM_TYPEKEYBOARD: {
                        auto& Keyboard = RawInput.data.keyboard;
                        switch(Keyboard.Message){
                            case WM_KEYDOWN:
                            case WM_KEYUP:
                            case WM_SYSKEYDOWN:
                            case WM_SYSKEYUP: {
                                auto Key = translate_rawinput(Keyboard);

                                if(Key != -1){
                                    push_key_event(Key, ((Keyboard.Flags&RI_KEY_BREAK) == 0));
                                }

                                break;
                            }
                        }

                        break;
                    }
                }
            }

            return 0;
        }
    }

    auto r = WndProc_Orig(Window, Message, WParam, LParam);

    DLOG("WndProc(0x%04X, 0x%08X, 0x%08X): %d\n", Message, WParam, LParam, r);

    return r;
}

namespace {

bool is_atom(const void* Str){
    return reinterpret_cast<std::uint32_t>(Str) < 0x10000;
}

int atom_int(const void* Str){
    return static_cast<int>(reinterpret_cast<std::uint32_t>(Str));
}

}

HWND WINAPI CreateWindowExW_Hook(
    DWORD ExStyle, const wchar_t* Class, const wchar_t* Title, DWORD Style,
    int x, int y, int Width, int Height, HWND Parent, HMENU Menu, HINSTANCE Instance, void* Param
){
    HWND r = nullptr;

    auto ExStyle2 = ExStyle;
    auto Style2 = Style;

    auto IsMainWindow = !is_atom(Class) && std::wcscmp(Class, L"LaunchUnrealUWindowsClient") == 0;

    if(IsMainWindow){
        Style2 = (Style2&~WS_CAPTION)|WS_POPUP;
    }

    r = CreateWindowExW_Orig(
            ExStyle2, Class, Title, Style2, x, y, Width, Height, Parent, Menu, Instance, Param);

    if(IsMainWindow){
        MainWindow = r;
    }

    if(is_atom(Class)){
        DLOG("CreateWindowExW(0x%08X, %d, \"%ls\", 0x%08X, %d, %d, %d, %d, 0x%p, 0x%p, 0x%p, 0x%p): 0x%p\n",
            ExStyle, atom_int(Class), Title, Style, x, y, Width, Height, Parent, Menu, Instance, Param, r);
    }else{
        DLOG("CreateWindowExW(0x%08X, \"%ls\", \"%ls\", 0x%08X, %d, %d, %d, %d, 0x%p, 0x%p, 0x%p, 0x%p): 0x%p\n",
            ExStyle, Class, Title, Style, x, y, Width, Height, Parent, Menu, Instance, Param, r);
    }

    return r;
}

BOOL WINAPI AdjustWindowRect_Hook(RECT* Rect, DWORD Style, BOOL Menu){
    auto Prev = *Rect;
    BOOL r = TRUE;
    DLOG("AdjustWindowRect(0x%p { %ld -> %ld, %ld -> %ld, %ld -> %ld, %ld -> %ld }, 0x%08X, %d): %d\n",
        Rect, Prev.left, Rect->left, Prev.top, Rect->top,
        Prev.right, Rect->right, Prev.bottom, Rect->bottom, Style, Menu, r);
    return r;
}

BOOL WINAPI AdjustWindowRectEx_Hook(RECT* Rect, DWORD Style, BOOL Menu, DWORD ExStyle){
    auto Prev = *Rect;
    BOOL r = TRUE;
    DLOG("AdjustWindowRectEx(0x%p { %ld -> %ld, %ld -> %ld, %ld -> %ld, %ld -> %ld }, 0x%08X, %d, 0x%08X): %d\n",
        Rect, Prev.left, Rect->left, Prev.top, Rect->top,
        Prev.right, Rect->right, Prev.bottom, Rect->bottom, Style, Menu, ExStyle, r);
    return r;
}

long WINAPI GetWindowLongW_Hook(HWND Window, int Index){
    auto r = GetWindowLongW_Orig(Window, Index);
    DLOG("GetWindowLongW(0x%p, %ld): 0x%08lX (%ld)\n", Window, Index, r, r);
    return r;
}

long WINAPI SetWindowLongW_Hook(HWND Window, int Index, long Value){
    long r;
    if(Window == MainWindow && Index == GWL_STYLE){
        r = GetWindowLongW_Orig(Window, Index);
    }else{
        if(Window == MainWindow && Index == GWL_WNDPROC){
            WndProc_Orig = reinterpret_cast<WNDPROC>(Value);
            Value = reinterpret_cast<long>(WndProc_Hook);

            RAWINPUTDEVICE Devices[] = {
                { // Mouse
                    .usUsagePage = 1,
                    .usUsage = 2,
                    .hwndTarget = Window,
                },
                { // Keyboard
                    .usUsagePage = 1,
                    .usUsage = 6,
                    .hwndTarget = Window,
                },
            };

            if(!RegisterRawInputDevices(Devices, std::size(Devices), sizeof(Devices[0]))){
                TerminateProcess(GetCurrentProcess(), __LINE__);
            }
        }

        r = SetWindowLongW_Orig(Window, Index, Value);
    }

    DLOG("SetWindowLongW(0x%p, %ld, 0x%08lX (%ld)): 0x%08lX (%ld)\n", Window, Index, Value, Value, r, r);

    return r;
}

long WINAPI GetWindowLongA_Hook(HWND Window, int Index){
    auto r = GetWindowLongA_Orig(Window, Index);
    DLOG("GetWindowLongA(0x%p, %ld): 0x%08lX (%ld)\n", Window, Index, r, r);
    return r;
}

long WINAPI SetWindowLongA_Hook(HWND Window, int Index, long Value){
    auto r = SetWindowLongA_Orig(Window, Index, Value);
    DLOG("SetWindowLongA(0x%p, %ld, 0x%08lX (%ld)): 0x%08lX (%ld)\n", Window, Index, Value, Value, r, r);
    return r;
}

BOOL WINAPI SystemParametersInfoA_Hook(UINT Action, UINT Param, void* Value, UINT WinIni){
    BOOL r = TRUE;
    if(Action == SPI_GETWORKAREA){
        FullScreen = {
            GetSystemMetrics(SM_XVIRTUALSCREEN),
            GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_CYVIRTUALSCREEN),
        };
        std::memcpy(Value, &FullScreen, sizeof(FullScreen));
    }else{
        r = SystemParametersInfoA_Orig(Action, Param, Value, WinIni);
    }
    DLOG("SystemParametersInfoA(0x%04X, 0x%X, 0x%p, 0x%X): %d\n", Action, Param, Value, WinIni, r);
    return r;
}

BOOL WINAPI SystemParametersInfoW_Hook(UINT Action, UINT Param, void* Value, UINT WinIni){
    BOOL r = TRUE;
    if(Action == SPI_GETWORKAREA){
        RECT Result = {
            GetSystemMetrics(SM_XVIRTUALSCREEN),
            GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_CYVIRTUALSCREEN),
        };
        std::memcpy(Value, &Result, sizeof(Result));
    }else{
        r = SystemParametersInfoW_Orig(Action, Param, Value, WinIni);
    }
    DLOG("SystemParametersInfoW(0x%04X, 0x%X, 0x%p, 0x%X): %d\n", Action, Param, Value, WinIni, r);
    return r;
}

BOOL WINAPI ClipCursor_Hook(const RECT* Rect){
    BOOL r = TRUE;

    if(!Rect){
        Rect = &FullScreen;
    }

    ClipCursorRect = *Rect;

    DLOG("ClipCursor(0x%p {%d, %d, %d, %d}): %d\n",
        Rect, Rect->left, Rect->top, Rect->right, Rect->bottom, r);

    return r;
}

BOOL WINAPI GetClipCursor_Hook(RECT* Rect){
    BOOL r = TRUE;

    *Rect = ClipCursorRect;

    DLOG("GetClipCursor(0x%p {%d, %d, %d, %d}): %d\n",
        Rect, Rect->left, Rect->top, Rect->right, Rect->bottom, r);

    return r;
}

HWND WINAPI SetCapture_Hook(HWND Window){
    auto r = WindowCapture;

    WindowCapture = Window;

    DLOG("SetCapture(0x%p): 0x%p\n", Window, r);

    return r;
}

HWND WINAPI GetCapture_Hook(){
    return WindowCapture;
}

BOOL WINAPI ReleaseCapture_Hook(){
    BOOL r = TRUE;

    WindowCapture = nullptr;

    DLOG("ReleaseCapture(): %d\n", r);

    return r;
}

BOOL WINAPI SetCursorPos_Hook(int x, int y){
    BOOL r = TRUE;
    CursorPos = {x, y};

    NYI("SetCursorPos(%d, %d): %d\n", x, y, r);

    // return r;
}

BOOL WINAPI GetCursorPos_Hook(POINT* Point){
    BOOL r = TRUE;
    *Point = CursorPos;

    DLOG("GetCursorPos(0x%p {%d, %d}): %d\n", Point, Point->x, Point->y, r);

    return r;
}

HWND WINAPI GetFocus_Hook(){
    auto r = MainWindow;

    DLOG("GetFocus(): 0x%p\n", r);

    return r;
}

HWND WINAPI GetForegroundWindow_Hook(){
    auto r = MainWindow;

    DLOG("GetForegroundWindow(): 0x%p\n", r);

    return r;
}

SHORT WINAPI GetKeyState_Hook(int Key){
    assert(0 < Key);
    assert(Key < 256);

    bool r;

    switch(Key){
        case VK_CONTROL: r = get_key(VK_LCONTROL) || get_key(VK_RCONTROL); break;
        case VK_SHIFT: r = get_key(VK_LSHIFT) || get_key(VK_RSHIFT); break;
        case VK_MENU: r = get_key(VK_LMENU) || get_key(VK_RMENU); break;
        default: r = get_key(Key); break;
    }

    return r?-128:0;
}

HRESULT WINAPI DirectInput8Create_Hook(
    HINSTANCE Instance, DWORD Version, const IID& Iid, void **Out, IUnknown* Outer
){
    assert(Outer == nullptr);

    *Out = &DirectInput;

    HRESULT r = S_OK;

    DLOG("DirectInput8Create(0x%p, %X, "
         "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}, *0x%p = 0x%p, 0x%p): %d\n",
        Instance, Version, Iid.Data1, Iid.Data2, Iid.Data3,
        Iid.Data4[0], Iid.Data4[1], Iid.Data4[2], Iid.Data4[3],
        Iid.Data4[4], Iid.Data4[5], Iid.Data4[6], Iid.Data4[7],
        Out, *Out, Outer, r
    );

    return r;
}

void WINAPI XInputEnable_Hook(BOOL Enable) noexcept {
    (void)Enable;
    NYI("XInputEnable()\n");
}

DWORD WINAPI XInputGetBatteryInformation_Hook(
    DWORD UserIndex, BYTE DevType, XINPUT_BATTERY_INFORMATION* Info
) noexcept {
    (void)UserIndex;
    (void)DevType;
    (void)Info;
    NYI("XInputGetBatteryInformation()\n");
}

DWORD WINAPI XInputGetCapabilities_Hook(
    DWORD UserIndex, DWORD Flags, XINPUT_CAPABILITIES* Capabilities
) noexcept {
    (void)UserIndex;
    (void)Flags;
    (void)Capabilities;
    NYI("XInputGetCapabilities()\n");
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids_Hook(
    DWORD UserIndex, GUID* RenderGuid, GUID* CaptureGuid
){
    (void)UserIndex;
    (void)RenderGuid;
    (void)CaptureGuid;
    NYI("XInputGetDSoundAudioDeviceGuids()\n");
}

DWORD WINAPI XInputGetKeystroke_Hook(
    DWORD UserIndex, DWORD Reserved, XINPUT_KEYSTROKE* Keystroke
) noexcept {
    (void)UserIndex;
    (void)Reserved;
    (void)Keystroke;
    NYI("XInputGetKeystroke()\n");
}

DWORD WINAPI XInputGetState_Hook(DWORD UserIndex, XINPUT_STATE* State) noexcept {
    DWORD r;
    if(UserIndex == 0){
        r = ERROR_SUCCESS;
        *State = XInputState;
    }else{
        r = ERROR_DEVICE_NOT_CONNECTED;
        *State = {};
    }

    DLOG("XInputGetState(%u, %p {%u, {0x%04X, %u, %u, %d, %d, %d, %d}}): %u\n",
        UserIndex, State,
        State->dwPacketNumber,
        State->Gamepad.wButtons,
        State->Gamepad.bLeftTrigger,
        State->Gamepad.bRightTrigger,
        State->Gamepad.sThumbLX,
        State->Gamepad.sThumbLY,
        State->Gamepad.sThumbRX,
        State->Gamepad.sThumbRY,
        r
    );

    return r;
}

DWORD WINAPI XInputSetState_Hook(DWORD UserIndex, XINPUT_VIBRATION* Vibration) noexcept {
    DWORD r;
    if(UserIndex == 0){
        r = ERROR_SUCCESS;
    }else{
        r = ERROR_DEVICE_NOT_CONNECTED;
    }

    DLOG("XInputSetState(%u, %p {%u, %u}): %u\n",
        UserIndex, Vibration, Vibration->wLeftMotorSpeed, Vibration->wRightMotorSpeed, r);

    return r;
}
