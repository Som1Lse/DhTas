from math import atan2, cos, pi, sin, sqrt
from typing import Any, Sequence

import _pytas
from _pytas import (
    save_state,
    move_mouse,
    get_mouse_pos,
)


def move_to(x: int, y: int):
    px, py = get_mouse_pos()
    move_mouse(x - px, y - py)


def wait(n: int):
    for _ in range(n):
        yield


def interactive(**env: object):
    yield

    i = 1

    globl: dict[str, Any] = globals() | _pytas.__dict__

    while True:
        source = input(f"# {i} ").strip()

        if source:
            if i == 1:
                print("    yield")
            elif i > 1:
                print(f"    yield from wait({i})")

            i = 0

            try:
                r = None

                if "=" in source:
                    exec(source, globl, env)
                elif source.startswith("yield from "):
                    r = yield from eval(source[11:], globl, env)
                else:
                    r = eval(source, globl, env)

                if r is None:
                    print(f"    {source}")
                else:
                    print(f"    {source}  # {r!r}")
            except Exception as e:
                for x in str(e).split("\n"):
                    print(f"# {x}")
        else:
            yield
            i += 1


def position():
    return save_state()["position"]


def velocity():
    return save_state()["velocity"]


def rotation() -> tuple[int, int, int]:
    return save_state()["rotation"]


def sqr_norm(seq: Sequence[float]):
    return sum(x * x for x in seq)


def norm(seq: Sequence[float]):
    return sqrt(sqr_norm(seq))


TURN_TO_RAD = pi / 32768
RAD_TO_TURN = 32768 / pi


# See docs/movement.pdf.
def compute_turn(turn: int, dt: float, a: float = 2000 / 600):
    f = 8

    u1 = cos(TURN_TO_RAD * turn)
    u2 = sin(TURN_TO_RAD * turn)

    dtaf = dt * (a + f)
    dtf1 = dt * f - 1
    dtf1u2 = dtf1 * u2

    m = sqrt(dtaf * dtaf - dtf1u2 * dtf1u2) - dtf1 * u1

    d1 = m * u1 + dtf1
    d2 = m * u2

    return round(RAD_TO_TURN * atan2(d2, d1))
