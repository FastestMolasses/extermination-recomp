#!/usr/bin/env python3
"""The AREA17 creature (src/game/em_area17_creature*.c) against the ORIGINAL
instructions: the behaviour 00133E30 of AREA17 code group 0x8263C0 [0], its
states 00133FD0 / 00134090 / 00136140 / 00136600, the moves 00134300,
00134500, 001348E0, 00134DC0, 00134FF0, 001352E0, 00135870, 00135D00, the
dying steps 00136240 / 00136490, the per-frame pieces 00136630, 001366E0,
001368D0, 00136CB0 / 00136C40, 00136F20, the spit 001370C0 / 001373B0 and
the haze 001E04E0 / 001E0240 / 001E00F0. Unwired.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_level17_boot_reference.py configures it. Every entry runs as original
code from the user's ELF over a fork recording's RAM (a03b_03_item2a: boot
code is the same in every image; its AREA03 overlay text is checked against
the user's extract/OVERLAY/AREA03.BIN and its boot text against the pinned
ELF). Calls between the family's functions run as original code at the top
level (every one is also an entry); every other callee is a hook with a
scripted result, except the pure helpers in MATH, which run as original
code nested in the oracle (their writes replayed to the native side). The
node's +0x4C method is stubbed and logged. The designed node, the bone
table behind 0x275B40, the render context behind 0x275670 and its packet
buffer live in zero RAM at FREE. Compared per case: every call and its
arguments, memory at every call entry, the memory accesses between calls
one for one in order, memory after the last store, the result, a poisoned
re-run, coverage of every reachable original word (DEAD excepted, each with
its reason), the callee inventory and the fail-stop contract. The three
jump-table dispatchers' table loads are left out of the access comparison
(TABLE_BYTES): the translation is a switch.

EM_TEST_FULL=1 widens the random sample. EM_AREA17_CREATURE_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA17_CREATURE_SOURCE=
<dir> tests another copy of the module sources; EM_AREA17_CREATURE_SHOW=<n>
prints n failures; EM_AREA17_CREATURE_MISSING=1 lists unexecuted words;
EM_AREA17_CREATURE_TRACE=<label> prints the original's calls and accesses
of matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area17_creature as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
UU = [U, U]
FUNCS = {
    0x00133E30: ('em_area17_creature_00133E30', 408, [U, S], None),
    0x00133FD0: ('em_area17_creature_00133FD0', 180, UU, None),
    0x00134090: ('em_area17_creature_00134090', 624, [U, U, S], None),
    0x00134300: ('em_area17_creature_00134300', 504, [U, U, S], None),
    0x00134500: ('em_area17_creature_00134500', 984, UU, None),
    0x001348E0: ('em_area17_creature_001348E0', 1240, UU, None),
    0x00134DC0: ('em_area17_creature_00134DC0', 548, UU, None),
    0x00134FF0: ('em_area17_creature_00134FF0', 740, UU, None),
    0x001352E0: ('em_area17_creature_001352E0', 1424, UU, None),
    0x00135870: ('em_area17_creature_00135870', 1168, UU, None),
    0x00135D00: ('em_area17_creature_00135D00', 1080, UU, None),
    0x00136140: ('em_area17_creature_00136140', 252, UU, None),
    0x00136240: ('em_area17_creature_00136240', 592, UU, None),
    0x00136490: ('em_area17_creature_00136490', 356, UU, None),
    0x00136600: ('em_area17_creature_00136600', 48, [U], None),
    0x00136630: ('em_area17_creature_00136630', 168, UU, None),
    0x001366E0: ('em_area17_creature_001366E0', 496, UU, None),
    0x001368D0: ('em_area17_creature_001368D0', 876, UU, I),
    0x00136C40: ('em_area17_creature_00136C40', 104, [I, U, U, F], None),
    0x00136CB0: ('em_area17_creature_00136CB0', 624, UU, None),
    0x00136F20: ('em_area17_creature_00136F20', 416, [U, I], None),
    0x001370C0: ('em_area17_creature_001370C0', 744, [I, U], None),
    0x001373B0: ('em_area17_creature_001373B0', 1148, [U], None),
    0x001E00F0: ('em_area17_creature_001E00F0', 332, [U, U, I], None),
    0x001E0240: ('em_area17_creature_001E0240', 316, [U, U, I, I, S], I),
    0x001E04E0: ('em_area17_creature_001E04E0', 612, [U, I, F, S], None),
}
BEAT = 'a03b_03_item2a'
FREE = 0x1E61000
A = FREE + 0x1000            # the designed creature node (ctl = A + 0x1F0)
CTL = A + 0x1F0
SPIT = FREE + 0x2000         # a spit node (001373B0) / 001AFA90's node
TABLE = FREE + 0x3000        # the bone table behind 0x275B40
BONES = FREE + 0x4000        # bone i at BONES + i * 0x100 (matrix at +0x90)
CTX = FREE + 0x8000          # the render context behind 0x275670
PKT = FREE + 0x10000         # its packet cursor
EFFECT = FREE + 0x9000       # an effect node (001EFE00's result)
OUT = FREE + 0xA000          # 001E00F0 / 001E0240 records
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
PLAYER = 0x810360
SCREEN = 0x70003AC0          # 001E0240's screen matrix
SELF_ENTRIES = {0x133E30, 0x134090, 0x136140}
# the pure helpers run as original code (their writes replayed): vector and
# matrix math, sin / cos / atan2 / fabs, the angle wrap, the bearing, the
# turn, the distance, float_to_int and the matrix slot 001CD370
MATH = frozenset((0x102760, 0x102948, 0x102958, 0x1026A0, 0x1028B8, 0x1029C0, 0x102BB0, 0x1031E0, 0x103230,
                  0x11DE90, 0x11DF78, 0x11E2A8, 0x11E620, 0x1281C0, 0x1B1240, 0x1B12B0, 0x1B1470, 0x1B15D0,
                  0x1CD370))
JUMPS = {0x134090: (0x26D140, 9), 0x1352E0: (0x26D170, 6), 0x135D00: (0x26D190, 9)}
# Words the static walk reaches that no caller can reach, and why.
DEAD = {
    # 00136F20's selector outside 0..3: the branch to the shared call with
    # the clip register never set (its delay-slot word scales it); the only
    # caller (001368D0) passes the literals 0..3. The translation fail-stops
    # there instead.
    0x136F60: 'selector outside 0..3 (no caller; an unset register)',
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
    """The harness's Replay with a faster dirty-set comparison at call
    entries: each 64 KiB page's span from its lowest to its highest dirty
    byte is compared in one memcmp. The bytes between dirty bytes hold the
    start image on both sides (O.Replay.compare's argument), so the span
    comparison finds every difference the per-entry one finds; on a
    difference the harness's own comparison runs and reports it. 001E04E0
    makes about 4000 calls over some 10000 dirty entries, which the
    per-entry walk compares at every call. The call-entry comparisons use
    the spans in EM_TEST_FULL=1 runs as well (they are exact); the final
    comparison is always all of memory."""

    def __init__(self, *args, **kwargs):
        BaseReplay.__init__(self, *args, **kwargs)
        self.dirty = Dirty()

    def compare(self, upto, where, full=None):
        if full:   # the final comparison (finish) stays all of memory
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
    if entry == 0x136C40:
        return [0xF, OUT, OUT + 0x10, O.fb(8.)]
    if entry == 0x136F20:
        return [A, 0]
    if entry == 0x1370C0:
        return [0x2A, 1]
    if entry == 0x1373B0:
        return [SPIT]
    if entry == 0x1E00F0:
        return [OUT, OUT + 0x40, 0x123]
    if entry == 0x1E0240:
        return [OUT, OUT + 0x40, 0x123, 6, O.STACK_TOP]
    if entry == 0x1E04E0:
        return [A + 0xB0, 0x77, O.fb(.5), O.STACK_TOP]
    out = []
    for k in kinds:
        out.append(O.STACK_TOP if k == S else (A if not out else CTL))
    return out


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'}
    O.OUT = ROOT / 'build/levels/area17_creature'
    O.SOURCES = ('em_area17_creature.c', 'em_area17_creature_moves.c', 'em_area17_creature_parts.c',
                 'em_area17_creature_spit.c', 'em_area17_creature_haze.c')
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
    O.HEAVY = {0x1E04E0}
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
    assert not any(ram[FREE:PKT + 0x6000]), 'the designed area is not zero in the capture'


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
NAN, INF, NINF, TINY = 0x7FC00000, 0x7F800000, 0xFF800000, 1
BYTES = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x7F, 0x80, 0xFF)
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x0F0F0F0F, 0x7FFF0000, 0x00012000, 0x80000000, 0x5A5A5A5A,
         0x000F8000, 0x00040000, 0x00080000, 0x00004000, 0x00000020, 0x00000800, 0x0000C000)


def matrix(tx=0., ty=0., tz=0., s=1.):
    return fw(s, 0., 0., 0., 0., s, 0., 0., 0., 0., s, 0., tx, ty, tz, 1.)


def common():
    """The node's method, the bone table, the render context (cursor and
    matrix slot), the screen matrix and the player."""
    p = [(A + 0x4C, w(CALLBACK)), (A + 0xB0, fw(330., 5., 280., 1.)), (A + 0xC4, f(.3)),
         (0x275B40, w(TABLE)), (0x275670, w(CTX)), (CTX + 0x1C, w(PKT)), (CTX + 0x2240, matrix(1., 2., 3.)),
         (SCREEN, fw(100., 0., 0., 0., 0., 100., 0., 0., 0., 0., 1., 1., 2048., 2048., 0., 1.)),
         (PLAYER, fw(350., 0., 300., 1.)), (SPIT + 0x4C, w(CALLBACK))]
    for i in range(0x30):
        p += [(TABLE + 4 * i, w(BONES + 0x100 * i)), (BONES + 0x100 * i + 0x90, matrix(i, 2. * i, -i))]
    return p


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2, 3)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c(range(10)))),
        (A + 6, b(c(range(10)))), (A + 0xD, b(c((0, 1, 2, 3)))), (A + 0x2C, h(c((0, 0x18, 0x1B, 0x8000, 0x801B, 1)))),
        (A + 0x34, h(c((0, 1, 50, 51, 150, 151, 300, 301, 400, 401, 1000, 0x8000)))),
        (A + 0x36, h(c((0, 1, 0x10, 0x4010, 0x8010, 0x1005, 0x2001, 0x4100, 0xFFF, 0xC010)))),
        (A + 0x3C, f(c((0., 7., 20., 21., 30., 37., 49., 62., 70., 80., 90., 130., 131.)))),
        (A + 0x9A, b(c(BYTES))), (A + 0x24, w(c((0, EFFECT)))),
        (CTL, w(c((0, 0x1000, 9, 5, 0x1005, 0x1009)))), (CTL + 4, w(c((0, 1, 2, 0x12B, 0x12C, 0x12D)))),
        (CTL + 8, w(c((0, 1, 5, 6)))), (CTL + 0xC, w(c((0, 1)))), (CTL + 0x10, w(c((0, 9, 10, 0x13, 0x14, 0x12D)))),
        (CTL + 0x20, f(c((1., .5)))), (CTL + 0x24, f(c((0., 1., -2.)))), (CTL + 0x28, f(c((0., -1., 1., -.1)))),
        (CTL + 0x2C, f(c((0., .4, .5, .6, .8, .9, -1.)))), (CTL + 0x30, f(c((0., .05)))),
        (CTL + 0x34, f(c((10., 49., 50., 51.)))), (CTL + 0x3C, f(c((10., 79., 80., 89., .02, -.02)))),
        (CTL + 0x50, h(c((0, 1, 2)))), (CTL + 0x52, h(c((0, 1, 2, 0x80)))), (CTL + 0x54, h(c((0, 10, 0x18, 0x19)))),
        (CTL + 0x56, h(c((0, 1)))), (CTL + 0x58, b(c((0, 1, 0x80)))), (CTL + 0x59, b(c((0, 1)))),
        (CTL + 0x5B, b(c((0, 1, 2, 3, 4, 8, 0xC, 0x10, 0x1F, 0x0F)))), (CTL + 0x5C, b(c((0, 1)))),
        (CTL + 0x5E, b(c((0, 1)))), (0x81070A, b(c((0, 1)))), (0x810805, b(c((0, 3, 4, 8, 9)))),
        (0x28A9A2, b(c((0, 1)))), (0x28A9A0, h(c((0, 2)))),
        (PLAYER, fw(c((350., 340., 400., 300.)), 0., c((300., 270., 280.)), 1.)),
    ]


def hook_values(rng):
    def ints(*values, n=8): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(*RANDS, n=24) for a in (0x122BB8,)}
    out.update({a: ints(0, 1, 2, -1) for a in (0x1B10B0, 0x1B1380, 0x1B3440, 0x21BE40, 0x1A7B80, 0x19A570,
                                                0x1C64F0, 0x1B17A0, 0x1FBD50)})
    out.update({0x1EFE00: ints(0, EFFECT), 0x1AFA90: ints(0, SPIT, SPIT)})
    return out


def cases():
    out, labels = [], set()
    base = common()
    s = O.Scribble

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
    # ---- 00133E30: the four states and the countdowns
    for state in (0, 1, 2, 3, 4, 0xFF):
        add(f'3e30/{state}', 0x133E30, st(state, 7) + [(CTL + 0x58, b(3)), (CTL + 0x5E, b(1)), (CTL + 0x52, h(2)),
                                                         (CTL + 0x56, h(0x8000)), (CTL + 0x50, h(1))])
    add('3e30/zero', 0x133E30, st(4))
    # ---- 00133FD0
    for flag in (0, 1):
        for r in (0, 1, -1):
            add(f'3fd0/{flag}/{r}', 0x133FD0, [(A + 4, b(0xFF)), (0x81070A, b(flag))], {0x1B10B0: [r]})
    # ---- 00134090: the near flag, every move, the header, the cry
    for move in range(10):
        add(f'4090/{move}', 0x134090, st(1, move) + [(CTL + 0x52, h(move & 1))])
    for tag in (9, 8):
        for r in (0, 0x7FFFFFFF, -1, 0x00090000, -0x90000):
            add(f'4090/cry/{tag}/{r}', 0x134090, st(1, 7) + [(CTL, w(tag))], {0x122BB8: [r]})
    for x in (340., 434., 434.1, 246.):
        add(f'4090/near/{x}', 0x134090, st(1, 7) + [(PLAYER, fw(x, 9., 270., 1.))])
    # ---- 00134300 (move 0)
    for r in (0, 0x1000, 0xF000, 0x10000):
        add(f'4300/0/{r:x}', 0x134300, st(1, 0, 0), {0x122BB8: [r]})
    for clip in (0, 1, 0x8000, 0x7FFF):
        for phase in (0, 1, 2):
            for frame in (130., 131.):
                add(f'4300/1/{clip:x}/{phase}/{frame}', 0x134300,
                    st(1, 0, 1) + [(A + 0x2C, h(clip)), (CTL + 4, w(phase)), (A + 0x3C, f(frame)),
                                   (CTL + 0x3C, f(10.)), (CTL + 0x5C, b(0))])
    for value in (10., 79.5, 80., 85.):
        add(f'4300/ease/{value}', 0x134300, st(1, 0, 1) + [(CTL + 4, w(1)), (CTL + 0x3C, f(value))])
    for t, sh in ((0x1000, 0), (0, 1), (0, 0)):
        add(f'4300/end/{t:x}/{sh}', 0x134300, st(1, 0, 1) + [(CTL, w(t)), (CTL + 0x56, h(sh)), (CTL + 4, w(2))])
    add('4300/haze', 0x134300, st(1, 0, 1) + [(CTL + 4, w(1)), (CTL + 0x3C, f(40.)), (CTL + 0x5C, b(1))],
        {0x122BB8: [0x12345678, -0x12345678] * 20}, run=MATH - {0x103230, 0x1028B8, 0x1026A0})
    add('4300/step/2', 0x134300, st(1, 0, 2))
    # ---- 00134500 (move 1)
    for r1 in (0, 0x4000, 0x8000, 0xC000):
        add(f'4500/0/{r1:x}', 0x134500, st(1, 1, 0), {0x122BB8: [0x12340000, r1, 0x01FE0000]})
    for count in (0, 1):
        for near in (0, 1):
            add(f'4500/1/{count}/{near}', 0x134500, st(1, 1, 1) + [(CTL + 4, w(count)), (CTL + 0x5C, b(near))])
    for x in (340., 350., 300., 341.):
        for pause in (0, 1):
            add(f'4500/far/{x}/{pause}', 0x134500, st(1, 1, 1) + [(CTL + 4, w(5)), (CTL + 0x5C, b(1)),
                                                                   (CTL + 0x50, h(pause)), (PLAYER, fw(x, 0., 270., 1.)),
                                                                   (A + 0xB0, fw(300., 5., 200., 1.))],
                {0x122BB8: [0x3C000]})
    for wait in (0, 3):
        for pause in (0, 1):
            for dist in (10., 50., 60.):
                for r in (0, 0x80000):
                    add(f'4500/near/{wait}/{pause}/{dist}/{r:x}', 0x134500,
                        st(1, 1, 1) + [(CTL + 4, w(5)), (CTL + 8, w(wait)), (CTL + 0x50, h(pause)),
                                       (CTL + 0x34, f(dist))], {0x122BB8: [0x001E0000, r]})
    add('4500/2', 0x134500, st(1, 1, 2))
    # ---- 001348E0 (move 2)
    for parts in (0, 1, 2, 3, 0x1C):
        add(f'48e0/0/{parts:x}', 0x1348E0, st(1, 2, 0) + [(CTL + 0x5B, b(parts))])
    for t in (0, 0x1000):
        for side in (0, 1):
            add(f'48e0/1/{t:x}/{side}', 0x1348E0, st(1, 2, 1) + [(CTL, w(t))],
                {0x122BB8: [0xFFFFFFFF, 0x700, 0x2000], 0x1B1380: [side]})
    for parts in (0, 1, 2, 3):
        for count in (0, 1):
            for flag in (0, 1):
                add(f'48e0/2/{parts}/{count}/{flag}', 0x1348E0,
                    st(1, 2, 2) + [(CTL + 0x5B, b(parts)), (CTL + 4, w(count)), (CTL + 8, w(count)),
                                   (0x81070A, b(flag)), (CTL + 0x3C, f(.017))])
    for repeat in (0, 1):
        add(f'48e0/2/end/{repeat}', 0x1348E0, st(1, 2, 2) + end + [(CTL + 0xC, w(repeat))])
    for t in (0, 0x1000):
        add(f'48e0/3/{t:x}', 0x1348E0, st(1, 2, 3) + [(CTL, w(t))], {0x122BB8: [0xF800]})
    add('48e0/4', 0x1348E0, st(1, 2, 4))
    # ---- 00134DC0 / 00134FF0 (moves 3, 4)
    for entry, tag in ((0x134DC0, '4dc0'), (0x134FF0, '4ff0')):
        for parts in (0, 4, 8, 0xC):
            add(f'{tag}/0/{parts}', entry, st(1, 3, 0) + [(CTL + 0x5B, b(parts))])
        for t in (0, 0x1000):
            for clip in (0, 0x8000):
                for frame in (7., 8.):
                    add(f'{tag}/1/{t:x}/{clip:x}/{frame}', entry,
                        st(1, 3, 1) + [(CTL, w(t)), (A + 0x2C, h(clip)), (A + 0x3C, f(frame))], {0x122BB8: [0x800]})
            add(f'{tag}/1/{t:x}/r0', entry, st(1, 3, 1) + [(CTL, w(t))], {0x122BB8: [0]})
        for t in (0, 0x1000, 5, 0x1005):
            for parts in (0, 4, 8, 0xC):
                add(f'{tag}/2/{t:x}/{parts}', entry, st(1, 3, 2) + [(CTL, w(t)), (CTL + 0x5B, b(parts))],
                    {0x1AFA90: [SPIT, 0]})
        for t in (0, 0x1000):
            add(f'{tag}/3/{t:x}', entry, st(1, 3, 3) + [(CTL, w(t))], {0x122BB8: [0xF800]})
        add(f'{tag}/4', entry, st(1, 3, 4))
    for shot in (0, 1):
        for clip in (0x18, 0x8018, 0x19):
            for frame in (35., 35.5, 39., 40.):
                for parts in (0, 0xC):
                    add(f'4ff0/shot/{shot}/{clip:x}/{frame}/{parts}', 0x134FF0,
                        st(1, 4, 2) + [(CTL + 4, w(shot)), (A + 0x2C, h(clip)), (A + 0x3C, f(frame)),
                                       (CTL + 0x5B, b(parts))], {0x1AFA90: [SPIT, SPIT]})
    for r in (0, 0x20):
        for count in (4, 5):
            for r2 in (0, 0x4000):
                add(f'4ff0/again/{r:x}/{count}/{r2:x}', 0x134FF0, st(1, 4, 2) + end + [(CTL + 4, w(1)), (CTL + 8, w(count))],
                    {0x122BB8: [r, r2]})
    # ---- 001352E0 (move 5)
    for step in range(7):
        add(f'52e0/{step}', 0x1352E0, st(1, 5, step) + [(CTL, w(0x1000))], {0x122BB8: [0x1F800]})
        add(f'52e0/{step}/idle', 0x1352E0, st(1, 5, step))
    for clip in (0, 0x8000):
        for frame in (30., 31.):
            add(f'52e0/1/{clip:x}/{frame}', 0x1352E0, st(1, 5, 1) + [(A + 0x2C, h(clip)), (A + 0x3C, f(frame))])
    for frame in (70., 62., 62.5, 49., 20., 19.9, 5.):
        for speed in (.5, .6):
            add(f'52e0/2/{frame}/{speed}', 0x1352E0, st(1, 5, 2) + [(A + 0x3C, f(frame)), (CTL + 0x2C, f(speed))])
    for speed in (.5, .6):
        add(f'52e0/3/{speed}', 0x1352E0, st(1, 5, 3) + [(CTL + 0x2C, f(speed))])
    for step, frame in ((2, 49.), (2, 49.5), (4, 37.), (4, 38.)):
        for done in (0, 1):
            for free in (0, 1):
                for reach in (0, 1):
                    for flag in (0, 1):
                        add(f'52e0/bite/{step}/{frame}/{done}/{free}/{reach}/{flag}', 0x1352E0,
                            st(1, 5, step) + [(A + 0x3C, f(frame)), (CTL + 4, w(done)), (0x81070A, b(flag))],
                            {0x21BE40: [free], 0x1A7B80: [reach]})
    for done in (0, 1):
        add(f'52e0/2/end/{done}', 0x1352E0, st(1, 5, 2) + end + [(CTL + 4, w(done)), (A + 0x3C, f(80.))])
    # ---- 00135870 (move 6)
    for step in range(6):
        add(f'5870/{step}', 0x135870, st(1, 6, step) + end, {0x122BB8: [0x1F800]})
        add(f'5870/{step}/idle', 0x135870, st(1, 6, step))
    for x in (380., 400.1, 300.):
        add(f'5870/1/{x}', 0x135870, st(1, 6, 1) + end + [(PLAYER, fw(x, 0., 280., 1.))])
    for x in (380., 400.1):
        for count in (0x12A, 0x12B):
            for far in (0, 1):
                for speed in (.8, .9):
                    add(f'5870/2/{x}/{count}/{far}/{speed}', 0x135870,
                        st(1, 6, 2) + [(PLAYER, fw(x, 0., 280., 1.)), (CTL + 4, w(count)), (CTL + 0x5C, b(far)),
                                       (CTL + 0x2C, f(speed))])
    for frame in (37., 21., 20.9, 30., 30.5):
        for speed in (.5, .6):
            for done in (0, 1):
                add(f'5870/3/{frame}/{speed}/{done}', 0x135870,
                    st(1, 6, 3) + [(A + 0x3C, f(frame)), (CTL + 0x2C, f(speed)), (CTL + 4, w(done))],
                    {0x21BE40: [0], 0x1A7B80: [1]})
    for free, reach, flag in ((1, 1, 0), (0, 0, 0), (0, 1, 1)):
        add(f'5870/3/bite/{free}/{reach}/{flag}', 0x135870, st(1, 6, 3) + [(A + 0x3C, f(25.)), (0x81070A, b(flag))],
            {0x21BE40: [free], 0x1A7B80: [reach]})
    # ---- 00135D00 (move 8)
    for step in range(10):
        for arena in (0, 4):
            add(f'5d00/{step}/{arena}', 0x135D00, st(1, 8, step) + end + [(0x810805, b(arena)), (CTL + 4, w(0))])
            add(f'5d00/{step}/{arena}/idle', 0x135D00, st(1, 8, step) + [(0x810805, b(arena)), (CTL + 4, w(3))])
    for clip in (0, 0x8000):
        for frame in (7., 70.):
            add(f'5d00/frame/{clip:x}/{frame}', 0x135D00, st(1, 8, 1) + [(A + 0x2C, h(clip)), (A + 0x3C, f(frame))])
            add(f'5d00/frame6/{clip:x}/{frame}', 0x135D00, st(1, 8, 6) + [(A + 0x2C, h(clip)), (A + 0x3C, f(frame))])
    for tag in (5, 4):
        add(f'5d00/spit/{tag}', 0x135D00, st(1, 8, 2) + [(CTL, w(tag))], {0x1AFA90: [SPIT, 0]})
    for x, z, heading, speed in ((423., 235., -2.2, .4), (423.5, 236., 1., .5), (300., 235., 0., .3),
                                 (420., 240., 1.5707964, .4)):
        add(f'5d00/4/{x}/{z}/{heading}/{speed}', 0x135D00,
            st(1, 8, 4) + [(A + 0xB0, fw(x, 1., z, 1.)), (A + 0xC4, f(heading)), (CTL + 0x2C, f(speed))])
    # the walk's arrival: the turn lands exactly on the bearing
    add('5d00/4/arrive', 0x135D00, st(1, 8, 4) + [(A + 0xB0, fw(423., 1., 235., 1.)), (A + 0xC4, f(1.5))],
        {0x1B12B0: [O.fb(1.5)], 0x1B1240: [O.fb(1.5)]}, run=MATH - {0x1B12B0, 0x1B1240})
    # ---- 00136140 / 00136240 / 00136490 / 00136600
    for move in (0, 1, 2):
        for tag in (9, 0):
            add(f'6140/{move}/{tag}', 0x136140, st(2, move) + [(CTL, w(tag))], {0x122BB8: [0x00050000]})
    for hp in (0, 5):
        for kind in (0, 0x2000, 0x8000, 0xA000):
            for r in (0, 1):
                add(f'6240/0/{hp}/{kind:x}/{r}', 0x136240, st(2, 0, 0) + [(A + 0x34, h(hp)), (A + 0x36, h(kind))],
                    {0x122BB8: [r]})
    for clip in (0x1B, 0x801B, 0x1C):
        for frame in (130., 131.):
            for speed in (0., -1., 1.):
                add(f'6240/1/{clip:x}/{frame}/{speed}', 0x136240,
                    st(2, 0, 1) + [(A + 0x2C, h(clip)), (A + 0x3C, f(frame)), (CTL + 0x2C, f(speed))])
    for hp in (0, 5):
        for r in (0, 0x20000):
            add(f'6240/1/end/{hp}/{r:x}', 0x136240, st(2, 0, 1) + end + [(A + 0x34, h(hp))], {0x122BB8: [r]})
    add('6240/2', 0x136240, st(2, 0, 2))
    for step in range(5):
        for arena in (8, 9):
            add(f'6490/{step}/{arena}', 0x136490, st(2, 1, step) + end + [(0x810805, b(arena))], {0x1EFE00: [EFFECT]})
    for flag, mode, e in ((0, 2, EFFECT), (1, 1, EFFECT), (1, 2, EFFECT), (1, 2, 0)):
        add(f'6490/3/{flag}/{mode}/{e:x}', 0x136490, st(2, 1, 3) + [(0x28A9A2, b(flag)), (0x28A9A0, h(mode)),
                                                                     (A + 0x24, w(e))])
    add('6600', 0x136600, [(A + 0x9A, b(0x81))])
    # ---- 00136630 / 001366E0
    for air in (0, 1):
        add(f'6630/{air}', 0x136630, [(CTL + 0x59, b(air)), (CTL + 0x28, f(1.)), (CTL + 0x2C, f(.5)), (CTL + 0x30, f(.05))])
    for x in (340., 402., 402.5, 280.):
        for fall in (0., -1.):
            for ground in (0, 1):
                add(f'66e0/{x}/{fall}/{ground}', 0x1366E0,
                    [(A + 0xB0, fw(x, 5., 270., 1.)), (CTL + 0x28, f(fall)), (CTL + 0x5A, b(0xF0))],
                    {0x1B3440: [ground]})
    # ---- 001368D0: damage
    for hit in (0, 0x10, 0x8010, 0x4010, 0x1010, 0x2010, 0xFFF):
        for grace in (0, 1):
            for window in (0, 1):
                add(f'68d0/{hit:x}/{grace}/{window}', 0x1368D0,
                    [(A + 0x36, h(hit)), (CTL + 0x52, h(grace)), (CTL + 0x58, b(window)), (A + 0x34, h(1000)),
                     (CTL + 0x54, h(4))])
    for hp, hit in ((10, 10), (11, 10), (402, 2), (300, 1), (302, 2), (152, 2), (52, 2), (401, 1), (10000, 0x4001)):
        for parts in (0x1F, 0x10, 0x02, 0x04, 0x01, 0):
            for fx in (0, 1):
                add(f'68d0/hp/{hp}/{hit:x}/{parts:x}/{fx}', 0x1368D0,
                    [(A + 0x36, h(hit)), (A + 0x34, h(hp)), (CTL + 0x5B, b(parts)), (CTL + 0x5E, b(fx))])
    for total in (0x10, 0x18):
        add(f'68d0/sum/{total:x}', 0x1368D0, [(A + 0x36, h(1)), (A + 0x34, h(1000)), (CTL + 0x58, b(1)),
                                               (CTL + 0x54, h(total))])
    # ---- 00136C40 / 00136CB0 / 00136F20
    add('6c40', 0x136C40, [(OUT, fw(1., 2., 3., 1.))])
    for parts in (0, 1, 2, 4, 0x10, 0x1F, 7):
        add(f'6cb0/{parts:x}', 0x136CB0, [(CTL + 0x5B, b(parts))])
    for which in range(4):
        add(f'6f20/{which}', 0x136F20, [], args=[A, which])
    # ---- 001370C0: the spit
    for kind in (0, 1, 2, 3, 0x102):
        for node in (SPIT, 0):
            add(f'70c0/{kind:x}/{node:x}', 0x1370C0, [], {0x1AFA90: [node]}, args=[0x2A, kind])
    add('70c0/steep', 0x1370C0, [(PLAYER, fw(57., 1000., -38., 1.))],
        {0x1AFA90: [SPIT], 0x1026A0: [s(0, [(0x700038A0, fw(47., 76., -38., 1.))])]}, run=MATH - {0x1026A0},
        args=[0x26, 0])
    for y in (-20., 0., 30., 300.):
        for x in (340., 500., 345.):
            add(f'70c0/aim/{y}/{x}', 0x1370C0, [(PLAYER, fw(x, y, 280., 1.))], {0x1AFA90: [SPIT]}, args=[0x26, 0])
    # ---- 001373B0: the spit's flight
    for state in range(5):
        add(f'73b0/{state}', 0x1373B0, [(SPIT + 4, b(state)), (SPIT + 0x20, w(EFFECT))],
            {0x1EFE00: [EFFECT], 0x122BB8: [0x00FF0000, 0x40000]})
    flight = [(SPIT + 4, b(1)), (SPIT + 0xB0, fw(340., 10., 270., 1.)), (SPIT + 0xC0, fw(.5, .1, .5, 0.)),
              (SPIT + 0x1F0, fw(350., 0., 290., 1.))]
    for count in (8, 9, 0x12C, 0x12D, 0x13, 0x14):
        for kind in (0, 1, 2):
            add(f'73b0/1/{count:x}/{kind}', 0x1373B0, flight + [(SPIT + 0x200, w(count)), (SPIT + 0xD, b(kind))])
    for x, y in ((340., 270.), (530., 270.), (608., 270.), (350., 290.)):
        for count in (0x10, 0x14):
            for kind in (0, 2):
                for wall in (0, 1):
                    add(f'73b0/range/{x}/{y}/{count:x}/{kind}/{wall}', 0x1373B0,
                        flight + [(SPIT + 0xB0, fw(x, 10., y, 1.)), (SPIT + 0x200, w(count)), (SPIT + 0xD, b(kind)),
                                  (0x700031B0, fw(1., 2., 3., 4.))], {0x19A570: [wall]})
    # ---- 001E00F0 / 001E0240 / 001E04E0: the haze
    for r1, r2 in ((0, 0), (0x7FFFFFFF, 0x40000000), (-1, 0x12345678)):
        add(f'00f0/{r1:x}/{r2:x}', 0x1E00F0, [(OUT + 0x40, fw(2048.5, 2100., 10.))], {0x122BB8: [r1, r2]})
    stack = O.STACK_TOP - 0x10
    for vec in ((100., 200., 1., 16.), (100., 200., 1., -16.), (-1., 200., 1., 16.), (65520., 200., 1., 16.),
                (65521., 2., 1., 16.), (5., -1., 1., 16.), (5., 65520., 1., 16.), (5., 70000., 1., 16.)):
        for seed in (0, 6, 0x10, -1):
            add(f'0240/{vec}/{seed}', 0x1E0240, [(OUT + 0x40, fw(1., 2., 3., 1.))],
                {0x1026A0: [s(0, [(stack, fw(*vec))])], 0x122BB8: [0x10000000, 0x20000000]},
                run=MATH - {0x1026A0}, args=[OUT, OUT + 0x40, 0x55, seed, O.STACK_TOP])
    add('0240/live', 0x1E0240, [(OUT + 0x40, fw(1., 2., 3., 1.))])
    add('04e0/stub', 0x1E04E0, [], {0x122BB8: [0x10000000] * 4}, run=())
    add('04e0/live', 0x1E04E0, [], {0x122BB8: [0x40000000, 0x10000000] * 30},
        run=MATH - {0x103230, 0x1028B8})
    # ---- a seeded random sample over every light entry
    rng = random.Random(0x17C2EA)
    light = [e for e in FUNCS if e not in (0x1E04E0, 0x1E0240, 0x1E00F0)]
    for entry in light:
        for n in range(O.reference_mode.pick(40, 3)):
            patches = fields(rng)
            if entry in (0x133E30, 0x134090, 0x134300):
                patches.append((CTL + 0x5C, b(0)))   # the haze only in its own cases (heavy)
            add(f'random/{entry:06X}/{n}', entry, patches, hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area17_creature.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area17_creature_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a17c_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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


def contract_site(site):
    """O.hook_contract_site; on a case that runs 001E04E0 (about 4000 calls) its
    sampled form (at most O.CONTRACT_CALLS failing calls plus one per
    callee, O.CONTRACT_ACCESSES refused accesses) also under EM_TEST_FULL=1,
    where every call and every access would each be a full native run."""
    heavy = O.CASES[site[0]][2] in O.HEAVY or O.CASES[site[0]][0] == '4300/haze'
    keep = O.reference_mode.FULL
    try:
        if heavy:
            O.reference_mode.FULL = False
        return O.hook_contract_site(site)
    finally:
        O.reference_mode.FULL = keep


def judge(results):
    only = os.environ.get('EM_AREA17_CREATURE_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA17_CREATURE_MISSING')):
            failures.append(('coverage', [hex(entry), [hex(m) for m in sorted(missing)]]))
    if not only:
        assert not set(DEAD) & seen, 'a word called dead ran'
        failures += [('callee inventory', p) for p in O.callee_checks()]
        if not failures:
            sites, missing = O.hook_sites(results)
            if missing:
                failures.append(('unreached hooks', missing))
            for problems, runs in O.reference_mode.parallel_map(contract_site, sites):
                if problems:
                    failures.append(('fail-stop contract', problems))
    return failures


def main():
    configure()
    O.NATIVE = O.build(os.environ.get('EM_AREA17_CREATURE_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA17_CREATURE_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA17_CREATURE_TRACE'):
        trace(os.environ['EM_AREA17_CREATURE_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA17_CREATURE_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
