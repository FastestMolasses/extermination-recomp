#!/usr/bin/env python3
"""AREA21's group members (src/game/em_area21_groups.c) against the
ORIGINAL instructions: the members of the groups 0x82B500 / 0x82BA70 that
the scripts 0x82B5E0 / 0x82C5C0 place, 0x823580 (its effect position a
frame local, compared at its original stack address), 0x8237B0, 0x823860,
0x8238E0 and the empty 0x823AA0, the effect nodes 0x824CF0 and 0x824FE0
(no static reference, but reached in the boss fight: lane PORT-A21-BOSS;
their packets frame locals; 0x825B00, 0014EC00's 001EFD90 node, g77
row 5893, the same; 0x824FE0's parent node and its 32 child
objects are designed at PAR), and the sub 0 placement [52] 0x8243B0 (its
packet a frame local). Decomp docs/AREA21_OVERLAY.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
tools/test_area21_doors_reference.py configures it. No AREA21 recording
exists: every entry runs as original code from the user's
extract/OVERLAY/AREA21.BIN, the WHOLE file loaded over the arena of the
fork recording a03b_03_item2a. Every callee is a hook with a scripted
result, except the pure helpers in MATH, which run as original code nested
in the oracle (their writes replayed to the native side). The entries
with a frame local take the stack pointer (kind S): the oracle runs with
sp = STACK_TOP and the frame locals live in the compared stack window. The
designed node lives in zero RAM at FREE. Compared per case: every call and
its arguments, memory at every call entry, the memory accesses between
calls one for one in order, memory after the last store, a poisoned
re-run, coverage of every reachable original word (DEAD excepted, each
with its reason), the callee inventory and the fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA21_GROUPS_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA21_GROUPS_SOURCE=
<dir> tests another copy of the module source; EM_AREA21_GROUPS_SHOW=<n>
prints n failures; EM_AREA21_GROUPS_MISSING=1 lists unexecuted words;
EM_AREA21_GROUPS_TRACE=<label> prints the original's calls and accesses of
matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area21_groups as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
# entry: (native symbol, byte length, argument kinds, result kind)
FUNCS = {
    0x823580: ('em_area21_groups_00823580', 0x224, [U, S], None),
    0x8237B0: ('em_area21_groups_008237B0', 0xA4, [U], None),
    0x823860: ('em_area21_groups_00823860', 0x78, [U], None),
    0x8238E0: ('em_area21_groups_008238E0', 0x12C, [U], None),
    0x823AA0: ('em_area21_groups_00823AA0', 0x8, [U], None),
    0x824CF0: ('em_area21_groups_00824CF0', 0x2F0, [U, S], None),
    0x8243B0: ('em_area21_groups_008243B0', 0x93C, [U, S], None),
    0x824FE0: ('em_area21_groups_00824FE0', 0xB18, [U, S], None),
    0x825B00: ('em_area21_groups_00825B00', 0x160, [U, S], None),
}
OVERLAY_ID = 0x12
AREA21 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA21.BIN'
AREA03 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'
BEAT = 'a03b_03_item2a'
DESIGNED = 'designed-a21-groups/' + BEAT   # the capture with AREA21.BIN loaded over the arena
FREE = 0x1E61000             # zero in the capture
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
# the pure helpers run as original code (their writes replayed): the
# vector copy, the sine, the matrix identity, rotation and translation and
# the float -> unsigned helper 001281C0 (0x8243B0's casts)
MATH = frozenset((0x102948, 0x11DE90, 0x1029C0, 0x102C58, 0x102918, 0x1281C0, 0x1026A0, 0x102BB0))
# Words the static walk reaches that no input can reach, and why.
DEAD = {
    # 0x824FE0's mode 10 picks k by a switch on its loop index j, which is
    # only 0 or 1: the switch's no-case path (a delay slot) never runs
    0x825544: 'mode 10: the switch on j (0 or 1) has no third path',
}
D_70003B68 = 0x70003B68     # the frame counter
D_700038A0 = 0x700038A0     # 0x8238E0's scratchpad vector


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
    return [A, O.STACK_TOP] if S in FUNCS[entry][2] else [A]


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {4: AREA03, OVERLAY_ID: AREA21}
    O.OUT = ROOT / 'build/area21_groups'
    O.SOURCES = ('em_area21_groups.c',)
    O.BEATS = [BEAT]
    O.BEAT_DIRS = {BEAT: refs.capture('levels-recordings-20261030-1/s87/route_a03b/' + BEAT)}
    O.RESIDENT = {BEAT: 4}
    O.BY_ADDRESS = {a: (n, args, res) for n, a, args, res in HOOKS}
    O.BY_NAME = {n: a for n, a, _, _ in HOOKS}
    O.HOOK_NAMES = [h[0] for h in HOOKS] + ['w_callback']
    O.Hooks = type('Hooks', (C.Structure,), {'_fields_':
        [('ctx', C.c_void_p), ('bytes', O.BYTES_FN)] +
        [(name, O.hook_proto(args, res)) for name, _, args, res in HOOKS] + [('w_callback', O.CALLBACK_FN)]})
    O.SELF_ENTRIES = set()
    O.SCRIBBLED_CALLBACKS = set()
    O.HEAVY = set()
    O.RETURN_SITES = O.RETURN_SITES_FULL = ()
    # no entry calls a node's +0x4C method (the field is the shared table shape)
    O.CONTRACT_UNREACHABLE = {'w_callback': 'no entry here calls a node method'}
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
    assert not any(designed[FREE:FREE + 0x10000]), 'the designed-record area is not zero'
    O.CAPTURES[DESIGNED] = (bytes(designed), spad)


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, -2, 3, 0x12345678, 0x40000000, -0x80000000, 0x00012000, 0x5A5A5A5A, -0x3FFFFFFF)
# 001C6190's results around every bound 0x823580 / 0x8237B0 test
DISTANCES = (-1, 0, 500, 501, 599, 600, 601, 949, 950, 1000, 1001, 1039, 1040, 1041, 1079, 1080, 0x7FFFFFFF)


def fbits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(fbits(x) + d)


def s32(v):
    return w(v & 0xFFFFFFFF)


def s16(v):
    return h(v & 0xFFFF)


def node():
    """The node's position and vector the members read."""
    return [(A + 0xB0, fw(10., 20., 30., 1.)), (A + 0xC0, fw(0., .5, 0., 0.)), (A + 0x100, fw(-40., 5., 72.5, 1.)),
            (A + 0xD, b(3))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return node() + [
        (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 0xD, b(c((0, 3, 4, 5, 6)))),
        (A + 0x94, s16(c((3, 5, 0, 4, 0x103)))), (A + 0x1F0, s32(c((0, 11, 19, 34, 14, -1, 0x7FFFFFFF)))),
        (A + 0x1F4, s32(c((0, 59, 60)))), (D_70003B68, s32(c((60, 0, 25, 30, 65, 325, -30, 61)))),
        (A + 0x28, s16(c((0, 1, -1, 0x7FFF, -0x8000)))), (A + 0x2A, s16(c((1, -1, 0x7FFF)))),
        (0x8101E4, b(c((0, 3, 4, 3)))), (0x81024E, s16(c((29, 30, 31, 32, 33, 0)))),
        (A + 0x1F8, w(c((0x3FC00000, 0x40000000, 0x3F000000, 0x40100000)))),
    ]


# 0x824FE0's parent node and its child objects (32 slots)
PAR = FREE + 0x2000
EFFECT = FREE + 0x6000      # a node 001EFD20 returns


def child(s):
    return FREE + 0x3000 + 0x100 * s


def fe0_world(ys=None, xs=None):
    """par (state 1) and its 32 children: each a matrix at +0x90 (a
    rotation and a translation) and a position at +0xC0."""
    out = [(A + 0x24, w(PAR)), (PAR + 4, b(1))]
    for s_ in range(32):
        c_ = child(s_)
        y = ys[s_] if ys and s_ in ys else 30. + s_
        x = xs[s_] if xs and s_ in xs else -5. + 2 * s_
        out += [(PAR + 0x110 + 4 * s_, w(c_)),
                (c_ + 0x90, fw(0., 0., -1., 0., 0., 1., 0., 0., 1., 0., 0., 0., 7. + s_, 3., -2. * s_, 1.)),
                (c_ + 0xC0, fw(x, 0., 50. - s_, 1.))]
        out.append((c_ + 0xC4, f(y) if isinstance(y, float) else y))
    return out


TBL7, TBL12 = 0x82B090, 0x82B0B0


def slots(table, n):
    """The child slot indices of a table, read from the user's AREA21.BIN
    (the 7-entry table is read on into the next: mode 10 reads entry 9)."""
    data = AREA21.read_bytes()
    return struct.unpack_from('<%di' % n, data, table - O.ARENA)


def fe0_cases(add, st, ulp):
    W = A + 0x1F0

    def node(state, mode=0, kind=6, prev=None, f0=1., tbl=TBL7, n=7, seed=0x12345678, saved=None, ph=None,
             ys=None, xs=None, frame=24):
        out = fe0_world(ys, xs) + st(state) + [
            (A + 5, b(mode)), (A + 6, b(mode if prev is None else prev)), (A + 0xD, b(kind)),
            (W + 0xF0, f(f0) if isinstance(f0, float) else f0), (W + 0xF4, w(tbl)), (W + 0xF8, s32(n)),
            (W + 0xFC, s32(seed)), (D_70003B68, s32(frame))]
        for k in range(12):
            y = saved[k] if saved and k in saved else 0.
            out.append((W + 16 * k, fw(1. + k, 0., 3. * k, 1.)))
            out.append((W + 16 * k + 4, f(y) if isinstance(y, float) else y))
            p = ph[k] if ph and k in ph else 0.1 * k
            out.append((W + 0xC0 + 4 * k, f(p) if isinstance(p, float) else p))
        return out

    s7, s12 = slots(TBL7, 10), slots(TBL12, 12)

    def slot(n, k):
        return (s7 if n == 7 else s12)[k]

    for state in (2, 3, 4, 0x80, 0xFF):
        add(f'4fe0/{state:x}', 0x824FE0, node(state))
    for kind in (6, 7, 0, 5, 0x86):
        for r in (0x12345678, -1, 0, 0x7FFF0000):
            add(f'4fe0/0/{kind}/{r & 0xFFFFFFFF:x}', 0x824FE0, node(0, mode=1, kind=kind, n=3),
                {0x122BB8: [r], 0x1CCF70: [4] * 12})
    add('4fe0/0/par3', 0x824FE0, node(0, kind=7) + [(PAR + 4, b(3))], {0x122BB8: [9]})
    for ps in (3, 0, 2, 4):
        add(f'4fe0/1/par{ps}', 0x824FE0, node(1, mode=0x63) + [(PAR + 4, b(ps))])
    for mode in (0, 4, 5, 9, 13, 0x62, 0x63, 0x64, 0xFF):
        for f0 in (0., 1., -0.):
            add(f'4fe0/1/mode{mode:x}/{f0}', 0x824FE0, node(1, mode=mode, f0=f0), {0x1CCF70: [6] * 12})
    # mode 1: saved y below -40, object y above -40
    for n, tbl in ((7, TBL7), (12, TBL12)):
        for sv in (-40., ulp(-40., 1), -41., -39., 0.):
            for oy in (-40., ulp(-40., -1), ulp(-40., 1), -39., -41., 100.):
                k = 3
                add(f'4fe0/1/1/{n}/{sv}/{oy}', 0x824FE0,
                    node(1, mode=1, tbl=tbl, n=n, saved={k: sv, 0: sv, n - 1: sv},
                         ys={slot(n, k): oy, slot(n, 0): oy, slot(n, n - 1): oy}), {0x1CCF70: [5] * 12})
    # mode 2: the strength and the downward crossings of 50
    for prev in (2, 1, 3):
        for f0 in (1., 1. / 60, ulp(1. / 60, 1), ulp(1. / 60, -1), 0., 0.5, -1.):
            add(f'4fe0/1/2/{prev}/{f0}', 0x824FE0, node(1, mode=2, prev=prev, f0=f0), {0x1CCF70: [5] * 12})
    for n, tbl in ((7, TBL7), (12, TBL12)):
        for sv in (50., ulp(50., 1), 51., 49.):
            for oy in (50., ulp(50., -1), 49., 51.):
                add(f'4fe0/1/2/{n}/{sv}/{oy}', 0x824FE0,
                    node(1, mode=2, tbl=tbl, n=n, saved={k: sv for k in range(12)},
                         ys={slot(n, k): oy for k in range(n)}), {0x1CCF70: [5] * 12})
    # mode 3: objects inside (-10, 20) on frames % 12
    for frame in (24, 25, 0, -12, -13, 0x7FFFFFFF):
        for oy in (-10., ulp(-10., 1), 0., ulp(20., -1), 20., 25., -11.):
            add(f'4fe0/1/3/{frame}/{oy}', 0x824FE0,
                node(1, mode=3, kind=7, tbl=TBL12, n=12, frame=frame, ys={slot(12, k): oy for k in (0, 5, 11)}),
                {0x1CCF70: [5] * 12})
    # mode 10: entries 6 and 9 crossing 46, the every-12th effect
    for n, tbl in ((7, TBL7), (12, TBL12)):
        for sv in (46., ulp(46., 1), 50.):
            for oy in (46., ulp(46., -1), 30.):
                add(f'4fe0/1/10/{n}/{sv}/{oy}', 0x824FE0,
                    node(1, mode=10, tbl=tbl, n=n, frame=25, saved={6: sv, 9: sv},
                         ys={slot(n, 6): oy, slot(n, 9): oy}))
    for frame in (24, 25, -12):
        for r in (0, 1, -1, 2):
            for res in (0, EFFECT):
                add(f'4fe0/1/10/fire/{frame}/{r}/{res:x}', 0x824FE0,
                    node(1, mode=10, frame=frame, saved={6: 50., 9: 0.}, ys={slot(7, 6): 30.}),
                    {0x122BB8: [r], 0x1EFD20: [0, 0, 0, res]})
    for mode in (11, 12):
        for n, tbl in ((7, TBL7), (12, TBL12)):
            add(f'4fe0/1/{mode}/{n}', 0x824FE0, node(1, mode=mode, tbl=tbl, n=n, f0=0.))
    # the packets: y above 10, the phases' wrap, seeds
    for oy in (10., ulp(10., 1), ulp(10., -1), -50.):
        add(f'4fe0/1/pkt/y/{oy}', 0x824FE0, node(1, mode=1, ys={s_: oy for s_ in range(32)}), {0x1CCF70: [7] * 12})
    import numpy as np
    edge = float(np.float32(2.0) - np.float32(0.015))
    for d in (-2, -1, 0, 1, 2):
        add(f'4fe0/1/pkt/ph/{d}', 0x824FE0, node(1, mode=1, ph={k: ulp(edge, d) for k in range(7)}),
            {0x1CCF70: [7] * 12})
    for p in (2., 3., -1., 100.):
        add(f'4fe0/1/pkt/ph/{p}', 0x824FE0, node(1, mode=1, ph={k: p for k in range(7)}), {0x1CCF70: [7] * 12})
    for seed in (0, 1, -1, 0x7FFFFFFF, -0x80000000, 0xFFFF0000 - 0x100000000, 0x0000FFFF, 0x7FFF8000):
        add(f'4fe0/1/pkt/seed/{seed & 0xFFFFFFFF:x}', 0x824FE0, node(1, mode=1, kind=7, tbl=TBL12, n=12, seed=seed),
            {0x1CCF70: [3] * 12})
    for n in (0, 1, -1):
        for mode in (1, 2, 3, 10):
            add(f'4fe0/1/n/{n}/{mode}', 0x824FE0, node(1, mode=mode, n=n, prev=0, frame=0,
                                                       saved={k: 60. for k in range(12)}, ys={s_: 0. for s_ in range(32)}))
        add(f'4fe0/0/n/{n}', 0x824FE0, node(0, mode=1, kind=0, n=n))
    # callees that rewrite what the original reads after them
    add('4fe0/1/scribble/fx', 0x824FE0, node(1, mode=2, saved={k: 60. for k in range(12)},
                                             ys={slot(7, k): 40. for k in range(7)}),
        {0x1EFD20: [O.Scribble(0, [(W + 0xF8, s32(3)), (W + 0xF0, f(0.25)), (child(slot(7, 1)) + 0xC4, f(-80.))])]})
    add('4fe0/1/scribble/pkt', 0x824FE0, node(1, mode=1),
        {0x1CFBE0: [O.Scribble(None, [(W + 0xC0 + 4, f(1.99)), (W + 0xF0, f(0.)), (W + 0xF8, s32(2)),
                                      (0x70003A20, f(5.))])],
         0x1CCF70: [O.Scribble(9, [(0x700036D4, f(-3.)), (W + 0xC4, f(1.996))])] * 12})
    add('4fe0/1/scribble/fog', 0x824FE0, node(1, mode=1),
        {0x21B9A0: [O.Scribble(None, [(W + 0xF0, f(0.)), (A + 5, b(2))]), O.Scribble(None, [(W + 0xFC, s32(77))])]})
    add('4fe0/1/scribble/fire', 0x824FE0, node(1, mode=10, frame=12, saved={6: 50.}, ys={slot(7, 6): 30.}),
        {0x122BB8: [O.Scribble(1, [(PAR + 0x110 + 4 * 7, w(child(8)))])],
         0x1EFD20: [O.Scribble(0, [(D_70003B68, s32(13)), (A + 0x24, w(PAR + 4))])] * 3 + [EFFECT]})


def hook_values(rng):
    def ints(*values, n=4): return [rng.choice(values) for _ in range(n)]
    return {0x122BB8: ints(*RANDS), 0x1C6190: ints(*DISTANCES), 0x1CCF70: ints(0, 7, -1, 0xFFFFFF, n=14)}


def cases():
    out, labels = [], set()

    def add(label, e, patches=(), queues=None, run=MATH, args=None):
        assert label not in labels, label
        labels.add(label)
        all_patches = node() + list(patches)
        out.append((label, DESIGNED, e, tuple(args if args is not None else O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    def st(state):
        return [(A + 4, b(state))]

    # ---- 0x823580: the frame count and the effects
    frame = 1000

    def counted(count, kind, link=True):
        """State 1 with the count reaching `count` (consecutive frames when
        `link`, else a reset to 0)."""
        return st(1) + [(D_70003B68, s32(frame)), (A + 0x1F4, s32(frame - 1 if link else frame - 5)),
                        (A + 0x1F0, s32(count - 1)), (A + 0x94, s16(kind))]
    for state in (0, 2, 3, 4, 0x80, 0x81, 0xFF):
        add(f'3580/{state:x}', 0x823580, st(state) + [(D_70003B68, s32(frame)), (A + 0x1F0, s32(9))])
    for kind in (0, 4, 0x103, 0x305, -3):
        add(f'3580/1/kind/{kind}', 0x823580, counted(12, kind))
    for count in (12, 13, 24, -12, -13, 0, -0x7FFFFFFF):
        add(f'3580/1/5/{count}', 0x823580, counted(count, 5))
    add('3580/1/5/reset', 0x823580, counted(7, 5, link=False))
    add('3580/1/5/wrap', 0x823580, st(1) + [(D_70003B68, s32(-0x80000000)), (A + 0x1F4, s32(0x7FFFFFFF)),
                                            (A + 0x1F0, s32(0x7FFFFFFF)), (A + 0x94, s16(5))])
    for v in DISTANCES:
        for count in (20, 35, 15, 105, 140, 1, 0):
            add(f'3580/1/3/{v}/{count}', 0x823580, counted(count, 3), {0x1C6190: [v], 0x122BB8: [0x40000000]})
    for r in RANDS:
        add(f'3580/1/3/rand/{r & 0xFFFFFFFF:x}', 0x823580, counted(20, 3), {0x1C6190: [0], 0x122BB8: [r]})
    add('3580/1/3/reset', 0x823580, counted(7, 3, link=False), {0x1C6190: [0]})
    add('3580/1/3/neg', 0x823580, counted(-20, 3), {0x1C6190: [0], 0x122BB8: [-5]})
    # callees that rewrite fields the original reads after them
    add('3580/1/3/scribble', 0x823580, counted(20, 3),
        {0x1C6190: [O.Scribble(0, [(A + 0x1F0, s32(21))])], 0x122BB8: [O.Scribble(0, [])]})
    add('3580/1/3/scribble/fire', 0x823580, counted(21, 3),
        {0x1C6190: [O.Scribble(0, [(A + 0x1F0, s32(40))])],
         0x122BB8: [O.Scribble(7, [(A + 0x100, f(3.)), (A + 0x108, f(-9.))])]})
    add('3580/1/5/scribble', 0x823580, counted(12, 5),
        {0x122BB8: [O.Scribble(7, [(A + 0x100, f(8.)), (A + 0x108, f(.5))])]})
    # ---- 0x8237B0 / 0x823860: the frame-counter effects
    for state in (0, 2, 3, 4, 0x80, 0x81, 0xFF):
        add(f'37b0/{state:x}', 0x8237B0, st(state))
        add(f'3860/{state:x}', 0x823860, st(state))
    for v in DISTANCES:
        for fr in (0, 25, 65, 325, 26, -25, -65, -0x80000000, 0x7FFFFFFF):
            add(f'37b0/1/{v}/{fr}', 0x8237B0, st(1) + [(D_70003B68, s32(fr))], {0x1C6190: [v]})
    add('37b0/1/scribble', 0x8237B0, st(1) + [(D_70003B68, s32(26))],
        {0x1C6190: [O.Scribble(0, [(D_70003B68, s32(50))])]})
    for fr in (0, 30, 31, 60, -30, -31, -0x80000000, 0x7FFFFFFF, 0x7FFFFFE2 - 0x7FFFFFE2 % 30):
        add(f'3860/1/{fr}', 0x823860, st(1) + [(D_70003B68, s32(fr))])
    # ---- 0x8238E0: the quad
    for state in (2, 3, 4, 0x80, 0x81, 0x82, 0x83, 0xFF):
        add(f'38e0/{state:x}', 0x8238E0, st(state) + [(A + 0x28, s16(5))])
    for r in RANDS:
        add(f'38e0/0/{r & 0xFFFFFFFF:x}', 0x8238E0, st(0) + [(A + 0x28, s16(77)), (A + 0x2A, s16(9))],
            {0x122BB8: [r]})
    for n in (0, 1, -1, 100, 0x7FFF, -0x8000):
        for d in (1, -1, 0x7FFF):
            add(f'38e0/1/{n}/{d}', 0x8238E0, st(1) + [(A + 0x28, s16(n)), (A + 0x2A, s16(d))])
    add('38e0/1/x', 0x8238E0, st(1) + [(A + 0x28, s16(3)), (A + 0xB0, ulp(-0.09, 1))])
    add('38e0/0/scribble', 0x8238E0, st(0), {0x122BB8: [O.Scribble(-3, [(A + 0x28, s16(4))])]})
    add('38e0/1/scribble', 0x8238E0, st(1) + [(A + 0x28, s16(3)), (A + 0x2A, s16(1))],
        {0x1CD520: [O.Scribble(None, [(A + 0x28, s16(-9)), (A + 0x2A, s16(4)), (D_700038A0, f(2.))])]})
    # ---- 0x823AA0
    add('3aa0', 0x823AA0)
    # ---- 0x824CF0 (no static reference; reached in g77)
    for state in (2, 3, 4, 0x80, 0x81, 0x82, 0x83, 0xFF):
        add(f'4cf0/{state:x}', 0x824CF0, st(state))
    for kind in (0, 3, 4, 5, 6, 0x83, 0x84, 0x85, 0xFF):
        for r in (0x12345678, -1, 0, 0x7FFF0000):
            add(f'4cf0/0/{kind}/{r & 0xFFFFFFFF:x}', 0x824CF0, st(0) + [(A + 0xD, b(kind))],
                {0x122BB8: [r], 0x1CCF70: [5]})
    for kind in (3, 5):
        for name, phase in (('0', f(0.)), ('1.17', f(1.17)), ('1.2-', ulp(1.17, 1)), ('1.2', f(1.2)), ('-1', f(-1.))):
            for seed in (0, 0xFFFF0000, 0x80001234):
                add(f'4cf0/1/{kind}/{name}/{seed:x}', 0x824CF0,
                    st(1) + [(A + 0xD, b(kind)), (A + 0x1F0, phase), (A + 0x1F8, w(seed))], {0x1CCF70: [3]})
    for name, phase in (('0', f(0.)), ('1.99', f(1.99)), ('1.99+', ulp(1.99, 1)), ('2', f(2.)), ('3', f(3.))):
        for count in (0, 239, 240, 241, -1, 0x7FFFFFFF):
            for scale in (1., 1. / 60, .01, 0., -.5):
                add(f'4cf0/1/4/{name}/{count}/{scale}', 0x824CF0,
                    st(1) + [(A + 0xD, b(4)), (A + 0x1F0, phase), (A + 0x1F4, f(scale)), (A + 0x1FC, s32(count)),
                             (A + 0x1F8, w(0xBEEF0000))], {0x1CCF70: [-1]})
    add('4cf0/1/scribble', 0x824CF0, st(1) + [(A + 0xD, b(6))],
        {0x1CCF70: [O.Scribble(4, [(A + 0xD, b(4)), (A + 0x1F8, w(0x7777AAAA))])]})
    add('4cf0/1/4/scribble', 0x824CF0, st(1) + [(A + 0xD, b(4)), (A + 0x1FC, s32(240))],
        {0x1CFBE0: [O.Scribble(None, [(A + 0x1F0, f(5.)), (A + 0x1FC, s32(5)), (A + 0x1F4, f(.001))])],
         0x1CFB50: [O.Scribble(None, [(A + 0x1F0, f(1.995))])]})
    add('4cf0/1/4/scribble2', 0x824CF0, st(1) + [(A + 0xD, b(4)), (A + 0x1FC, s32(9))],
        {0x1CFBE0: [O.Scribble(None, [(A + 0x1F0, f(1.995)), (A + 0x1FC, s32(500)), (A + 0x1F4, f(.001))])]})
    add('4cf0/1/3/scribble', 0x824CF0, st(1) + [(A + 0xD, b(3))],
        {0x1CFBE0: [0, O.Scribble(None, [(A + 0x1F0, f(1.19))])]})
    # ---- 0x825B00 (no static reference; 0014EC00's 001EFD90 node, g77 row 5893)
    for state in (2, 3, 4, 0x80, 0x81, 0x82, 0x83, 0xFF):
        add(f'5b00/{state:x}', 0x825B00, st(state))
    for r in RANDS:
        add(f'5b00/0/{r & 0xFFFFFFFF:x}', 0x825B00, st(0), {0x122BB8: [r], 0x1CCF70: [5]})
    for name, phase in (('0', f(0.)), ('1.49', f(1.49)), ('1.5-', ulp(1.5 - 1. / 120, 1)),
                        ('1.5-1', ulp(1.5 - 1. / 120, -1)), ('1.5-0', f(1.5 - 1. / 120)), ('1.5', f(1.5)),
                        ('-1', f(-1.)), ('inf', w(0x7F800000)), ('-0', w(0x80000000))):
        for h_ in (0, 3, -1):
            add(f'5b00/1/{name}/{h_}', 0x825B00, st(1) + [(A + 0x1F0, phase), (A + 0x1F4, f(.375))], {0x1CCF70: [h_]})
    add('5b00/1/scribble', 0x825B00, st(1) + [(A + 0x1F0, f(.5))],
        {0x1CCF70: [O.Scribble(4, [(A + 0x1F0, f(1.)), (A + 0x1F4, f(-.25))])],
         0x1CFBE0: [O.Scribble(None, [(A + 0x1F0, f(1.495)), (A + 4, b(7))])]})
    add('5b00/1/scribble2', 0x825B00, st(1) + [(A + 0x1F0, f(1.49))],
        {0x1CFAE0: [O.Scribble(None, [(A + 0x1F0, f(0.)), (A + 4, b(2))])]})
    add('5b00/0/scribble', 0x825B00, st(0),
        {0x122BB8: [O.Scribble(-7, [(A + 0x1F0, f(2.)), (A + 4, b(3))])], 0x1CCF70: [1]})
    # ---- 0x8243B0, placement [52]
    W = A + 0x1F0
    D_8101E4, D_81024E, D_3A20, D_3A24 = 0x8101E4, 0x81024E, 0x70003A20, 0x70003A24

    def detail(kind, stage):
        return [(D_8101E4, b(kind)), (D_81024E, s16(stage))]

    def work(phases=(0.25, 0.5, 0.75, 1.0), seed=0x12345678):
        return [(W + 4 * k, f(v) if isinstance(v, float) else v) for k, v in enumerate(phases)] + [(W + 0x10, w(seed))]

    def handles(*values):
        return {0x1CCF70: list(values)}

    for state in (2, 3, 4, 0x80, 0x81, 0xFF):
        add(f'43b0/{state:x}', 0x8243B0, st(state) + detail(3, 29) + work())
    for r in RANDS:
        add(f'43b0/0/{r & 0xFFFFFFFF:x}', 0x8243B0, st(0) + detail(0, 0), {0x122BB8: [r, 0x7FFF0000, 5, r, -r]})
    add('43b0/0/scribble', 0x8243B0, st(0) + detail(3, 30),
        {0x122BB8: [1, 2, 3, 4, O.Scribble(0x2468ACE0, [(D_8101E4, b(7))])]})
    for kind in (0, 1, 2, 3, 4, 0x83, 0xFF):
        stages = (28, 29, 30, 31, 32, 33, 0, -1, 0x7FFF, -0x8000, 0x101D, 0x11D) if kind == 3 else (30, 32, 29)
        for stage in stages:
            for hs in ((5,) * 14, (0xFFFFFF,) * 14, (7, 0xFFFFFF, -1, 0, 0x1000000) * 3):
                add(f'43b0/1/{kind}/{stage}/{hs[0]:x}/{len(set(hs))}', 0x8243B0,
                    st(1) + detail(kind, stage) + work(), handles(*hs))
    for seed in (0, 1, -1, 0x7FFFFFFF, -0x80000000, 0x12345678, 0xFFFF0000 - 0x100000000, 0x0000FFFF, 0x7FFF8000):
        add(f'43b0/1/seed/{seed & 0xFFFFFFFF:x}', 0x8243B0, st(1) + detail(0, 0) + work(seed=seed), handles(*[3] * 14))
    # the phase advances and the wraps: x + step is at most 2.0, exactly 2.0 and just above
    import numpy as np
    def f32(x):
        return float(np.float32(x))
    for k, step in enumerate((0.01, 0.005, 0.02, 0.01)):
        base = f32(2.0 - f32(step))
        for d in (-2, -1, 0, 1, 2):
            phases = [0.1, 0.2, 0.3, 0.4]
            phases[k] = w(fbits(base) + d)
            add(f'43b0/1/adv{k}/{d}', 0x8243B0, st(1) + detail(3, 29) + work(phases), handles(*[5] * 14))
    for p in (0., 1.5, 1.99, 2., 3., -1., 100.):
        add(f'43b0/1/adv/{p}', 0x8243B0, st(1) + detail(3, 29) + work((p, p, p, p)), handles(*[5] * 14))
    # the default step's wraps: D_70003A20 / 24 = phase + random, at most 2.0 or above
    for p2 in (0., 1., 1.5, 1.9, 1.9999, 2., 2.5, 3.):
        for p3 in (0., 1.2, 1.9999, 2., 5.):
            for seed in (0x00010000, 0x7FFF0000, 0xFFFF0000 - 0x100000000):
                add(f'43b0/1/wrap/{p2}/{p3}/{seed & 0xFFFFFFFF:x}', 0x8243B0,
                    st(1) + detail(1, 0) + work((0.1, 0.2, p2, p3), seed), handles(*[5] * 14))
    # callees that rewrite what the original reads after them
    add('43b0/1/scribble/flags', 0x8243B0, st(1) + detail(0, 0) + work(),
        {0x1CCF70: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14],
         0x1CFBE0: [O.Scribble(None, [(D_8101E4, b(3)), (D_81024E, s16(29)), (W, f(1.99)), (W + 4, f(3.)),
                                      (W + 8, f(1.)), (W + 12, f(1.))])]})
    add('43b0/1/scribble/random', 0x8243B0, st(1) + detail(0, 0) + work(), handles(*[5] * 14) | {
        0x1CFB50: [O.Scribble(None, [(0x70003A34, f(9.)), (0x70003A38, f(8.)), (0x70003A3C, f(7.)),
                                     (D_3A24, f(1.5)), (D_3A20, f(6.))])] * 28})
    add('43b0/1/scribble/fog', 0x8243B0, st(1) + detail(0, 0) + work(), handles(*[5] * 14) | {
        0x21B9A0: [O.Scribble(None, [(W + 0x10, w(0x5A5A5A5A))])]})
    add('43b0/1/scribble/matrix', 0x8243B0, st(1) + detail(0, 0) + work(), handles(*[5] * 14) | {
        0x1CCF70: [O.Scribble(5, [(W + 8, f(1.995)), (W + 12, f(1.995))])]})
    # ---- 0x824FE0, the node of [43] / [44]'s deferred spawn
    fe0_cases(add, st, ulp)
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA21B)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(24, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_groups.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_groups_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21p_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(' % symbol, header, re.M), symbol


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
    only = os.environ.get('EM_AREA21_GROUPS_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_GROUPS_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA21_GROUPS_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA21_GROUPS_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_GROUPS_TRACE'):
        trace(os.environ['EM_AREA21_GROUPS_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_GROUPS_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
