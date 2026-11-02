#!/usr/bin/env python3
"""AREA21's shared creature helpers and its first creature
(src/game/em_area21_boot*.c) against the ORIGINAL instructions: the
spawners 001283D0 and 0014D1E0, the small creature 0014C220 with its
states 0014C2E0 / 0014C3A0 / 0014CCC0 / 0014CD00, its moves 0014C680 /
0014C9D0 / 0014C490, its motion 0014CD30 and its attack 0014CDD0; the
creature 00137830 (AREA21 code group 0x82A600 [0]) with its states
00137920 / 00137A50 / 00137C80 / 00137E80, its damage step 001381D0 and
its rig followers 00137EB0. Unwired.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area17_creature_reference.py configures it. Every entry runs as
original code from the user's ELF over a fork recording's RAM
(a03b_03_item2a: boot code is the same in every image; its AREA03 overlay
text is checked against the user's extract/OVERLAY/AREA03.BIN and its boot
text against the pinned ELF). Calls between the family's functions run as
original code at the top level (every one is also an entry); every other
callee is a hook with a scripted result, except the pure helpers in MATH,
which run as original code nested in the oracle (their writes replayed to
the native side). The node's +0x4C method is stubbed and logged. The
designed nodes, vectors and bone tables live in zero RAM at FREE.
Compared per case: every call and its arguments, memory at every call
entry, the memory accesses between calls one for one in order, memory
after the last store, the result, a poisoned re-run, coverage of every
reachable original word (DEAD excepted, each with its reason), the callee
inventory and the fail-stop contract. None of the family's functions
dispatches through a jump table.

EM_TEST_FULL=1 widens the random sample. EM_AREA21_BOOT_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA21_BOOT_SOURCE=<dir>
tests another copy of the module sources; EM_AREA21_BOOT_SHOW=<n> prints n
failures; EM_AREA21_BOOT_MISSING=1 lists unexecuted words;
EM_AREA21_BOOT_TRACE=<label> prints the original's calls and accesses of
matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
from pathlib import Path
import gen_area21_boot as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
UU = [U, U]
FUNCS = {
    0x001283D0: ('em_area21_boot_001283D0', 260, [U, I, U], I),
    0x0014C220: ('em_area21_boot_0014C220', 180, [U], None),
    0x0014C2E0: ('em_area21_boot_0014C2E0', 188, UU, None),
    0x0014C3A0: ('em_area21_boot_0014C3A0', 232, UU, None),
    0x0014C490: ('em_area21_boot_0014C490', 488, UU, None),
    0x0014C680: ('em_area21_boot_0014C680', 840, UU, None),
    0x0014C9D0: ('em_area21_boot_0014C9D0', 740, UU, None),
    0x0014CCC0: ('em_area21_boot_0014CCC0', 60, [U], None),
    0x0014CD00: ('em_area21_boot_0014CD00', 48, [U], None),
    0x0014CD30: ('em_area21_boot_0014CD30', 156, UU, None),
    0x0014CDD0: ('em_area21_boot_0014CDD0', 1032, UU, None),
    0x0014D1E0: ('em_area21_boot_0014D1E0', 128, [U, U, I], None),
    0x00137830: ('em_area21_boot_00137830', 240, [U], None),
    0x00137920: ('em_area21_boot_00137920', 292, UU, None),
    0x00137A50: ('em_area21_boot_00137A50', 556, UU, None),
    0x00137C80: ('em_area21_boot_00137C80', 512, UU, None),
    0x00137E80: ('em_area21_boot_00137E80', 48, [U], None),
    0x00137EB0: ('em_area21_boot_00137EB0', 796, [U], None),
    0x001381D0: ('em_area21_boot_001381D0', 492, UU, I),
}
BEAT = 'a03b_03_item2a'
FREE = 0x1E61000
A = FREE + 0x1000            # the designed node (ctl = A + 0x1F0)
CTL = A + 0x1F0
OTHER = FREE + 0x2000        # 00137EB0's creature (+0x20), 00137C80's +0x20 node
SPAWN = FREE + 0x3000        # 001AFA90's nodes
SPAWN2 = FREE + 0x4000
VEC1 = FREE + 0x5000         # 001283D0 / 0014D1E0 vectors
VEC2 = FREE + 0x5010
TABLE = FREE + 0x6000        # the bone table behind 0x275B40
BONES = FREE + 0x7000        # bone i at BONES + i * 0x100 (matrix at +0x90)
OBONES = FREE + 0x9000       # OTHER's bones (pointers at OTHER + 0x110)
BONEREC = FREE + 0xB000      # 001AF780's records
EFFECT = FREE + 0xC000       # 001EFE00's node
TOP = FREE + 0xD000          # the end of the designed area
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
PLAYER = 0x810360
COUNT = 0x700031F4           # 001283D0's live count
HARD = 0x81070A
SELF_ENTRIES = {0x14C220, 0x14C3A0, 0x137830, 0x137A50, 0x137C80, 0x137EB0}
# the pure helpers run as original code (their writes replayed): vector
# and matrix copy / product / sum / identity / turn, sin / cos / fabs /
# atan2, the bearing, the turn toward, the angle wrap and the distance
MATH = frozenset((0x1026A0, 0x1028B8, 0x102948, 0x102958, 0x1029C0, 0x102B08, 0x11DE90, 0x11DF78, 0x11E2A8,
                  0x11E620, 0x1B1240, 0x1B12B0, 0x1B1470, 0x1B15D0))
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
    if entry == 0x1283D0:
        return [VEC1, 1, VEC2]
    if entry == 0x14D1E0:
        return [VEC1, VEC2, 2]
    return [A, CTL][:len(FUNCS[entry][2])]


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'}
    O.OUT = ROOT / 'build/area21_boot'
    O.SOURCES = ('em_area21_boot.c', 'em_area21_boot_small.c')
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
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[BEAT][0]
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    O.JUMP_TABLES = {}
    O.TABLE_BYTES = frozenset()
    ram = O.CAPTURES[BEAT][0]
    assert not any(ram[FREE:TOP]), 'the designed area is not zero in the capture'


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x0F0F0F0F, 0x7FFF0000, 0x00012000, 0x80000000, 0x5A5A5A5A,
         0x000F8000, 0x00040000, 0x00080000, 0x00004000, 0x00000020, 0x00000800, 0x0000C000)


def matrix(tx=0., ty=0., tz=0., s=1.):
    return fw(s, 0., 0., 0., 0., s, 0., 0., 0., 0., s, 0., tx, ty, tz, 1.)


def common():
    """The nodes' methods, the bone tables, the vectors and the player."""
    p = [(A + 0x4C, w(CALLBACK)), (A + 0xB0, fw(330., 5., 280., 1.)), (A + 0xC4, f(.3)), (A + 0x20, w(OTHER)),
         (0x275B40, w(TABLE)), (VEC1, fw(1., 2., 3., 1.)), (VEC2, fw(-4., 5., -6., 1.)),
         (PLAYER, fw(350., 0., 300., 1.)), (SPAWN + 0x4C, w(CALLBACK)), (OTHER + 0xD, b(1))]
    for i in range(0x10):
        p += [(TABLE + 4 * i, w(BONES + 0x100 * i)), (BONES + 0x100 * i + 0x90, matrix(i, 2. * i, -i)),
              (OTHER + 0x110 + 4 * i, w(OBONES + 0x100 * i)),
              (OBONES + 0x100 * i + 0x90, matrix(3. * i, 1., 2., .5))]
    return p


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 2, 3)))),
        (A + 6, b(c((0, 1, 2, 3)))), (A + 0xD, b(c((0, 1, 2)))), (A + 0x9A, b(c((0, 5, 0x80)))),
        (A + 0x34, h(c((0, 1, 5, 10, 100, 0x7FFF)))),
        (A + 0x36, h(c((0, 1, 0x10, 0x4010, 0x8010, 0x1005, 0x2001, 0x5001, 0xFFF, 0xC010)))),
        (A + 0xB4, f(c((0., 5., 5.1, 40., -3.)))), (A + 0x24, w(c((0, 1)))), (A + 0x20, w(c((OTHER, SPAWN)))),
        (CTL, w(c((0, 0x1000, 9, 0x1009)))), (CTL + 4, w(c((0, 1, 2)))), (CTL + 0x14, w(c((0, 1)))),
        (CTL + 0x18, w(c((0, 1, 0xF0)))), (CTL + 0x24, f(c((0., 1., -1., -.97, -.5, .001, .002)))),
        (CTL + 0x28, f(c((0., -.01, .01, 1.)))), (CTL + 0x2C, f(c((0., .5, -.2)))), (CTL + 0x30, h(c((0, 0x9F, 0xA0)))),
        (CTL + 0x32, h(c((0, 1, 0x100)))), (CTL + 0x38, f(c((0., 2., 40.)))), (CTL + 0x40, b(c((0, 1, 0xFF)))),
        (CTL + 0x41, b(c((0, 1, 0x80)))), (CTL + 0x42, b(c((0, 1)))), (CTL + 0x43, b(c((0, 1, 0x7F)))),
        (CTL + 0x44, b(c((0, 1, 0xF0)))), (HARD, b(c((0, 1)))), (0x81080F, b(c((0, 1, 2)))),
        (0x28A9A2, b(c((0, 1)))), (0x28A9A0, h(c((0, 2)))), (OTHER + 4, b(c((0, 3)))), (OTHER + 0xD, b(c((0, 1)))),
        (COUNT, w(c((0, 0x14, 0x15)))), (PLAYER, fw(c((350., 330., 300.)), c((0., 5., 20., -10.)), 300., 1.)),
    ]


def hook_values(rng):
    def ints(*values, n=8): return [rng.choice(values) for _ in range(n)]
    out = {0x122BB8: ints(*RANDS, n=24)}
    out.update({a: ints(0, 1, 2, -1) for a in (0x1B10B0, 0x19AD00, 0x19AB20, 0x21BE40, 0x1A7B80, 0x1B1560,
                                                0x1C64F0, 0x1B17A0, 0x1FBD50)})
    out.update({0x1EFE00: ints(0, EFFECT), 0x1AFA90: ints(0, SPAWN, SPAWN2), 0x1C6150: ints(0, 2, 3),
                0x1AF780: ints(BONEREC, BONEREC + 0x100)})
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

    alloc = {0x1AFA90: [SPAWN, SPAWN2]}
    # ---- 001283D0 / 0014D1E0: the spawners
    for count in (0, 0x14, 0x15, -1):
        for kind in (1, 0, 2):
            for node in (SPAWN, 0):
                add(f'83d0/{count:x}/{kind}/{node:x}', 0x1283D0, [(COUNT, w(count))], {0x1AFA90: [node]},
                    args=[VEC1, kind, VEC2])
    for node in (SPAWN, 0):
        for kind in (0, 2, 0x81):
            add(f'd1e0/{node:x}/{kind}', 0x14D1E0, [], {0x1AFA90: [node]}, args=[VEC1, VEC2, kind])
    # ---- 0014C220: the states and the countdown
    for state in range(5):
        for t in (0, 1, 0x80):
            add(f'c220/{state}/{t:x}', 0x14C220, st(state, 3) + [(CTL + 0x41, b(t)), (CTL + 0x18, w(1)),
                                                                 (A + 0xB4, f(40.))])
    # ---- 0014C2E0
    for hard in (0, 1):
        for r in (0, 1):
            add(f'c2e0/{hard}/{r}', 0x14C2E0, [(HARD, b(hard)), (A + 4, b(0xFF)), (A + 0xD, b(2))], {0x1B10B0: [r]})
    # ---- 0014C3A0: every move
    for move in range(4):
        add(f'c3a0/{move}', 0x14C3A0, st(1, move) + [(CTL + 0x18, w(1)), (A + 0xB4, f(40.)),
                                                    (CTL + 0x24, f(.5)), (CTL + 0x2C, f(.25))])
    # ---- 0014C490: the rise
    for r in (0, 0x200, 0xE00, -1):
        add(f'c490/0/{r:x}', 0x14C490, st(1, 2, 0), {0x122BB8: [r]})
    for y, base_ in ((10., 6.), (9.5, 6.), (9.4, 6.), (0., 0.)):
        add(f'c490/1/{y}/{base_}', 0x14C490, st(1, 2, 1) + [(A + 0xB4, f(y)), (CTL + 0x38, f(base_))])
    for rate in (.002, .0021, .001, -1.):
        add(f'c490/2/{rate}', 0x14C490, st(1, 2, 2) + [(CTL + 0x24, f(rate))])
    add('c490/3', 0x14C490, st(1, 2, 3))
    # ---- 0014C680: the hop
    for py in (20., 15.1, 15., 0.):
        add(f'c680/0/{py}', 0x14C680, st(1, 0, 0) + [(PLAYER, fw(350., py, 300., 1.)), (CTL + 0x14, w(3))])
    for ang in (0, 0x80000000, 0x3F000000):   # the climb angle's sign (scripted atan2)
        add(f'c680/0/ang/{ang:x}', 0x14C680, st(1, 0, 0) + [(PLAYER, fw(350., 20., 300., 1.)), (CTL + 0x14, w(3))],
            {0x11E620: [ang]}, run=MATH - {0x11E620})
    for count in (0, 2):
        for rate in (0., -.01, .01):
            for py in (12., 6.5, 6.4, -7.):
                for x in (350., 330.1, 600.):
                    for view in (0, 1):
                        add(f'c680/1/{count}/{rate}/{py}/{x}/{view}', 0x14C680,
                            st(1, 0, 1) + [(CTL + 0x14, w(count)), (CTL + 0x28, f(rate)),
                                           (PLAYER, fw(x, 5. + py, 280., 1.))],
                            {0x1B1560: [view], 0x122BB8: [0x12345678, 0x7FFFF800]})
    add('c680/2', 0x14C680, st(1, 0, 2))
    # ---- 0014C9D0: the leap
    for r1, r2 in ((0, 0), (-1, -1), (0x12345678, 0x0FF000)):
        add(f'c9d0/0/{r1:x}/{r2:x}', 0x14C9D0, st(1, 1, 0) + [(CTL + 0x14, w(2))], {0x122BB8: [r1, r2]})
    for flag in (0, 1, 2):
        for height in (0., -.5, -.96, -.97, 1.):
            for rate in (0., -.01):
                for count in (0, 1):
                    add(f'c9d0/1/{flag}/{height}/{rate}/{count}', 0x14C9D0,
                        st(1, 1, 1) + [(0x81080F, b(flag)), (CTL + 0x24, f(height)), (CTL + 0x28, f(rate)),
                                       (CTL + 0x14, w(count))], {0x122BB8: [0x00038000]})
    add('c9d0/2', 0x14C9D0, st(1, 1, 2))
    # ---- 0014CCC0 / 0014CD00 / 0014CD30
    add('ccc0', 0x14CCC0, st(2))
    for k in (0, 7, 0xFF):
        add(f'cd00/{k}', 0x14CD00, [(A + 0x9A, b(k))])
    for held in (0, 1, 0x80):
        add(f'cd30/{held:x}', 0x14CD30, [(CTL + 0x40, b(held)), (CTL + 0x24, f(1.5)), (CTL + 0x28, f(-.25)),
                                         (CTL + 0x2C, f(.75)), (CTL + 0x30, f(.125)), (A + 0xC4, f(1.))])
    # ---- 0014CDD0: the attack
    add('cdd0/budget0', 0x14CDD0, [(CTL + 0x18, w(0))])
    for y in (5., 4.9, 5.1):
        add(f'cdd0/low/{y}', 0x14CDD0, [(CTL + 0x18, w(2)), (A + 0xB4, f(y))])
    for t in (0, 1, 0x80):
        for ground in (0, 1):
            for fall in (-1., 0., 1.):
                for seg in (0, 1):
                    add(f'cdd0/probe/{t:x}/{ground}/{fall}/{seg}', 0x14CDD0,
                        [(CTL + 0x18, w(2)), (A + 0xB4, f(40.)), (CTL + 0x41, b(t)), (CTL + 0x24, f(fall)),
                         (A + 0xD, b(seg + 1))], {0x19AD00: [ground], 0x19AB20: [seg], 0x21BE40: [1]})
    for free in (0, 1):
        for bite in (0, 1):
            for hard in (0, 1):
                add(f'cdd0/bite/{free}/{bite}/{hard}', 0x14CDD0,
                    [(CTL + 0x18, w(2)), (A + 0xB4, f(40.)), (CTL + 0x41, b(1)), (HARD, b(hard)),
                     (0x8102B0, b(0x41)), (A + 0x36, h(3)), (A + 0x34, h(10))],
                    {0x21BE40: [free], 0x1A7B80: [bite]})
    for hit, hp in ((0, 10), (3, 10), (10, 10), (11, 10), (0x800A, 10), (0xF009, 10), (0x7, -1), (0x8001, 0x7FFF)):
        add(f'cdd0/hit/{hit:x}/{hp}', 0x14CDD0, [(CTL + 0x18, w(2)), (A + 0xB4, f(40.)), (CTL + 0x41, b(1)),
                                                 (A + 0x36, h(hit)), (A + 0x34, h(hp))], {0x21BE40: [1]})
    # ---- 00137830: the states and the countdowns
    for state in range(5):
        for t in (0, 1, 0x80):
            add(f'7830/{state}/{t:x}', 0x137830,
                st(state, 3) + [(CTL + 0x40, b(t)), (CTL + 0x41, b(t ^ 1)), (CTL + 0x42, b(t)), (CTL + 0x44, b(t ^ 1))])
    # ---- 00137920: the set-up
    for hard in (0, 1):
        for r in (0, 1):
            for nodes in ((SPAWN, SPAWN2), (SPAWN, 0), (0,)):
                add(f'7920/{hard}/{r}/{len(nodes)}/{nodes[-1]:x}', 0x137920, [(HARD, b(hard)), (A + 4, b(7))],
                    {0x1B10B0: [r], 0x1AFA90: list(nodes)})
    # ---- 00137A50: alive
    for move in (0, 1, 3):
        for clip in (0, 0x1000):
            for guard in (0, 1):
                add(f'7a50/{move}/{clip:x}/{guard}', 0x137A50, st(1, move) + [(CTL, w(clip)), (CTL + 0x40, b(guard)),
                                                                             (CTL + 0x32, h(1))])
    for side in (0, 1, 2):
        for count in (0, 1):
            for spawn in (0, 1):
                for repeat in (0, 1, 0x7F):
                    add(f'7a50/2/{side}/{count}/{spawn}/{repeat:x}', 0x137A50,
                        st(1, 2) + [(A + 0xD, b(side)), (CTL + 4, w(count)), (CTL + 0x32, h(spawn)),
                                    (CTL + 0x43, b(repeat))], {**alloc, 0x122BB8: [-1, 0x12345678]})
    for hit in (0x10, 0x8010):
        add(f'7a50/hit/{hit:x}', 0x137A50, st(1, 3) + [(A + 0x36, h(hit)), (A + 0x34, h(1000))])
    # ---- 00137C80: hurt / dying
    for hp in (0, 5):
        add(f'7c80/0/{hp}', 0x137C80, st(2, 0) + [(A + 0x34, h(hp))], {0x1EFE00: [EFFECT]})
    for clip in (0, 0x1000):
        for hp in (0, 5):
            for hit in (0, 0x8000, 0x7FFF):
                add(f'7c80/1/{clip:x}/{hp}/{hit:x}', 0x137C80,
                    st(2, 1) + [(CTL, w(clip)), (A + 0x34, h(hp)), (A + 0x36, h(hit))], {0x122BB8: [-1]})
    for flag in (0, 1):
        for mode in (0, 2, 0x102):
            for e in (0, 1):
                add(f'7c80/2/{flag}/{mode:x}/{e}', 0x137C80,
                    st(2, 2) + [(0x28A9A2, b(flag)), (0x28A9A0, h(mode)), (A + 0x24, w(e)), (A + 0x20, w(SPAWN))])
    add('7c80/3', 0x137C80, st(2, 3))
    # ---- 00137E80
    add('7e80', 0x137E80, [(A + 0x9A, b(9))])
    # ---- 00137EB0: the rig follower
    for side in (0, 1):
        for count, free in ((2, 3), (3, 3), (3, 2), (0, 0), (1, -1)):
            add(f'7eb0/0/{side}/{count}/{free}', 0x137EB0, [(A + 4, b(0)), (A + 0xD, b(side)), (A + 0x44, w(0x1234)),
                                                            (0x275BCC, h(free))],
                {0x1C6150: [count], 0x1AF780: [BONEREC, BONEREC + 0x100, BONEREC + 0x200]})
    for side in (0, 1):
        for oside in (0, 1):
            for count in (0, 1, 2):
                for hard in (0, 1):
                    for ostate in (1, 3):
                        add(f'7eb0/1/{side}/{oside}/{count}/{hard}/{ostate}', 0x137EB0,
                            [(A + 4, b(1)), (A + 0xD, b(side)), (OTHER + 0xD, b(oside)), (A + 0x1F4, w(count)),
                             (HARD, b(hard)), (OTHER + 4, b(ostate))])
    for state in (2, 3, 4):
        add(f'7eb0/{state}', 0x137EB0, [(A + 4, b(state))])
    # ---- 001381D0: the damage step
    add('81d0/none', 0x1381D0, [(A + 0x36, h(0))])
    add('81d0/guard', 0x1381D0, [(A + 0x36, h(5)), (CTL + 0x40, b(3))])
    for hit in (0x10, 0x8010, 0x1010, 0x4010, 0x5010, 0x2010, 0xC010, 0x9010, 0x0FFF):
        for flash in (0, 1):
            for run_ in (0, 1):
                for fx in (0, 1):
                    add(f'81d0/{hit:x}/{flash}/{run_}/{fx}', 0x1381D0,
                        [(A + 0x36, h(hit)), (CTL + 0x44, b(flash)), (CTL + 0x42, b(run_)), (CTL + 0x41, b(fx)),
                         (A + 0x34, h(1000)), (CTL + 0x30, h(0x20))])
    for hp, total in ((16, 0), (17, 0), (1000, 0x90), (1000, 0x8F), (15, 0), (1000, -5)):
        add(f'81d0/hp/{hp}/{total}', 0x1381D0, [(A + 0x36, h(0x10)), (A + 0x34, h(hp)), (CTL + 0x42, b(1)),
                                                (CTL + 0x30, h(total))])
    # ---- a seeded random sample over every entry
    rng = random.Random(0x21B007)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(40, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_boot.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_boot_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21b_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(.*\);$' % symbol, header, re.M), symbol


def entry_guards():
    """A latched fault, a NULL hook table, a NULL fault pointer and (for the
    entries with a result) a NULL result: every entry refuses with no hook
    or memory call."""
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
    only = os.environ.get('EM_AREA21_BOOT_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_BOOT_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA21_BOOT_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA21_BOOT_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_BOOT_TRACE'):
        trace(os.environ['EM_AREA21_BOOT_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_BOOT_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
