#include "defines.h"

#include "pytas.h"

#include <Python.h>

#include <memory>

#include "state.h"
#include "steam.h"
#include "hooks.h"
#include "window.h"

namespace {

struct vec3 {
    float x, y, z;
};

struct rot3 {
    int x, y, z;
};

struct game_state {
    vec3* Position;
    vec3* Velocity;
    rot3* Rotation;
};

game_state get_game_state(){
    char* PlayerPawn;
    char* PlayerController;

    auto Base = static_cast<char*>(GameModule.lpBaseOfDll);

    switch(GameVersion){
        case game_version::V12: {
            PlayerPawn = *reinterpret_cast<char**>(Base+0x1052DE8);
            PlayerController = *reinterpret_cast<char**>(Base+0x1052DD4);

            break;
        }
        case game_version::V14: {
            PlayerPawn = *reinterpret_cast<char**>(Base+0x1052DE8);
            PlayerController = *reinterpret_cast<char**>(Base+0x1052DD4);

            break;
        }
        default: {
            std::abort();
        }
    }

    return {
        .Position = reinterpret_cast<vec3*>(PlayerPawn+0xC4),
        .Velocity = reinterpret_cast<vec3*>(PlayerPawn+0x1B4),
        .Rotation = reinterpret_cast<rot3*>(PlayerController+0xD0),
    };
}

// Python stuff.
struct py_object_deleter {
    void operator()(PyObject* p){
        Py_DECREF(p);
    }
};

struct py_object {
    constexpr py_object(std::nullptr_t = nullptr):Object(nullptr) {}

    explicit py_object(PyObject* Object):Object(Object) {}

    py_object(py_object&& Rhs)=default;
    py_object& operator=(py_object&& Rhs)=default;

    py_object(const py_object& Rhs):Object(Rhs.Object.get()) {
        Py_XINCREF(Object.get());
    }

    py_object& operator=(const py_object& Rhs){
        Object.reset(Rhs);
        Py_XINCREF(Object.get());
        return *this;
    }

    PyObject* release(){ return Object.release(); }

    PyObject* get() const { return Object.get(); }
    operator PyObject*() const { return Object.get(); }

    std::unique_ptr<PyObject, py_object_deleter> Object;
};

struct pymem_deleter {
    void operator()(void* p) const {
        PyMem_Free(p);
    }
};

template <typename T>
using unique_pymem = std::unique_ptr<T, pymem_deleter>;

PyObject* py_set_frame_time(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwTime[] = "time";
    char* Kw[] = {KwTime, nullptr};

    int Time;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "i:set_frame_time", Kw, &Time)){
        return nullptr;
    }

    if(Time <= 0){
        std::abort();
    }

    FrameTime = Time;

    return Py_None;
}

PyObject* py_get_frame_time(PyObject*, PyObject*){
    return PyLong_FromLong(FrameTime);
}

PyObject* py_set_frame_wait(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwWait[] = "wait";
    char* Kw[] = {KwWait, nullptr};

    int Wait;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "p:set_frame_wait", Kw, &Wait)){
        return nullptr;
    }

    FrameWait = (Wait != 0);

    return Py_None;
}

PyObject* py_is_in_movie(PyObject*, PyObject*){
    return PyBool_FromLong(IsInMovie);
}

PyObject* py_save_cloud(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwName[] = "name";
    char* Kw[] = {KwName, nullptr};

    PyObject* NameObj;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "U:save_cloud", Kw, &NameObj)){
        return nullptr;
    }

    unique_pymem<wchar_t[]> Name(PyUnicode_AsWideCharString(NameObj, nullptr));
    assert(Name != nullptr);

    return PyBool_FromLong(save_steam_cloud(Name.get()));
}

PyObject* py_save_state(PyObject*, PyObject*){
    auto [p, v, r] = get_game_state();
    auto& Gamepad = XInputState.Gamepad;
    return Py_BuildValue("{s(fff)s(fff)s(iii)s(IIIIIIII)s(ii)s(iiiiiii)}",
        "position", p->x, p->y, p->z,
        "velocity", v->x, v->y, v->z,
        "rotation", r->x, r->y, r->z,
        "keys",
            KeyState[0], KeyState[1], KeyState[2], KeyState[3],
            KeyState[4], KeyState[5], KeyState[6], KeyState[7],
        "cursor", CursorPos.x, CursorPos.y,
        "gamepad",
            Gamepad.wButtons, Gamepad.bLeftTrigger, Gamepad.bRightTrigger,
            Gamepad.sThumbLX, Gamepad.sThumbLY, Gamepad.sThumbRX, Gamepad.sThumbRY
    );
}

PyObject* py_load_state(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwPosition[] = "position";
    static char KwVelocity[] = "velocity";
    static char KwRotation[] = "rotation";
    static char KwKeys[] = "keys";
    static char KwCursor[] = "cursor";
    static char KwGamepad[] = "gamepad";
    char* Kw[] = {KwPosition, KwVelocity, KwRotation, KwKeys, KwCursor, KwGamepad, nullptr};

    float px = 0.0f, py = 0.0f, pz = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    int rx = 0, ry = 0, rz = 0;
    std::uint32_t b[8] = {};
    int mx, my = {};
    int gb, glt, grt, glx, gly, grx, gry;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs,
            "(fff)|$(fff)(iii)(IIIIIIII)(ii)(iiiiiii):load_state", Kw,
            &px, &py, &pz, &vx, &vy, &vz, &rx, &ry, &rz,
            b, b+1, b+2, b+3, b+4, b+5, b+6, b+7,
            &mx, &my,
            &gb, &glt, &grt, &glx, &gly, &grx, &gry
    )){
        return nullptr;
    }

    auto [p, v, r] = get_game_state();
    *p = {px, py, pz};
    *v = {vx, vy, vz};
    *r = {rx, ry, rz};

    for(int i = 0; auto& Keys:KeyState){
        for(std::uint32_t m = 1; m != 0; m <<= 1, ++i){
            set_key(i, (Keys&m) != 0);
        }
    }

    CursorPos = {mx, my};

    XInputState.Gamepad = {
        .wButtons = static_cast<std::uint16_t>(gb),
        .bLeftTrigger = static_cast<std::uint8_t>(glt),
        .bRightTrigger = static_cast<std::uint8_t>(grt),
        .sThumbLX = static_cast<std::int16_t>(glx),
        .sThumbLY = static_cast<std::int16_t>(gly),
        .sThumbRX = static_cast<std::int16_t>(grx),
        .sThumbRY = static_cast<std::int16_t>(gry),
    };

    return Py_None;
}

PyObject* py_get_mouse_pos(PyObject*, PyObject*){
    py_object x(PyLong_FromLong(CursorPos.x));
    py_object y(PyLong_FromLong(CursorPos.y));

    PyObject* r = nullptr;

    if(x && y){
        r = PyTuple_Pack(2, x.get(), y.get());
        if(r){
            x.release();
            y.release();
        }
    }

    return r;
}

PyObject* py_get_key_events(PyObject*, PyObject*){
    py_object r(PyList_New(KeyEvents.size()));
    if(r){
        for(Py_ssize_t i = 0; auto& [Key, Down]:KeyEvents){
            py_object pKey(PyLong_FromLong(Key));
            py_object pDown(PyBool_FromLong(Down));
            if(pKey && pDown){
                auto Event = PyTuple_Pack(2, pKey.get(), pDown.get());
                if(!Event){
                    r = nullptr;
                    break;
                }

                pKey.release();
                pDown.release();

                PyList_SET_ITEM(r, i++, Event);
            }
        }

        return r.release();
    }

    return r.release();
}

PyObject* py_get_move_events(PyObject*, PyObject*){
    py_object r(PyList_New(MoveEvents.size()));
    if(r){
        for(Py_ssize_t i = 0; auto& [x, y]:MoveEvents){
            py_object px(PyLong_FromLong(x));
            py_object py(PyLong_FromLong(y));
            if(px && py){
                auto Event = PyTuple_Pack(2, px.get(), py.get());
                if(!Event){
                    r = nullptr;
                    break;
                }

                px.release();
                py.release();

                PyList_SET_ITEM(r, i++, Event);
            }
        }

        return r.release();
    }

    return r.release();
}

PyObject* py_get_scroll_events(PyObject*, PyObject*){
    py_object r(PyList_New(ScrollEvents.size()));
    if(r){
        for(Py_ssize_t i = 0; auto& Scroll:ScrollEvents){
            constexpr double Scale = 1.0/WHEEL_DELTA;
            auto Event = PyFloat_FromDouble(Scale*static_cast<double>(Scroll));
            if(!Event){
                r = nullptr;
                break;
            }

            PyList_SET_ITEM(r, i++, Event);
        }

        return r.release();
    }

    return r.release();
}

PyObject* py_query_performance_counter(PyObject*, PyObject*){
    LARGE_INTEGER r;
    QueryPerformanceCounter_Orig(&r);

    auto Freq = QpcFrequency_Orig.QuadPart;

    return PyLong_FromLongLong(
        r.QuadPart/Freq*QpcFrequency+
        r.QuadPart%Freq*QpcFrequency/Freq
    );
}

PyObject* py_clip_cursor(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwClip[] = "clip";
    char* Kw[] = {KwClip, nullptr};

    int Clip = 1;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "|p:clip_cursor", Kw, &Clip)){
        return nullptr;
    }

    clip_cursor(Clip != 0);

    return Py_None;
}

PyObject* py_get_clip_rect(PyObject*, PyObject*){
    py_object x0(PyLong_FromLong(ClipCursorRect.left));
    py_object y0(PyLong_FromLong(ClipCursorRect.top));
    py_object x1(PyLong_FromLong(ClipCursorRect.right));
    py_object y1(PyLong_FromLong(ClipCursorRect.bottom));

    PyObject* r = nullptr;

    if(x0 && y0 && x1 && y1){
        r = PyTuple_Pack(4, x0.get(), y0.get(), x1.get(), y1.get());
        if(r){
            x0.release();
            y0.release();
            x1.release();
            y1.release();
        }
    }

    return r;
}

PyObject* py_move_mouse(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwX[] = "x";
    static char KwY[] = "y";
    char* Kw[] = {KwX, KwY, nullptr};

    int x = 0, y = 0;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "|ii:move_mouse", Kw, &x, &y)){
        return nullptr;
    }

    move_mouse(x, y);

    return Py_None;
}

PyObject* py_scroll_wheel(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwValue[] = "value";
    char* Kw[] = {KwValue, nullptr};

    double Value = 0.0f;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "d:scroll_wheel", Kw, &Value)){
        return nullptr;
    }

    auto Delta = static_cast<std::int16_t>(
        std::clamp(std::round(WHEEL_DELTA*Value), -32768.0, 32767.0));

    scroll_wheel(Delta);

    return Py_None;
}

PyObject* py_set_key(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwKey[] = "key";
    static char KwDown[] = "down";
    char* Kw[] = {KwKey, KwDown, nullptr};

    int Key = 0, Down = 0;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "ip:set_key", Kw, &Key, &Down)){
        return nullptr;
    }

    set_key(Key, Down != 0);

    return Py_None;
}

// TODO: Move the logic to events.cpp?
static short convert_stick(double x){
    return static_cast<short>(std::clamp(
        std::round(x*(x < 0?32768.0:32767.0)), -32768.0, 32767.0
    ));
}

static BYTE convert_trigger(double x){
    return static_cast<BYTE>(std::clamp(std::round(x*255.0), 0.0, 255.0));
}

PyObject* py_set_gamepad_lstick(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwX[] = "x";
    static char KwY[] = "y";
    char* Kw[] = {KwX, KwY, nullptr};

    double x, y;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "ff:set_gamepad_lstick", Kw, &x, &y)){
        return nullptr;
    }

    XInputState.Gamepad.sThumbLX = convert_stick(x);
    XInputState.Gamepad.sThumbLY = convert_stick(y);

    return Py_None;
}

PyObject* py_set_gamepad_rstick(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwX[] = "x";
    static char KwY[] = "y";
    char* Kw[] = {KwX, KwY, nullptr};

    float x, y;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "ff:set_gamepad_rstick", Kw, &x, &y)){
        return nullptr;
    }

    XInputState.Gamepad.sThumbRX = convert_stick(x);
    XInputState.Gamepad.sThumbRY = convert_stick(y);

    return Py_None;
}

PyObject* py_set_gamepad_ltrigger(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwValue[] = "value";
    char* Kw[] = {KwValue, nullptr};

    float Value = 0;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "f:set_gamepad_ltrigger", Kw, &Value)){
        return nullptr;
    }

    XInputState.Gamepad.bLeftTrigger = convert_trigger(Value);

    return Py_None;
}

PyObject* py_set_gamepad_rtrigger(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwValue[] = "value";
    char* Kw[] = {KwValue, nullptr};

    float Value = 0;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "f:set_gamepad_rtrigger", Kw, &Value)){
        return nullptr;
    }

    XInputState.Gamepad.bRightTrigger = convert_trigger(Value);

    return Py_None;
}

PyObject* py_set_gamepad_button(PyObject*, PyObject* Args, PyObject *Kwargs){
    static char KwButton[] = "button";
    static char KwDown[] = "down";
    char* Kw[] = {KwButton, KwDown, nullptr};

    int Button = 0, Down = 0;
    if(!PyArg_ParseTupleAndKeywords(Args, Kwargs, "ip:set_gamepad_button", Kw, &Button, &Down)){
        return nullptr;
    }

    assert(0 <= Button);
    assert(Button < 16);

    if(Down){
        XInputState.Gamepad.wButtons =
            static_cast<WORD>(XInputState.Gamepad.wButtons|(1 << Button));
    }else{
        XInputState.Gamepad.wButtons =
            static_cast<WORD>(XInputState.Gamepad.wButtons&~(1 << Button));
    }

    return Py_None;
}

PyObject* init_pytas_module(){
    static PyMethodDef Methods[] = {
        {
            "set_frame_time", reinterpret_cast<PyCFunction>(py_set_frame_time), METH_VARARGS|METH_KEYWORDS,
            "Set the frame time.",
        },
        {
            "get_frame_time", py_get_frame_time, METH_NOARGS,
            "Get the frame time.",
        },
        {
            "set_frame_wait", reinterpret_cast<PyCFunction>(py_set_frame_wait), METH_VARARGS|METH_KEYWORDS,
            "Set the frame time.",
        },
        {
            "is_in_movie", py_is_in_movie, METH_NOARGS,
            "Tell whether the game is in a movie.",
        },
        {
            "save_cloud", reinterpret_cast<PyCFunction>(py_save_cloud), METH_VARARGS|METH_KEYWORDS,
            "Save the steam cloud to a folder.",
        },
        {
            "save_state", py_save_state, METH_NOARGS,
            "Save the current state.",
        },
        {
            "load_state", reinterpret_cast<PyCFunction>(py_load_state), METH_VARARGS|METH_KEYWORDS,
            "Load a previously saved state.",
        },
        {
            "get_mouse_pos", py_get_mouse_pos, METH_NOARGS,
            "Get the mouse location.",
        },
        {
            "get_key_events", py_get_key_events, METH_NOARGS,
            "Get key events since last frame.",
        },
        {
            "get_move_events", py_get_move_events, METH_NOARGS,
            "Get move events since last frame.",
        },
        {
            "get_scroll_events", py_get_scroll_events, METH_NOARGS,
            "Get scroll events since last frame.",
        },
        {
            "query_performance_counter", py_query_performance_counter, METH_NOARGS,
            "Query the time.",
        },
        {
            "clip_cursor", reinterpret_cast<PyCFunction>(py_clip_cursor), METH_VARARGS|METH_KEYWORDS,
            "Clip the cursor to the window.",
        },
        {
            "get_clip_rect", py_get_clip_rect, METH_NOARGS,
            "Get the clipping rectangle.",
        },
        {
            "move_mouse", reinterpret_cast<PyCFunction>(py_move_mouse), METH_VARARGS|METH_KEYWORDS,
            "Mouse the mouse.",
        },
        {
            "scroll_wheel", reinterpret_cast<PyCFunction>(py_scroll_wheel), METH_VARARGS|METH_KEYWORDS,
            "Scroll the wheel.",
        },
        {
            "set_key", reinterpret_cast<PyCFunction>(py_set_key), METH_VARARGS|METH_KEYWORDS,
            "Set a key.",
        },
        {
            "set_gamepad_lstick", reinterpret_cast<PyCFunction>(py_set_gamepad_lstick), METH_VARARGS|METH_KEYWORDS,
            "Set left gamepad stick.",
        },
        {
            "set_gamepad_rstick", reinterpret_cast<PyCFunction>(py_set_gamepad_rstick), METH_VARARGS|METH_KEYWORDS,
            "Set right gamepad stick.",
        },
        {
            "set_gamepad_ltrigger", reinterpret_cast<PyCFunction>(py_set_gamepad_ltrigger), METH_VARARGS|METH_KEYWORDS,
            "Set left gamepad trigger.",
        },
        {
            "set_gamepad_rtrigger", reinterpret_cast<PyCFunction>(py_set_gamepad_rtrigger), METH_VARARGS|METH_KEYWORDS,
            "Set right gamepad trigger.",
        },
        {
            "set_gamepad_button", reinterpret_cast<PyCFunction>(py_set_gamepad_button), METH_VARARGS|METH_KEYWORDS,
            "Set gamepad button.",
        },
        {nullptr, nullptr, 0, nullptr},
    };

    static PyModuleDef Def = {};
    Def.m_base    = PyModuleDef_HEAD_INIT;
    Def.m_name    = "_pytas";
    Def.m_size    = -1;
    Def.m_methods = Methods;

    auto r = PyModule_Create(&Def);
    if(!r){
        std::abort();
    }

    if(PyModule_AddIntMacro(r, VK_LBUTTON) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RBUTTON) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_CANCEL) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_MBUTTON) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_XBUTTON1) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_XBUTTON2) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BACK) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_TAB) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_CLEAR) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RETURN) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_SHIFT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_CAPITAL) != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_ESCAPE) != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_SPACE) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_PRIOR) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NEXT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_END) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_HOME) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LEFT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_UP) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RIGHT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_DOWN) != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_SNAPSHOT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_INSERT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_DELETE) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_HELP) != 0){ std::abort(); }

    if(PyModule_AddIntConstant(r, "VK_0", '0') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_1", '1') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_2", '2') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_3", '3') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_4", '4') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_5", '5') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_6", '6') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_7", '7') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_8", '8') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_9", '9') != 0){ std::abort(); }

    if(PyModule_AddIntConstant(r, "VK_A", 'A') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_B", 'B') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_C", 'C') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_D", 'D') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_E", 'E') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_F", 'F') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_G", 'G') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_H", 'H') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_I", 'I') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_J", 'J') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_K", 'K') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_L", 'L') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_M", 'M') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_N", 'N') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_O", 'O') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_P", 'P') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_Q", 'Q') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_R", 'R') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_S", 'S') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_T", 'T') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_U", 'U') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_V", 'V') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_W", 'W') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_X", 'X') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_Y", 'Y') != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "VK_Z", 'Z') != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_LWIN) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RWIN) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_APPS) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_SLEEP) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD0) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD1) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD2) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD3) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD4) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD5) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD6) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD7) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD8) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMPAD9) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_MULTIPLY) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_ADD) != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_SUBTRACT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_DECIMAL) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_DIVIDE) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F1) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F2) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F3) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F4) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F5) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F6) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F7) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F8) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F9) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F10) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F11) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F12) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F13) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F14) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F15) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F16) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F17) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F18) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F19) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F20) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F21) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F22) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F23) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_F24) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_NUMLOCK) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_SCROLL) != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_LSHIFT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RSHIFT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LCONTROL) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RCONTROL) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LMENU) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_RMENU) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_BACK) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_FORWARD) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_REFRESH) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_STOP) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_SEARCH) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_FAVORITES) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_BROWSER_HOME) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_VOLUME_MUTE) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_VOLUME_DOWN) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_VOLUME_UP) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_MEDIA_NEXT_TRACK) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_MEDIA_PREV_TRACK) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_MEDIA_STOP) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_MEDIA_PLAY_PAUSE) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LAUNCH_MAIL) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LAUNCH_MEDIA_SELECT) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LAUNCH_APP1) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_LAUNCH_APP2) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_1) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_PLUS) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_COMMA) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_MINUS) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_PERIOD) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_2) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_3) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_4) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_5) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_6) != 0){ std::abort(); }
    if(PyModule_AddIntMacro(r, VK_OEM_7) != 0){ std::abort(); }

    if(PyModule_AddIntMacro(r, VK_OEM_102) != 0){ std::abort(); }

    if(PyModule_AddIntConstant(r, "GAMEPAD_DPAD_UP", 0) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_DPAD_DOWN", 1) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_DPAD_LEFT", 2) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_DPAD_RIGHT", 3) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_START", 4) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_BACK", 5) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_LEFT_THUMB", 6) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_RIGHT_THUMB", 7) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_LEFT_SHOULDER", 8) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_RIGHT_SHOULDER", 9) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_A", 12) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_B", 13) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_X", 14) != 0){ std::abort(); }
    if(PyModule_AddIntConstant(r, "GAMEPAD_Y", 15) != 0){ std::abort(); }

    if(PyModule_AddIntConstant(r, "FREQUENCY", QpcFrequency) != 0){ std::abort(); }

    return r;
}

constinit py_object Recipe = nullptr;

}

// TODO: Call Py_FinalizeEx() somewhere.

void pytas_init(int Argc, wchar_t** Argv){
    PyConfig Config;
    PyConfig_InitIsolatedConfig(&Config);

    for(int i = 0; i < Argc; ++i){
        if(PyStatus_Exception(PyWideStringList_Append(&Config.argv, Argv[i]))){
            std::abort();
        }
    }

    PyImport_AppendInittab("_pytas", init_pytas_module);

    auto Status = Py_InitializeFromConfig(&Config);
    if(PyStatus_Exception(Status)){
        std::abort();
    }

    PyConfig_Clear(&Config);

    auto Path = PySys_GetObject("path");
    if(!Path || !PyList_Check(Path)){
        throw std::runtime_error("Unable to get `sys.path`.");
    }

    auto& ScriptsStr = CmdArgs.Scripts.native();

    py_object ScriptsPath(PyUnicode_FromWideChar(ScriptsStr.c_str(), ScriptsStr.size()));
    if(!ScriptsPath){
        throw std::runtime_error("Unable to create scripts path.");
    }

    if(PyList_Insert(Path, 0, ScriptsPath) != 0){
        throw std::runtime_error("Unable to augment `sys.path`.");
    }

    ScriptsPath = nullptr;

    py_object PyModuleName(PyUnicode_FromWideChar(CmdArgs.Main.c_str(), CmdArgs.Main.size()));
    if(!PyModuleName){
        std::abort();
    }

    py_object PyModule(PyImport_Import(PyModuleName));
    if(!PyModule){
        std::abort();
    }

    PyModuleName = nullptr;

    py_object Func(PyObject_GetAttrString(PyModule, "main"));
    if(!Func || !PyCallable_Check(Func)){
        std::abort();
    }

    Recipe = py_object(PyObject_CallNoArgs(Func));
    if(!Recipe){
        std::abort();
    }

    if(!PyIter_Check(Recipe)){
        std::abort();
    }
}

void pytas_next(){
    if(py_object r(PyIter_Next(Recipe)); !r){
        if(PyErr_Occurred()){
            PyErr_Print();
        }
        std::abort();
    }
}
