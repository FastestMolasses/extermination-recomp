#!/usr/bin/env python3
"""AREA21's mounted guns (src/game/em_area21_overlay.c and
em_area21_overlay_cannon.c) against the ORIGINAL instructions: the Gatling
[62] 0x8297C0 with 0x8260A0, 0x829310 (shared with the cannon; the step
table 0x82DB10), 0x8294C0, 0x829590, 0x829F10 and 0x829F40 (its matrix
and packet are frame locals: it and 0x8297C0 take the entry sp, kind S),
and the cannon [61] 0x828700 with 0x828410, 0x828E80 and the projectile
0x828F30. Decomp docs/AREA21_OVERLAY.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
tools/test_area21_doors_reference.py configures it. No AREA21 recording
exists: every entry runs as original code from the user's
extract/OVERLAY/AREA21.BIN, the WHOLE file (header, text and data: the
step table and the packet group are overlay data) loaded over the arena of
the fork recording a03b_03_item2a (its AREA03 overlay text checked against
the user's AREA03.BIN and its boot text against the pinned ELF before the
load). An entry that calls another entry runs the callee's original words
inline (the native side calls its own translation). Every other callee is
a hook with a scripted result, except the pure helpers in MATH, which run
as original code nested in the oracle (their writes replayed to the native
side). A node's +0x4C method is stubbed and logged. The designed nodes and
records live in zero RAM at FREE; the player record D_008102B0 is patched
in place. Compared per case: every call and its arguments, memory at
every call entry (the stack window included), the memory accesses between
calls one for one in order, memory after the last store, the result, a
poisoned re-run, coverage of every reachable original word, the callee
inventory and the fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA21_OVERLAY_ONLY=<label
prefix> runs a subset (no coverage or contract);
EM_AREA21_OVERLAY_SOURCE=<dir> tests another copy of the module sources;
EM_AREA21_OVERLAY_SHOW=<n> prints n failures; EM_AREA21_OVERLAY_MISSING=1
lists unexecuted words; EM_AREA21_OVERLAY_TRACE=<label> prints the
original's calls and accesses of matching cases (no instruction text).
Reports hold addresses and counts only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area21_overlay as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
# entry: (native symbol, byte length, argument kinds, result kind)
FUNCS = {
    0x8297C0: ('em_area21_overlay_008297C0', 0x74C, [U, S], None),
    0x8260A0: ('em_area21_overlay_008260A0', 0xD4, [F], None),
    0x829310: ('em_area21_overlay_00829310', 0x1B0, [U, I], None),
    0x8294C0: ('em_area21_overlay_008294C0', 0xD0, [U, U], None),
    0x829590: ('em_area21_overlay_00829590', 0x230, [U, U], None),
    0x829F10: ('em_area21_overlay_00829F10', 0x2C, [U], None),
    0x829F40: ('em_area21_overlay_00829F40', 0x28C, [U, S], None),
    0x828700: ('em_area21_overlay_00828700', 0x77C, [U], None),
    0x828410: ('em_area21_overlay_00828410', 0x2F0, [U, U], None),
    0x828E80: ('em_area21_overlay_00828E80', 0xAC, [U, U, I], I),
    0x828F30: ('em_area21_overlay_00828F30', 0x3D8, [U], None),
}
OVERLAY_ID = 0x12
AREA21 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA21.BIN'
AREA03 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'
BEAT = 'a03b_03_item2a'
DESIGNED = 'designed-a21-overlay/' + BEAT   # the capture with AREA21.BIN loaded over the arena
FREE = 0x1E61000             # zero in the capture
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
W = A + 0x1F0                # its work area
MODEL = FREE + 0x2000        # the +0x118 model (+0x70 elevation, +0x7C recoil, +0x90 matrix)
MODEL2 = FREE + 0x2400       # the cannon's +0x110 model
MODEL3 = FREE + 0x2800       # the Gatling's +0x114 model (+0xC0 matrix)
CHILD = FREE + 0x3000        # the +0x20 child
OB = FREE + 0x4000           # an object a hook returns
HIT = FREE + 0x5000          # a hit object (D_700031D4)
SURF = FREE + 0x6000         # a hit surface (D_700031D0)
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
PLAYER = 0x8102B0            # D_008102B0
E64, E65, E70 = 0x810E64, 0x810E65, 0x810E70
B74, B78, B68 = 0x70003B74, 0x70003B78, 0x70003B68
D_0081080E = 0x81080E
TABLE = 0x82DB10             # 0x829310's step limits
SELF_ENTRIES = {0x8297C0, 0x828700}
# the pure helpers run as original code (their writes replayed): vector and
# matrix copies, products, the turn matrices, normalise, scale, cos, fabs,
# float_to_int and the angle wrap
MATH = frozenset((0x1026A0, 0x1026D0, 0x102760, 0x1028D0, 0x102918, 0x102948, 0x102958, 0x1029C0, 0x102B08,
                  0x102BB0, 0x1031E0, 0x103230, 0x11DE90, 0x11DF78, 0x1281C0, 0x1B1470))
# Words the static walk reaches that no input can reach, and why.
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
    return {0x8297C0: [A, O.STACK_TOP], 0x829F40: [A, O.STACK_TOP], 0x8260A0: [fbits(.5)],
            0x829310: [W + 0x34, 0x90], 0x8294C0: [A, PLAYER], 0x829590: [A, W], 0x828410: [A, W],
            0x828E80: [A, W, 0]}.get(entry, [A])


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {4: AREA03, OVERLAY_ID: AREA21}
    O.OUT = ROOT / 'build/area21_overlay'
    O.SOURCES = ('em_area21_overlay.c', 'em_area21_overlay_cannon.c')
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
    O.SCRIBBLED_CALLBACKS = set()
    O.HEAVY = set()
    O.RETURN_SITES = O.RETURN_SITES_FULL = tuple(e for e, v in FUNCS.items() if v[3] is not None)
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.JUMP_TABLES = {}
    O.TABLE_BYTES = frozenset()
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[DESIGNED][0]
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    # the whole overlay file (header, text and data) over the arena
    data = AREA21.read_bytes()
    assert data[4] == OVERLAY_ID
    ram, spad = O.CAPTURES[BEAT]
    designed = bytearray(ram)
    designed[O.ARENA:O.ARENA + len(data)] = data
    O.CAPTURES[DESIGNED] = (bytes(designed), spad)


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x40000000, -0x80000000, 0x00012000, 0x5A5A5A5A, 3, 0x10000)


def fbits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(fbits(x) + d)


def table(n):
    """0x829310's limit n as read from the designed image (float)."""
    return struct.unpack_from('<f', O.CAPTURES[DESIGNED][0], TABLE + 4 * n)[0]


def matrix(tx=10., ty=20., tz=30., c=.8, s=.6):
    """A turn about y with a translation (row-major 4x4)."""
    return fw(c, 0., -s, 0., 0., 1., 0., 0., s, 0., c, 0., tx, ty, tz, 1.)


def common():
    """The node's method, models, child, work and the player record's keys
    (the keyed state of both guns)."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0x118, w(MODEL)), (A + 0x110, w(MODEL2)), (A + 0x114, w(MODEL3)),
            (A + 0x20, w(CHILD)), (A + 0xB0, fw(1., 2., 3., 1.)), (A + 0xC0, fw(.25, -.5, .75, 0.)),
            (A + 0xC4, f(.125)), (MODEL + 0x70, f(.25)), (MODEL + 0x7C, fw(.5, .25, -.125)),
            (MODEL + 0x90, matrix()), (MODEL2 + 0x90, matrix(-5., 1., 7., .6, .8)),
            (MODEL3 + 0xC0, matrix(3., 4., 5., 0., 1.)), (PLAYER + 0x230, w(0x28)), (PLAYER + 0x1F0, b(0x38)),
            (PLAYER + 0x1F1, b(1)), (E64, b(0)), (E65, b(0)), (E70, h(0)), (B74, h(0)), (B78, h(0)),
            (D_0081080E, b(0)), (SURF + 0x1C, w(0x1234)), (0x700031B0, fw(7., 8., 9., 5.)),
            (0x700031D0, w(SURF)), (0x700031D4, w(HIT))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A + 4, b(c((0, 1, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c((0, 1, 1, 2, 3)))), (A + 6, b(c((0, 1, 1, 2)))),
        (A + 7, b(c((0, 1, 2, 3, 4, 5)))), (A + 0x28, h(c((0, 1, 2, 6, 10, 0xFFFF)))),
        (A + 0x2A, h(c((0, 1, 2, 20)))), (A + 0x20, w(c((0, CHILD)))),
        (W + 0x30, f(c((0., .01, -.01)))), (W + 0x34, f(c((0., .02, -.02)))), (W + 0x38, f(c((0., 1.5707964)))),
        (W + 0x3C, f(c((0., .5, 1., 899., 900., 901., 1800.)))), (W + 0x40, w(c((0, 1, 2, 50)))),
        (W + 0x50, f(c((0., 1.995, 2.)))), (W + 0x54, w(c(RANDS))),
        (W + 0x58, w(c((0, 1, 2)))), (W + 0x5C, w(c((0, 253, 254, 255)))),
        (A + 0xC4, f(c((0., .7, -.7, .75049156, -.75049156)))), (MODEL + 0x70, f(c((0., .6, -.17, .69813174)))),
        (PLAYER + 0x230, w(c((0x28, 0x28, 0x27)))), (PLAYER + 0x1F0, b(c((0x38, 0x39)))),
        (PLAYER + 0x1F1, b(c((0, 1, 1)))), (E64, b(c((0, 0x10, 0x90)))), (E65, b(c((0, 0x7F, 0x80)))),
        (E70, h(c((0, 0x40)))), (B74, h(c((0, 0x40)))), (B78, h(c((0, 0x40, 0x41)))),
        (D_0081080E, b(c((0, 0x10)))), (B68, w(c((0, 1, 2, 3, -3)))),
        (A + 0xB4, f(c((9., 10., 50.)))), (A + 0x38, f(c((0., 6.95, 7.)))),
        (HIT, b(c((0, 1, 2)))), (HIT + 2, b(c((2, 0xE2, 3)))), (0x700031D4, w(c((0, HIT)))),
    ]


def hook_values(rng):
    def ints(*values, n=6): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(0, 1, -1) for a in (0x1B0FD0,)}
    out[0x1B5DC0] = ints(0, 1, 2, 3)
    out[0x122BB8] = ints(*RANDS)
    out[0x0019A570] = ints(0, 1, 2, 3)
    for a in (0x1F4F40, 0x1EFEB0, 0x1AFA90, 0x1EFE00, 0x1C5570):
        out[a] = ints(0, OB)
    out[0x1CCF70] = ints(0, 7, -1)
    return out


def cases():
    out, labels = [], set()
    base = common()

    def add(label, e, patches=(), queues=None, run=MATH, args=None):
        assert label not in labels, label
        labels.add(label)
        all_patches = base + list(patches)
        out.append((label, DESIGNED, e, tuple(args if args is not None else O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    def st(state, step=0, sub=None, shot=None):
        out_ = [(A + 4, b(state)), (A + 5, b(step))]
        if sub is not None:
            out_.append((A + 6, b(sub)))
        if shot is not None:
            out_.append((A + 7, b(shot)))
        return out_

    fire = {'e70b74': [(E70, h(0x40)), (B74, h(0x40))], 'e70b78': [(E70, h(0x41)), (B78, h(0x01))],
            'none': [(E70, h(0x40)), (B74, h(0x20)), (B78, h(0x80))]}
    # ---- the Gatling 0x8297C0
    for state in (2, 3, 4, 0xFF):
        add(f'97c0/{state:x}', 0x8297C0, st(state))
    for r in (0, 1, -1):
        add(f'97c0/0/{r}', 0x8297C0, st(0), {0x1B0FD0: [r], 0x1C5570: [OB]})
    add('97c0/0/scribble', 0x8297C0, st(0), {0x1C6380: [O.Scribble(None, [(A + 0x22C, f(5.))])]})
    for key in (0x28, 0x27):
        for kind in (0x38, 0x39):
            for heat in (0., 1., -0.):
                add(f'97c0/1/0/{key:x}/{kind:x}/{heat}', 0x8297C0,
                    st(1, 0) + [(PLAYER + 0x230, w(key)), (PLAYER + 0x1F0, b(kind)), (W + 0x3C, f(heat))])
    for step in (3, 0xFF):
        add(f'97c0/1/{step:x}', 0x8297C0, st(1, step))
    add('97c0/1/1/away', 0x8297C0, st(1, 1, 1) + [(PLAYER + 0x230, w(0x29))])
    add('97c0/1/1/away/scribble', 0x8297C0, st(1, 1, 1) + [(PLAYER + 0x230, w(0))],
        {0x1C6380: [O.Scribble(None, [(A + 0x118, w(MODEL2))])]})
    for mount in (0, 1, 2):
        add(f'97c0/1/1/0/{mount}', 0x8297C0, st(1, 1, 0) + [(PLAYER + 0x1F1, b(mount))])
    add('97c0/1/1/2', 0x8297C0, st(1, 1, 2))
    aim = st(1, 1, 1, 0)
    # the elevation clamps exactly at and one ulp beyond the limits (no input)
    for name, v in (('max', f(.69813174)), ('max+', ulp(.69813174, 1)), ('min', f(-.69813174)),
                    ('min-', ulp(-.69813174, 1)), ('mid', f(.3))):
        add(f'97c0/1/1/1/clamp/{name}', 0x8297C0, aim + [(MODEL + 0x70, v), (W + 0x30, f(0.)), (W + 0x34, f(0.))],
            {0x1B5DC0: [0, 0]})
    for k64, k65, n in ((0x10, 0x90, (1, 2)), (0x90, 0x10, (3, 1)), (0, 0, (0, 0))):
        add(f'97c0/1/1/1/aim/{k64:x}/{k65:x}', 0x8297C0, aim + [(E64, b(k64)), (E65, b(k65))],
            {0x1B5DC0: list(n)})
    # the fire test both ways, the shot start (child or none), the heat
    for name, bits in fire.items():
        for heat in (0., 899., 900., 901.):
            for child in (0, OB):
                add(f'97c0/1/1/1/fire/{name}/{heat}/{child:x}', 0x8297C0, aim + bits + [(W + 0x3C, f(heat))],
                    {0x1F4F40: [child]})
    for name, bits in fire.items():
        for counter in (1, 2, 0):
            for heat in (0., 900., 901.):
                add(f'97c0/1/1/1/recoil/{name}/{counter}/{heat}', 0x8297C0,
                    st(1, 1, 1, 1) + bits + [(A + 0x28, h(counter)), (W + 0x38, f(1.2)), (W + 0x3C, f(heat))])
    add('97c0/1/1/1/scribble/shot', 0x8297C0, aim + fire['e70b74'],
        {0x1F4F40: [O.Scribble(OB, [(A + 7, b(0)), (A + 0x118, w(MODEL2))])],
         0x1A2370: [O.Scribble(None, [(W + 0x3C, f(1000.))])],
         0x182F90: [O.Scribble(None, [(A + 0xC4, f(-2.))])]})
    add('97c0/1/1/1/scribble/none', 0x8297C0, aim + fire['e70b74'],
        {0x1F4F40: [O.Scribble(0, [(A + 7, b(3))])], 0x1B1E20: [O.Scribble(None, [(A + 0x28, h(9))])]})
    add('97c0/1/1/1/scribble/model', 0x8297C0, aim,
        {0x1B5DC0: [O.Scribble(0, [(W + 0x34, f(.5)), (A + 0xC4, f(1.))]), O.Scribble(0, [(MODEL + 0x70, f(.1))])],
         0x1C6380: [O.Scribble(None, [(A + 0x118, w(MODEL2))])]})
    # the cool-down 0x829F40 inside the Gatling's +5 2
    for heat in (1800., 1., .5, 0.):
        for r in (0, 1):
            add(f'97c0/1/2/{heat}/{r}', 0x8297C0, st(1, 2) + [(W + 0x3C, f(heat)), (B68, w(3))],
                {0x122BB8: [r, 0x40000000, -0x40000000], 0x1EFEB0: [OB]})
    add('97c0/1/nochild', 0x8297C0, st(1, 0) + [(A + 0x20, w(0))])
    add('97c0/1/scribble/tail', 0x8297C0, st(1, 0),
        {0x1B17A0: [O.Scribble(None, [(A + 0x20, w(OB)), (A + 0xC4, f(3.)), (A + 0x118, w(MODEL2))])]})
    # ---- 0x829310 on its own: every branch
    for n in (1, 2, 3):
        lim = table(n)
        step = .001 * n
        for key in (0x90, 0x80, 0x7F, 0x10, 0x180, -0x80):
            sign = 1. if key & 0xFF >= 0x80 else -1.
            t = sign * lim
            for name, v in (('far-', t - 1.), ('near-', t - step / 2), ('at', t), ('near+', t + step / 2),
                            ('far+', t + 1.), ('ulp+', None), ('ulp-', None)):
                vb = f(v) if v is not None else ulp(t, 1 if name == 'ulp+' else -1) if t != 0 else f(0.)
                add(f'9310/{n}/{key & 0xFFFFFFFF:x}/{name}', 0x829310, [(W + 0x34, vb)], {0x1B5DC0: [n]},
                    args=[W + 0x34, key])
    for n in (0, -1, 4):
        add(f'9310/n/{n}', 0x829310, [(W + 0x34, f(.5))], {0x1B5DC0: [n]}, args=[W + 0x34, 0x90])
    for name, v in (('0', f(0.)), ('-0', f(-0.)), ('small', f(.005)), ('small-', f(-.005)), ('eq', f(.01)),
                    ('eq-', f(-.01)), ('big', f(.5)), ('big-', f(-.5)), ('ulp', ulp(.01, 1))):
        add(f'9310/0/{name}', 0x829310, [(W + 0x34, v)], {0x1B5DC0: [0]}, args=[W + 0x34, 0x80])
    add('9310/scribble', 0x829310, [(W + 0x34, f(.5))],
        {0x1B5DC0: [O.Scribble(0, [(W + 0x34, f(-.25))])]}, args=[W + 0x34, 0x80])
    add('9310/scribble/n', 0x829310, [(W + 0x34, f(.5))],
        {0x1B5DC0: [O.Scribble(2, [(W + 0x34, f(-.25))])]}, args=[W + 0x34, 0x80])
    # ---- 0x8294C0, 0x829590, 0x829F10
    for turn in (0., 1., -3.1, 3.1415927):
        add(f'94c0/{turn}', 0x8294C0, [(A + 0xC4, f(turn))])
    add('94c0/scribble', 0x8294C0, [], {0x182F90: [O.Scribble(None, [(A + 0xC4, f(2.5))])]})
    add('9590', 0x829590)
    add('9590/scribble', 0x829590, [],
        {0x1C6380: [O.Scribble(None, [(A + 0x118, w(MODEL2)), (MODEL + 0x90, matrix(1., 1., 1.))])]})
    for r in RANDS[:4]:
        add(f'9f10/{r & 0xFFFFFFFF:x}', 0x829F10, [], {0x122BB8: [r]})
    # ---- 0x829F40 on its own: the random gates and the wrap
    for r in (0, 1, 2, 3, -1):
        for q in (0, 1, 2, 3, -3, -4, 0x7FFFFFFF, -0x80000000):
            add(f'9f40/{r}/{q & 0xFFFFFFFF:x}', 0x829F40, [(B68, w(q))],
                {0x122BB8: [r, 0x7FFFFFFF, -0x80000000], 0x1EFEB0: [OB], 0x1CCF70: [5]})
    for name, v in (('0', f(0.)), ('1.99', f(1.99)), ('1.995', f(1.995)), ('2', f(2.)), ('2-', ulp(1.99, 1))):
        for seed in (0, -1, 0x12345678):
            add(f'9f40/wrap/{name}/{seed & 0xFFFFFFFF:x}', 0x829F40, [(W + 0x50, v), (W + 0x54, w(seed)),
                                                                      (W + 0x3C, f(900.))], {0x122BB8: [0]})
    add('9f40/none', 0x829F40, [(B68, w(0))], {0x122BB8: [1], 0x1EFEB0: [0]})
    add('9f40/scribble', 0x829F40, [(B68, w(0))],
        {0x122BB8: [O.Scribble(1, [(A + 0x114, w(MODEL))])],
         0x1EFEB0: [O.Scribble(OB, [(A + 0x118, w(MODEL3)), (W + 0x3C, f(18.))])],
         0x1CCF70: [O.Scribble(3, [(W + 0x50, f(1.999)), (W + 0x54, w(0xFFFF0000))])],
         0x1CFBE0: [O.Scribble(None, [(W + 0x50, f(2.5))])]})
    add('9f40/scribble/pkt', 0x829F40, [(B68, w(1))],
        {0x1CFB50: [O.Scribble(None, [(O.STACK_TOP - 0x60, fw(1., 2., 3., 4.)), (W + 0x50, f(1.995))])]})
    # ---- 0x8260A0: t, and the unsigned conversion both ways
    for t in (0., .5, 1., -1., 1e7, 1.4e7, -1.4e7, 3e9, -3e9, -.0001):
        add(f'60a0/{t}', 0x8260A0, args=[fbits(t)])
    add('60a0/nan', 0x8260A0, args=[0x7FC00000])
    # ---- the cannon 0x828700
    for state in (2, 3, 4, 0xFF):
        add(f'8700/{state:x}', 0x828700, st(state))
    for r in (0, 1, -1):
        add(f'8700/0/{r}', 0x828700, st(0), {0x1B0FD0: [r]})
    for key in (0x28, 0x27):
        for kind in (0x39, 0x38):
            add(f'8700/1/0/{key:x}/{kind:x}', 0x828700,
                st(1, 0) + [(PLAYER + 0x230, w(key)), (PLAYER + 0x1F0, b(kind))])
    for step in (2, 0xFF):
        add(f'8700/1/{step:x}', 0x828700, st(1, step))
    add('8700/1/1/away', 0x828700, st(1, 1, 1) + [(PLAYER + 0x230, w(0x29))])
    for mount in (0, 1, 2):
        add(f'8700/1/1/0/{mount}', 0x828700, st(1, 1, 0) + [(PLAYER + 0x1F1, b(mount))])
    add('8700/1/1/2', 0x828700, st(1, 1, 2))
    gun = st(1, 1, 1, 0)
    for name, bits in fire.items():
        for spawn in (0, OB):
            add(f'8700/1/1/1/0/{name}/{spawn:x}', 0x828700, gun + bits, {0x1AFA90: [spawn]})
    # the turn and barrel clamps exactly at and one ulp beyond (no input)
    for name, turn, lift in (('t+', f(.75049156), f(.3)), ('t+u', ulp(.75049156, 1), f(.3)),
                             ('t-', f(-.75049156), f(.3)), ('t-u', ulp(-.75049156, 1), f(.3)),
                             ('l+', f(.1), f(.6108653)), ('l+u', f(.1), ulp(.6108653, 1)),
                             ('l-', f(.1), f(-.17453294)), ('l-u', f(.1), ulp(-.17453294, 1))):
        add(f'8700/1/1/1/0/clamp/{name}', 0x828700,
            gun + [(A + 0xC4, turn), (MODEL + 0x70, lift), (W + 0x30, f(0.)), (W + 0x34, f(0.))],
            {0x1B5DC0: [0, 0]})
    # a moving step inside the limits: the sound 0x455 by the +0x230 count
    for wait in (0, 1, 2, 50):
        for n in ((1, 0), (0, 1), (2, 3)):
            add(f'8700/1/1/1/0/move/{wait}/{n[0]}{n[1]}', 0x828700,
                gun + [(W + 0x40, w(wait)), (E64, b(0x90)), (E65, b(0x10)), (A + 0xC4, f(0.)),
                       (MODEL + 0x70, f(.2))], {0x1B5DC0: list(n)})
    for shot in (1, 3):
        for left in (1, 2, 0):
            for counter in (0, 1, 5):
                add(f'8700/1/1/1/{shot}/{left}/{counter}', 0x828700,
                    st(1, 1, 1, shot) + [(A + 0x2A, h(left)), (A + 0x28, h(counter)), (W + 0x38, f(1.))])
    for shot in (2, 4, 5, 0xFF):
        for spawn in (0, OB):
            add(f'8700/1/1/1/{shot:x}/{spawn:x}', 0x828700, st(1, 1, 1, shot), {0x1AFA90: [spawn]})
    for counter in (0, 0x10):
        for flag in (0, 1, 2):
            for count in (0, 253, 254, 255, -1):
                add(f'8700/1/end/{counter:x}/{flag}/{count}', 0x828700,
                    st(1, 0) + [(D_0081080E, b(counter)), (W + 0x58, w(flag)), (W + 0x5C, w(count))])
    add('8700/1/scribble', 0x828700, gun + fire['e70b74'],
        {0x1AFA90: [O.Scribble(OB, [(A + 0x118, w(MODEL2)), (W + 0x10, fw(4., 5., 6.))])],
         0x1B17A0: [O.Scribble(None, [(A + 0x110, w(MODEL)), (W + 0x58, w(1)), (W + 0x5C, w(300))])]})
    add('8700/1/scribble/aim', 0x828700, gun + [(W + 0x40, w(0))],
        {0x1B5DC0: [O.Scribble(1, [(W + 0x40, w(1))]), O.Scribble(0, [(W + 0x40, w(0)), (W + 0x30, f(.5))])],
         0x1FB9F0: [O.Scribble(None, [(W + 0x40, w(7))])]})
    # ---- 0x828410 and 0x828E80
    add('8410', 0x828410)
    add('8410/scribble', 0x828410, [], {0x1C6380: [O.Scribble(None, [(A + 0x118, w(MODEL2))])]})
    for which in (0, 1, -1):
        for spawn in (0, OB):
            add(f'8e80/{which}/{spawn:x}', 0x828E80, [(W + 0x10, fw(1., 2., 3., 4., 5., 6., 7., 8., 9.))],
                {0x1AFA90: [spawn]}, args=[A, W, which])
    add('8e80/scribble', 0x828E80, [], {0x1AFA90: [O.Scribble(OB, [(W + 0x10, f(-1.)), (A + 0x70, f(2.))])]},
        args=[A, W, 0])
    # ---- the projectile 0x828F30
    shell = [(A + 0xB0, fw(100., 50., -30., 1.)), (A + 0xC0, fw(3., 0., 4., 0.)), (A + 0x20, w(CHILD))]
    for state in (3, 4, 0xFF):
        add(f'8f30/{state:x}', 0x828F30, shell + st(state))
    for spawn in (0, OB):
        add(f'8f30/0/{spawn:x}', 0x828F30, shell + st(0), {0x1EFE00: [spawn]})
    for kind in (0, 1, 2):
        add(f'8f30/2/{kind}', 0x828F30, shell + st(2, kind))
    for left in (1, 2, 0x8000):
        add(f'8f30/1/0/{left:x}', 0x828F30, shell + st(1, 0) + [(A + 0x28, h(left))])
    for name, fall in (('0', f(0.)), ('6.95', f(6.95)), ('6.96', f(6.96)), ('7', f(7.)), ('8', f(8.)),
                       ('ulp', ulp(6.95, 1))):
        add(f'8f30/1/1/{name}', 0x828F30, shell + st(1, 1) + [(A + 0x38, fall)])
    for name, y in (('10', f(10.)), ('10-', ulp(10., -1)), ('9', f(9.)), ('-5', f(-5.))):
        add(f'8f30/1/2/{name}', 0x828F30, shell + st(1, 2) + [(A + 0xB4, y)])
    for r in (1, 2, 3, -1, 0x7FFFFFFF):
        for name, obj in (('null', [(0x700031D4, w(0))]), ('dead', [(HIT, b(0))]),
                          ('k2live1', [(HIT, b(1)), (HIT + 2, b(2))]), ('k2live2', [(HIT, b(2)), (HIT + 2, b(0xE2))]),
                          ('k3', [(HIT, b(1)), (HIT + 2, b(3))]), ('k22', [(HIT, b(1)), (HIT + 2, b(0x22))])):
            add(f'8f30/1/hit/{r & 0xFFFFFFFF:x}/{name}', 0x828F30, shell + st(1, 5) + obj, {0x19A570: [r]})
    add('8f30/1/scribble', 0x828F30, shell + st(1, 0),
        {0x19A570: [O.Scribble(1, [(0x700031D4, w(HIT)), (HIT, b(1)), (HIT + 2, b(0x42)), (0x700031D0, w(SURF + 0x40)),
                                   (SURF + 0x5C, w(0x77)), (A + 0xB0, f(1.))])],
         0x1B41F0: [O.Scribble(None, [(A + 0xB4, f(2.))])]})
    add('8f30/1/scribble/miss', 0x828F30, shell + st(1, 0),
        {0x19A570: [O.Scribble(0, [(A + 0xB4, f(9.5)), (A + 0xBC, f(3.))])]})
    add('8f30/2/scribble', 0x828F30, shell + st(2, 0),
        {0x1FBD50: [O.Scribble(None, [(A + 0x20, w(OB)), (A + 4, b(7))])]})
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA21C)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(12, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_overlay.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_overlay_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21g_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(' % symbol, header, re.M), symbol


def entry_guards():
    """A latched fault, a NULL hook table, a NULL fault pointer (and for
    0x828E80 a NULL result pointer): every entry refuses with no hook or
    memory call and writes no result."""
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
        tries = [(C.byref(hooks), O.Fault(0x1234, 7), True, (0x1234, 7)), (None, O.Fault(), True, (0, 0)),
                 (C.byref(hooks), None, True, None)]
        if kind is not None:
            tries.append((C.byref(hooks), O.Fault(), False, (0, 0)))
        for table_, fault, with_result, expected in tries:
            touched.clear()
            result = C.c_int32(0x5A5A5A5A)
            args = [table_] + O.native_args(entry, O.DEFAULT_ARGS[entry])
            if kind is not None:
                args.append(C.byref(result) if with_result else None)
            rc = getattr(O.NATIVE, symbol)(*args, C.byref(fault) if fault is not None else None)
            assert rc == -1 and not touched and result.value == 0x5A5A5A5A, symbol
            if fault is not None:
                assert (fault.address, fault.code) == expected, symbol


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
    only = os.environ.get('EM_AREA21_OVERLAY_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_OVERLAY_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA21_OVERLAY_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA21_OVERLAY_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_OVERLAY_TRACE'):
        trace(os.environ['EM_AREA21_OVERLAY_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_OVERLAY_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
