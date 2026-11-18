#!/usr/bin/env python3
"""AREA21's own placement owners and script callbacks
(src/game/em_area21_owners.c) against the ORIGINAL instructions: [42]
0x8261A0; [43] / [44] 0x8262D0 with its halves 0x826310 and 0x8264D0; [45]
0x8266F0 with its steps 0x826840 and 0x8268F0; script 0x82BEC0's op09
callbacks 0x8269A0 and 0x8269F0; [53] 0x827040 (the ending request) with
script 0x82CBD0's op09 callback 0x827250; [58] 0x827CD0 (decomp
src/overlays/AREA21/, byte-identical C; docs/AREA21_OVERLAY.md).

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_level17_boot_reference.py configures it. No AREA21 recording exists:
every entry runs as original code from the user's extract/OVERLAY/
AREA21.BIN, the WHOLE file (header, text and data: the owners pass script
and group addresses and 0x8269F0 reads its two offset tables) loaded over
the arena of the fork recording a03b_03_item2a (its AREA03 overlay text
checked against the user's AREA03.BIN and its boot text against the pinned
ELF). The node, op09 record and work block are designed in zero RAM at
FREE; the flag and counter bytes each owner reads are patched per case.
Every callee is a hook with a scripted result, except the pure helpers in
MATH, which run as original code nested in the oracle (their writes
replayed to the native side); a call from one entry to another (0x8262D0,
0x8266F0) runs the original callee inline, the native side its own
translation. A node's +0x4C method is stubbed and logged. Compared per
case: every call and its arguments, memory at every call entry, the memory
accesses between calls one for one in order, memory after the last store,
the result, a poisoned re-run, coverage of every reachable original word
(DEAD excepted, each with its reason), the callee inventory and the
fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_AREA21_OWNERS_ONLY=<label
prefix> runs a subset (no coverage or contract);
EM_AREA21_OWNERS_SOURCE=<dir> tests another copy of the module source;
EM_AREA21_OWNERS_SHOW=<n> prints n failures; EM_AREA21_OWNERS_MISSING=1
lists unexecuted words; EM_AREA21_OWNERS_TRACE=<label> prints the
original's calls and accesses of matching cases (no instruction text).
Reports hold addresses and counts only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area21_owners as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
FUNCS = {
    0x8261A0: ('em_area21_owners_008261A0', 0x124, [U], None),
    0x8262D0: ('em_area21_owners_008262D0', 0x38, [U], None),
    0x826310: ('em_area21_owners_00826310', 0x1C0, [U], None),
    0x8264D0: ('em_area21_owners_008264D0', 0x218, [U], None),
    0x8266F0: ('em_area21_owners_008266F0', 0x148, [U], None),
    0x826840: ('em_area21_owners_00826840', 0xA8, [U], None),
    0x8268F0: ('em_area21_owners_008268F0', 0xA8, [U], None),
    0x8269A0: ('em_area21_owners_008269A0', 0x48, [], I),
    0x8269F0: ('em_area21_owners_008269F0', 0x1BC, [U, U, U], I),
    0x827040: ('em_area21_owners_00827040', 0x210, [U], None),
    0x827250: ('em_area21_owners_00827250', 0x6C, [], I),
    0x827CD0: ('em_area21_owners_00827CD0', 0x1A0, [U], None),
}
OVERLAY_ID = 0x12
AREA21 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA21.BIN'
BEAT = 'a03b_03_item2a'
DESIGNED = 'designed-a21-owners/' + BEAT   # the capture with AREA21.BIN loaded over its arena
FREE = 0x1E61000             # zero in that recording
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
REC = FREE + 0x2000          # 0x8269F0's op09 record
WORK = FREE + 0x2100         # 0x8269F0's work block
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
# the entries that reach the node's +0x4C method (directly or through a
# translated callee)
SELF_ENTRIES = {0x8262D0, 0x8264D0, 0x8266F0, 0x827040, 0x827CD0}
# the pure helpers run as original code (their writes replayed): the vector
# copy, the matrix-vector product, the vector sum, the float-to-int
MATH = frozenset((0x102948, 0x1026A0, 0x1028B8, 0x1281C0))
DEAD = {}
# flag / counter bytes the owners read
D_0081078C, D_0081078D, D_0081078E = 0x81078C, 0x81078D, 0x81078E
D_0081080C, D_0081080D, D_0081080E, D_0081080F = 0x81080C, 0x81080D, 0x81080E, 0x81080F
D_00810702, D_00810856 = 0x810702, 0x810856


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
    return {0x8269A0: [], 0x827250: [], 0x8269F0: [A, REC, WORK]}.get(entry, [A])


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN', OVERLAY_ID: AREA21}
    O.OUT = ROOT / 'build/area21_owners'
    O.SOURCES = ('em_area21_owners.c',)
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
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    # the whole overlay file over the arena (header, text and data)
    data = AREA21.read_bytes()
    assert data[4] == OVERLAY_ID
    ram, spad = O.CAPTURES[BEAT]
    designed = bytearray(ram)
    designed[O.ARENA:O.ARENA + len(data)] = data
    assert not any(designed[FREE:FREE + 0x10000]), 'the designed-record area is not zero'
    O.CAPTURES[DESIGNED] = (bytes(designed), spad)
    O.image_for = lambda entry: O.CAPTURES[DESIGNED][0]


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 3, -1, 0x12345678, 0x7FFFFFFF, -0x80000000)


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(struct.unpack('<I', struct.pack('<f', x))[0] + d)


def common():
    """The node's method and its +0xD byte."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0xD, b(94))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A + 3, b(c((0, 0, 1, 0x80)))), (A + 4, b(c((0, 1, 1, 1, 2, 3, 4)))), (A + 5, b(c((0, 1, 2, 3, 0xFF)))),
        (A + 0xB, b(c((0, 4, 0xFB)))), (A + 0x28, h(c((0, 1, 829, 830, 0x7FFF, 0xFFFF)))),
        (D_0081078C, b(c((0, 1)))), (D_0081078D, b(c((0, 1)))), (D_0081078E, b(c((0, 0xFF, 0xFE)))),
        (D_0081080D, b(c((0, 1, 2, 3)))), (D_0081080E, b(c((0, 1, 2, 3, 4, 0x10, 0x11, 0xFF, 5)))),
        (D_0081080F, b(c((0, 1, 0xFF)))), (D_00810702, b(c((0, 1, 2, 5, 6)))),
        (REC + 4, b(c((0, 1, 2)))), (WORK + 0xC, f(c((0., 3., 40.)))), (WORK + 0x10, f(c((0., 1., 2., 39., 40.)))),
    ]


def hook_values(rng):
    def ints(*values, n=4): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(0, 1, -1, 3) for a in (0x1BA1C0, 0x1BA1F0, 0x1B0FD0)}
    out[0x1B6660] = ints(*RANDS)
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

    # ---- 0x8261A0
    for r in (0, 1, -1):
        add(f'61a0/0/{r}', 0x8261A0, st(0), {0x1BA1C0: [r]})
    for v in (0, 1, 0xFF):
        add(f'61a0/1/0/{v}', 0x8261A0, st(1, 0) + [(D_0081078C, b(v))])
    for r in (0, 1, -1):
        add(f'61a0/1/1/{r}', 0x8261A0, st(1, 1), {0x1BA1F0: [r]})
    for step in (2, 0xFF):
        add(f'61a0/1/{step}', 0x8261A0, st(1, step) + [(D_0081078C, b(1))])
    for state in (2, 3, 4, 0xFF):
        add(f'61a0/{state}', 0x8261A0, st(state))
    # ---- 0x826310, directly and through 0x8262D0 (+3 0)
    for entry, tag in ((0x826310, '6310'), (0x8262D0, '62d0/0')):
        for r in (0, 1):
            add(f'{tag}/0/{r}', entry, st(0), {0x1BA1C0: [r]})
        for e702 in (0, 1, 2, 5, 6):
            for e80d in (0, 1, 2, 3):
                add(f'{tag}/1/0/{e702}/{e80d}', entry, st(1, 0) + [(D_00810702, b(e702)), (D_0081080D, b(e80d))])
        for e702 in (0, 1, 2, 5, 6):
            add(f'{tag}/1/1/{e702}', entry, st(1, 1) + [(D_00810702, b(e702))])
        for r in (0, 1, -1):
            add(f'{tag}/1/2/{r}', entry, st(1, 2) + [(D_00810856, b(0x77))], {0x1BA1F0: [r], 0x1B6660: [0x12345678]})
        add(f'{tag}/1/3', entry, st(1, 3))
        for state in (2, 3, 4):
            add(f'{tag}/{state}', entry, st(state))
    # a hook that changes D_0081080D / D_00810702 between the rereads cannot
    # run there (no call in step 0); the step-1 arrival entry is read once
    # ---- 0x8264D0, directly and through 0x8262D0 (+3 not 0)
    for entry, tag in ((0x8264D0, '64d0'), (0x8262D0, '62d0/1')):
        third = [] if entry == 0x8264D0 else [(A + 3, b(1))]
        for r in (0, 1):
            add(f'{tag}/0/{r}', entry, third + st(0), {0x1BA1C0: [r]})
        for e78d in (0, 1):
            for bits in (0, 4, 0xFB, 0xFF):
                add(f'{tag}/1/0/{e78d}/{bits:x}', entry, third + st(1, 0) + [(D_0081078D, b(e78d)), (A + 0xB, b(bits))])
        for r in (0, 1):
            add(f'{tag}/1/1/{r}', entry, third + st(1, 1) + [(A + 0xB, b(4))], {0x1BA1F0: [r]})
        add(f'{tag}/1/2', entry, third + st(1, 2))
        add(f'{tag}/1/3', entry, third + st(1, 3))
        for state in (2, 3, 4):
            add(f'{tag}/{state}', entry, third + st(state))
    add('62d0/1/x80', 0x8262D0, [(A + 3, b(0x80))] + st(1, 2))
    # the +0xD byte rewritten by a hook before its second read
    add('64d0/0/scribble', 0x8264D0, st(0), {0x1C63E0: [O.Scribble(None, [(A + 0xD, b(17))])], 0x1BA1C0: [1]})
    add('64d0/1/0/scribble', 0x8264D0, st(1, 0) + [(A + 0xB, b(4))],
        {0x1BA1A0: [O.Scribble(None, [(A + 0xD, b(33)), (A + 5, b(9))])]})
    add('64d0/1/1/scribble', 0x8264D0, st(1, 1),
        {0x1C67E0: [O.Scribble(None, [(A + 0xD, b(55))])], 0x1BA1F0: [1]})
    # ---- 0x8266F0
    for r1 in (0, 1):
        for r2 in (0, 1, -1):
            add(f'66f0/0/{r1}/{r2}', 0x8266F0, st(0), {0x1BA1C0: [r1], 0x1B0FD0: [r2]})
    for v in (0, 1, 2, 3, 4, 5, 0xF, 0x10, 0x11, 0x12, 0xFE, 0xFF):
        for step in (0, 1, 2):
            add(f'66f0/1/{v:x}/{step}', 0x8266F0, st(1, step) + [(D_0081080E, b(v))], {0x1BA1F0: [1]})
    add('66f0/1/1/1/0', 0x8266F0, st(1, 1) + [(D_0081080E, b(1))], {0x1BA1F0: [0]})
    add('66f0/1/2/1/0', 0x8266F0, st(1, 1) + [(D_0081080E, b(2))], {0x1BA1F0: [0]})
    for state in (2, 3, 4):
        add(f'66f0/{state}', 0x8266F0, st(state))
    # ---- 0x826840 / 0x8268F0 directly
    for entry, tag in ((0x826840, '6840'), (0x8268F0, '68f0')):
        for v in (0, 1, 2, 3):
            add(f'{tag}/0/{v}', entry, st(1, 0) + [(D_0081080E, b(v)), (A + 0x2E, h(0x1234))])
        for r in (0, 1, -1):
            add(f'{tag}/1/{r}', entry, st(1, 1) + [(D_0081080E, b(2))], {0x1BA1F0: [r]})
        add(f'{tag}/2', entry, st(1, 2) + [(D_0081080E, b(1))])
    # ---- 0x8269A0 / 0x827250
    for r in RANDS:
        add(f'69a0/{r & 0xFFFFFFFF:x}', 0x8269A0, [], {0x1B6660: [r]})
    add('7250', 0x827250)
    add('7250/scribble', 0x827250, [], {0x15C700: [O.Scribble(None, [(0x810858, f(5.))])]})
    # ---- 0x8269F0
    for step in (0, 1, 2, 0xFF):
        for name, count, t in (('0/0', 0., 0.), ('3/0', 3., 0.), ('3/1', 3., 1.), ('3/2', 3., 2.), ('3/3', 3., 3.),
                               ('40/30', 40., 30.), ('40/31', 40., 31.5), ('3/2-', 3., None)):
            tv = ulp(3., -1) if t is None else f(t)
            add(f'69f0/{step}/{name}', 0x8269F0, [(REC + 4, b(step)), (WORK + 0xC, f(count)), (WORK + 0x10, tv)])
    # 001281C0 scripted (not run): any index, and a count rewritten between
    # its calls (the third call rereads the count)
    for n1, n2, n3 in ((1, 1, 1), (0x21, 0x3F, -1), (-1, 0x7FFFFFFF, 0x20), (2, 5, 7), (3, 0x1E, 0x1F)):
        add(f'69f0/idx/{n1}/{n2}/{n3}', 0x8269F0, [(REC + 4, b(1)), (WORK + 0xC, f(40.)), (WORK + 0x10, f(4.))],
            {0x1281C0: [n1, n2, n3]}, run=MATH - {0x1281C0})
    add('69f0/scribble', 0x8269F0, [(REC + 4, b(1)), (WORK + 0xC, f(40.)), (WORK + 0x10, f(4.))],
        {0x1281C0: [1, O.Scribble(5, [(WORK + 0x10, f(9.))]), 6]}, run=MATH - {0x1281C0})
    # ---- 0x827040
    for r in (0, 1, -1):
        add(f'7040/0/{r}', 0x827040, st(0), {0x1B0FD0: [r]})
    for f80f in (0, 1):
        for r in (0, 1):
            add(f'7040/1/0/{f80f}/{r}', 0x827040, st(1, 0) + [(D_0081080F, b(f80f))],
                {0x1BA1C0: [r], 0x1B6660: [0x2468ACE0]})
        add(f'7040/1/1/{f80f}', 0x827040, st(1, 1) + [(D_0081080F, b(f80f))])
    for r in (0, 1, 2, 3, 4, -1, 0x103):
        add(f'7040/1/2/{r}', 0x827040, st(1, 2) + [(D_0081080F, b(0)), (0x70003B93, b(0x5A))], {0x1BA1F0: [r]})
    add('7040/1/2/0/flag', 0x827040, st(1, 2) + [(D_0081080F, b(1))], {0x1BA1F0: [0]})
    add('7040/1/3', 0x827040, st(1, 3) + [(D_0081080F, b(0xFF))])
    add('7040/1/1/scribble', 0x827040, st(1, 1) + [(D_0081080F, b(1))],
        {0x1D2830: [O.Scribble(None, [(D_0081080F, b(0))])]})
    add('7040/1/2/scribble', 0x827040, st(1, 2) + [(D_0081080F, b(1))],
        {0x1FBC50: [O.Scribble(None, [(D_0081080F, b(0)), (A + 4, b(1))])], 0x1BA1F0: [3]})
    for state in (2, 3, 4):
        add(f'7040/{state}', 0x827040, st(state))
    # ---- 0x827CD0
    for r in (0, 1, -1):
        add(f'7cd0/0/{r}', 0x827CD0, st(0, 7), {0x1B0FD0: [r]})
    for e78e in (0, 0xFE, 0xFF):
        for step in (0, 1, 2, 3):
            for e80e in (2, 3):
                add(f'7cd0/1/{e78e:x}/{step}/{e80e}', 0x827CD0,
                    st(1, step) + [(D_0081078E, b(e78e)), (D_0081080E, b(e80e)), (A + 0x28, h(100))])
    for n in (0, 828, 829, 830, 831, 0x7FFE, 0x7FFF, 0x8000, 0xFFFF):
        add(f'7cd0/1/count/{n}', 0x827CD0, st(1, 1) + [(A + 0x28, h(n))])
    for state in (2, 3, 4):
        add(f'7cd0/{state}', 0x827CD0, st(state))
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA21)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(24, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area21_owners.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area21_owners_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a21w_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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


def frame_sizes():
    """Each entry's first word is its stack adjustment (the frame size the
    composition's area_frame needs); returned per entry (no instruction
    text)."""
    ram = O.image_for(0x8261A0)
    out = {}
    for entry in FUNCS:
        word = O.u32(ram, entry)
        assert word >> 16 == 0x27BD, hex(entry)   # the stack pointer adjusted by an immediate
        out[entry] = -(((word & 0xFFFF) ^ 0x8000) - 0x8000)
    return out


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
    only = os.environ.get('EM_AREA21_OWNERS_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA21_OWNERS_MISSING')):
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
    O.NATIVE = O.build(os.environ.get('EM_AREA21_OWNERS_SOURCE'))
    header_checks()
    entry_guards()
    frames = frame_sizes()
    selected = cases()
    only = os.environ.get('EM_AREA21_OWNERS_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA21_OWNERS_TRACE'):
        trace(os.environ['EM_AREA21_OWNERS_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA21_OWNERS_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    print('frames:', ' '.join(f'{e:06X}={n:#x}' for e, n in sorted(frames.items())))
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
