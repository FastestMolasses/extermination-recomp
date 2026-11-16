#!/usr/bin/env python3
"""AREA14's own overlay owners (src/game/em_area14_overlay.c) against the
ORIGINAL instructions: [35] / [36] 0x823580, [5] 0x823760 with its script's
op09 callback 0x8238C0, [0] 0x8238E0, [2] 0x823B30, [4] 0x823E30 (NEARMISS
in the decomp: the oracle runs the original words), [6] 0x825FD0 with its
script's op09 callback 0x825C40; and the two boot functions only AREA14
reaches, its camera arm 00230230 and the arm's area test 00191350
(byte-matched decomp C). docs/AREA14_PORT.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area00s2_creature_reference.py configures it. Every entry runs as
original code from the user's extract/OVERLAY/AREA14.BIN, resident in the
fork recordings of the constructed start k01 (k01_02 .. k01_05: AREA14
loaded; the overlay text checked against the file and the boot text against
the pinned ELF). Every callee is a hook with a scripted result, except the
pure helpers in MATH, which run as original code nested in the oracle
(their writes replayed to the native side). A node's +0x4C method is
stubbed and logged. The designed node, op09 record and work block live in
zero RAM at FREE; the capture cases run every live AREA14 owner node of the
four recordings as recorded. Compared per case: every call and its
arguments, memory at every call entry, the memory accesses between calls
one for one in order, memory after the last store, the result, a poisoned
re-run, coverage of every reachable original word (DEAD excepted, each with
its reason), the callee inventory and the fail-stop contract. Also checked: 001F6B00 (called by 0x823760) is
001F6B30 word for word in the ELF (the runtime binds it to that
translation).

EM_TEST_FULL=1 widens the random sample. EM_AREA14_OVERLAY_ONLY=<label
prefix> runs a subset (no coverage or contract);
EM_AREA14_OVERLAY_SOURCE=<dir> tests another copy of the module source;
EM_AREA14_OVERLAY_SHOW=<n> prints n failures; EM_AREA14_OVERLAY_MISSING=1
lists unexecuted words; EM_AREA14_OVERLAY_TRACE=<label> prints the
original's calls and accesses of matching cases (no instruction text).
Reports hold addresses and counts only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area14_overlay as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
FUNCS = {
    0x823580: ('em_area14_overlay_00823580', 0x1B4, [U, S], None),
    0x823760: ('em_area14_overlay_00823760', 0x160, [U], None),
    0x8238C0: ('em_area14_overlay_008238C0', 0x20, [], I),
    0x8238E0: ('em_area14_overlay_008238E0', 0x244, [U, S], None),
    0x823B30: ('em_area14_overlay_00823B30', 0x300, [U], None),
    0x823E30: ('em_area14_overlay_00823E30', 0x18C, [U], None),
    0x825C40: ('em_area14_overlay_00825C40', 0x38C, [U, U, U], I),
    0x825FD0: ('em_area14_overlay_00825FD0', 0x1A8, [U], None),
    # boot: the camera arm and its area test
    0x230230: ('em_area14_overlay_00230230', 0x5B4, [U, U], I),
    0x191350: ('em_area14_overlay_00191350', 0x38, [U], I),
}
OVERLAY_ID = 0x0B
ROUTE = 'levels-recordings-20261102-1/s87/route_k01/'
BEATS = ('k01_02_item26', 'k01_03_item19', 'k01_04_face', 'k01_05_plateau')
BEAT = BEATS[0]
FREE = 0x1E61000             # zero in the four recordings
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
REC = FREE + 0x2000          # the op09 record 0x825C40 steps
WORK = FREE + 0x2100         # the script's work block
CAM = FREE + 0x3000          # a designed camera record (00230230's a0)
PL = FREE + 0x4000           # a designed player record (00230230's a1)
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
SELF_ENTRIES = {0x8238E0, 0x823B30, 0x825FD0}
# the pure helpers run as original code (their writes replayed): the
# vector copy and difference, the matrix identity / product / translation,
# the sine
MATH = frozenset((0x102948, 0x1028D0, 0x1029C0, 0x102C58, 0x102918, 0x11E2A8, 0x11DE90, 0x11DF78, 0x1B1470,
                  0x1B12B0, 0x191120))
# Words the static walk reaches that no caller can reach, and why: inside
# 00230230's states 10 / 25 the original tests the state against 21 again
# (a branch on the same register the dispatch compared) and holds arms for
# it that cannot run there (the decomp C keeps them, "never true here").
DEAD = {pc: "00230230's state-21 arm inside states 10 / 25"
        for pc in list(range(0x2304C8, 0x230568, 4)) + list(range(0x230644, 0x230684, 4))}
PLAYER_Y = 0x810354
ROOT_Y = 0x810364


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
    return {0x823580: [A, O.STACK_TOP], 0x8238E0: [A, O.STACK_TOP], 0x8238C0: [],
            0x825C40: [A, REC, WORK], 0x230230: [CAM, PL], 0x191350: [PL]}.get(entry, [A])


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {OVERLAY_ID: ROOT.parent / 'Extermination/extract/OVERLAY/AREA14.BIN'}
    O.OUT = ROOT / 'build/area14_overlay'
    O.SOURCES = ('em_area14_overlay.c',)
    O.BEATS = list(BEATS)
    O.BEAT_DIRS = {beat: refs.capture(ROUTE + beat) for beat in BEATS}
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
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x40000000, -0x80000000, 0x00012000, 0x5A5A5A5A)


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(struct.unpack('<I', struct.pack('<f', x))[0] + d)


def common():
    """The node's method, its +0xD byte and a player."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0xD, b(94)), (0x810350, fw(400., 432., 1280., 1.))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 3)))),
        (A + 0xB, b(c((0, 4, 0xFB)))), (A + 0x28, h(c((0, 1, 360, 399, 400, 0xFFFF)))),
        (A + 0x38, f(c((0., 1.99, 2., .5)))), (A + 0x1F4, f(c((.8, 1.999, 2., 0.)))),
        (0x810816, b(c((0, 1)))), (0x810C8C, b(c((0, 1)))), (0x810702, b(c((0, 1, 2)))),
        (0x70003B92, b(c((0, 1)))), (PLAYER_Y, f(c((400., 434.9, 435., 675., 680.)))),
        (ROOT_Y, f(c((700., 810., 811., 900., 901.)))), (0x8105E4, f(c((700., 815., 816.)))),
        (REC + 4, b(c((0, 1, 2, 3)))), (REC + 5, b(c((0, 1, 2)))), (WORK + 0x10, f(c((80., 1., 0., 179., 180.)))),
    ]


def hook_values(rng):
    def ints(*values, n=4): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(0, 1, -1) for a in (0x1BA1C0, 0x1BA1F0, 0x1B1EA0, 0x1B0FD0, 0x1C64F0, 0x1CCF70)}
    out[0x122BB8] = ints(*RANDS)
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

    # ---- every live AREA14 owner node of the four recordings, as recorded
    for beat in BEATS:
        ram = O.CAPTURES[beat][0]
        for entry in FUNCS:
            if entry in (0x8238C0, 0x825C40):
                continue
            for node in O.owners(ram, entry):
                args = [node, O.STACK_TOP] if FUNCS[entry][2] == [U, S] else [node]
                add(f'live/{beat}/{entry:x}/{node:x}', entry, args=args, beat=beat)
    # ---- 0x823580
    for state in (0, 1, 2, 3, 4):
        add(f'3580/{state}', 0x823580, st(state) + [(A + 0x1F4, f(.8))], {0x122BB8: [0x40000000], 0x1CCF70: [7]})
    for r in RANDS:
        add(f'3580/rand/{r & 0xFFFFFFFF:x}', 0x823580, st(0), {0x122BB8: [r]})
    for name, v in (('1.998', f(1.998)), ('1.999', f(1.999)), ('2-', ulp(2., -1)), ('2', f(2.)), ('3', f(3.)),
                    ('0', f(0.)), ('-1', f(-1.))):
        add(f'3580/wrap/{name}', 0x823580, st(1) + [(A + 0x1F4, v), (A + 0x1F0, f(.25))], {0x1CCF70: [-1]})
    add('3580/scribble', 0x823580, st(1) + [(A + 0x1F4, f(1.)), (A + 0x1F0, f(.5))],
        {0x1CFB50: [O.Scribble(None, [(A + 0x1F4, f(1.9995)), (A + 0x1F0, f(.125))])],
         0x1CFBE0: [O.Scribble(None, [(A + 0x1F4, f(1.9999))])]})
    # ---- 0x823760
    for r in (0, 1):
        add(f'3760/0/{r}', 0x823760, st(0), {0x1BA1C0: [r]})
    for n in (0, 1, 398, 399, 400, 401, 0x8000, 0xFFFF):
        for r in (0, 1):
            add(f'3760/1/{n}/{r}', 0x823760, st(1) + [(A + 0x28, h(n))], {0x1BA1F0: [r]})
    add('3760/1/scribble', 0x823760, st(1) + [(A + 0x28, h(10))],
        {0x1BA1F0: [O.Scribble(1, [(A + 0x28, h(400))])]})
    for state in (2, 3, 4, 0xFF):
        add(f'3760/{state}', 0x823760, st(state))
    # ---- 0x8238C0
    add('38c0', 0x8238C0)
    # ---- 0x8238E0
    for r in (0, 1):
        add(f'38e0/0/{r}', 0x8238E0, st(0) + [(0x28A610, w(0x12345678))], {0x1BA1C0: [r]})
    for name, y in (('400', f(400.)), ('434.9', f(434.9)), ('435-', ulp(435., -1)), ('435', f(435.)),
                    ('436', f(436.))):
        for r in (0, 1, -1):
            add(f'38e0/1/0/{name}/{r}', 0x8238E0, st(1, 0) + [(PLAYER_Y, y)], {0x1B1EA0: [r]})
    for r in (0, 1):
        for flag in (0, 1):
            add(f'38e0/1/1/{r}/{flag}', 0x8238E0, st(1, 1) + [(0x70003B92, b(flag))], {0x1BA1F0: [r]})
    for step in (2, 3):
        add(f'38e0/1/{step}', 0x8238E0, st(1, step))
    for state in (2, 3, 4):
        add(f'38e0/{state}', 0x8238E0, st(state))
    # ---- 0x823B30
    add('3b30/0', 0x823B30, st(0) + [(0x28A5EC, w(0x0BADF00D))])
    add('3b30/1/flag', 0x823B30, st(1), {0x1BA1C0: [1]})
    for name, y in (('674', f(674.)), ('675-', ulp(675., -1)), ('675', f(675.)), ('700', f(700.))):
        for r in (0, 1):
            add(f'3b30/1/0/0/{name}/{r}', 0x823B30, st(1, 0) + [(PLAYER_Y, y)], {0x1B1EA0: [r], 0x1C64F0: [3]})
    for r in (0, 1):
        add(f'3b30/1/0/1/{r}', 0x823B30, st(1, 1) + [(0x28A5E8, w(0x2468ACE0))], {0x1BA1F0: [r], 0x1C64F0: [-3]})
    add('3b30/1/0/2', 0x823B30, st(1, 2))
    for bits in (0, 4, 0xFB, 0xFF):
        add(f'3b30/1/1/0/{bits:x}', 0x823B30, st(1, 0) + [(0x810816, b(1)), (A + 0xB, b(bits))])
    for r in (0, 1):
        add(f'3b30/1/1/1/{r}', 0x823B30, st(1, 1) + [(0x810816, b(1)), (A + 0xB, b(4))], {0x1BA1F0: [r]})
    add('3b30/1/1/2', 0x823B30, st(1, 2) + [(0x810816, b(2))])
    for state in (2, 3, 4):
        add(f'3b30/{state}', 0x823B30, st(state))
    # ---- 0x823E30
    for r in (0, 1):
        for c8c in (0, 1):
            add(f'3e30/0/{r}/{c8c}', 0x823E30, st(0) + [(0x810C8C, b(c8c))], {0x1BA1C0: [r]})
    for v in (0, 1, 2):
        add(f'3e30/1/0/{v}', 0x823E30, st(1, 0) + [(0x810702, b(v))])
    for n in (0, 1, 2, 360, 0xFFFF, 0x8000):
        add(f'3e30/1/1/{n}', 0x823E30, st(1, 1) + [(A + 0x28, h(n))], {0x1BA1F0: [1]})
    for r in (0, 1):
        add(f'3e30/1/2/{r}', 0x823E30, st(1, 2), {0x1BA1F0: [r]})
    add('3e30/1/3', 0x823E30, st(1, 3))
    for state in (2, 3, 4):
        add(f'3e30/{state}', 0x823E30, st(state))
    # ---- 0x825C40
    rec = [(REC + 0x10, fw(100., 200., 300., 1.)), (WORK + 0x20, fw(110., 220., 330., 1.)),
           (0x810360, fw(222., 690., 1180., 1.)), (0x8105E0, fw(1., 2., 3., 1.)), (0x8105D0, fw(4., 5., 6., 1.)),
           (A + 0xB4, f(680.)), (A + 0x38, f(.5))]
    add('5c40/0', 0x825C40, rec + [(REC + 4, b(0))])
    for name, n in (('80', f(80.)), ('2', f(2.)), ('1', f(1.)), ('1+', ulp(1., 1)), ('0', f(0.)), ('-1', f(-1.))):
        add(f'5c40/1/{name}', 0x825C40, rec + [(REC + 4, b(1)), (WORK + 0x10, n)])
    for name, y in (('700', f(700.)), ('810', f(810.)), ('810+', ulp(810., 1)), ('900', f(900.)),
                    ('900+', ulp(900., 1))):
        for sub in (0, 1, 2):
            add(f'5c40/2/{sub}/{name}', 0x825C40, rec + [(REC + 4, b(2)), (REC + 5, b(sub)), (0x810364, y),
                                                         (WORK + 0x10, f(10.))])
    for name, v in (('0', f(0.)), ('1.99', f(1.99)), ('1.995', f(1.995)), ('2', f(2.)), ('2.5', f(2.5))):
        add(f'5c40/2/speed/{name}', 0x825C40, rec + [(REC + 4, b(2)), (A + 0x38, v), (WORK + 0x10, f(180.))])
    for name, y in (('815', f(815.)), ('816-', ulp(816., -1)), ('816', f(816.)), ('900', f(900.))):
        add(f'5c40/2/rise/{name}', 0x825C40, rec + [(REC + 4, b(2)), (0x8105E4, y), (WORK + 0x10, f(180.))])
    for name, n in (('0', f(0.)), ('1', f(1.)), ('89', f(89.)), ('90', f(90.)), ('179', f(179.)),
                    ('180-', ulp(180., -1)), ('180', f(180.)), ('200', f(200.))):
        add(f'5c40/2/ease/{name}', 0x825C40, rec + [(REC + 4, b(2)), (WORK + 0x10, n)])
    add('5c40/2/scribble', 0x825C40, rec + [(REC + 4, b(2)), (WORK + 0x10, f(5.))],
        {0x183010: [O.Scribble(None, [(A + 0x38, f(.75)), (0x8105E4, f(900.)), (WORK + 0x10, f(179.5))])]})
    add('5c40/3', 0x825C40, rec + [(REC + 4, b(3))])
    # ---- 0x825FD0
    for r in (0, 1):
        add(f'5fd0/0/{r}', 0x825FD0, st(0), {0x1B0FD0: [r]})
    for c8c in (0, 1):
        for bits in (0, 4, 0xFB):
            add(f'5fd0/1/0/{c8c}/{bits:x}', 0x825FD0, st(1, 0) + [(0x810C8C, b(c8c)), (A + 0xB, b(bits))])
    for r in (0, 1):
        add(f'5fd0/1/1/{r}', 0x825FD0, st(1, 1), {0x1BA1F0: [r]})
    add('5fd0/1/2', 0x825FD0, st(1, 2))
    add('5fd0/1/3', 0x825FD0, st(1, 3))
    for state in (2, 3, 4):
        add(f'5fd0/{state}', 0x825FD0, st(state))
    # ---- 00191350 / 00230230
    for r in (0, 1, -1, 2):
        add(f'1350/{r}', 0x191350, [(PL + 0xA0, fw(400., 432., 1280., 1.))], {0x1B1EA0: [r]})
    cam = [(CAM + 0xC, f(30.)), (CAM + 0x44, f(.5)), (CAM + 0x5C, f(4.)), (CAM + 0x8C, f(2.)),
           (CAM + 0x10, fw(440., 500., 1300., 1.)), (0x8105E0, fw(400., 432., 1280., 1.)),
           (PL + 0xA0, fw(400., 432., 1280., 1.)), (PL + 0xB4, f(3.)), (PL + 0xC4, f(1.2)), (PL + 0x218, f(-2.)),
           (PL + 0x38, f(1.))]
    add('0230/out', 0x230230, cam + [(PL + 0x230, w(20))], {0x1B1EA0: [0]})
    for name, y in (('300', f(300.)), ('400', f(400.)), ('432', f(432.)), ('450', f(450.)), ('555.4', f(555.4)),
                    ('650', f(650.)), ('700', f(700.)), ('432.0001', ulp(432., 1))):
        for st in (20, 21, 10, 25, 0, 22, -1):
            add(f'0230/{name}/{st}', 0x230230, cam + [(PL + 0xA4, y), (PL + 0x230, w(st))], {0x1B1EA0: [1]})
    for st in (10, 25):
        for name, v in (('0', f(0.)), ('-0', w(0x80000000)), ('1', f(1.)), ('-1', f(-1.)), ('tiny', w(1))):
            for x, z in ((446., 1300.), (447., 1339.), (447., 1340.), (500., 1400.), (447.0001, 1339.9)):
                add(f'0230/{st}/{name}/{x}/{z}', 0x230230,
                    cam + [(PL + 0x230, w(st)), (PL + 0x38, v), (0x8105E0, fw(x - 30., 432., z - 30., 1.)),
                           (CAM + 0x44, f(.9)), (PL + 0xA4, f(500.))], {0x1B1EA0: [1]})
    add('0230/scribble', 0x230230, cam + [(PL + 0x230, w(10))],
        {0x1B1EA0: [1], 0x192010: [O.Scribble(None, [(CAM + 0x10, f(460.)), (CAM + 0x18, f(1200.)),
                                                      (CAM + 0x14, f(77.))])],
         0x191D40: [O.Scribble(None, [(CAM + 0x14, f(66.))])]})
    add('0230/scribble21', 0x230230, cam + [(PL + 0x230, w(21))],
        {0x1B1EA0: [1], 0x191D40: [O.Scribble(None, [(CAM + 0x14, f(66.))])]})
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA14)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(24, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng), beat=rng.choice(BEATS))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area14_overlay.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area14_overlay_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a14_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(' % symbol, header, re.M), symbol


def elf_checks():
    """001F6B00 (0x823760's) is 001F6B30 word for word: the runtime binds it
    to em_effects_live_001F6B30 (as 001F6AD0 and 001F6B60)."""
    elf = O.read_elf()
    at = lambda a: a - 0x100000 + 0x300
    assert elf[at(0x1F6B00):at(0x1F6B30)] == elf[at(0x1F6B30):at(0x1F6B60)], '001F6B00 differs from 001F6B30'


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
    only = os.environ.get('EM_AREA14_OVERLAY_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA14_OVERLAY_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA14_OVERLAY_SOURCE'))
    header_checks()
    elf_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA14_OVERLAY_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA14_OVERLAY_TRACE'):
        trace(os.environ['EM_AREA14_OVERLAY_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA14_OVERLAY_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
