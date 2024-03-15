from typing import TypedDict

FREQUENCY = 10000000

VK_LBUTTON: int
VK_RBUTTON: int
VK_CANCEL: int
VK_MBUTTON: int
VK_XBUTTON1: int
VK_XBUTTON2: int
VK_BACK: int
VK_TAB: int
VK_CLEAR: int
VK_RETURN: int
VK_SHIFT: int
VK_CONTROL: int
VK_MENU: int
VK_PAUSE: int
VK_CAPITAL: int

VK_ESCAPE: int

VK_SPACE: int
VK_PRIOR: int
VK_NEXT: int
VK_END: int
VK_HOME: int
VK_LEFT: int
VK_UP: int
VK_RIGHT: int
VK_DOWN: int

VK_SNAPSHOT: int
VK_INSERT: int
VK_DELETE: int
VK_HELP: int

VK_0: int
VK_1: int
VK_2: int
VK_3: int
VK_4: int
VK_5: int
VK_6: int
VK_7: int
VK_8: int
VK_9: int

VK_A: int
VK_B: int
VK_C: int
VK_D: int
VK_E: int
VK_F: int
VK_G: int
VK_H: int
VK_I: int
VK_J: int
VK_K: int
VK_L: int
VK_M: int
VK_N: int
VK_O: int
VK_P: int
VK_Q: int
VK_R: int
VK_S: int
VK_T: int
VK_U: int
VK_V: int
VK_W: int
VK_X: int
VK_Y: int
VK_Z: int

VK_LWIN: int
VK_RWIN: int
VK_APPS: int
VK_SLEEP: int
VK_NUMPAD0: int
VK_NUMPAD1: int
VK_NUMPAD2: int
VK_NUMPAD3: int
VK_NUMPAD4: int
VK_NUMPAD5: int
VK_NUMPAD6: int
VK_NUMPAD7: int
VK_NUMPAD8: int
VK_NUMPAD9: int
VK_MULTIPLY: int
VK_ADD: int

VK_SUBTRACT: int
VK_DECIMAL: int
VK_DIVIDE: int
VK_F1: int
VK_F2: int
VK_F3: int
VK_F4: int
VK_F5: int
VK_F6: int
VK_F7: int
VK_F8: int
VK_F9: int
VK_F10: int
VK_F11: int
VK_F12: int
VK_F13: int
VK_F14: int
VK_F15: int
VK_F16: int
VK_F17: int
VK_F18: int
VK_F19: int
VK_F20: int
VK_F21: int
VK_F22: int
VK_F23: int
VK_F24: int
VK_NAVIGATION_VIEW: int
VK_NAVIGATION_MENU: int
VK_NAVIGATION_UP: int
VK_NAVIGATION_DOWN: int
VK_NAVIGATION_LEFT: int
VK_NAVIGATION_RIGHT: int
VK_NAVIGATION_ACCEPT: int
VK_NAVIGATION_CANCEL: int
VK_NUMLOCK: int
VK_SCROLL: int

VK_LSHIFT: int
VK_RSHIFT: int
VK_LCONTROL: int
VK_RCONTROL: int
VK_LMENU: int
VK_RMENU: int
VK_BROWSER_BACK: int
VK_BROWSER_FORWARD: int
VK_BROWSER_REFRESH: int
VK_BROWSER_STOP: int
VK_BROWSER_SEARCH: int
VK_BROWSER_FAVORITES: int
VK_BROWSER_HOME: int
VK_VOLUME_MUTE: int
VK_VOLUME_DOWN: int
VK_VOLUME_UP: int
VK_MEDIA_NEXT_TRACK: int
VK_MEDIA_PREV_TRACK: int
VK_MEDIA_STOP: int
VK_MEDIA_PLAY_PAUSE: int
VK_LAUNCH_MAIL: int
VK_LAUNCH_MEDIA_SELECT: int
VK_LAUNCH_APP1: int
VK_LAUNCH_APP2: int
VK_OEM_1: int
VK_OEM_PLUS: int
VK_OEM_COMMA: int
VK_OEM_MINUS: int
VK_OEM_PERIOD: int
VK_OEM_2: int
VK_OEM_3: int
VK_OEM_4: int
VK_OEM_5: int
VK_OEM_6: int
VK_OEM_7: int

VK_OEM_102: int

class __STATE(TypedDict):
    position: tuple[float, float, float]
    velocity: tuple[float, float, float]
    rotation: tuple[int, int, int]
    keys: tuple[int, int, int, int, int, int, int, int]
    cursor: tuple[int, int]
    gamepad: tuple[int, int, int, int, int, int, int]

def set_frame_time(time: int):
    pass

def get_frame_time() -> int:
    pass

def set_frame_wait(wait: bool):
    pass

def is_in_movie() -> bool:
    pass

def save_cloud(name: str) -> bool:
    pass

def save_state() -> __STATE:
    pass

def load_state(
    position: tuple[float, float, float],
    *,
    velocity: tuple[float, float, float] = ...,
    rotation: tuple[int, int, int] = ...,
    keys: tuple[int, int, int, int, int, int, int, int] = ...,
    cursor: tuple[int, int] = ...,
    gamepad: tuple[int, int, int, int, int, int, int] = ...,
):
    pass

def get_mouse_pos() -> tuple[int, int]:
    pass

def get_key_events() -> list[tuple[int, bool]]:
    pass

def get_move_events() -> list[tuple[int, int]]:
    pass

def get_scroll_events() -> list[int]:
    pass

def query_performance_counter() -> int:
    pass

def clip_cursor(clip: bool = ...):
    pass

def get_clip_rect() -> tuple[int, int, int, int]:
    pass

def move_mouse(x: int = ..., y: int = ...):
    pass

def scroll_wheel(value: float):
    pass

def set_key(key: int, down: bool):
    pass

def set_gamepad_lstick(x: float, y: float):
    pass

def set_gamepad_rstick(x: float, y: float):
    pass

def set_gamepad_ltrigger(value: float):
    pass

def set_gamepad_rtrigger(value: float):
    pass

def set_gamepad_button(button: int, down: bool):
    pass
