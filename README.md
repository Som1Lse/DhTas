# DhTas

A TAS tool for Dishonored. Still in very early development.

The main approach is to run Dishonored in a sandboxed environment that can be controlled via Python. This is achieved by injecting a DLL into the game on launch. The DLL then hooks various functions in order to control the behaviour of the game, and exposes them to a Python script.

# Running

In order to run the TAS tool, run `dhtas.exe`. It will launch Dishonored and inject `dhtashook.dll` into it, and forward command line parameters.

The command line arguments are supported:
 - `--exe <path>`: Which Dishonored executable to run. By default it will use the registry to find the Steam version. The Steam 1.2 and 1.4 versions are supported.
 - `--documents <path>`: What the game will treat as the "Documents" folder. It will read and write config files to this location. By default this is either `datafiles/documents12` or `datafiles/documents14` depending on the game version.
 - `--userdata <path>`: Where to load the Steam cloud from. The reads the files from this folder on startup and then feeds them to the game whenever it uses the Steam cloud. By default `datafiles/userdata` is used.
 - `--scripts <path>`: Where to load Python scripts from. By default `datafiles/scripts` is used.
 - `--main <name>`: The module name to load the `main` function from. By default `main` is used, which will load `main.py`.

The tool requires `bSmoothFrameRate` to be `FALSE`.

## Scripting

The Python script loaded by the TAS tool must expose a `main` function. This function should be a generator, every time it `yield`s the TAS will run a frame.

The `_pytas` module provides various functions to control the game. See `typings/_pytas.pyi` for an overview of these. The most obviously useful ones are `set_frame_time` to set how long a frame should last, and `move_mouse`/`scroll_wheel`/`set_key`/`set_gamepad_lstick`/`set_gamepad_rstick`/`set_gamepad_ltrigger`/`set_gamepad_rtrigger`/`set_gamepad_button` to simulate game inputs.

See `any.py` for an example of how to TAS a level, and `record.py` for how to run the game with user inputs, while recording them to a file (`recording.py`).

# Building

Since Dishonored is a 32-bit game, the TAS tool should be built with a 32-bit compiler. Currently the code base is written assuming MSVC, but this shouldn't be too hard to change.

You will need [CMake](https://cmake.org/), [Ninja](https://github.com/ninja-build/ninja/releases), and [Python (32-bit version)](https://www.python.org/downloads/windows/). Simply run:
```batch
> "<path-to-visual-studio>\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
> mkdir build
> cd build
> cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DPython3_ROOT_DIR=<path-to-python> -DCMAKE_INSTALL_PREFIX="%cd%/install"
> ninja
> ninja install
```

If you want to be able to Debug the tool, you will need to build a Debug build of Python from source. You can read the [official build guide](https://devguide.python.org/getting-started/setup-building/) for instructions. The following commands worked for me:
```batch
> git clone https://github.com/python/cpython
> cd cpython
> git checkout 3.12
> PCbuild\build.bat -c Debug -p Win32
```
and then copying `cpython/PCbuild/win32/python312_d.dll` and `cpython/PCbuild/win32/python312_d.lib` to the Python installation, next to `python312.dll` and `libs/python312.lib` respectively.
