from _pytas import (
    FREQUENCY,
    set_frame_wait,
    set_frame_time,
    is_in_movie,
    move_mouse,
    set_key,
    VK_LBUTTON,
    VK_LSHIFT,
    VK_SPACE,
    VK_F,
    VK_D,
    VK_W,
)

from utilities import wait, compute_turn, interactive


FAST_INTRO = True


def main():
    set_frame_time(FREQUENCY // 60)

    yield from wait(8)

    set_key(VK_F, True)  # Press any key

    yield

    set_key(VK_F, False)

    yield from wait(40)

    set_key(VK_F, True)  # New game

    yield

    set_key(VK_F, False)

    yield from wait(19)

    set_key(VK_W, True)  # Normal -> Easy
    set_key(VK_F, True)  # Easy
    yield
    set_key(VK_F, False)
    set_key(VK_W, False)

    yield from wait(15)

    set_key(VK_F, True)  # Brightness

    yield

    set_key(VK_F, False)

    yield from wait(15)

    set_key(VK_F, True)  # Yes

    yield

    set_key(VK_F, False)

    yield from wait(88)

    assert not is_in_movie()

    yield

    assert is_in_movie()

    set_key(VK_F, True)  # Loading

    yield

    set_key(VK_F, False)

    yield

    assert is_in_movie()

    set_key(VK_LBUTTON, True)  # Movie

    yield

    set_key(VK_LBUTTON, False)

    while is_in_movie():
        yield

    if FAST_INTRO:
        set_frame_wait(False)
        set_frame_time(FREQUENCY * 2 // 5)
        yield from wait(251)
        set_frame_time(FREQUENCY // 60)
        yield from wait(3)
    else:
        yield from wait(5930)

    set_key(VK_LSHIFT, True)
    set_key(VK_W, True)

    yield from wait(78)

    set_key(VK_SPACE, True)

    yield from wait(4)

    set_key(VK_W, False)
    move_mouse(x=9000)

    yield from wait(8)

    move_mouse(x=-9500)
    set_key(VK_W, True)
    set_key(VK_SPACE, False)

    yield from wait(8)

    yield from wait(42)
    set_key(VK_SPACE, True)
    yield from wait(32)
    set_key(VK_SPACE, False)
    yield from wait(12)
    set_key(VK_SPACE, True)
    yield
    set_key(VK_SPACE, False)
    yield from wait(43)

    yield from wait(95)

    move_mouse(x=-14500)

    yield from wait(10)
    move_mouse(x=-4500)

    yield from wait(15)
    set_key(VK_W, False)
    move_mouse(x=-28250)

    yield from wait(5)

    set_key(VK_SPACE, True)
    set_frame_time(FREQUENCY // 250)

    yield from wait(70)

    set_key(VK_W, True)

    yield from wait(37)

    set_frame_time(FREQUENCY // 60)
    set_key(VK_SPACE, False)

    yield from wait(15)

    move_mouse(-8750, 8000)
    set_key(VK_D, True)

    yield from wait(18)

    turn = compute_turn(-8192, 1 / 2.5)
    move_mouse(x=turn)

    yield

    set_frame_time(FREQUENCY * 2 // 5)

    set_key(VK_SPACE, True)

    yield

    set_key(VK_W, False)
    set_key(VK_D, False)
    set_key(VK_SPACE, False)

    set_frame_time(FREQUENCY // 60)

    move_mouse(-2642)
    yield
    move_mouse(-1)
    yield from wait(10)

    set_key(VK_W, True)

    yield from wait(125)

    set_key(VK_SPACE, True)
    yield from wait(60)
    set_key(VK_SPACE, False)

    set_frame_wait(True)

    yield from wait(20)

    set_key(VK_SPACE, True)
    yield
    set_key(VK_SPACE, False)
    set_key(VK_W, False)

    yield

    set_key(VK_F, True)

    move_mouse(x=-10997)

    yield from wait(94)

    move_mouse(x=-26800)
    set_key(VK_W, True)

    yield from wait(110)

    t = -6800
    r = t
    n = 5
    for _ in range(n):
        x = t // n
        r -= x
        move_mouse(x=x)
        yield

    if r:
        move_mouse(x=r)

    yield from wait(101)

    move_mouse(x=-16000)

    yield

    set_frame_time(FREQUENCY // 8)

    yield

    set_frame_time(FREQUENCY // 60)

    yield from interactive()
