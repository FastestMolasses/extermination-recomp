#!/usr/bin/env python3
"""AREA21's creature (src/game/em_area21_creature_b*.c) against the ORIGINAL
instructions: the behaviour 0014EC00 of AREA21 code group 0x82A540 [0], its
set-up 0014ED10, the states 0014EDD0 / 00150190 / 00150820, the moves
0014EEF0, 0014F390, 0014FBC0, 00150260, 001504D0, 00150600, and the pieces
00150850, 00150900, 001509A0, 00150DA0, 00150F40, 00151200, 001513B0.
Unwired.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area17_creature_reference.py configures it. Every entry runs as
original code from the user's ELF over a fork recording's RAM
(a03b_03_item2a: boot code is the same in every image; its AREA03 overlay
text is checked against the user's extract/OVERLAY/AREA03.BIN and its boot
text against the pinned ELF). Calls between the family's functions run as
original code at the top level (every one is also an entry); every other
callee is a hook with a scripted result, except the pure helpers in MATH,
which run as original code nested in the oracle (their writes replayed to
the native side). 001283D0 and 0014D1E0 are hooks here (a sibling module
translates them). The node's +0x4C method is stubbed and logged. The
designed node, its linked effect and the bone table behind 0x275B40 live in
zero RAM at FREE. Compared per case: every call and its arguments, memory
at every call entry, the memory accesses between calls one for one in
order, memory after the last store, the result, a poisoned re-run,
coverage of every reachable original word (DEAD excepted, each with its
reason), the callee inventory and the fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA21_CREATURE_B_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA21_CREATURE_B_SOURCE=
<dir> tests another copy of the module sources; EM_AREA21_CREATURE_B_SHOW=<n>
prints n failures; EM_AREA21_CREATURE_B_MISSING=1 lists unexecuted words;
EM_AREA21_CREATURE_B_TRACE=<label> prints the original's calls and accesses
of matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
from pathlib import Path
import gen_area21_creature_b as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
UU = [U, U]
FUNCS = {
    0x0014EC00: ('em_area21_creature_b_0014EC00', 0x108, [U], None),
    0x0014ED10: ('em_area21_creature_b_0014ED10', 0xC0, UU, None),
    0x0014EDD0: ('em_area21_creature_b_0014EDD0', 0x114, UU, None),
    0x0014EEF0: ('em_area21_creature_b_0014EEF0', 0x4A0, UU, None),
    0x0014F390: ('em_area21_creature_b_0014F390', 0x82C, UU, None),
    0x0014FBC0: ('em_area21_creature_b_0014FBC0', 0x5CC, UU, None),
    0x00150190: ('em_area21_creature_b_00150190', 0xC8, UU, None),
    0x00150260: ('em_area21_creature_b_00150260', 0x26C, UU, None),
    0x001504D0: ('em_area21_creature_b_001504D0', 0x128, UU, None),
    0x00150600: ('em_area21_creature_b_00150600', 0x21C, UU, None),
    0x00150820: ('em_area21_creature_b_00150820', 0x30, [U], None),
    0x00150850: ('em_area21_creature_b_00150850', 0xA8, [U], None),
    0x00150900: ('em_area21_creature_b_00150900', 0x9C, UU, None),
    0x001509A0: ('em_area21_creature_b_001509A0', 0x400, UU, None),
    0x00150DA0: ('em_area21_creature_b_00150DA0', 0x198, UU, None),
    0x00150F40: ('em_area21_creature_b_00150F40', 0x2B4, UU, None),
    0x00151200: ('em_area21_creature_b_00151200', 0x1A8, UU, None),
    0x001513B0: ('em_area21_creature_b_001513B0', 0x1E0, UU, I),
}
BEAT = 'a03b_03_item2a'
FREE = 0x1E61000
A = FREE + 0x1000            # the designed creature node (ctl = A + 0x1F0)
CTL = A + 0x1F0
LINK = FREE + 0x2000         # its linked effect (+0x20)
TABLE = FREE + 0x3000        # the bone table behind 0x275B40
BONES = FREE + 0x4000        # bone i at BONES + i * 0x100 (matrix at +0x90)
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
PLAYER = 0x810360
SELF_ENTRIES = {0x14EC00, 0x14EDD0, 0x150190}
# the pure helpers run as original code (their writes replayed): vector and
# matrix copies and products, the rotation, sin / cos / fabs, the bearing,
# the turn, the angle wrap and the flat distance
MATH = frozenset((0x102948, 0x102958, 0x1026A0, 0x1028B8, 0x1029C0, 0x102BB0, 0x11DE90, 0x11DF78, 0x11E2A8,
                  0x1B1240, 0x1B12B0, 0x1B1470, 0x1B15D0))
# Words the static walk reaches that no caller can reach, and why.
DEAD = {}


def spec(args):
    words, floats, out = 4, 12, []
    for kind in args:
        if kind == F:
            out.append((F, floats))
            floats += 1
        else:
            out.append((kind, words))
            words += 1
    return out


HOOKS = [(f'w_{a:08X}', a, spec(args), result) for a, (args, result) in sorted(G.HOOKS.items())]


def default_args(entry):
    return [A] if len(FUNCS[entry][2]) == 1 else [A, CTL]


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'}
    O.OUT = ROOT / 'build/area21_creature_b'
    O.SOURCES = ('em_area21_creature_b.c', 'em_area21_creature_b_moves.c', 'em_area21_creature_b_parts.c')
    O.BEATS = [BEAT]
    O.BEAT_DIRS = {BEAT: refs.capture('levels-recordings-20261030-1/s87/route_a03b/' + BEAT)}
    O.RESIDENT = {BEAT: 4}
    O.BY_ADDRESS = {a: (n, args, res) for n, a, args, res in HOOKS}
    O.BY_NAME = {n: a for n, a, _, _ in HOOKS}
    O.HOOK_NAMES = [h[0] for h in HOOKS] + ['w_callback']
    O.Hooks = type('Hooks', (C.Structure,), {'_fields_':
        [('ctx', C.c_void_p), ('bytes', O.BYTES_FN)] +
        [(name, O.hook_proto(args, res)) for name, _, args, res in HOOKS] + [('w_callback', O.CALLBACK_FN)]})
    O.SELF_ENTRIES = set(SELF_ENTRIES)
    O.HEAVY = set()
    O.RETURN_SITES = O.RETURN_SITES_FULL = ()
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.JUMP_TABLES = {}
    O.TABLE_BYTES = frozenset()
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[BEAT][0]
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    ram = O.CAPTURES[BEAT][0]
    assert not any(ram[FREE:BONES + 0x3000]), 'the designed area is not zero in the capture'


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x0F0F0F0F, 0x7FFF0000, 0x00012000, 0x80000000, 0x5A5A5A5A,
         0x000F8000, 0x00040000, 0x00080000, 0x00004000, 0x00000020, 0x00000800, 0x0000C000, 0x00008000,
         0x00000180, 0x0003C000)
PI2 = 1.5707964


def matrix(tx=0., ty=0., tz=0., s=1.):
    return fw(s, 0., 0., 0., 0., s, 0., 0., 0., 0., s, 0., tx, ty, tz, 1.)


def common():
    """The node's method and position, the bone table, the player."""
    p = [(A + 0x4C, w(CALLBACK)), (A + 0xB0, fw(120., 5., 80., 1.)), (0x275B40, w(TABLE)),
         (PLAYER, fw(150., 0., 120., 1.))]
    for i in range(0x30):
        p += [(TABLE + 4 * i, w(BONES + 0x100 * i)), (BONES + 0x100 * i + 0x90, matrix(i, 2. * i, -i))]
    return p


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2)))), (A + 4, b(c((0, 1, 1, 1, 2, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 3, 0x63)))),
        (A + 6, b(c((0, 1, 2, 3, 4)))), (A + 0x20, w(c((0, LINK)))), (A + 0x9A, b(c((0, 7, 0x81)))),
        (A + 0x34, h(c((0, 1, 5, 24, 25, 26, 1000)))),
        (A + 0x36, h(c((0, 1, 0x18, 0x19, 0x8005, 0x4010, 0x2001, 0x1001, 0xA003, 0xFFF, 0x0400)))),
        (A + 0x3C, f(c((0., 58., 59., 106., 107., 118., 119.)))),
        (A + 0xB0, fw(c((120., -60., 260., 300., 4., 196., 89., 91., 109., 111.)), c((5., 88., 89., -96., -95., 40.,
                                                                                         -100., -140., -161., 91.)),
                      c((80., 5., 351., 9., -69., -71., 139., 141.)), 1.)),
        (A + 0xC4, f(c((0., 1., -2., 3.1)))),
        (CTL, w(c((0, 0x1000, 9, 0x1009)))), (CTL + 4, w(c((0, 1, 2, 3, 0x4AF, 0x4B0, 0x95F, 0x960)))),
        (CTL + 8, w(c((0, 1, 2, 3, 0x1C, 0x1D, 5)))), (CTL + 0xC, w(c((0, 1, 2, 3)))),
        (CTL + 0x20, f(c((1., .5)))), (CTL + 0x28, f(c((0., 1., -2.)))),
        (CTL + 0x2C, f(c((0., .5, -.5, 2., -2.)))), (CTL + 0x30, f(c((0., 1., -1., 3.)))),
        (CTL + 0x34, f(c((0., .1, -.1)))), (CTL + 0x38, f(c((0., 1., -1., -4., -5.)))),
        (CTL + 0x3C, f(c((0., -.02)))), (CTL + 0x40, f(c((0., 1., -1., -2., -3., -4., -5.)))),
        (CTL + 0x44, f(c((0., 2., -2.)))), (CTL + 0x48, f(c((0., .1, -.1)))),
        (CTL + 0x50, h(c((0, 10, 0x18, 0x19)))), (CTL + 0x52, h(c((0, 199, 200)))), (CTL + 0x54, h(c((0, 1)))),
        (CTL + 0x60, b(c((0, 1, 0xFF)))), (CTL + 0x61, b(c((0, 1)))), (CTL + 0x62, b(c((0, 1)))),
        (CTL + 0x63, b(c((0, 0, 1)))), (CTL + 0x64, b(c((0, 1, 2, 4, 6, 0x80)))), (CTL + 0x65, b(c((0, 1, 2, 3, 4, 5)))),
        (CTL + 0x66, b(c((0, 1, 2, 3, 4, 5)))), (CTL + 0x67, b(c((0, 1, 0x80)))),
        (CTL + 0x68, bytes(c((0, 1, 3)) for _ in range(4))),
        (CTL + 0x6C, bytes(c((0, 1, 2)) for _ in range(4))),
        (0x81070A, b(c((0, 1)))), (0x81080E, b(c((0, 3, 4)))),
        (PLAYER, fw(c((150., -300., 300.)), 0., c((120., -200., 200.)), 1.)),
    ]


def hook_values(rng):
    def ints(*values, n=8): return [rng.choice(values) for _ in range(n)]
    out = {0x122BB8: ints(*RANDS, n=24)}
    out.update({a: ints(0, 1, 2, -1) for a in (0x1B10B0, 0x21BE40, 0x1B1EA0, 0x1B17A0, 0x1FBD50, 0x1283D0,
                                                0x1C64F0)})
    out.update({0x1EFE00: ints(0, LINK)})
    return out


def cases():
    out, labels = [], set()
    base = common()

    def add(label, e, patches=(), queues=None, run=MATH, args=None):
        assert label not in labels, label
        labels.add(label)
        all_patches = base + list(patches)
        out.append((label, BEAT, e, tuple(args or O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    def st(state, move=0, step=0):
        return [(A + 4, b(state)), (A + 5, b(move)), (A + 6, b(step))]

    end = [(CTL, w(0x1000))]
    link = [(A + 0x20, w(LINK))]
    counts = [(CTL + 0x61, b(3)), (CTL + 0x54, h(2)), (CTL + 0x62, b(0xFF)), (CTL + 0x60, b(1))]
    # ---- 0014EC00: the four states and the countdowns
    for state in (0, 1, 2, 3, 4):
        add(f'ec00/{state}', 0x14EC00, st(state, 3) + counts)
    add('ec00/zero', 0x14EC00, st(4))
    # ---- 0014ED10
    for flag in (0, 1):
        for r in (0, 1):
            add(f'ed10/{flag}/{r}', 0x14ED10, [(0x81070A, b(flag))], {0x1B10B0: [r], 0x1EFE00: [LINK]})
    # ---- 0014EDD0 / 00150190: every move, the header
    for move in (0, 1, 2, 3):
        for y in (-100., -99.9, -101.):
            add(f'edd0/{move}/{y}', 0x14EDD0, st(1, move, 9) + [(A + 0xB0, fw(1., y, 2., 1.))])
        add(f'0190/{move}', 0x150190, st(2, move, 9))
    # ---- 0014EEF0 (move 0)
    for step in (0, 1, 4):
        add(f'eef0/{step}', 0x14EEF0, st(1, 0, step) + link, {0x122BB8: [0x1FE0, 0x03FC0000, 0x000FF000]})
    add('eef0/1/nolink', 0x14EEF0, st(1, 0, 1), {0x122BB8: [0, 0, 0]})
    for c38 in (-5., -4., -.5, 1.):
        for c40 in (-5., -.5, 0., 1.):
            add(f'eef0/2/{c38}/{c40}', 0x14EEF0, st(1, 0, 2) + link + [(CTL + 0x38, f(c38)), (CTL + 0x40, f(c40))])
    for top in (0, 0x80):
        for r in (0, 0x8000):
            for lk in (0, LINK):
                add(f'eef0/2/top/{top}/{r:x}/{lk:x}', 0x14EEF0,
                    st(1, 0, 2) + [(A + 0x20, w(lk)), (CTL + 0x38, f(-1.)), (CTL + 0x67, b(top))],
                    {0x122BB8: [r]})
    for c38 in (-1., 1.):
        for flags in (0, 1, 2, 3):
            for y in (90., 90.5, -95., -95.5):
                for r in (0, 0x4000):
                    add(f'eef0/2/cry/{c38}/{flags}/{y}/{r:x}', 0x14EEF0,
                        st(1, 0, 2) + [(CTL + 0x38, f(c38)), (CTL + 0xC, w(flags)), (CTL + 0x67, b(0x80)),
                                       (A + 0xB0, fw(1., y, 2., 1.))], {0x122BB8: [r]})
    for t in (0, 0x1000):
        for y in (-160., -159.5):
            add(f'eef0/2/end/{t:x}/{y}', 0x14EEF0, st(1, 0, 2) + [(CTL, w(t)), (A + 0xB0, fw(1., y, 2., 1.)),
                                                                  (CTL + 0xC, w(3))], {0x122BB8: [0x7FFFFFFF]})
    for wait in (0, 5):
        for pattern in (0, 2):
            for tally in (199, 200):
                for count in (0x4AF, 0x4B0, 0x95F, 0x960):
                    for r in (0, 0x80):
                        add(f'eef0/3/{wait}/{pattern}/{tally}/{count:x}/{r:x}', 0x14EEF0,
                            st(1, 0, 3) + [(CTL + 8, w(wait)), (CTL + 0x65, b(pattern)), (CTL + 0x52, h(tally)),
                                           (CTL + 4, w(count))], {0x122BB8: [r]})
    # ---- 0014F390 (move 1)
    for pattern in range(6):
        for r in (0, 0x5000):
            add(f'f390/0/{pattern}/{r:x}', 0x14F390, st(1, 1, 0) + link + [(CTL + 0x65, b(pattern))],
                {0x122BB8: [r, 0x1E00, 0x3C00, 0x7800, 0xF000]})
    add('f390/0/nolink', 0x14F390, st(1, 1, 0))
    add('f390/2', 0x14F390, st(1, 1, 2))
    for dv in (.1, -.1, 0.):
        for x2c, x30 in ((1., 2.), (2., 1.), (1., 1.)):
            for y in (-200., 200.):
                add(f'f390/1/swing/{dv}/{x2c}/{x30}/{y}', 0x14F390,
                    st(1, 1, 1) + [(CTL + 0x34, f(dv)), (CTL + 0x2C, f(x2c)), (CTL + 0x30, f(x30)),
                                   (CTL + 0x48, f(dv)), (CTL + 0x40, f(x2c)), (CTL + 0x44, f(x30)),
                                   (A + 0xB0, fw(1., y, 2., 1.)), (CTL + 4, w(1))])
    for c38 in (-1000., 1.):
        for top in (0, 0x80):
            for lk in (0, LINK):
                add(f'f390/1/top/{c38}/{top}/{lk:x}', 0x14F390,
                    st(1, 1, 1) + [(A + 0x20, w(lk)), (CTL + 0x38, f(c38)), (CTL + 0x67, b(top)), (CTL + 4, w(1))])
    for pattern in (1, 0):
        for x2c in (1., -1., 0.):
            for dv in (0., -1., 1.):
                for x in (89., 90., 110., 111.):
                    add(f'f390/1/flip/{pattern}/{x2c}/{dv}/{x}', 0x14F390,
                        st(1, 1, 1) + [(CTL + 0x66, b(pattern)), (CTL + 0x2C, f(x2c)), (CTL + 0x34, f(dv)),
                                       (CTL + 0x30, f(-x2c)), (A + 0xB0, fw(x, 5., 2., 1.)), (CTL + 4, w(0))])
    for pattern in (2, 0):
        for z in (139., 140.):
            add(f'f390/1/double/{pattern}/{z}', 0x14F390,
                st(1, 1, 1) + [(CTL + 0x66, b(0)), (CTL + 0x65, b(pattern)), (A + 0xB0, fw(1., 5., z, 1.)),
                               (CTL + 4, w(0))])
    for c38 in (-1., 1.):
        for flags in (0, 1, 2, 3):
            for y in (90., 90.5, -95., -95.5):
                add(f'f390/1/cry/{c38}/{flags}/{y}', 0x14F390,
                    st(1, 1, 1) + [(CTL + 0x38, f(c38)), (CTL + 0x3C, f(0.)), (CTL + 8, w(flags)), (CTL + 4, w(1)),
                                   (CTL + 0x66, b(2)), (A + 0xB0, fw(1., y, 2., 1.))])
    for t in (0, 0x1000):
        for y in (-160., -159.5):
            add(f'f390/1/end/{t:x}/{y}', 0x14F390, st(1, 1, 1) + [(CTL, w(t)), (A + 0xB0, fw(1., y, 2., 1.)),
                                                                  (CTL + 8, w(3)), (CTL + 4, w(1))])
    # ---- 0014FBC0 (move 2)
    add('fbc0/0', 0x14FBC0, st(1, 2, 0) + link)
    add('fbc0/0/nolink', 0x14FBC0, st(1, 2, 0))
    add('fbc0/4', 0x14FBC0, st(1, 2, 4))
    add('fbc0/5', 0x14FBC0, st(1, 2, 5))
    for flag in (0, 1):
        for y in (-35., -35.5):
            for t in (0, 0x1000):
                add(f'fbc0/1/{flag}/{y}/{t:x}', 0x14FBC0,
                    st(1, 2, 1) + [(CTL + 0xC, w(flag)), (A + 0xB0, fw(1., y, 2., 1.)), (CTL, w(t)),
                                   (A + 0xC4, f(1.))])
    for count in (0, 1, 2, 3):
        for frame in (106., 106.5):
            for tick in (0, 0x1C, 0x1D):
                for flag in (0, 1):
                    add(f'fbc0/2/{count}/{frame}/{tick:x}/{flag}', 0x14FBC0,
                        st(1, 2, 2) + [(CTL + 4, w(count)), (A + 0x3C, f(frame)), (CTL + 8, w(tick)),
                                       (CTL + 0xC, w(flag))], {0x21BE40: [0]})
    for grab in (0, 1):
        add(f'fbc0/2/grab/{grab}', 0x14FBC0, st(1, 2, 2) + [(CTL + 4, w(1)), (A + 0x3C, f(10.)), (CTL + 8, w(0x1D)),
                                                             (0x8102B0, b(0x41))], {0x21BE40: [grab]})
    for c40 in (0., -2., -1.9, -3.):
        for frame in (118., 118.5):
            for t in (0, 0x1000):
                add(f'fbc0/2/fall/{c40}/{frame}/{t:x}', 0x14FBC0,
                    st(1, 2, 2) + [(CTL + 4, w(5)), (CTL + 0x40, f(c40)), (A + 0x3C, f(frame)), (CTL, w(t))])
    for c3c in (0., -.1):
        for frame in (58., 58.5):
            for flag in (0, 1):
                for c38 in (-1., 0.):
                    for y in (40., 40.5, -140., -139.5):
                        for t in (0, 0x1000):
                            add(f'fbc0/3/{c3c}/{frame}/{flag}/{c38}/{y}/{t:x}', 0x14FBC0,
                                st(1, 2, 3) + [(CTL + 0x3C, f(c3c)), (A + 0x3C, f(frame)), (CTL + 0xC, w(flag)),
                                               (CTL + 0x38, f(c38)), (A + 0xB0, fw(1., y, 2., 1.)), (CTL, w(t))])
    # ---- 00150260 (state 2 move 0)
    for c40 in (1., 0., -1.):
        for hp in (0, 5):
            for kind in (0, 0x8000):
                add(f'0260/0/{c40}/{hp}/{kind:x}', 0x150260,
                    st(2, 0, 0) + [(CTL + 0x40, f(c40)), (A + 0x34, h(hp)), (A + 0x36, h(kind))])
    add('0260/2', 0x150260, st(2, 0, 2))
    for c40 in (-5., -4.):
        for c38 in (-1., 0.):
            for top in (0, 1):
                for lk in (0, LINK):
                    add(f'0260/1/{c40}/{c38}/{top}/{lk:x}', 0x150260,
                        st(2, 0, 1) + [(A + 0x20, w(lk)), (CTL + 0x40, f(c40)), (CTL + 0x38, f(c38)),
                                       (CTL + 0x67, b(top)), (CTL + 4, w(1))])
    for flag in (0, 1):
        for c38 in (-1., 0.):
            for y in (88., 88.5):
                add(f'0260/1/cry/{flag}/{c38}/{y}', 0x150260,
                    st(2, 0, 1) + [(CTL + 4, w(flag)), (CTL + 0x38, f(c38)), (CTL + 0x67, b(1)),
                                   (A + 0xB0, fw(1., y, 2., 1.))])
    for t in (0, 0x1000):
        for y in (-130., -129.5):
            for hp in (0, 5):
                for kind in (0, 0x8000):
                    add(f'0260/1/end/{t:x}/{y}/{hp}/{kind:x}', 0x150260,
                        st(2, 0, 1) + [(CTL, w(t)), (A + 0xB0, fw(1., y, 2., 1.)), (A + 0x34, h(hp)),
                                       (A + 0x36, h(kind)), (CTL + 4, w(1))])
    # ---- 001504D0 (state 2 move 1)
    add('04d0/0', 0x1504D0, st(2, 1, 0) + link)
    add('04d0/0/nolink', 0x1504D0, st(2, 1, 0))
    add('04d0/2', 0x1504D0, st(2, 1, 2))
    for t in (0, 0x1000):
        for y in (-130., -129.5):
            add(f'04d0/1/{t:x}/{y}', 0x1504D0, st(2, 1, 1) + [(CTL, w(t)), (A + 0xB0, fw(1., y, 2., 1.))])
    # ---- 00150600 (state 2 move 2)
    add('0600/0', 0x150600, st(2, 2, 0))
    add('0600/3', 0x150600, st(2, 2, 3))
    for gate in (3, 4):
        for lk in (0, LINK):
            add(f'0600/1/{gate}/{lk:x}', 0x150600, st(2, 2, 1) + [(0x81080E, b(gate)), (A + 0x20, w(lk))])
    for c38 in (-1., 0.):
        for flags in (0, 1, 2, 3):
            for y in (40., 40.5, -35., -35.5):
                add(f'0600/2/{c38}/{flags}/{y}', 0x150600,
                    st(2, 2, 2) + [(CTL + 0x38, f(c38)), (CTL + 4, w(flags)), (A + 0xB0, fw(1., y, 2., 1.))])
    for t in (0, 0x1000):
        for y in (-130., -129.5):
            for lk in (0, LINK):
                add(f'0600/2/end/{t:x}/{y}/{lk:x}', 0x150600,
                    st(2, 2, 2) + [(CTL, w(t)), (A + 0xB0, fw(1., y, 2., 1.)), (CTL + 4, w(3)), (A + 0x20, w(lk))])
    # ---- 00150820 / 00150850 / 00150900
    add('0820', 0x150820, [(A + 0x9A, b(0x81))])
    for move in (0x63, 0):
        add(f'0850/{move:x}', 0x150850, [(A + 5, b(move))])
    for flag in (0, 1):
        add(f'0900/{flag}', 0x150900, [(CTL + 0x63, b(flag)), (CTL + 0x28, f(.7)), (CTL + 0x38, f(1.)),
                                        (CTL + 0x3C, f(-.5)), (CTL + 0x40, f(2.)), (CTL + 0x48, f(.25))])
    # ---- 001509A0: the circle and the rails
    for bits in (0, 2, 4, 6):
        for pattern in (0, 1, 2):
            for x, z in ((100., 150.), (600., 150.), (100., 600.), (-400., -300.)):
                add(f'09a0/circle/{bits}/{pattern}/{x}/{z}', 0x1509A0,
                    [(CTL + 0x64, b(bits)), (CTL + 0x65, b(pattern)), (A + 0xB0, fw(x, 5., z, 1.))])
    for pattern in (0, 1):
        for x in (-60., -50., -49., 0., 249., 250., 300.):
            for z in (351., 350., 9.9, 10., -100.):
                for one in (0, 1):
                    add(f'09a0/rail/{pattern}/{x}/{z}/{one}', 0x1509A0,
                        [(CTL + 0x64, b(2 | one)), (CTL + 0x65, b(pattern)), (A + 0xB0, fw(x, 5., z, 1.))])
    for x in (4., 5., 100., 195., 196., 250.):
        for z in (-69., -70., -80., -300.):
            add(f'09a0/two/{x}/{z}', 0x1509A0, [(CTL + 0x64, b(2)), (CTL + 0x65, b(2)), (A + 0xB0, fw(x, 5., z, 1.))])
    # ---- 00150DA0
    for pattern in (0, 1, 2):
        add(f'0da0/{pattern}', 0x150DA0, [(CTL + 0x65, b(pattern))],
            {0x122BB8: [0x7FFFFFFF, 0x12345678, -1, 0x00ABC000]})
    # ---- 00150F40: the three clamps of each jet's yaw
    for pattern in (0, 1, 2):
        add(f'0f40/{pattern}', 0x150F40, [(CTL + 0x65, b(pattern))])
        for bearing in (3., -3., 0.25):
            add(f'0f40/{pattern}/{bearing}', 0x150F40, [(CTL + 0x65, b(pattern))],
                {0x1B1240: [O.fb(bearing)] * 4}, run=MATH - {0x1B1240})
    # ---- 00151200: the bursts
    for inside in (0, 1):
        add(f'1200/{inside}', 0x151200, [(CTL + 0x6C, bytes((0, 1, 2, 0))), (CTL + 0x68, bytes((0, 0, 0, 3)))],
            {0x1B1EA0: [inside], 0x122BB8: [0x7C000, 0, 0x4000]})
    # ---- 001513B0: the hit tally
    for hit in (0, 0x10, 0x8010, 0x4010, 0x2010, 0x1010, 0xA010, 0x0FFF):
        for grace in (0, 3):
            for combo in (0, 1):
                for fx in (0, 1):
                    add(f'13b0/{hit:x}/{grace}/{combo}/{fx}', 0x1513B0,
                        st(1, 2, 3) + [(A + 0x36, h(hit)), (CTL + 0x60, b(grace)), (CTL + 0x61, b(combo)),
                                       (CTL + 0x62, b(fx)), (A + 0x34, h(1000)), (CTL + 0x50, h(3))])
    for state in (1, 2):
        for hp, hit in ((10, 10), (11, 10), (100, 2), (100, 0x8005), (100, 0xA001), (100, 24), (100, 25)):
            for tally in (0, 24):
                add(f'13b0/state/{state}/{hp}/{hit:x}/{tally}', 0x1513B0,
                    st(state, 2, 3) + [(A + 0x36, h(hit)), (A + 0x34, h(hp)), (CTL + 0x61, b(1)),
                                       (CTL + 0x50, h(tally))])
    # ---- a seeded random sample over every entry
    rng = random.Random(0x21B)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(40, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_creature_b.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_creature_b_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21b_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(.*\);$' % symbol, header, re.M), symbol


def entry_guards():
    """A latched fault, a NULL hook table, a NULL fault pointer and (for the
    entry with a result) a NULL result: every entry refuses with no hook or
    memory call."""
    touched = []

    def refuse(*_):
        touched.append(1)
        return -1
    keep = [O.BYTES_FN(refuse), O.CALLBACK_FN(refuse)]
    fields_ = {'ctx': None, 'bytes': keep[0], 'w_callback': keep[1]}
    for name, _, args, result in HOOKS:
        fields_[name] = O.hook_proto(args, result)(refuse)
        keep.append(fields_[name])
    hooks = O.Hooks(**fields_)
    for entry, (symbol, _, _, kind) in FUNCS.items():
        for table, fault, with_result in ((C.byref(hooks), O.Fault(0x1234, 7), True),
                                          (None, O.Fault(), True), (C.byref(hooks), None, True),
                                          (C.byref(hooks), O.Fault(), False)):
            if not with_result and kind is None:
                continue
            out = C.c_int32(0x5A5A5A5A)
            touched.clear()
            args = [table] + O.native_args(entry, O.DEFAULT_ARGS[entry])
            if kind is not None:
                args.append(C.byref(out) if with_result else None)
            rc = getattr(O.NATIVE, symbol)(*args, C.byref(fault) if fault is not None else None)
            assert rc == -1 and not touched and out.value == 0x5A5A5A5A, symbol
            if fault is not None:
                assert (fault.address, fault.code) == ((0x1234, 7) if table is not None and with_result else (0, 0))


def trace(prefix, selected):
    for case in selected:
        if not case[0].startswith(prefix):
            continue
        label, beat, entry, args, patches, spad, script, run = case
        ram0, spad0 = O.CAPTURES[beat]
        ram, sp = O.patched(ram0, patches), O.patched(spad0, spad)
        oracle = O.Oracle(ram, sp, entry, (args, O.callbacks_of(entry, args, ram)), O.copy_script(script), run, set())
        print('==', label)
        for t in oracle.trace:
            print(' ', t[0], *(hex(x) if isinstance(x, int) else x for x in t[1:]))


def judge(results):
    only = os.environ.get('EM_AREA21_CREATURE_B_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_CREATURE_B_MISSING')):
            failures.append(('coverage', [hex(entry), [hex(m) for m in sorted(missing)]]))
    if not only:
        assert not set(DEAD) & seen, 'a word called dead ran'
        failures += [('callee inventory', p) for p in O.callee_checks()]
        if not failures:
            sites, missing = O.hook_sites(results)
            if missing:
                failures.append(('unreached hooks', missing))
            for problems, runs in O.reference_mode.parallel_map(O.hook_contract_site, sites):
                if problems:
                    failures.append(('fail-stop contract', problems))
    return failures


def main():
    configure()
    O.NATIVE = O.build(os.environ.get('EM_AREA21_CREATURE_B_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA21_CREATURE_B_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_CREATURE_B_TRACE'):
        trace(os.environ['EM_AREA21_CREATURE_B_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_CREATURE_B_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
