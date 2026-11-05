#!/usr/bin/env python3
"""AREA00 sub 2's creature (src/game/em_area00s2_creature*.c) against the
ORIGINAL instructions: the behaviour 0014D260 of the record the owner
0x825D70 spawns (group 0x827EF0, node 0x7B6190 in the k01 captures), its
set-up 0014D3B0, the states 0014D470 / 0014E370 / 0014E640, the moves
0014D5F0, 0014D7C0, 0014DC30, 0014E050, 0014E2C0, 0014E420, 0014E4F0 and
the pieces 0014E670, 0014E7A0, 0014E890, 0014E9C0. Bound through the later
levels' fallback dispatch (tools/gen_later_dispatch.py).

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area21_creature_c_reference.py configures it (its stack window and
FastReplay included: 0014E890 keeps a frame local). Every entry runs as
original code from the user's ELF over the fork recordings of the
constructed start k01 (k01_00_load and k01_01_slab: AREA00 resident, its
overlay text checked against the user's extract/OVERLAY/AREA00.BIN and the
boot text against the pinned ELF). Calls between the family's functions
run as original code at the top level (every one is also an entry); every
other callee is a hook with a scripted result, except the pure helpers in
MATH, which run as original code nested in the oracle (their writes
replayed to the native side). The node's +0x4C method is stubbed and
logged. The designed node, its linked records and the records behind
+0x138 / +0x13C / +0x140 / +0x174 live in zero RAM at FREE; the capture
cases run the live node 0x7B6190 as recorded. Compared per case: every call
and its arguments, memory at every call entry, the memory accesses between
calls one for one in order, memory after the last store, the result, a
poisoned re-run, coverage of every reachable original word (DEAD excepted,
each with its reason), the callee inventory and the fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA00S2_CREATURE_ONLY=<label
prefix> runs a subset (no coverage or contract);
EM_AREA00S2_CREATURE_SOURCE=<dir> tests another copy of the module sources;
EM_AREA00S2_CREATURE_SHOW=<n> prints n failures;
EM_AREA00S2_CREATURE_MISSING=1 lists unexecuted words;
EM_AREA00S2_CREATURE_TRACE=<label> prints the original's calls and accesses
of matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area00s2_creature as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
UU = [U, U]
FUNCS = {
    0x0014D260: ('em_area00s2_creature_0014D260', 0x144, [U, S], None),
    0x0014D3B0: ('em_area00s2_creature_0014D3B0', 0xC0, UU, None),
    0x0014D470: ('em_area00s2_creature_0014D470', 0x180, [U, U, S], None),
    0x0014D5F0: ('em_area00s2_creature_0014D5F0', 0x1CC, UU, None),
    0x0014D7C0: ('em_area00s2_creature_0014D7C0', 0x46C, UU, None),
    0x0014DC30: ('em_area00s2_creature_0014DC30', 0x414, UU, None),
    0x0014E050: ('em_area00s2_creature_0014E050', 0x270, UU, None),
    0x0014E2C0: ('em_area00s2_creature_0014E2C0', 0xA8, UU, None),
    0x0014E370: ('em_area00s2_creature_0014E370', 0xB0, [U, U, S], None),
    0x0014E420: ('em_area00s2_creature_0014E420', 0xC4, UU, None),
    0x0014E4F0: ('em_area00s2_creature_0014E4F0', 0x148, UU, None),
    0x0014E640: ('em_area00s2_creature_0014E640', 0x30, [U], None),
    0x0014E670: ('em_area00s2_creature_0014E670', 0x130, UU, None),
    0x0014E7A0: ('em_area00s2_creature_0014E7A0', 0xF0, UU, None),
    0x0014E890: ('em_area00s2_creature_0014E890', 0x12C, [U, U, S], None),
    0x0014E9C0: ('em_area00s2_creature_0014E9C0', 0x240, UU, I),
}
ROUTE = 'levels-recordings-20261102-1/s87/route_k01/'
BEATS = ('k01_00_load', 'k01_01_slab')
BEAT = BEATS[0]
LIVE = 0x7B6190              # the node in both captures (state 1, move 4, step 1)
FREE = 0x1E61000
A = FREE + 0x1000            # the designed creature node (ctl = A + 0x1F0)
CTL = A + 0x1F0
LINK = FREE + 0x2000         # its linked effect (+0x20)
LINK2 = FREE + 0x2100        # the effect at +0x24
R138 = FREE + 0x3000         # the records behind +0x138 / +0x13C / +0x140 (matrix at +0x90) / +0x174
R13C = FREE + 0x3100
R140 = FREE + 0x3200
R174 = FREE + 0x3300
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
PLAYER = 0x810360
SELF_ENTRIES = {0x14D260, 0x14D470, 0x14E370}
# the pure helpers run as original code (their writes replayed): the vector
# copy and normalise, the quadword copy, the matrix product, modff, the
# bearing, float_to_int, the turn, the angle wrap and the flat distance
MATH = frozenset((0x1026A0, 0x102760, 0x102948, 0x102958, 0x11E0A8, 0x11E620, 0x1281C0, 0x1B12B0, 0x1B1470,
                  0x1B15D0))
# Words the static walk reaches that no caller can reach, and why.
DEAD = {
    0x14D9DC: "0014D7C0's second link test: reached only when the link is 0",
}


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
    """The harness's Replay with test_area21_creature_c_reference's span
    comparison at call entries (exact: the bytes between dirty bytes hold
    the start image on both sides; on a difference the harness's own
    comparison runs and reports it). The final comparison is always all of
    memory."""

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


def default_args(entry):
    out = []
    for k in FUNCS[entry][2]:
        out.append(O.STACK_TOP if k == S else (A if not out else CTL))
    return out


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {1: ROOT.parent / 'Extermination/extract/OVERLAY/AREA00.BIN'}
    O.OUT = ROOT / 'build/area00s2_creature'
    O.SOURCES = ('em_area00s2_creature.c', 'em_area00s2_creature_moves.c', 'em_area00s2_creature_parts.c')
    O.BEATS = list(BEATS)
    O.BEAT_DIRS = {beat: refs.capture(ROUTE + beat) for beat in BEATS}
    O.RESIDENT = {beat: 1 for beat in BEATS}
    O.BY_ADDRESS = {a: (n, args, res) for n, a, args, res in HOOKS}
    O.BY_NAME = {n: a for n, a, _, _ in HOOKS}
    O.HOOK_NAMES = [h[0] for h in HOOKS] + ['w_callback']
    O.Hooks = type('Hooks', (C.Structure,), {'_fields_':
        [('ctx', C.c_void_p), ('bytes', O.BYTES_FN)] +
        [(name, O.hook_proto(args, res)) for name, _, args, res in HOOKS] + [('w_callback', O.CALLBACK_FN)]})
    O.SELF_ENTRIES = set(SELF_ENTRIES)
    O.HEAVY = {0x14D260, 0x14D470, 0x14E370}
    O.RETURN_SITES = O.RETURN_SITES_FULL = ()
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.JUMP_TABLES = {}
    O.TABLE_BYTES = frozenset()
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[BEAT][0]
    O.Replay = FastReplay
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    for beat in BEATS:
        ram = O.CAPTURES[beat][0]
        assert not any(ram[FREE:R174 + 0x100]), 'the designed area is not zero in the capture'
        assert ram[LIVE + 4] == 1 and ram[LIVE + 5] == 4 and O.u32(ram, LIVE + 0x4C) == CALLBACK, beat


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x0F0F0F0F, 0x7FFF0000, 0x00012000, 0x80000000, 0x5A5A5A5A,
         0x000F8000, 0x00040000, 0x00080000, 0x00004000, 0x00000080, 0x00000800, 0x0000C000, 0x00FE0000,
         0x0007F000, 0x07F80000)
PI2 = 0x3FC90FDB             # pi/2 (bits)


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(struct.unpack('<I', struct.pack('<f', x))[0] + d)


def matrix(tx=0., ty=0., tz=0., s=1.):
    return fw(s, 0., 0., 0., 0., s, 0., 0., 0., 0., s, 0., tx, ty, tz, 1.)


def common():
    """The node's method, position and records, the player."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0xB0, fw(10., 5., 20., 1.)), (A + 0x138, w(R138)),
            (A + 0x13C, w(R13C)), (A + 0x140, w(R140)), (A + 0x174, w(R174)), (R140 + 0x90, matrix(3., 4., 5.)),
            (CTL + 0x40, f(1.)), (PLAYER, fw(30., 0., 50., 1.))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    out = [
        (A, b(c((0, 1, 2)))), (A + 4, b(c((0, 1, 1, 1, 2, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 3, 4, 5)))),
        (A + 6, b(c((0, 1, 2, 3, 4)))), (A + 7, b(c((0, 1, 2)))), (A + 0x20, w(c((0, LINK)))),
        (A + 0x24, w(c((0, LINK2)))), (A + 0x2C, h(c((0, 2, 3, 0x8002)))), (A + 0x9A, b(c((0, 7, 0x81)))),
        (A + 0x34, h(c((0, 1, 5, 24, 25, 26, 240, 241, 1200)))),
        (A + 0x36, h(c((0, 0, 1, 0x18, 0x19, 0x8005, 0x4010, 0x2001, 0x1001, 0xA003, 0xFFF, 0x0400)))),
        (A + 0x3C, f(c((0., 11.5, 12., 14., 22., 24., 26., 26.5, 27., 30., 33., 33.5, 49., 49.5, 52., 52.5)))),
        (A + 0x5D, b(c((0, 0x41, 0xC1)))),
        (A + 0xB0, fw(c((10., -40., 30., 31.)), 5., c((20., 50., 49., 90., -10.)), 1.)),
        (CTL, w(c((0, 0x1000, 9, 0x1009)))), (CTL + 4, w(c((0, 1, 2, 0, 1, 2, 3, 5)))),
        (CTL + 8, w(c((0, 1, 2, 0x5A)))), (CTL + 0xC, w(c((0, 1)))),
        (CTL + 0x20, f(c((1., .5, .9, .85, .95)))), (CTL + 0x24, f(c((0., 1., -1.)))),
        (CTL + 0x28, f(c((0., .01, -.03, 1., -1.)))), (CTL + 0x34, f(c((0., 25., 25.5, 100.)))),
        (CTL + 0x38, f(c((0., 1., -1., 1.5, -1.6)))), (CTL + 0x3C, f(c((.5, 2.5, 0.)))),
        (CTL + 0x40, f(c((1., 1.04, 1.05, 1.06, 1.5, 1.79, 1.8, 1.85, 2.)))),
        (CTL + 0x50, h(c((0, 0, 1, 10)))), (CTL + 0x52, h(c((0, 0, 1, 99, 100)))), (CTL + 0x54, h(c((0, 1, 24, 25)))),
        (CTL + 0x56, h(c((0, 0, 1, 240)))), (CTL + 0x58, b(c((0, 1)))), (CTL + 0x59, b(c((0, 1)))),
        (CTL + 0x5A, b(c((0, 1, 0xFF)))), (CTL + 0x5B, b(c((0, 1)))), (CTL + 0x5C, b(c((0, 1)))),
        (CTL + 0x5D, b(c((0, 1)))), (CTL + 0x5E, b(c((0, 0, 1, 5)))),
        (R174 + 0x78, f(c((0., .6108653, .5, .01)))),
        (0x810803, b(c((0, 0, 1, 2, 0x80, 0x81)))), (0x81070A, b(c((0, 1)))), (0x8104E4, b(c((0, 1)))),
        (0x28A9A2, b(c((0, 1)))), (0x28A9A0, h(c((0, 2)))),
        (PLAYER, fw(c((30., -30., 10., 9.)), 0., c((50., -50., 20., 21.)), 1.)),
    ]
    # 0014D7C0's step 2 with a phase outside 0..2 and the flag clear compares
    # with a register the function never set (the native faults there):
    # random inputs keep the flag set in that combination.
    vals = dict(out)
    if vals[A + 6] == b(2) and vals[CTL + 4] not in (w(0), w(1), w(2)):
        out.append((CTL + 0xC, w(1)))
    return out


def hook_values(rng):
    def ints(*values, n=8): return [rng.choice(values) for _ in range(n)]
    out = {0x122BB8: ints(*RANDS, n=24)}
    out.update({a: ints(0, 1, 2, -1) for a in (0x1B10B0, 0x21BE40, 0x1A7B80, 0x1B17A0, 0x1FBD50, 0x1C64F0)})
    out.update({0x1EFE00: ints(0, LINK2)})
    return out


def cases():
    out, labels = [], set()
    base = common()

    def add(label, e, patches=(), queues=None, run=MATH, args=None, beat=BEAT):
        assert label not in labels, label
        labels.add(label)
        all_patches = (base if args is None else []) + list(patches)
        out.append((label, beat, e, tuple(args or O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    def st(state, move=0, step=0):
        return [(A + 4, b(state)), (A + 5, b(move)), (A + 6, b(step))]

    end = [(CTL, w(0x1000))]
    link = [(A + 0x20, w(LINK))]
    counts = [(CTL + 0x5A, b(3)), (CTL + 0x56, h(2)), (CTL + 0x5B, b(0xFF)), (CTL + 0x5E, b(1)),
              (CTL + 0x52, h(0x8000)), (CTL + 0x50, h(7))]
    # ---- the live node as recorded (state 1, move 4, step 1), and with the
    # byte 0x810803 at 2 (0014E2C0 lets it go) and 1 (nothing runs)
    for beat in BEATS:
        live = [LIVE, O.STACK_TOP]
        add(f'live/{beat}', 0x14D260, args=live, beat=beat)
        add(f'live/{beat}/803', 0x14D260, [(0x810803, b(2))], args=live, beat=beat)
        add(f'live/{beat}/stop', 0x14D260, [(0x810803, b(1))], args=live, beat=beat)
        add(f'live/{beat}/d470', 0x14D470, args=[LIVE, LIVE + 0x1F0, O.STACK_TOP], beat=beat)
    # ---- 0014D260: the states and the countdowns
    for state in (0, 1, 2, 3, 4):
        add(f'd260/{state}', 0x14D260, st(state, 5) + counts)
        add(f'd260/{state}/zero', 0x14D260, st(state, 5))
    add('d260/stop', 0x14D260, st(1, 5) + counts + [(0x810803, b(1))])
    # ---- 0014D3B0
    for flag in (0, 1):
        for r in (0, 1):
            add(f'd3b0/{flag}/{r}', 0x14D3B0, [(0x81070A, b(flag))], {0x1B10B0: [r], 0x1EFE00: [LINK]})
    # ---- 0014D470 / 0014E370: every move, the header, the 0x58 flag
    for move in range(6):
        for busy in (0, 3):
            for flag in (0, 1):
                add(f'd470/{move}/{busy}/{flag}', 0x14D470,
                    st(1, move, 9) + [(CTL + 0x52, h(busy)), (CTL + 0x58, b(flag)), (A + 0x5D, b(0x41 | 0x80 * flag))])
    for move in (0, 1, 2):
        add(f'e370/{move}', 0x14E370, st(2, move, 9))
    # ---- 0014D5F0 (move 0)
    for clip in (2, 0x8002, 3, 0x4002):
        for lk in (0, LINK):
            add(f'd5f0/0/{clip:x}/{lk:x}', 0x14D5F0, st(1, 0, 0) + [(A + 0x2C, h(clip)), (A + 0x20, w(lk)),
                                                                   (CTL + 0x38, f(1.25)), (CTL + 0x5E, b(4))])
    for wait in (0, 1):
        for timer in (0, 2):
            for near in (25., 25.5, 0., 'up'):
                for flag in (0, 1):
                    for r in (0, 0x80):
                        add(f'd5f0/1/{wait}/{timer}/{near}/{flag}/{r:x}', 0x14D5F0,
                            st(1, 0, 1) + [(CTL + 4, w(wait)), (CTL + 0x50, h(timer)),
                                           (CTL + 0x34, ulp(25., 1) if near == 'up' else f(near)),
                                           (0x8104E4, b(flag)), (CTL + 0x38, f(-.5)), (CTL + 0x28, f(.3)),
                                           (CTL + 0x5E, b(1))],
                            {0x122BB8: [0x7FC000 | r, r]})
    for r in (0, 0x80, 0x100, 0x180, 0x1F800):
        add(f'd5f0/2/sound/{r:x}', 0x14D5F0, st(1, 0, 2), {0x122BB8: [r, r]})
    # ---- 0014D7C0 (move 1)
    for lk in (0, LINK):
        add(f'd7c0/0/{lk:x}', 0x14D7C0, st(1, 1, 0) + [(A + 0x20, w(lk))])
    for t in (0, 0x1000):
        for phase in (0, 1, 2):
            add(f'd7c0/1/{t:x}/{phase}', 0x14D7C0, st(1, 1, 1) + [(CTL, w(t)), (CTL + 4, w(phase))])
    bands = {0: (11.5, 12., 26., 26.5), 1: (13.5, 14., 24., 24.5), 2: (21.5, 22., 30., 30.5)}
    for phase, xs in bands.items():
        edges = [(str(x), f(x)) for x in xs] + [(f'{xs[1]}-', ulp(xs[1], -1)), (f'{xs[2]}+', ulp(xs[2], 1))]
        for name, x in edges:
            for flag in (0, 1):
                for lk in (0, LINK):
                    add(f'd7c0/2/{phase}/{name}/{flag}/{lk:x}', 0x14D7C0,
                        st(1, 1, 2) + [(CTL + 4, w(phase)), (A + 0x3C, x), (CTL + 0xC, w(flag)),
                                       (A + 0x20, w(lk))], {0x21BE40: [0], 0x1A7B80: [1]})
        for g1 in (0, 1):
            for g2 in (0, 1):
                for hard in (0, 1):
                    add(f'd7c0/2/{phase}/gate/{g1}/{g2}/{hard}', 0x14D7C0,
                        st(1, 1, 2) + [(CTL + 4, w(phase)), (A + 0x3C, f(xs[2])), (CTL + 0xC, w(1)),
                                       (0x81070A, b(hard)), (0x8102B0, b(0x41))], {0x21BE40: [g1], 0x1A7B80: [g2]})
        for lk in (0, LINK):
            add(f'd7c0/2/{phase}/end/{lk:x}', 0x14D7C0,
                st(1, 1, 2) + end + [(CTL + 4, w(phase)), (A + 0x3C, f(50.)), (CTL + 0xC, w(1)), (A + 0x20, w(lk)),
                                     (CTL + 0x20, f(.525))])
    for phase in (3, 5, -1):
        add(f'd7c0/2/out/{phase}', 0x14D7C0, st(1, 1, 2) + end + [(CTL + 4, w(phase)), (CTL + 0xC, w(1))])
    for t in (0, 0x1000):
        add(f'd7c0/3/{t:x}', 0x14D7C0, st(1, 1, 3) + [(CTL, w(t))], {0x122BB8: [0x000FF000]})
    for timer in (0, 1, 0x5A):
        add(f'd7c0/4/{timer}', 0x14D7C0, st(1, 1, 4) + [(CTL + 8, w(timer)), (CTL + 0x38, f(1.)),
                                                        (CTL + 0x28, f(.9))])
    # ---- 0014DC30 (move 2)
    for c38 in (-1., 0., 1.):
        add(f'dc30/0/{c38}', 0x14DC30, st(1, 2, 0) + [(CTL + 0x38, f(c38))])
    for flag in (0, 1):
        for name, x in (('33', f(33.)), ('33.5', f(33.5)), ('33+', ulp(33., 1))):
            for c28 in (0., .01, .5):
                for t in (0, 0x1000):
                    add(f'dc30/1/{flag}/{name}/{c28}/{t:x}', 0x14DC30,
                        st(1, 2, 1) + [(CTL + 8, w(flag)), (A + 0x3C, x), (CTL + 0x28, f(c28)), (CTL, w(t)),
                                       (CTL + 4, w(flag))])
    for side in (0, 1, 2):
        xs = [(str(x), f(x)) for x in (23.5, 24., 26.5, 27., 49., 49.5, 52., 52.5)]
        xs += [('24-', ulp(24., -1)), ('27-', ulp(27., -1)), ('49+', ulp(49., 1)), ('52+', ulp(52., 1))]
        for name, x in xs:
            for flag in (0, 1):
                add(f'dc30/2/{side}/{name}/{flag}', 0x14DC30,
                    st(1, 2, 2) + link + [(CTL + 4, w(side)), (A + 0x3C, x), (CTL + 8, w(flag))],
                    {0x21BE40: [0], 0x1A7B80: [1]})
    for g1 in (0, 1):
        for g2 in (0, 1):
            for hard in (0, 1):
                for lk in (0, LINK):
                    add(f'dc30/2/gate/{g1}/{g2}/{hard}/{lk:x}', 0x14DC30,
                        st(1, 2, 2) + end + [(CTL + 4, w(1)), (A + 0x3C, f(40.)), (CTL + 8, w(1)), (A + 0x20, w(lk)),
                                             (0x81070A, b(hard)), (0x8102B0, b(0x41))],
                        {0x21BE40: [g1], 0x1A7B80: [g2]})
    for hp in (240, 241):
        for t in (0, 0x1000):
            add(f'dc30/3/{hp}/{t:x}', 0x14DC30, st(1, 2, 3) + [(A + 0x34, h(hp)), (CTL, w(t))],
                {0x122BB8: [0x07FFF000]})
    add('dc30/4', 0x14DC30, st(1, 2, 4))
    # ---- 0014E050 (move 3)
    add('e050/0', 0x14E050, st(1, 3, 0))
    for c28 in (0., .03, .5):
        for t in (0, 0x1000):
            add(f'e050/1/{c28}/{t:x}', 0x14E050, st(1, 3, 1) + [(CTL + 0x28, f(c28)), (CTL, w(t))])
    for step in (2, 3):
        for t in (0, 0x1000):
            add(f'e050/{step}/{t:x}', 0x14E050, st(1, 3, step) + [(CTL, w(t))], {0x122BB8: [0x07F80000]})
    add('e050/4', 0x14E050, st(1, 3, 4))
    # ---- 0014E2C0 (move 4)
    add('e2c0/0', 0x14E2C0, st(1, 4, 0))
    for flag in (0, 1, 2, 0x81):
        add(f'e2c0/1/{flag:x}', 0x14E2C0, st(1, 4, 1) + [(0x810803, b(flag))])
    add('e2c0/2', 0x14E2C0, st(1, 4, 2))
    # ---- 0014E420 / 0014E4F0 (state 2)
    add('e420/0', 0x14E420, st(2, 0, 0))
    for t in (0, 0x1000):
        add(f'e420/1/{t:x}', 0x14E420, st(2, 0, 1) + [(CTL, w(t))])
    add('e420/2', 0x14E420, st(2, 0, 2))
    add('e4f0/0', 0x14E4F0, st(2, 1, 0))
    for flag in (0x80, 0x81):
        for lk in (0, LINK):
            add(f'e4f0/1/{flag:x}/{lk:x}', 0x14E4F0, st(2, 1, 1) + [(0x810803, b(flag)), (A + 0x20, w(lk))],
                {0x1EFE00: [LINK2]})
    for done in (0, 1, -1):
        for count in (2, 1, 0x102):
            for lk in (0, LINK2):
                add(f'e4f0/2/{done}/{count:x}/{lk:x}', 0x14E4F0,
                    st(2, 1, 2) + [(0x28A9A2, b(done)), (0x28A9A0, h(count)), (A + 0x24, w(lk))])
    add('e4f0/3', 0x14E4F0, st(2, 1, 3))
    # ---- 0014E640
    add('e640', 0x14E640, [(A + 0x9A, b(0x81))])
    # ---- 0014E670: the tilt
    for timer in (0, 3):
        for side in (0, 1, 2):
            add(f'e670/0/{timer}/{side}', 0x14E670, [(A + 7, b(0)), (CTL + 0x56, h(timer)), (CTL + 0x59, b(side)),
                                                     (R174 + 0x78, f(.3))], {0x122BB8: [0x00FE0000]})
    for side in (0, 1):
        for cur in (0., .6108653, .3, .005, .6, -.01):
            for speed in (.5, 2.5):
                add(f'e670/1/{side}/{cur}/{speed}', 0x14E670,
                    [(A + 7, b(1)), (CTL + 0x59, b(side)), (R174 + 0x78, f(cur)), (CTL + 0x3C, f(speed))])
    add('e670/2', 0x14E670, [(A + 7, b(2))])
    # ---- 0014E7A0: the bearing and its clamp
    for dx, dz in ((20., 30.), (-20., 30.), (20., -30.), (-20., -30.), (0., 1.), (1., 0.), (-1., 0.), (0., -1.),
                   (0., 0.)):
        add(f'e7a0/{dx}/{dz}', 0x14E7A0, [(PLAYER, fw(10. + dx, 0., 20. + dz, 1.))])
    for ang in (PI2, PI2 + 1, PI2 - 1, PI2 | 0x80000000, (PI2 + 1) | 0x80000000, (PI2 - 1) | 0x80000000, 0,
                0x40490FDB, 0xC0490FDB):
        add(f'e7a0/clamp/{ang:x}', 0x14E7A0, [], {0x1B1470: [ang]}, run=MATH - {0x1B1470})
    # ---- 0014E890: the pulse
    for grow in (0, 1):
        for s in (1., 1.04, 1.05, 1.06, 1.5, 1.79, 1.8, 1.81, 2., 2.5, .5, 0.):
            add(f'e890/{grow}/{s}', 0x14E890, [(CTL + 0x5C, b(grow)), (CTL + 0x40, f(s))])
    for grow, s in ((1, 0x3FE66667), (1, 0x3FE66668), (0, 0x3F866667)):   # + 0.2 gives 2.0; - 0.05 gives 1.0 + 1 ulp
        add(f'e890/edge/{grow}/{s:x}', 0x14E890, [(CTL + 0x5C, b(grow)), (CTL + 0x40, w(s))])
    # ---- 0014E9C0: the hit tally
    for hit in (0, 0x10, 0x8010, 0x4010, 0x2010, 0x1010, 0xA010, 0x0FFF, 0x5003, 0x4003):
        for busy in (0, 3):
            for combo in (0, 1):
                for fx in (0, 1):
                    add(f'e9c0/{hit:x}/{busy}/{combo}/{fx}', 0x14E9C0,
                        [(A + 0x36, h(hit)), (CTL + 0x52, h(busy)), (CTL + 0x5A, b(combo)), (CTL + 0x5B, b(fx)),
                         (A + 0x34, h(1000)), (CTL + 0x54, h(3)), (CTL + 0x5D, b(1)), (CTL + 0x20, f(.5))])
    for hp, hit in ((10, 10), (11, 10), (2, 0x8001), (11, 0x8002), (100, 0x8005), (100, 24), (100, 25), (100, 1)):
        for tally in (0, 24, 25):
            add(f'e9c0/count/{hp}/{hit:x}/{tally}', 0x14E9C0,
                [(A + 0x36, h(hit)), (A + 0x34, h(hp)), (CTL + 0x5A, b(1)), (CTL + 0x54, h(tally))])
    for ramp in (0, 1):
        for x20 in (0, 0x3F59999A, 0x3F666665, 0x3F666666, 0x3F666667, 0x3F733333, 0x3F800000):   # .85 .. 1.0, 0.9's neighbours
            add(f'e9c0/ramp/{ramp}/{x20:x}', 0x14E9C0,
                [(A + 0x36, h(1)), (A + 0x34, h(100)), (CTL + 0x5D, b(ramp)), (CTL + 0x20, w(x20))])
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA0052)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(40, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng), beat=rng.choice(BEATS))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area00s2_creature.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area00s2_creature_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a0s2_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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


def phase_fault():
    """0014D7C0 step 2 with a phase outside 0..2 and ctl+0xC clear: the
    original would compare +0x3C with a register it never set; the native
    reads the phase, ctl+0xC and +0x3C, then faults at 0014D7C0 with no hook
    call and no store."""
    ram = bytearray(O.CAPTURES[BEAT][0])
    for a, v in [(A + 6, b(2)), (CTL + 4, w(3)), (CTL + 0xC, w(0)), (A + 0x3C, f(10.))]:
        ram[a:a + len(v)] = v
    reads, calls = [], []

    def view(_ctx, address, size):
        reads.append((address, size))
        return C.addressof(buf) + address if address < len(ram) else None

    def hook(*_):
        calls.append(1)
        return 0
    buf = (C.c_uint8 * len(ram)).from_buffer(ram)
    keep = [O.BYTES_FN(view), O.CALLBACK_FN(hook)]
    fields_ = {'ctx': None, 'bytes': keep[0], 'w_callback': keep[1]}
    for name, _, args, result in HOOKS:
        fields_[name] = O.hook_proto(args, result)(hook)
        keep.append(fields_[name])
    hooks = O.Hooks(**fields_)
    fault = O.Fault()
    before = bytes(ram)
    rc = O.NATIVE.em_area00s2_creature_0014D7C0(C.byref(hooks), A, CTL, C.byref(fault))
    assert rc == -1 and (fault.address, fault.code) == (0x14D7C0, 2) and not calls, (rc, fault.address, fault.code)
    assert reads == [(A + 6, 1), (CTL + 4, 4), (CTL + 0xC, 4), (A + 0x3C, 4)], reads
    assert bytes(ram) == before


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
    only = os.environ.get('EM_AREA00S2_CREATURE_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA00S2_CREATURE_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA00S2_CREATURE_SOURCE'))
    header_checks()
    entry_guards()
    phase_fault()
    selected = cases()
    only = os.environ.get('EM_AREA00S2_CREATURE_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA00S2_CREATURE_TRACE'):
        trace(os.environ['EM_AREA00S2_CREATURE_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA00S2_CREATURE_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
