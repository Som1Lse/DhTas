import os
import sys

from pathlib import Path
from typing import Protocol

from _pytas import (
    FREQUENCY,
    set_frame_wait,
    set_frame_time,
    move_mouse,
    save_cloud,
    set_key,
    scroll_wheel,
    clip_cursor,
    query_performance_counter,
    get_key_events,
    get_move_events,
    get_scroll_events,
    VK_F1,
    VK_F2,
    VK_F3,
    VK_F4,
    VK_H,
    VK_K,
)


class Writeable(Protocol):
    def write(self, __s: str) -> int: ...

    def flush(self): ...


def record_input(f: Writeable):
    f.write(
        """from _pytas import (
    move_mouse, scroll_wheel, set_frame_time, set_frame_wait, set_key
)

def main():
"""
    )

    clip_cursor()

    pdt = 0

    t0 = 0

    set_frame_wait(False)

    min_dt = FREQUENCY // 250

    do_wait = True

    while True:
        if do_wait and t0 != 0:
            t1 = query_performance_counter()

            tn = max(t0 + min_dt, t1)

            while t1 < tn:
                t1 = query_performance_counter()

            dt = tn - t0

            t0 = tn
        else:
            t0 = query_performance_counter()

            dt = min_dt

        if dt != pdt:
            f.write(f"    set_frame_time({dt})\n")
            set_frame_time(dt)
            pdt = dt

        for k, d in get_key_events():
            if d:
                if k == VK_F1:
                    min_dt = FREQUENCY * 2 // 5
                elif k == VK_F2:
                    min_dt = FREQUENCY // 5
                elif k == VK_F3:
                    min_dt = FREQUENCY // 100
                elif k == VK_F4:
                    min_dt = FREQUENCY // 250

            if k == VK_H:
                do_wait = not d
                f.write(f"    set_frame_wait({d})\n")

            if k == VK_K and d:
                save_cloud(os.path.join(sys.prefix, "cloud"))

            set_key(k, d)
            f.write(f"    set_key({k}, {d})\n")

        for x, y in get_move_events():
            move_mouse(x, y)
            f.write(f"    move_mouse({x}, {y})\n")

        for value in get_scroll_events():
            scroll_wheel(value)
            f.write(f"    scroll_wheel({value})\n")

        f.write("    yield\n")
        f.flush()

        yield


def main():
    with open(
        Path(sys.prefix) / "recording.py", "w", newline="\n", encoding="utf8"
    ) as f:
        print(f.name)
        yield from record_input(f)
