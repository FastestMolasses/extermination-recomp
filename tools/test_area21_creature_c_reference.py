#!/usr/bin/env python3
"""The AREA21 creature (src/game/em_area21_creature_c*.c) against the ORIGINAL
instructions: the behaviour 00151590 of AREA21 code group 0x82A5A0 [0], its
states 001516B0 / 00151770 / 00152CC0 / 00152E80, the moves 00151940,
00151E50, 00152040, 00152330, 00152660, 00152930, 00152B60, the per-frame
pieces 00152EB0, 00152F20, 00153180, 00153770, the spawn 00153290 and the
spawned pieces' behaviour 00153540. Unwired.

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
designed node, a spawned piece, the bone table behind 0x275B40 and an
effect node live in zero RAM at FREE. Compared per case: every call and its
arguments, memory at every call entry, the memory accesses between calls
one for one in order, memory after the last store, the result, a poisoned
re-run, coverage of every reachable original word (DEAD excepted, each with
its reason), the callee inventory and the fail-stop contract. The move
dispatcher's table loads (00151770) are left out of the access comparison
(TABLE_BYTES): the translation is a switch.

001283D0 and 0014D1E0 are hooks here (their translations live in
em_area21_boot).

EM_TEST_FULL=1 widens the random sample. EM_AREA21_CREATURE_C_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA21_CREATURE_C_SOURCE=
<dir> tests another copy of the module sources; EM_AREA21_CREATURE_C_SHOW=<n>
prints n failures; EM_AREA21_CREATURE_C_MISSING=1 lists unexecuted words;
EM_AREA21_CREATURE_C_TRACE=<label> prints the original's calls and accesses
of matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area21_creature_c as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
UU = [U, U]
FUNCS = {
    0x00151590: ('em_area21_creature_c_00151590', 0x11C, [U, S], None),
    0x001516B0: ('em_area21_creature_c_001516B0', 0xBC, UU, None),
    0x00151770: ('em_area21_creature_c_00151770', 0x1CC, [U, U, S], None),
    0x00151940: ('em_area21_creature_c_00151940', 0x504, UU, None),
    0x00151E50: ('em_area21_creature_c_00151E50', 0x1F0, UU, None),
    0x00152040: ('em_area21_creature_c_00152040', 0x2E8, UU, None),
    0x00152330: ('em_area21_creature_c_00152330', 0x324, UU, None),
    0x00152660: ('em_area21_creature_c_00152660', 0x2CC, UU, None),
    0x00152930: ('em_area21_creature_c_00152930', 0x224, UU, None),
    0x00152B60: ('em_area21_creature_c_00152B60', 0x15C, UU, None),
    0x00152CC0: ('em_area21_creature_c_00152CC0', 0x1BC, UU, None),
    0x00152E80: ('em_area21_creature_c_00152E80', 0x30, [U], None),
    0x00152EB0: ('em_area21_creature_c_00152EB0', 0x70, [U], None),
    0x00152F20: ('em_area21_creature_c_00152F20', 0x260, [U, U, S], None),
    0x00153180: ('em_area21_creature_c_00153180', 0x10C, UU, None),
    0x00153290: ('em_area21_creature_c_00153290', 0x2B0, [U], None),
    0x00153540: ('em_area21_creature_c_00153540', 0x22C, [U], None),
    0x00153770: ('em_area21_creature_c_00153770', 0x1DC, UU, I),
}
BEAT = 'a03b_03_item2a'
FREE = 0x1E61000
A = FREE + 0x1000            # the designed creature node (ctl = A + 0x1F0)
CTL = A + 0x1F0
OBJ = FREE + 0x2000          # a spawned piece (00153540) / 001AFA90's node
TABLE = FREE + 0x3000        # the bone table behind 0x275B40
BONES = FREE + 0x4000        # bone i at BONES + i * 0x100 (matrix at +0x90)
EFFECT = FREE + 0x9000       # an effect node (the node's +0x20)
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
PLAYER = 0x810360
SELF_ENTRIES = {0x151590, 0x151770, 0x152CC0}
# the pure helpers run as original code (their writes replayed): vector and
# matrix math, sin / cos / fabs / modff, the angle wrap, the bearing, the
# turn, the distance and float_to_int
MATH = frozenset((0x1026A0, 0x1028B8, 0x102948, 0x102958, 0x1029C0, 0x102B08, 0x102BB0, 0x1031E0, 0x103230,
                  0x11DE90, 0x11DF78, 0x11E0A8, 0x11E2A8, 0x1281C0, 0x1B1240, 0x1B12B0, 0x1B1470, 0x1B15D0))
JUMPS = {0x151770: (0x26D2F0, 10)}
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


class Dirty(dict):
    """Replay.dirty that also keeps, per 64 KiB page of each memory, the
    lowest and highest byte any entry covers (FastReplay.compare)."""

    def __init__(self):
        super().__init__()
        self.pages = {}

    def __setitem__(self, key, size):
        dict.__setitem__(self, key, size)
        page = key >> 16
        lo, hi = self.pages.get(page, (key, key + size))
        self.pages[page] = (min(lo, key), max(hi, key + size))


BaseReplay = O.Replay


class FastReplay(BaseReplay):
    """The harness's Replay with the span comparison of
    test_area17_creature_reference.FastReplay at call entries (exact: the
    bytes between dirty bytes hold the start image on both sides; on a
    difference the harness's own comparison runs and reports it). The
    final comparison is always all of memory."""

    def __init__(self, *args, **kwargs):
        BaseReplay.__init__(self, *args, **kwargs)
        self.dirty = Dirty()

    def compare(self, upto, where, full=None):
        if full:
            return BaseReplay.compare(self, upto, where, full)
        for key, data in self.stores[self.applied:upto]:
            if O.in_window(key):
                C.memmove(self.expect_stack_base + key - O.STACK_LO, data, len(data))
            elif key >= 0x70000000:
                C.memmove(self.expect_spad_base + key - 0x70000000, data, len(data))
            else:
                C.memmove(self.expect_ram_base + key, data, len(data))
            if len(data) > self.dirty.get(key, 0):
                self.dirty[key] = len(data)
        self.applied = upto
        for lo, hi in self.dirty.pages.values():
            if O.STACK_LO <= lo < O.STACK_TOP:
                lo, hi = max(lo, O.STACK_LO), min(hi, O.STACK_TOP)
                a, b_ = self.expect_stack_base + lo - O.STACK_LO, self.stack_base + lo - O.STACK_LO
            elif lo >= 0x70000000:
                a, b_ = self.expect_spad_base + lo - 0x70000000, self.spad_base + lo - 0x70000000
            else:
                a, b_ = self.expect_ram_base + lo, self.ram_base + lo
            if O.LIBC.memcmp(a, b_, hi - lo):
                return BaseReplay.compare(self, upto, where, full)
        return True


def jump_tables(ram):
    """The register jump of each dispatcher (its single non-return jr)."""
    out = {}
    for entry, (table, count) in JUMPS.items():
        size = FUNCS[entry][1]
        sites = [pc for pc in range(entry, entry + size, 4)
                 if (O.u32(ram, pc) & 0xFC1FFFFF) == 8 and (O.u32(ram, pc) >> 21 & 31) != 31]
        assert len(sites) == 1, hex(entry)
        out[sites[0]] = (table, count)
    return out


def default_args(entry):
    kinds = FUNCS[entry][2]
    if entry == 0x153540:
        return [OBJ]
    out = []
    for k in kinds:
        out.append(O.STACK_TOP if k == S else (A if not out else CTL))
    return out


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'}
    O.OUT = ROOT / 'build/area21_creature_c'
    O.SOURCES = ('em_area21_creature_c.c', 'em_area21_creature_c_moves.c', 'em_area21_creature_c_parts.c')
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
    O.HEAVY = {0x151590, 0x151770, 0x153290}
    O.RETURN_SITES = O.RETURN_SITES_FULL = ()
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[BEAT][0]
    O.Replay = FastReplay
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    O.JUMP_TABLES = jump_tables(O.CAPTURES[BEAT][0])
    O.TABLE_BYTES = frozenset(a for table, n in O.JUMP_TABLES.values() for a in range(table, table + 4 * n))
    ram = O.CAPTURES[BEAT][0]
    assert not any(ram[FREE:EFFECT + 0x1000]), 'the designed area is not zero in the capture'


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x0F0F0F0F, 0x7FFF0000, 0x00012000, 0x80000000, 0x5A5A5A5A,
         0x000F8000, 0x00040000, 0x00080000, 0x00004000, 0x00002000, 0x00000800, 0x0000C000)
FRONT, BACK = 0xBFC6D3F2, 0x3FC6D3F2   # the front arc's limits (bits)


def matrix(tx=0., ty=0., tz=0., s=1.):
    return fw(s, 0., 0., 0., 0., s, 0., 0., 0., 0., s, 0., tx, ty, tz, 1.)


def common():
    """The node's method, its pose, the bone table, the player and the
    piece's method."""
    p = [(A + 0x4C, w(CALLBACK)), (A + 0xB0, fw(100., 35., 95., 1.)), (A + 0xC4, f(.3)),
         (0x275B40, w(TABLE)), (PLAYER, fw(110., 30., 140., 1.)), (OBJ + 0x4C, w(CALLBACK))]
    for i in range(0x30):
        p += [(TABLE + 4 * i, w(BONES + 0x100 * i)), (BONES + 0x100 * i + 0x90, matrix(i, 2. * i, -i))]
    return p


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 3, 4, 5, 6, 9, 99)))),
        (A + 6, b(c(range(6)))), (A + 0x20, w(c((0, EFFECT)))), (A + 0x9A, b(c((0, 3, 0x81)))),
        (A + 0x2C, h(c((0, 1, 2, 3, 4, 5, 0x8000, 0x8002)))),
        (A + 0x34, h(c((0, 1, 5, 0x100, 0x7D0, 0x8000)))),
        (A + 0x36, h(c((0, 1, 0x10, 0x2010, 0x4010, 0x8010, 0x1005, 0x5001, 0x6100, 0xFFF, 0xE010)))),
        (A + 0x3C, f(c((0., 5., 9., 9.5, 30., 50., 51., 52., 53., 80., 81., 150., 151.)))),
        (A + 0xB0, fw(c((100., 10., 200., 20., 180.)), 35., c((95., 140., 160.)), 1.)),
        (A + 0xC0, fw(c((1., -1., 0.)), .5, c((.5, -.5)), 0.)),
        (A + 0xC4, f(c((0., .3, 3., -3., 1.5, -1.5)))),
        (CTL, w(c((0, 0x1000, 9, 8, 0x1009, 0x1008)))), (CTL + 4, w(c((0, 1, 3)))), (CTL + 8, w(c((0, 3, 4)))),
        (CTL + 0xC, w(c((0, 1)))), (CTL + 0x20, f(c((.01, .1)))), (CTL + 0x28, f(c((1., 2., .5, 3.5)))),
        (CTL + 0x2C, f(c((0., .3, 3.1415927, -1.)))), (CTL + 0x30, f(c((0., .5)))), (CTL + 0x34, f(c((0., -.1)))),
        (CTL + 0x38, f(c((0., 1.)))), (CTL + 0x3C, f(c((0., .2)))),
        (CTL + 0x40, h(c((0, 5, 0x100, 0x7FFF)))), (CTL + 0x42, h(c((0, 1)))), (CTL + 0x44, h(c((0, 1)))),
        (CTL + 0x50, b(c((0, 1, 0xFF)))), (CTL + 0x51, b(c((0, 1)))), (CTL + 0x52, b(c((0, 1)))),
        (CTL + 0x53, b(c((0, 1, 2)))), (CTL + 0x54, b(c((0, 1)))), (CTL + 0x55, b(c((0, 4, 8, 0xC)))),
        (CTL + 0x56, b(c((0, 1)))), (CTL + 0x57, b(c((0, 1)))), (CTL + 0x58, b(c((0, 1)))),
        (0x81070A, b(c((0, 1)))), (0x81080E, b(c((0, 2, 3)))), (0x700031F4, w(c((0, 0x13, 0x14, 0x15)))),
        (PLAYER, fw(c((110., 100., 300.)), c((74., 75., 76.)), c((140., 95., 300.)), 1.)),
        (OBJ + 4, b(c((0, 1, 2, 3, 4)))), (OBJ + 0x20, w(EFFECT)),
        (OBJ + 0xB0, fw(c((100., 10., 200.)), 35., 95., 1.)), (OBJ + 0xC0, fw(c((1., -1.)), .5, .5, 0.)),
    ]


def hook_values(rng):
    def ints(*values, n=8): return [rng.choice(values) for _ in range(n)]
    out = {0x122BB8: ints(*RANDS, n=24)}
    out.update({a: ints(0, 1, 2, -1) for a in (0x1B10B0, 0x1B1380, 0x1B1560, 0x21BE40, 0x1A7B80, 0x19A570,
                                                0x1C64F0, 0x1B17A0, 0x1FBD50, 0x1283D0)})
    out.update({0x1EFE00: ints(0, EFFECT), 0x1AFA90: ints(OBJ, OBJ, OBJ, 0, n=12), 0x1C6160: ints(10, 30, 1)})
    return out


def cases():
    out, labels = [], set()
    base = common()
    ram = O.CAPTURES[BEAT][0]
    s16 = lambda a: struct.unpack_from('<h', ram, a)[0]   # noqa: E731
    f32 = lambda a: struct.unpack_from('<f', ram, a)[0]   # noqa: E731

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
    gate = {0x21BE40: [0], 0x1A7B80: [1]}
    # ---- 00151590: the four states, the countdowns
    for state in (0, 1, 2, 3, 4):
        add(f'1590/{state}', 0x151590, st(state, 6) + [(CTL + 0x51, b(3)), (CTL + 0x52, b(1)), (CTL + 0x42, h(2)),
                                                         (CTL + 0x50, b(0x80)), (CTL + 0x44, h(0x8000))])
    add('1590/zero', 0x151590, st(4))
    # ---- 001516B0
    for flag in (0, 1):
        for r in (0, 1, -1):
            add(f'16b0/{flag}/{r}', 0x1516B0, [(A + 4, b(0xFF)), (0x81070A, b(flag))], {0x1B10B0: [r], 0x1EFE00: [EFFECT]})
    # ---- 00151770: the far flag, every move, the clip events
    for move in range(10):
        add(f'1770/{move}', 0x151770, st(1, move))
    for tag in (9, 8, 0x1009):
        for r in (0, 0x2000, -1):
            add(f'1770/cry/{tag:x}/{r:x}', 0x151770, st(1, 6) + [(CTL, w(tag))], {0x122BB8: [r]})
    for y in (74., 75., 76.):
        add(f'1770/far/{y}', 0x151770, st(1, 7) + [(PLAYER, fw(110., y, 140., 1.))])
    add('1770/live', 0x151770, st(1, 7) + [(A + 0x36, h(0x2010)), (CTL + 0x53, b(1)), (A + 0x2C, h(0)),
                                           (A + 0x3C, f(5.)), (CTL + 0x28, f(2.)), (A + 0x34, h(100))],
        {0x1C6160: [30, 30], 0x1C64F0: [0x1000]})
    # ---- 00151940 (move 0)
    for effect in (0, EFFECT):
        add(f'1940/0/{effect:x}', 0x151940, st(1, 0, 0) + [(A + 0x20, w(effect))])
    add('1940/9', 0x151940, st(1, 0, 9))
    # the arcs: the limits themselves and inside / outside
    for heading in (FRONT, BACK, 0x3F000000, 0xBF000000, 0x40400000, 0xC0400000):
        for bits in (0, 4, 8):
            add(f'1940/arc/{heading:x}/{bits}', 0x151940, st(1, 0, 1) + [(A + 0xC4, w(heading)),
                                                                       (CTL + 0x55, b(bits)), (CTL + 0x2C, f(9.))])
    for heading in (0x3F000000, 0xBF000000):
        for y in (119., 120.):
            for r in (0, -1, 0x00380000):
                add(f'1940/front/{heading:x}/{y}/{r:x}', 0x151940,
                    st(1, 0, 1) + [(A + 0xC4, w(heading)), (CTL + 0x2C, w(heading)), (A + 0xB0, fw(100., 35., y, 1.))],
                    {0x122BB8: [0x10, r], 0x1B12B0: [heading]}, run=MATH - {0x1B12B0})
    for heading in (0x40400000, 0xC0400000):
        for r in (0, -1, 0x00380000):
            add(f'1940/back/{heading:x}/{r:x}', 0x151940,
                st(1, 0, 1) + [(A + 0xC4, w(heading)), (CTL + 0x2C, w(heading))], {0x122BB8: [0x10, r]})
    for flag in (0, 1):
        for r1, r2 in ((0, 1), (1, 0), (0, 0)):
            add(f'1940/alert/{flag}/{r1}/{r2}', 0x151940, st(1, 0, 1) + [(0x81070A, b(flag))],
                {0x21BE40: [r1], 0x1A7B80: [r2]})
    for burst in (0, 1):
        for near in (0, 1):
            for pause in (0, 1):
                for count in (0x13, 0x14):
                    for r in (0, 0x40000):
                        add(f'1940/move/{burst}/{near}/{pause}/{count:x}/{r:x}', 0x151940,
                            st(1, 0, 1) + [(CTL + 0x44, h(burst)), (CTL + 0x58, b(near)), (CTL + 0x42, h(pause)),
                                           (0x700031F4, w(count))], {0x122BB8: [0, r]})
    for lunges in (0, 1):
        for side, dist in ((0, 50.), (1, 50.), (1, 90.), (1, 91.)):
            add(f'1940/lunge/{lunges}/{side}/{dist}', 0x151940,
                st(1, 0, 1) + [(CTL + 0x42, h(1)), (CTL + 0x57, b(lunges))],
                {0x1B1560: [side], 0x1B15D0: [O.fb(dist)]}, run=MATH - {0x1B15D0})
    add('1940/flag', 0x151940, st(1, 0, 1) + [(CTL + 0x55, b(0xC))])
    # ---- 00151E50 (move 1)
    for y in (150., 150.5):
        for effect in (0, EFFECT):
            add(f'1e50/0/{y}/{effect:x}', 0x151E50, st(1, 1, 0) + [(A + 0xB0, fw(100., 35., y, 1.)),
                                                                 (A + 0x20, w(effect))])
    add('1e50/7', 0x151E50, st(1, 1, 7))
    for clip in (0, 0x8000):
        for frame in (80., 80.5):
            add(f'1e50/1/{clip:x}/{frame}', 0x151E50, st(1, 1, 1) + [(A + 0x2C, h(clip)), (A + 0x3C, f(frame))])
    for t in (0, 0x1000):
        for land in (0, 1):
            for near in (0, 1):
                for x in (110., 100., 199.9, 200.):
                    add(f'1e50/1/end/{t:x}/{land}/{near}/{x}', 0x151E50,
                        st(1, 1, 1) + [(CTL, w(t)), (CTL + 0x58, b(near)), (CTL + 0x2C, f(.5)),
                                       (PLAYER, fw(x, 30., 95., 1.))],
                        {0x1B12B0: [O.fb(.5) if land else O.fb(.4)]}, run=MATH - {0x1B12B0})
    # ---- 00152040 (move 2)
    for y in (149.5, 150.):
        for effect in (0, EFFECT):
            add(f'2040/0/{y}/{effect:x}', 0x152040, st(1, 2, 0) + [(A + 0xB0, fw(100., 35., y, 1.)),
                                                                 (A + 0x20, w(effect))])
    add('2040/7', 0x152040, st(1, 2, 7))
    for clip in (0, 0x8000):
        for done in (0, 1):
            for frame in (52., 52.5):
                add(f'2040/1/{clip:x}/{done}/{frame}', 0x152040,
                    st(1, 2, 1) + [(A + 0x2C, h(clip)), (CTL + 4, w(done)), (A + 0x3C, f(frame))])
    for t in (0, 0x1000):
        for land in (0, 1):
            add(f'2040/1/end/{t:x}/{land}', 0x152040, st(1, 2, 1) + [(CTL, w(t)), (CTL + 0x2C, f(.5))],
                {0x1B12B0: [O.fb(.5) if land else O.fb(.4)]}, run=MATH - {0x1B12B0})
    for t in (0, 0x1000):
        for flag in (0, 1):
            for r1, r2 in ((0, 1), (1, 1), (0, 0)):
                add(f'2040/2/{t:x}/{flag}/{r1}/{r2}', 0x152040, st(1, 2, 2) + [(CTL, w(t)), (0x81070A, b(flag))],
                    {0x21BE40: [r1], 0x1A7B80: [r2]})
        add(f'2040/3/{t:x}', 0x152040, st(1, 2, 3) + [(CTL, w(t))])
    # ---- 00152330 (move 3)
    for side in (0, 1, 0x100):
        for effect in (0, EFFECT):
            add(f'2330/0/{side:x}/{effect:x}', 0x152330, st(1, 3, 0) + [(A + 0x20, w(effect))], {0x1B1380: [side]})
    add('2330/7', 0x152330, st(1, 3, 7))
    for t in (0, 0x1000):
        for side in (0, 1):
            add(f'2330/1/{t:x}/{side}', 0x152330, st(1, 3, 1) + [(CTL, w(t)), (CTL + 0x56, b(side))])
            for effect in (0, EFFECT):
                add(f'2330/2/{t:x}/{side}/{effect:x}', 0x152330,
                    st(1, 3, 2) + [(CTL, w(t)), (CTL + 0x56, b(side)), (A + 0x20, w(effect))], gate)
        add(f'2330/3/{t:x}', 0x152330, st(1, 3, 3) + [(CTL, w(t)), (CTL + 0x57, b(0x7F))])
    for frame in (9., 9.5):
        for flag in (0, 1):
            for r1, r2 in ((0, 1), (1, 1), (0, 0)):
                add(f'2330/2/alert/{frame}/{flag}/{r1}/{r2}', 0x152330,
                    st(1, 3, 2) + [(A + 0x3C, f(frame)), (0x81070A, b(flag))], {0x21BE40: [r1], 0x1A7B80: [r2]})
    # ---- 00152660 (move 4)
    for effect in (0, EFFECT):
        add(f'2660/0/{effect:x}', 0x152660, st(1, 4, 0) + [(A + 0x20, w(effect))])
    add('2660/7', 0x152660, st(1, 4, 7))
    for t in (0, 0x1000):
        add(f'2660/1/{t:x}', 0x152660, st(1, 4, 1) + [(CTL, w(t))])
        add(f'2660/3/{t:x}', 0x152660, st(1, 4, 3) + [(CTL, w(t))], {0x122BB8: [0x12345678]})
    for t in (0, 0x1000):
        for cry in (0, 1):
            for wait in (0, 3):
                for count in (3, 4):
                    add(f'2660/2/{t:x}/{cry}/{wait}/{count}', 0x152660,
                        st(1, 4, 2) + [(CTL, w(t)), (CTL + 0xC, w(cry)), (CTL + 4, w(wait)), (CTL + 8, w(count))],
                        {0x122BB8: [0x5A5A5A5A, 0x00FC0000, -1]})
    # ---- 00152930 (move 5)
    for effect in (0, EFFECT):
        add(f'2930/0/{effect:x}', 0x152930, st(1, 5, 0) + [(A + 0x20, w(effect))])
    add('2930/7', 0x152930, st(1, 5, 7))
    for t in (0, 0x1000):
        for step in (1, 2, 4):
            add(f'2930/{step}/{t:x}', 0x152930, st(1, 5, step) + [(CTL, w(t))], {0x122BB8: [0x01FF0000]})
        for done in (0, 1):
            for frame in (50., 50.5):
                add(f'2930/3/{t:x}/{done}/{frame}', 0x152930,
                    st(1, 5, 3) + [(CTL, w(t)), (CTL + 4, w(done)), (A + 0x3C, f(frame))], {0x1AFA90: [OBJ] * 6})
    # ---- 00152B60 (move 9)
    for step in (0, 1, 3):
        for t in (0, 0x1000):
            add(f'2b60/{step}/{t:x}', 0x152B60, st(1, 9, step) + [(CTL, w(t))])
    for t in (0, 0x1000):
        for done in (0, 1):
            for frame in (150., 150.5):
                for effect in (0, EFFECT):
                    add(f'2b60/2/{t:x}/{done}/{frame}/{effect:x}', 0x152B60,
                        st(1, 9, 2) + [(CTL, w(t)), (CTL + 4, w(done)), (A + 0x3C, f(frame)), (A + 0x20, w(effect))])
    # ---- 00152CC0 (state 2)
    for hp in (0, 1):
        for kind in (0, 0x8000):
            for effect in (0, EFFECT):
                add(f'2cc0/0/{hp}/{kind:x}/{effect:x}', 0x152CC0,
                    st(2, 0) + [(A + 0x34, h(hp)), (A + 0x36, h(kind)), (A + 0x20, w(effect))])
    for t in (0, 0x1000):
        for hp in (0, 1):
            for effect in (0, EFFECT):
                add(f'2cc0/1/{t:x}/{hp}/{effect:x}', 0x152CC0,
                    st(2, 1) + [(CTL, w(t)), (A + 0x34, h(hp)), (A + 0x20, w(effect))], {0x122BB8: [-1]})
    for arena in (2, 3):
        add(f'2cc0/2/{arena}', 0x152CC0, st(2, 2) + [(0x81080E, b(arena))])
    add('2cc0/3', 0x152CC0, st(2, 3))
    # ---- 00152E80 / 00152EB0
    add('2e80', 0x152E80, [(A + 0x9A, b(0x81))])
    for move in (0, 9, 99, 98):
        add(f'2eb0/{move}', 0x152EB0, [(A + 5, b(move))])
    # ---- 00152F20: the two motion modes
    for mode in (0, 2, 0xFF):
        add(f'2f20/{mode}', 0x152F20, [(CTL + 0x53, b(mode)), (CTL + 0x30, f(.5)), (CTL + 0x34, f(-.1)),
                                       (CTL + 0x38, f(1.)), (CTL + 0x3C, f(.2))])
    for kind in (0, 1, 2, 3, 4, 5, 0x8000):
        add(f'2f20/kind/{kind:x}', 0x152F20, [(CTL + 0x53, b(1)), (A + 0x2C, h(kind)), (A + 0x3C, f(5.)),
                                               (CTL + 0x28, f(2.))], {0x1C6160: [30, 30]})
    for n, frame in ((30, 29.5), (30, 5.), (10, 9.), (10, 10.5), (1, 0.)):
        for rate in (.5, 1., 2., 3.5):
            add(f'2f20/walk/{n}/{frame}/{rate}', 0x152F20,
                [(CTL + 0x53, b(1)), (A + 0x2C, h(2)), (A + 0x3C, f(frame)), (CTL + 0x28, f(rate))],
                {0x1C6160: [n, n, n]})
    # ---- 00153180: the bounds
    xs = (f32(0x246720), f32(0x246724))
    zs = (f32(0x246728), f32(0x24672C))
    for x in (xs[0] + 1., xs[1] - 1., xs[0], xs[1], (xs[0] + xs[1]) / 2):
        for z in (zs[0] + 1., zs[1] - 1., (zs[0] + zs[1]) / 2):
            for y in (34., 35.):
                add(f'3180/{x}/{z}/{y}', 0x153180, [(A + 0xB0, fw(x, y, z, 1.)), (CTL + 0x55, b(0xF0))])
    add('3180/held', 0x153180, [(CTL + 0x54, b(1)), (A + 0xB0, fw(1e6, 0., 1e6, 1.))])
    # ---- 00153290: the spawn
    add('3290/all', 0x153290, [], {0x1AFA90: [OBJ] * 6, 0x122BB8: [0x7F8000, 0, -1, 0x123456, 1, 0x40000]})
    for refused in (0, 3, 5):
        add(f'3290/refused/{refused}', 0x153290, [], {0x1AFA90: [OBJ] * refused + [0]})
    # ---- 00153540: a spawned piece
    for state in (0, 2, 3, 4):
        add(f'3540/{state}', 0x153540, [(OBJ + 4, b(state)), (OBJ + 0x20, w(EFFECT))], {0x1EFE00: [EFFECT]})
    for hit in (0, 1):
        for vx, x in ((-1., 21.), (-1., 21.5), (-1., 15.), (1., 178.), (1., 179.), (1., 100.)):
            add(f'3540/1/{hit}/{vx}/{x}', 0x153540,
                [(OBJ + 4, b(1)), (OBJ + 0xB0, fw(x, 35., 95., 1.)), (OBJ + 0xC0, fw(vx, .5, -.5, 0.)),
                 (0x700031B0, fw(1., 2., 3., 4.))], {0x19A570: [hit]})
    # ---- 00153770: the damage step
    limit = s16(0x27543A)
    add('3770/none', 0x153770, [(A + 0x36, h(0))])
    add('3770/paused', 0x153770, [(A + 0x36, h(0x10)), (CTL + 0x50, b(1))])
    for hit in (0x10, 0x2010, 0xA010, 0x4010, 0x5010, 0x1010, 0x8010, 0x6010):
        for stunned in (0, 1):
            for armed in (0, 1):
                for hp in (0x10, 0x11, 0x50, 0x51, 1000):
                    add(f'3770/{hit:x}/{stunned}/{armed}/{hp:x}', 0x153770,
                        [(A + 0x36, h(hit)), (CTL + 0x51, b(stunned)), (CTL + 0x52, b(armed)), (A + 0x34, h(hp)),
                         (CTL + 0x40, h(3)), (A + 5, b(2))], {0x1EFE00: [EFFECT]})
    for move in (1, 2):
        for total in (limit - 0x11, limit - 0x10, limit):
            for hit in (0x2010, 0x10):
                add(f'3770/limit/{move}/{total}/{hit:x}', 0x153770,
                    [(A + 0x36, h(hit)), (CTL + 0x51, b(1)), (A + 0x34, h(1000)), (CTL + 0x40, h(total)),
                     (A + 5, b(move))])
    # ---- a seeded random sample over every entry
    rng = random.Random(0x21C0C)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(40, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_creature_c.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_creature_c_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21c_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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
    only = os.environ.get('EM_AREA21_CREATURE_C_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_CREATURE_C_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA21_CREATURE_C_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA21_CREATURE_C_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_CREATURE_C_TRACE'):
        trace(os.environ['EM_AREA21_CREATURE_C_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_CREATURE_C_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
