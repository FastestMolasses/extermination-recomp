#!/usr/bin/env python3
"""AREA21's doors and small sub-0 owners (src/game/em_area21_doors.c)
against the ORIGINAL instructions: the doors [30] 0x826BB0 and [27] / [33]
0x826E60 (jump-table dispatchers: the tables at 0x82DC00 / 0x82DC20 are
left out of the access comparison, the translation is a switch), [54] /
[55] 0x82A1D0, the rounds 0x825C70 with its code pointer 0x825C60, and the
further sub-0 owners in FUNCS. Decomp docs/AREA21_OVERLAY.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
tools/test_level17_boot_reference.py configures it. No AREA21 recording
exists: every entry runs as original code from the user's
extract/OVERLAY/AREA21.BIN, the WHOLE file (header, text and data: the
owners read overlay data) loaded over the arena of the fork recording
a03b_03_item2a (its AREA03 overlay text checked against the user's
AREA03.BIN and its boot text against the pinned ELF before the load). Every
callee is a hook with a scripted result, except the pure helpers in MATH,
which run as original code nested in the oracle (their writes replayed to
the native side). A node's +0x4C method is stubbed and logged. The
designed nodes and records live in zero RAM at FREE. Compared per case:
every call and its arguments, memory at every call entry, the memory
accesses between calls one for one in order, memory after the last store,
a poisoned re-run, coverage of every reachable original word (DEAD
excepted, each with its reason), the callee inventory and the fail-stop
contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA21_DOORS_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA21_DOORS_SOURCE=<dir>
tests another copy of the module source; EM_AREA21_DOORS_SHOW=<n> prints n
failures; EM_AREA21_DOORS_MISSING=1 lists unexecuted words;
EM_AREA21_DOORS_TRACE=<label> prints the original's calls and accesses of
matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area21_doors as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
# entry: (native symbol, byte length, argument kinds, result kind)
FUNCS = {
    0x826BB0: ('em_area21_doors_00826BB0', 0x2A8, [U], None),
    0x826E60: ('em_area21_doors_00826E60', 0x1D8, [U], None),
    0x82A1D0: ('em_area21_doors_0082A1D0', 0x190, [U], None),
    0x825C70: ('em_area21_doors_00825C70', 0x42C, [U], None),
    0x825C60: ('em_area21_doors_00825C60', 0x8, [U], None),
    0x8272C0: ('em_area21_doors_008272C0', 0xFC, [U], None),
    0x8273C0: ('em_area21_doors_008273C0', 0x280, [U], None),
    0x827640: ('em_area21_doors_00827640', 0x234, [U], None),
    0x827880: ('em_area21_doors_00827880', 0x1A8, [U], None),
    0x827A30: ('em_area21_doors_00827A30', 0x294, [U], None),
    0x827E70: ('em_area21_doors_00827E70', 0x4D4, [U], None),
    0x828350: ('em_area21_doors_00828350', 0xC0, [U], None),
}
OVERLAY_ID = 0x12
AREA21 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA21.BIN'
AREA03 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'
BEAT = 'a03b_03_item2a'
DESIGNED = 'designed-a21-doors/' + BEAT   # the capture with AREA21.BIN loaded over the arena
FREE = 0x1E61000             # zero in the capture
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
B = FREE + 0x2000            # another node (the +0x18 object, a hit object)
SURF = FREE + 0x3000         # a hit surface record
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
CALLBACK2 = 0x1CAA40         # the method word a callee writes (stubbed too)
BONES = FREE + 0x4000        # a designed bone array (D_00275B40)
OBJ1, OBJ2 = FREE + 0x5000, FREE + 0x6000   # its +4 / +8 objects
ROOMS = FREE + 0x7000        # a designed D_0024D7C0[area] (one record array per room)
RECS = FREE + 0x8000         # a designed record array
SELF_ENTRIES = {0x82A1D0, 0x8272C0, 0x8273C0, 0x827640, 0x827880, 0x827A30, 0x827E70}
# the pure helpers run as original code (their writes replayed): the
# vector copy, normalise, scale and sum, the absolute value, the angle wrap
MATH = frozenset((0x102948, 0x102760, 0x103230, 0x1028B8, 0x11DF78, 0x1B1470))
# The door step dispatch of 0x826BB0 / 0x826E60: a register jump through a
# six-entry table in the overlay data (the translation is a switch, so the
# table's loads are left out of the access comparison).
# 0x8273C0 / 0x827640's wobble step (+0x28 & 7): the same through eight-
# entry tables.
JUMP_TABLES = {0x826D00: (0x82DC00, 6), 0x826EE0: (0x82DC20, 6), 0x827464: (0x82DC40, 8),
               0x8276DC: (0x82DC60, 8)}
# Words the static walk reaches that no input can reach, and why.
DEAD = {
    0x82744C: "0x8273C0's table bounds test after & 7 never fails (the branch-likely's annulled slot)",
    0x827B70: "0x827A30's effect loop: the switch on i (0..2) has no other arm (its branch's slot)",
    0x827F8C: "0x827E70's effect loop: the switch on i (0..2) has no other arm (its branch's slot)",
}
D_0081080E = 0x81080E
D_00810700 = 0x810700
LOCKS = 0x810841


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


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {4: AREA03, OVERLAY_ID: AREA21}
    O.OUT = ROOT / 'build/area21_doors'
    O.SOURCES = ('em_area21_doors.c',)
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
    O.SCRIBBLED_CALLBACKS = {CALLBACK2}
    O.HEAVY = set()
    O.RETURN_SITES = O.RETURN_SITES_FULL = ()
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.JUMP_TABLES = dict(JUMP_TABLES)
    O.TABLE_BYTES = frozenset(a for t, n in JUMP_TABLES.values() for a in range(t, t + 4 * n))
    O.DEFAULT_ARGS = {e: [A] for e in FUNCS}
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
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x40000000, -0x80000000, 0x00012000, 0x5A5A5A5A)


def fbits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(fbits(x) + d)


def common():
    """The node's method and its +0x18 object."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0x18, w(B))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A + 3, b(c((0x4C, 0x4C, 0)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 3, 4, 5, 6)))),
        (A + 7, b(c((0, 1, 2, 3)))), (A + 0x28, h(c((0, 1491, 1492, 1999, 2000, 0x7FFF)))),
        (A + 0x34, h(c((0, 1, 3, 7, 0x20, 0xFFFF)))), (D_0081080E, b(c((0, 3, 0x10)))),
        (D_00810700, b(c((0, 1, 2)))), (LOCKS, b(c((0, 0xFF, 0x08)))), (LOCKS + 1, b(c((0, 0xFF)))),
        (A + 0xB4, f(c((9., 10., 100.)))), (A + 0x20, f(c((1., 2., 180.)))), (A + 0xC0, fw(0., 0., 1., 0.)),
        (A + 0x1F0, f(c((0., .005, -.005)))), (A + 0x1F4, f(c((0., .3, -.3)))), (A + 0xC4, f(c((0., .3, .305)))),
        (0x275B40, w(BONES)), (BONES + 4, w(OBJ1)), (BONES + 8, w(c((OBJ1, OBJ2)))),
        (OBJ1 + 0x74, f(c((0., 3.2, -3.2)))), (OBJ1 + 0x78, f(c((0., .4, -3.)))),
    ]


def hook_values(rng):
    def ints(*values, n=4): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(0, 1, -1) for a in (0x1BBE40, 0x1BC0E0, 0x1BC290, 0x1B17A0, 0x1B0FD0)}
    out[0x19A570] = ints(0, 1, 2, 3)
    out[0x122BB8] = ints(*RANDS)
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

    def st(state, step=0):
        return [(A + 4, b(state)), (A + 5, b(step))]

    # ---- the doors: the shared steps (D_0081080E not 3 for 0x826BB0)
    for door in (0x826BB0, 0x826E60):
        tag = f'{door & 0xFFFF:04x}'
        for state in (0, 2, 3, 4, 0xFF):
            add(f'{tag}/{state:x}', door, st(state))
        for area in (0, 1, 5):
            for lock in (0, 3, 7, 8, 0x20, 0x23, 0xFFFF, 0x8001):
                for bits in (0, 0x08, 0x80, 0xFF):
                    for r in (0, 1):
                        add(f'{tag}/1/0/{area}/{lock:x}/{bits:x}/{r}', door,
                            st(1, 0) + [(A + 0x34, h(lock)), (D_00810700, b(area)), (LOCKS + area, b(bits)),
                                        (D_0081080E, b(0x10))], {0x1BBE40: [r]})
        for step, hook in ((1, 0x1BC0E0), (2, 0x1BC0E0), (3, 0x1BC0E0), (5, 0x1BC290)):
            for r in (0, 1, -1):
                add(f'{tag}/1/{step}/{r}', door, st(1, step), {hook: [r]})
        add(f'{tag}/1/4', door, st(1, 4))
        for step in (6, 7, 0xFF):
            add(f'{tag}/1/{step}', door, st(1, step))
        # a callee that rewrites +5 (the original re-reads it for ++)
        for step, hook in ((0, 0x1BBE40), (1, 0x1BC0E0), (3, 0x1BC0E0), (4, 0x1BC240)):
            add(f'{tag}/1/{step}/scribble', door, st(1, step) + [(A + 0x34, h(3))],
                {hook: [O.Scribble(1, [(A + 5, b(0xFF))])]})
    # ---- 0x826BB0's timer (D_0081080E == 3)
    timer = [(D_0081080E, b(3))]
    for phase in (0, 2, 3, 0xFF):
        add(f'6bb0/timer/{phase:x}', 0x826BB0, st(1, 0) + timer + [(A + 7, b(phase)), (A + 0x28, h(77))])
    for n in (0, 1490, 1491, 1492, 1998, 1999, 2000, 0x7FFF, 0xFFFF):
        for lock in (0, 3, 7, 0x1F, 0x20, 0xFF):
            for area in (0, 2):
                add(f'6bb0/timer/1/{n}/{lock:x}/{area}', 0x826BB0,
                    st(1, 0) + timer + [(A + 7, b(1)), (A + 0x28, h(n)), (A + 0x34, b(lock)),
                                        (D_00810700, b(area)), (LOCKS + area, b(0x5A))])
    # ---- 0x82A1D0
    for state in (0, 2, 3, 4):
        add(f'a1d0/{state}', 0x82A1D0, st(state), {0x1B0FD0: [1]})
    add('a1d0/0/-1', 0x82A1D0, st(0), {0x1B0FD0: [-1]})
    for kind in (0, 0x4B, 0x4D):
        for r in (0, 1, 2):
            add(f'a1d0/1/copy/{kind:x}/{r}', 0x82A1D0, st(1) + [(A + 3, b(kind)), (B + 0xC4, f(.25))],
                {0x1B17A0: [r]})
    swing = [(A + 3, b(0x4C))]
    for name, step, angle, target in (('far', .005, .2, .4), ('near', .005, .3, .305), ('neg', -.005, -.3, -.31),
                                      ('edge', 0., .01, 0.), ('edge-', 0., .01, None), ('zero', 0., 0., 0.)):
        tb = f(target) if target is not None else ulp(.01, -1)
        for r in RANDS:
            add(f'a1d0/1/swing/{name}/{r & 0xFFFFFFFF:x}', 0x82A1D0,
                st(1) + swing + [(A + 0x1F0, f(step)), (A + 0xC4, f(angle)), (A + 0x1F4, tb)],
                {0x122BB8: [r], 0x1B17A0: [1]})
    # the new target equal to +0xC4 (r 0 gives -pi/6) and one ulp either side
    for name, angle in (('eq', w(0xBF060A92)), ('above', w(0xBF060A91)), ('below', w(0xBF060A93))):
        add(f'a1d0/1/swing/le/{name}', 0x82A1D0,
            st(1) + swing + [(A + 0x1F0, f(0.)), (A + 0xC4, angle), (A + 0x1F4, angle)], {0x122BB8: [0]})
    # ---- 0x825C70 (MATH runs the vector helpers; 001CD390 is a stub)
    rounds = [(A + 0xB0, fw(100., 50., -30., 1.)), (A + 0xC0, fw(3., 0., 4., 0.)), (A + 0x70, fw(9., 9., 9., 9.)),
              (A + 0x200, fw(2., -1., .5, 0.)), (A + 0x20, f(180.)), (0x810374, f(.75)),
              (0x700031B0, fw(11., 12., 13., 14.)), (0x700031D0, w(SURF)), (0x700031D4, w(B)),
              (SURF + 0x1C, w(0x1234)), (SURF + 0x24, fw(.1, .2, .3)), (B, b(1)), (B + 2, b(2)),
              (B + 0x70, fw(1., 2., 3., 4.))]
    for state in (2, 3, 4):
        add(f'5c70/{state}', 0x825C70, rounds + st(state))
    for r in (0, 1, 2, 3, -1):
        add(f'5c70/0/{r}', 0x825C70, rounds + st(0), {0x19A570: [r]})
    for r in (0, 1, 2, 3, -1):
        add(f'5c70/1/{r}', 0x825C70, rounds + st(1), {0x19A570: [r]})
    for name, obj in (('null', [(0x700031D4, w(0))]), ('dead', [(B, b(0))]), ('kind2', [(B, b(2))]),
                      ('bits22', [(B + 2, b(0x22))]), ('bitse2', [(B + 2, b(0xE2))]), ('bits3', [(B + 2, b(3))]),
                      ('bits42', [(B + 2, b(0x42))])):
        for r in (1, 2):
            add(f'5c70/1/{r}/{name}', 0x825C70, rounds + st(1) + obj, {0x19A570: [r]})
    for name, y in (('9', f(9.)), ('10-', ulp(10., -1)), ('10', f(10.)), ('-5', f(-5.)), ('11', f(11.))):
        for life in (1., 2., 0., -1.):
            add(f'5c70/1/0/{name}/{life}', 0x825C70, rounds + st(1) + [(A + 0xB4, y), (A + 0x20, f(life))],
                {0x19A570: [0]})
    add('5c70/1/scribble', 0x825C70, rounds + st(1),
        {0x1CEEE0: [O.Scribble(None, [(A + 0xB4, f(5.)), (A + 0xB0, f(7.))])],
         0x19A570: [O.Scribble(0, [(A + 0xB8, f(8.)), (A + 0x20, f(1.))])]})
    add('5c70/1/scribble/hit', 0x825C70, rounds + st(1),
        {0x19A570: [O.Scribble(1, [(0x700031D4, w(B)), (B + 2, b(0x02)), (0x700031D0, w(SURF + 0x100))])],
         0x1B41F0: [O.Scribble(None, [(0x810374, f(-1.))])]})
    add('5c70/1/scribble/hit3', 0x825C70, rounds + st(1),
        {0x19A570: [O.Scribble(3, [(0x700031D0, w(SURF + 0x100)), (SURF + 0x124, fw(4., 5., 6.))])]})
    # ---- 0x825C60
    add('5c60', 0x825C60)
    # ---- 0x8272C0 / 0x8273C0 / 0x827640: the bone array's objects
    bones = [(0x275B40, w(BONES)), (BONES + 4, w(OBJ1)), (BONES + 8, w(OBJ2)), (OBJ1 + 0x74, f(.5)),
             (OBJ2 + 0x74, f(-1.)), (OBJ1 + 0x78, f(2.)), (A + 0xB0, fw(10., 20., 30.)),
             (A + 0x1F0, fw(-4., 8., 0.))]
    for entry in (0x8272C0, 0x8273C0, 0x827640):
        tag = f'{entry & 0xFFFF:04x}'
        for state in (0, 2, 3, 4):
            add(f'{tag}/{state}', entry, bones + st(state), {0x1B0FD0: [1]})
        for r in (0, 1, -1):
            add(f'{tag}/1/{r}', entry, bones + st(1), {0x1B17A0: [r]})
    for name, a74 in (('0', f(0.)), ('pi', f(3.1415927)), ('-pi', f(-3.1415927)), ('3.2', f(3.2)),
                      ('-3.0', f(-3.)), ('7', f(7.)), ('-pi-', ulp(-3.1415927, 1))):
        add(f'72c0/1/a/{name}', 0x8272C0, bones + st(1) + [(OBJ1 + 0x74, a74), (OBJ2 + 0x74, a74)], {0x1B17A0: [1]})
        add(f'73c0/1/a/{name}', 0x8273C0, bones + st(1) + [(OBJ1 + 0x78, a74)])
    add('72c0/1/same', 0x8272C0, bones + st(1) + [(BONES + 8, w(OBJ1))], {0x1B17A0: [1]})
    for entry in (0x8273C0, 0x827640):
        tag = f'{entry & 0xFFFF:04x}'
        for n in (0, 1, 2, 3, 4, 5, 6, 7, 0x7FFF, 0xFFFF, 0x8003):
            add(f'{tag}/1/step/{n:x}', entry, bones + st(1) + [(A + 0x28, h(n))])
    add('73c0/1/scribble', 0x8273C0, bones + st(1) + [(A + 0x28, h(3))],
        {0x1FC3C0: [O.Scribble(None, [(0x275B40, w(BONES + 0x10)), (BONES + 0x14, w(OBJ2)), (OBJ2 + 0x78, f(1.))])]})
    # ---- 0x827880 / 0x827A30 / 0x827E70: the timed effects
    for entry, frames in ((0x827880, 0xFF), (0x827A30, 0x41), (0x827E70, 0xDC)):
        tag = f'{entry & 0xFFFF:04x}'
        for state in (2, 3, 4, 0xFF):
            add(f'{tag}/{state}', entry, st(state))
        for r in (0, 1, -1):
            add(f'{tag}/0/{r}', entry, st(0, 7), {0x1B0FD0: [r]})
        for counter in (0, 0x10, 0x11, 3):
            add(f'{tag}/1/0/{counter:x}', entry, st(1, 0) + [(D_0081080E, b(counter)), (A + 0x28, h(99))])
        for n in (0, frames - 2, frames - 1, frames, 0x7FFF, 0xFFFF):
            add(f'{tag}/1/1/{n:x}', entry, st(1, 1) + [(A + 0x28, h(n))])
        add(f'{tag}/1/2', entry, st(1, 2) + [(A + 0xB0, fw(10., 20., 30., 1.))])
        add(f'{tag}/1/2/scribble', entry, st(1, 2) + [(A + 0xB0, fw(10., 20., 30., 1.))],
            {0x1EFD20: [O.Scribble(None, [(A + 0xB0, f(-7.)), (A + 0xB4, f(.5))])],
             0x1EFD90: [O.Scribble(None, [(A + 5, b(0x41)), (A + 4, b(9))])]})
        for step in (4, 5, 0xFF):
            add(f'{tag}/1/{step}', entry, st(1, step))
        add(f'{tag}/1/0/scribble', entry, st(1, 0) + [(D_0081080E, b(0x10))],
            {0x1B1B70: [O.Scribble(None, [(A + 0x4C, w(CALLBACK2))])]})
    # 0x827E70's +5 3: the pose from the placement record
    for area, room, slot in ((0, 0, 0), (1, 2, 3), (2, 1, 0xFF)):
        rec = RECS + slot * 0x28
        add(f'7e70/1/3/{area}/{room}/{slot}', 0x827E70,
            st(1, 3) + [(D_00810700, b(area)), (0x810701, b(room)), (A + 0x9A, b(slot)),
                        (0x24D7C0 + area * 4, w(ROOMS)), (ROOMS + room * 4, w(RECS)),
                        (rec + 0x2C, fw(0x5A5A1234, 1., 2., 3., 4., 5., 6., 7.)), (0x28A59C, w(0x31))],
            {0x1C6120: [0x77]})
    add('7e70/1/3/live', 0x827E70, st(1, 3) + [(A + 0x9A, b(0))], {0x1C6120: [-1]})
    # ---- 0x828350
    for state in (0, 2, 3, 4, 0xFF):
        add(f'8350/{state:x}', 0x828350, st(state) + [(A + 0x9A, b(0x81))])
    for counter in (0, 0x10, 0x11):
        for n in (0, 0xF9, 0xFA, 0xFB, 0x7FFF, 0xFFFF):
            add(f'8350/1/{counter:x}/{n:x}', 0x828350, st(1) + [(D_0081080E, b(counter)), (A + 0x28, h(n))])
    add('8350/1/scribble', 0x828350, st(1) + [(D_0081080E, b(0x11)), (A + 0x28, h(0xFA))],
        {0x15AE20: [O.Scribble(None, [(D_0081080E, b(0x10)), (A + 0x28, h(0x100))])]})
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA21D)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(24, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_doors.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_doors_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21d_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(' % symbol, header, re.M), symbol


def table_checks():
    """Each jump table's entries lie inside its function (the static walk
    follows them)."""
    ram = O.CAPTURES[DESIGNED][0]
    for pc, (table, count) in JUMP_TABLES.items():
        entry = max(e for e in FUNCS if e <= pc)
        size = FUNCS[entry][1]
        for i in range(count):
            target = O.u32(ram, table + 4 * i)
            assert entry <= target < entry + size, (hex(pc), i)


def entry_guards():
    """A latched fault, a NULL hook table and a NULL fault pointer: every
    entry refuses with no hook or memory call."""
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
        for table, fault in ((C.byref(hooks), O.Fault(0x1234, 7)), (None, O.Fault()), (C.byref(hooks), None)):
            touched.clear()
            args = [table] + O.native_args(entry, O.DEFAULT_ARGS[entry])
            rc = getattr(O.NATIVE, symbol)(*args, C.byref(fault) if fault is not None else None)
            assert rc == -1 and not touched, symbol
            if fault is not None:
                assert (fault.address, fault.code) == ((0x1234, 7) if table is not None else (0, 0))


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
    only = os.environ.get('EM_AREA21_DOORS_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_DOORS_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA21_DOORS_SOURCE'))
    header_checks()
    table_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA21_DOORS_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_DOORS_TRACE'):
        trace(os.environ['EM_AREA21_DOORS_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_DOORS_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
