#!/usr/bin/env python3
"""AREA17's own overlay owners (src/game/em_area17_overlay.c and
em_area17_overlay_npc.c) against the ORIGINAL instructions: the effect
objects 0x823580, 0x8235E0, 0x8236F0, 0x823800, 0x823860, the group points
0x8239D0, [30] 0x823BA0 with script 0x8270A0's op09 callbacks 0x823F70 /
0x824280, [39] 0x8242D0, [31] 0x8243E0, [33] 0x8245A0, [34] 0x824700,
[42] 0x824CF0, [40] 0x824E40, [41] 0x8250E0, the part 0x8251B0, [38]
0x825470, [5]..[8] 0x825560 (NEARMISS in the decomp: the oracle runs the
original words), [36] 0x825EA0, the deferred member 0x8260C0 and [37]
0x826170. docs/AREA17_PORT.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area14_overlay_reference.py configures it. Every entry runs as
original code from the user's extract/OVERLAY/AREA17.BIN, resident in two
end states of Claude's guided run (decomp build/guided_play/: g46 (the
ramp) and g47 (after Roger), AREA17 loaded; their eeMemory and scratchpad
are extracted once into build/area17_overlay/states/ with the decomp's
venv; the overlay text is checked against the file and the boot text
against the pinned ELF). Every callee is a hook with a scripted result,
except the pure helpers in MATH, which run as original code nested in the
oracle (their writes replayed to the native side). A node's +0x4C method
is stubbed and logged. The designed node, op09 record and work block live
in zero RAM at FREE; the capture cases run every live AREA17 owner node of
the two states as recorded. Compared per case: every call and its
arguments, memory at every call entry, the memory accesses between calls
one for one in order, memory after the last store, the result, a
poisoned re-run, coverage of every reachable original word, the callee
inventory and the fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA17_OVERLAY_ONLY=<label
prefix> runs a subset (no coverage or contract);
EM_AREA17_OVERLAY_SOURCE=<dir> tests another copy of the module sources;
EM_AREA17_OVERLAY_SHOW=<n> prints n failures; EM_AREA17_OVERLAY_MISSING=1
lists unexecuted words; EM_AREA17_OVERLAY_TRACE=<label> prints the
original's calls and accesses of matching cases (no instruction text).
Reports hold addresses and counts only.
"""
import ctypes as C
import os
import random
import re
import struct
import subprocess
from pathlib import Path
import gen_area17_overlay as G
import test_level14_port_reference as O
ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
FUNCS = {
    0x823580: ('em_area17_overlay_00823580', 0x58, [U], None),
    0x8235E0: ('em_area17_overlay_008235E0', 0x110, [U], None),
    0x8236F0: ('em_area17_overlay_008236F0', 0x110, [U], None),
    0x823800: ('em_area17_overlay_00823800', 0x58, [U], None),
    0x823860: ('em_area17_overlay_00823860', 0x168, [U, S], None),
    0x8239D0: ('em_area17_overlay_008239D0', 0x1A4, [U], None),
    0x823BA0: ('em_area17_overlay_00823BA0', 0x3CC, [U], None),
    0x823F70: ('em_area17_overlay_00823F70', 0x310, [U, U, U], I),
    0x824280: ('em_area17_overlay_00824280', 0x4C, [], I),
    0x8242D0: ('em_area17_overlay_008242D0', 0x104, [U], None),
    0x8243E0: ('em_area17_overlay_008243E0', 0x1B4, [U], None),
    0x8245A0: ('em_area17_overlay_008245A0', 0x158, [U, S], None),
    0x824700: ('em_area17_overlay_00824700', 0x5F0, [U, S], None),
    0x824CF0: ('em_area17_overlay_00824CF0', 0x150, [U], None),
    0x824E40: ('em_area17_overlay_00824E40', 0x29C, [U], None),
    0x8250E0: ('em_area17_overlay_008250E0', 0xCC, [U], None),
    0x8251B0: ('em_area17_overlay_008251B0', 0x2BC, [U], None),
    0x825470: ('em_area17_overlay_00825470', 0xE4, [U], None),
    0x825560: ('em_area17_overlay_00825560', 0x934, [U], None),
    0x825EA0: ('em_area17_overlay_00825EA0', 0x214, [U], None),
    0x8260C0: ('em_area17_overlay_008260C0', 0xA4, [U], None),
    0x826170: ('em_area17_overlay_00826170', 0xBC, [U], None),
}
OVERLAY_ID = 0x0E
GUIDED = DECOMP / 'build/guided_play'
BEATS = ('g46_area17_ramp', 'g47_area17_roger')
BEAT = BEATS[0]
STATES = ROOT / 'build/area17_overlay/states'
FREE = 0x1E60000             # zero in both states
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
OWN = FREE + 0x1800          # a designed owner / group / child record
GRP = FREE + 0x1C00
REC = FREE + 0x2000          # the op09 record
WORK = FREE + 0x2100         # the script's work block
OBJ = FREE + 0x3000          # the node's +0x1C / +0x114 / +0x138 object
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
SELF_ENTRIES = {0x823BA0, 0x8242D0, 0x8243E0, 0x8245A0, 0x824700, 0x824CF0, 0x824E40, 0x8250E0, 0x8251B0,
                0x825560, 0x825EA0, 0x8260C0, 0x826170}
# the pure helpers run as original code (their writes replayed)
MATH = frozenset((0x1026A0, 0x1028D0, 0x102738, 0x102918, 0x102948, 0x1029C0, 0x102B08, 0x11DE90,
                  0x11E2A8, 0x11E748, 0x1B1470))
DEAD = {}


def extract_states():
    """eeMemory.bin and scratchpad.bin of each state's end.p2s, cached (the
    decomp's venv has zstd)."""
    missing = [b for b in BEATS if not (STATES / b / 'scratchpad.bin').exists()]
    if missing:
        code = ('import sys\nfrom pathlib import Path\nfrom tools.parse_pcsx2_state import extract_zstd_entry\n'
                'for n in sys.argv[3:]:\n'
                '    o = Path(sys.argv[1]) / n; o.mkdir(parents=True, exist_ok=True)\n'
                '    p = Path(sys.argv[2]) / n / "end.p2s"\n'
                '    (o / "eeMemory.bin").write_bytes(extract_zstd_entry(p, "eeMemory.bin"))\n'
                '    (o / "scratchpad.bin").write_bytes(extract_zstd_entry(p, "Scratchpad.bin"))\n')
        subprocess.run([str(DECOMP / '.venv/bin/python'), '-c', code, str(STATES), str(GUIDED), *missing],
                       cwd=DECOMP, check=True)
    return {b: STATES / b for b in BEATS}


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
    kinds = FUNCS[entry][2]
    if entry == 0x823F70:
        return [A, REC, WORK]
    return [A, O.STACK_TOP][:len(kinds)] if kinds else []


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {OVERLAY_ID: DECOMP / 'extract/OVERLAY/AREA17.BIN'}
    O.OUT = ROOT / 'build/area17_overlay'
    O.SOURCES = ('em_area17_overlay.c', 'em_area17_overlay_npc.c')
    O.BEATS = list(BEATS)
    O.BEAT_DIRS = extract_states()
    O.RESIDENT = {beat: OVERLAY_ID for beat in BEATS}
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


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x40000000, -0x80000000, 0x00012300, 0x5A5A5A5A, 0x2F0, 0x1F)


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(struct.unpack('<I', struct.pack('<f', x))[0] + d)


def common():
    """The node's method, its +0xD byte, its objects and a player."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0xD, b(34)), (A + 0x1C, w(OBJ)), (A + 0x114, w(OBJ)),
            (A + 0x138, w(OBJ)), (A + 0x20, w(OWN)), (A + 0x24, w(GRP)), (0x810350, fw(150., 220., 290., 1.)),
            (OBJ + 0x90, fw(1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 10., 20., 30., 1.)),
            (OBJ + 0xC0, fw(5., 6., 7., 1.)), (A + 0xB0, fw(100., 200., 300., 1.)), (A + 0xC0, fw(.5, 1., 1.5, 0.))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2)))), (A + 3, b(c((10, 12, 0x18, 0x2A, 3)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))),
        (A + 5, b(c((0, 1, 2, 3, 4, 5, 0x19)))), (A + 0xB, b(c((0, 4, 0xFB)))),
        (A + 0x28, h(c((0, 1, 2, 8, 120, 139, 140, 179, 180, 521, 567, 745, 1106, 2283, 3699, 3989, 0xFFFF)))),
        (A + 0x2E, h(c((0, 1, 3, 15)))), (A + 0x36, h(c((0, 1, 0x2000, 0x2001)))), (A + 0x38, f(c((0., 1.5, -2.)))),
        (A + 0x94, h(c((4, 5, 6, 7, 8, 9)))), (A + 0x1F4, f(c((0., .8, 1.4999, 1.5, 1.9995, 2.)))),
        (A + 0x268, f(c((-1., 0., -3.95, 2.)))), (A + 0xB4, f(c((0., 4.9, 5., 300.)))),
        (OWN + 4, h(c((0, 1, 0x270C, 0x270D)))), (OWN + 0xA, h(c((0, 3, 4, 5, 6)))), (GRP + 0x2E, h(c((0, 2, 8)))),
        (0x810785, b(c((0, 1)))), (0x810786, b(c((0, 1, 2)))), (0x810787, b(c((0, 1, 0xFF)))),
        (0x810805, b(c((0, 1, 2, 3, 8)))), (0x810806, b(c((0, 1)))), (0x810807, b(c((0, 1, 2, 3, 0xFF)))),
        (0x810835, b(c((0, 1)))), (0x810700, b(c((0x11, 0x15)))), (0x70003B92, b(c((0, 1)))),
        (REC + 4, b(c((0, 1, 2, 3)))), (WORK + 0x10, f(c((0., 379., 380., 599., 600.)))),
    ]


def hook_values(rng):
    def ints(*values, n=6): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(0, 1, 2, 3, -1) for a in (0x1BA1C0, 0x1BA1F0, 0x1B1EA0, 0x1B0FD0, 0x1B17A0, 0x1B1020, 0x1BAD40,
                                            0x19A570, 0x19AB20, 0x19AD00, 0x1B0D80, 0x1CCF70, 0x1C6120, 0x1C64F0)}
    out[0x122BB8] = ints(*RANDS)
    out[0x1B6660] = ints(0, 0x7A6000, 0x7A62F0)
    out[0x1C5570] = ints(0, OBJ)
    return out


def cases():
    out, labels = [], set()
    base = common()

    def add(label, e, patches=(), queues=None, run=MATH, args=None, beat=BEAT):
        assert label not in labels, label
        labels.add(label)
        all_patches = (base if args is None else []) + list(patches)
        out.append((label, beat, e, tuple(args if args is not None else O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    def st(state, step=0):
        return [(A + 4, b(state)), (A + 5, b(step))]

    # ---- every live AREA17 owner node of the two states, as recorded
    for beat in BEATS:
        ram = O.CAPTURES[beat][0]
        for entry in FUNCS:
            if entry in (0x823F70, 0x824280):
                continue
            for node in O.owners(ram, entry):
                args = [node, O.STACK_TOP] if FUNCS[entry][2] == [U, S] else [node]
                add(f'live/{beat}/{entry:x}/{node:x}', entry, args=args, beat=beat)
    # ---- the effect objects
    for state in (0, 1, 2, 3, 4):
        for e in (0x823580, 0x823800, 0x8239D0):
            add(f'{e:x}/{state}', e, st(state) + [(A + 0x94, h(8))])
        for e in (0x8235E0, 0x8236F0, 0x823860):
            add(f'{e:x}/{state}', e, st(state) + [(A + 0x1F4, f(.8)), (A + 0x1F0, w(0x123))],
                {0x122BB8: [0x40000000], 0x1CCF70: [7]})
    for kind in (0, 4, 5, 6, 7, 8, 9, -1):
        add(f'39d0/{kind}', 0x8239D0, st(1) + [(A + 0x94, h(kind))])
    for e in (0x8235E0, 0x8236F0, 0x823860):
        for name, v in (('.8', f(.8)), ('.8+', ulp(.8, 1)), ('1.488', f(1.488)), ('1.4999', f(1.4999)),
                        ('1.5', f(1.5)), ('1.999', f(1.999)), ('2-', ulp(2., -1)), ('2', f(2.)), ('-1', f(-1.))):
            add(f'{e:x}/ramp/{name}', e, st(1) + [(A + 0x1F4, v), (A + 0x1F0, w(0xFFFFFF85))])
        for r in RANDS:
            add(f'{e:x}/seed/{r & 0xFFFFFFFF:x}', e, st(0), {0x122BB8: [r]})
    add('3860/scribble', 0x823860, st(1) + [(A + 0x1F4, f(1.)), (A + 0x1F0, w(5))],
        {0x1CFB50: [O.Scribble(None, [(A + 0x1F4, f(1.4995)), (A + 0x1F0, w(9))])],
         0x1CFBE0: [O.Scribble(None, [(A + 0x1F4, f(1.6))])]})
    # ---- [30] 0x823BA0
    for r in (0, 1):
        for d in (0, 1):
            add(f'3ba0/0/{r}/{d}', 0x823BA0, st(0), {0x1BA1C0: [r], 0x1B0FD0: [d]})
    for step in (0, 1, 2, 3, 4, 5, 6, 0x19):
        for v805 in (0, 1, 2, 3, 8):
            for r in (0, 1, 2, 3):
                for drawn in (0, 1):
                    if step in (5, 6) and (r or v805):
                        continue
                    add(f'3ba0/1/{step:x}/{v805}/{r}/{drawn}', 0x823BA0, st(1, step) + [(0x810805, b(v805))],
                        {0x1BA1F0: [r], 0x1B17A0: [drawn], 0x1B6660: [0x7A6000]})
    add('3ba0/1/19/scribble', 0x823BA0, st(1, 0x19) + [(0x810805, b(0))],
        {0x1B6660: [O.Scribble(0x7A6000, [(0x810805, b(2))])]})
    for state in (2, 3, 4):
        add(f'3ba0/{state}', 0x823BA0, st(state))
    # ---- 0x823F70 / 0x824280
    rec = []
    for name, p in (('a', fw(150., 220., 290., 1.)), ('b', fw(-30.5, 2., 900.25, 1.)), ('c', fw(0., 0., 0., 1.))):
        add(f'3f70/0/{name}', 0x823F70, rec + [(REC + 4, b(0)), (0x810350, p)])
    for step in (1, 2):
        for name, n in (('0', f(0.)), ('1', f(1.)), ('379', f(379.)), ('380-', ulp(380., -1)), ('380', f(380.)),
                        ('599', f(599.)), ('599.5', f(599.5)), ('600', f(600.)), ('1000', f(1000.))):
            add(f'3f70/{step}/{name}', 0x823F70, [(REC + 4, b(step)), (WORK + 0x10, n),
                                                  (0x969A80, fw(10., 20., 30., 7.5))])
    add('3f70/3', 0x823F70, [(REC + 4, b(3))])
    add('4280', 0x824280)
    # ---- [39] / [31] / [41] / [37] / 0x8260C0 / [38] / [42] / [40]
    for d in (0, 1):
        for e in (0x8242D0, 0x8243E0, 0x8250E0, 0x826170, 0x824CF0, 0x824E40, 0x825EA0, 0x8245A0, 0x825560):
            add(f'{e:x}/0/{d}', e, st(0), {0x1B0FD0: [d], 0x1BA1C0: [d]})
        add(f'26c0/0/{d}', 0x8260C0, st(0), {0x1B1020: [d]})
    for e in (0x8242D0, 0x8243E0, 0x8250E0, 0x826170, 0x824CF0, 0x824E40, 0x8260C0, 0x825470, 0x825EA0, 0x8245A0,
              0x824700, 0x8251B0, 0x825560):
        for state in (2, 3, 4):
            add(f'{e:x}/{state}', e, st(state) + [(OWN + 4, h(1))])
    for n in (0, 1, 119, 120, 121, 179, 180, 181, -1, 0x7FFF):
        for r in (0, 1):
            for v805 in (0, 1):
                add(f'42d0/1/{n}/{r}/{v805}', 0x8242D0, st(1) + [(A + 0x28, h(n)), (0x810805, b(v805))],
                    {0x1BA1C0: [r], 0x1B17A0: [1]})
    for n in (0, 1, 2, -1):
        for r in (0, 1):
            add(f'43e0/1/{n}/{r}', 0x8243E0, st(1) + [(A + 0x28, h(n)), (0x28A59C, w(0x456))],
                {0x1BA1C0: [r], 0x1B17A0: [r], 0x1C6120: [0x99]})
    add('43e0/2', 0x8243E0, st(2), {0x1B17A0: [1]})
    for r in (0, 1):
        for b92 in (0, 1):
            for v787 in (0, 1):
                for e in (0x8250E0, 0x824E40):
                    add(f'{e:x}/1/{r}/{b92}/{v787}', e, st(1) + [(0x70003B92, b(b92)), (0x810787, b(v787))],
                        {0x1BA1C0: [r]})
    for r in (0, 1):
        add(f'6170/1/{r}', 0x826170, st(1), {0x1BA1C0: [r]})
        add(f'26c0/1/{r}', 0x8260C0, st(1) + [(0x810787, b(r))])
    for n in (0, 1, 565, 566, 567, 745, 746, 1105, 1106, 1124, 1125, -1):
        for v806 in (0, 1):
            add(f'4cf0/1/{n}/{v806}', 0x824CF0, st(1) + [(A + 0x28, h(n)), (0x810806, b(v806))])
    for step in (0, 1, 2):
        for v835 in (0, 1):
            for r in (0, 1):
                add(f'5470/1/{step}/{v835}/{r}', 0x825470, st(1, step) + [(0x810835, b(v835))], {0x1BA1F0: [r]})
    add('5470/0', 0x825470, st(0))
    # ---- [33] 0x8245A0
    for step in (0, 1, 2):
        for r in (0, 1, 2):
            for drawn in (0, 1):
                add(f'45a0/1/{step}/{r}/{drawn}', 0x8245A0, st(1, step),
                    {0x1BA1C0: [r], 0x1B1EA0: [r], 0x1B17A0: [drawn]})
    # ---- [34] 0x824700
    pose = [(0x28A5E8, w(0x31415926)), (0x28A5EC, w(0x27182818)), (0x810785, b(1))]
    for r in (0, 1):
        add(f'4700/0/{r}', 0x824700, st(0) + pose, {0x1BA1C0: [r]})
    add('4700/1/off', 0x824700, st(1) + [(0x810785, b(0))])
    for step in (0, 1, 2):
        for r in (0, 1):
            add(f'4700/786/{step}/{r}', 0x824700, st(1, step) + pose + [(0x810786, b(1))],
                {0x1BA1F0: [r], 0x1C64F0: [0x12345]})
    for phase in (0, 1, 2, 3, 4, 0xFF):
        for step in (0, 1, 2):
            for r in (0, 1, 2):
                for bits in (0, 4):
                    if bits and phase != 1:
                        continue
                    add(f'4700/807/{phase}/{step}/{r}/{bits}', 0x824700,
                        st(1, step) + pose + [(0x810786, b(0)), (0x810807, b(phase)), (A + 0xB, b(bits))],
                        {0x1BA1F0: [r], 0x1B1EA0: [r], 0x1C64F0: [-7]})
    add('4700/807/3/1/scribble', 0x824700, st(1, 1) + pose + [(0x810807, b(3))],
        {0x1BA1F0: [1], 0x1C64F0: [O.Scribble(5, [(A + 0x114, w(OBJ + 0x40)), (0x700038A0, fw(1., 2., 3., 4.))])]})
    # ---- the part 0x8251B0
    for st0 in (0, 1):
        for r in (0, 1):
            for bit in (0, 1):
                for kind in (0, 0x270C, 0x270D):
                    for mode in (0, 3, 4, 5, 6):
                        if kind and mode:
                            continue
                        add(f'51b0/{st0}/{r}/{bit}/{kind:x}/{mode}', 0x8251B0,
                            st(st0) + [(GRP + 0x2E, h(8 if bit else 1)), (A + 0x2E, h(3)), (OWN + 4, h(kind)),
                                       (OWN + 0xA, h(mode)), (OWN + 0xC, f(.75)), (A + 0x28, h(600))],
                            {0x1BAD40: [r]})
    for n in (519, 520, 521, 2281, 2282, 2283, 3697, 3698, 3699, 3988, 3989, 3990, -1):
        add(f'51b0/mark/{n}', 0x8251B0, st(1) + [(A + 0x28, h(n)), (OWN + 0xA, h(3))])
    for kind, mode in ((0x270D, 0), (1, 0), (1, 2)):
        add(f'51b0/2/{kind:x}/{mode}', 0x8251B0, st(2) + [(OWN + 4, h(kind)), (OWN + 0xA, h(mode))])
    # ---- the breakables 0x825560
    for shot in (0, 1, 0x2000):
        for hit in (0, 1):
            for v805 in (0, 2, 3, 9):
                for name, p in (('near', fw(110., 200., 300., 1.)), ('edge', fw(150., 200., 300., 1.)),
                                ('far', fw(150.001, 200., 300., 1.))):
                    if hit and (v805 or name != 'near'):
                        continue
                    add(f'5560/1/{shot:x}/{hit}/{v805}/{name}', 0x825560,
                        st(1) + [(A + 0x36, h(shot)), (0x810805, b(v805)), (0x810350, p)], {0x19A570: [hit]})
    for kind in (10, 12, 0x18, 0x2A, 3):
        for r in (0, 0xF0, 0x31F, 0x2FF):
            add(f'5560/2/0/{kind:x}/{r:x}', 0x825560, st(2, 0) + [(A + 3, b(kind)), (A + 0xC4, f(1.25))],
                {0x122BB8: [r, r ^ 0x100, r ^ 0x200, r]})
    for kind in (10, 0x18, 0x2A):
        for n in (1, 2, 0):
            add(f'5560/2/1/{kind:x}/{n}', 0x825560, st(2, 1) + [(A + 3, b(kind)), (A + 0x28, h(n))])
    for step in (2, 3):
        for n in (0, 1):
            for name, rise in (('up', f(1.)), ('top', f(0.06)), ('down', f(-1.)), ('floor', f(-3.95)),
                               ('-4', f(-4.))):
                for speed in (0., 1.5):
                    add(f'5560/2/{step}/{n}/{name}/{speed}', 0x825560,
                        st(2, step) + [(A + 0x28, h(n)), (A + 0x268, rise), (A + 0x38, f(speed)),
                                       (A + 0x264, f(.3))], {0x19AD00: [1], 0x19AB20: [0]})
    for shot in (0, 1, 0x2001):
        for land in (0, 1):
            for tick in (0, 1):
                for b0d80 in (0, 1):
                    for area in (0x11, 0x15):
                        for y in (4., 5., 6.):
                            if land and (tick or b0d80 or area == 0x11 or y != 4.):
                                continue
                            add(f'5560/2/3/{shot:x}/{land}/{tick}/{b0d80}/{area:x}/{y}', 0x825560,
                                st(2, 3) + [(A + 0x36, h(shot)), (A + 0x268, f(-1.)), (A + 0xB4, f(y + 1.)),
                                            (0x70003B68, w(0x40 + tick)), (0x70003B8A, h(0)),
                                            (0x810700, b(area))],
                                {0x19AB20: [land], 0x1B0D80: [b0d80]})
    add('5560/0/scribble', 0x825560, st(0), {0x1B0FD0: [O.Scribble(0, [(A + 0xB0, fw(1., 2., 3., 4.))])]})
    # ---- [36] 0x825EA0
    for step in (0, 1, 2):
        for bits in (0, 4):
            for v807 in (0, 2, 3):
                for v787 in (0, 1, 0xFF):
                    for n in (0, 1, 139, 140):
                        for child in (0, OBJ):
                            if child == 0 and n not in (139, 140):
                                continue
                            add(f'5ea0/1/{step}/{bits}/{v807}/{v787:x}/{n}/{child:x}', 0x825EA0,
                                st(1, step) + [(A + 0xB, b(bits)), (0x810807, b(v807)), (0x810787, b(v787)),
                                               (A + 0x28, h(n)), (A + 0x2EC, w(child))])
    for child in (OBJ, 0):
        for r in (0, 1):
            add(f'5ea0/0/{child:x}/{r}', 0x825EA0, st(0) + [(OBJ + 0xB4, f(3.))], {0x1C5570: [child], 0x1BA1C0: [r]})
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA17)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(24, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng), beat=rng.choice(BEATS))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area17_overlay.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area17_overlay_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a17_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(' % symbol, header, re.M), symbol


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
    only = os.environ.get('EM_AREA17_OVERLAY_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA17_OVERLAY_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA17_OVERLAY_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA17_OVERLAY_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA17_OVERLAY_TRACE'):
        trace(os.environ['EM_AREA17_OVERLAY_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA17_OVERLAY_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
