#!/usr/bin/env python3
"""AREA20's own overlay owners (src/game/em_area20_overlay.c) against the
ORIGINAL instructions: [32] 0x8239B0 with its callees 0x823B00 (the
arrival script 0x826830) and 0x823BE0 (the quad 0x8273B0, script 0x8270B0,
the AREA21 request), the breakables [19]..[31] 0x823D30, and the effect
objects 0x823580, 0x823600, 0x823660, 0x823780, 0x823840 (all
byte-identical decomp C). docs/AREA20_PORT.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area14_overlay_reference.py configures it. No recording of the
designed images has AREA20 resident, so every image is a capture's boot RAM
(k01_02, AREA14's recording; its boot text checked against the pinned
ELF) with the user's extract/OVERLAY/AREA20.BIN written over the overlay
arena, as test_second_half_state_reference places the overlays. Every
callee is a hook with a scripted result, except the pure helpers in MATH,
which run as original code nested in the oracle (their writes replayed to
the native side). A node's +0x4C method is stubbed and logged. The
designed node lives in zero RAM at FREE. Compared per case: every call and
its arguments, memory at every call entry, the memory accesses between
calls one for one in order, memory after the last store, a poisoned
re-run, coverage of every reachable original word, the callee inventory
and the fail-stop contract.

Live nodes (optional): when build/area20_overlay/live/<take>/ holds
eeMemory.bin and scratchpad.bin of an AREA20 state (the guided run's
g73 / g74 end states; `--extract-live` writes them from the takes' end.p2s
and needs the decomp's .venv python for zstandard), every live AREA20 owner
node there runs as recorded too (the overlay text checked against the
file).

EM_TEST_FULL=1 widens the random sample. EM_AREA20_OVERLAY_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA20_OVERLAY_SOURCE=
<dir> tests another copy of the module source; EM_AREA20_OVERLAY_SHOW=<n>
prints n failures; EM_AREA20_OVERLAY_MISSING=1 lists unexecuted words;
EM_AREA20_OVERLAY_TRACE=<label> prints the original's calls and accesses of
matching cases (no instruction text). Reports hold addresses and counts
only.
"""
import ctypes as C
import os
import random
import re
import struct
import sys
from pathlib import Path
import gen_area20_overlay as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
FUNCS = {
    0x823580: ('em_area20_overlay_00823580', 0x74, [U], None),
    0x823600: ('em_area20_overlay_00823600', 0x58, [U], None),
    0x823660: ('em_area20_overlay_00823660', 0x120, [U], None),
    0x823780: ('em_area20_overlay_00823780', 0xB4, [U], None),
    0x823840: ('em_area20_overlay_00823840', 0x14C, [U], None),
    0x8239B0: ('em_area20_overlay_008239B0', 0x150, [U, S], None),
    0x823B00: ('em_area20_overlay_00823B00', 0xD4, [U], None),
    0x823BE0: ('em_area20_overlay_00823BE0', 0x144, [U, S], None),
    0x823D30: ('em_area20_overlay_00823D30', 0x710, [U], None),
}
OVERLAY_ID = 0x11
OVERLAY_FILE = DECOMP / 'extract/OVERLAY/AREA20.BIN'
HOST = 'levels-recordings-20261102-1/s87/route_k01/k01_02_item26'
IMAGE = 'a20/k01_02'
LIVE_ROOT = ROOT / 'build/area20_overlay/live'
TAKES = ('g73_area04_area20', 'g74_area20_tag8')
TAKE_ROOT = DECOMP / 'build/guided_play/_tries'
FREE = 0x1E61000             # zero in the host and in both live states
A = FREE + 0x1000            # the designed node (0x2F0 bytes)
MADE = FREE + 0x3000         # the object a scripted 001EFEB0 returns
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
SELF_ENTRIES = {0x8239B0, 0x823B00, 0x823BE0, 0x823D30}
# the vector / matrix and float helpers run as original code (their writes
# replayed)
MATH = frozenset((0x102948, 0x1029C0, 0x102B08, 0x102C58, 0x102918, 0x11DE90, 0x11E2A8, 0x1B1470))
C0C = 0x81080C               # counter 0x34
B92 = 0x70003B92
B68 = 0x70003B68
B8A = 0x70003B8A


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
    return [A, O.STACK_TOP] if FUNCS[entry][2] == [U, S] else [A]


def extract_live():
    """Write eeMemory.bin / scratchpad.bin of the takes' end states (a
    PCSX2 state is a zip whose members are zstd streams)."""
    import zipfile
    import zstandard
    for take in TAKES:
        state = TAKE_ROOT / take / 'abandoned/end.p2s'
        if not state.exists():
            print('no state', state)
            continue
        z = zipfile.ZipFile(state)
        out = LIVE_ROOT / take
        out.mkdir(parents=True, exist_ok=True)
        for member, name in (('eeMemory.bin', 'eeMemory.bin'), ('Scratchpad.bin', 'scratchpad.bin')):
            info = z.getinfo(member)
            z.fp.seek(info.header_offset)
            head = z.fp.read(30)
            n, e = struct.unpack_from('<HH', head, 26)
            z.fp.seek(info.header_offset + 30 + n + e)
            raw = z.fp.read(info.compress_size)
            (out / name).write_bytes(zstandard.ZstdDecompressor().decompress(raw, max_output_size=info.file_size))
        print('wrote', out)


def live_takes():
    return [t for t in TAKES if (LIVE_ROOT / t / 'eeMemory.bin').exists()]


def load_images(elf):
    """The host's boot RAM with AREA20.BIN over the arena, and the live
    states as recorded (their overlay text equal to the file). Every image's
    boot text equals the pinned ELF and its designed-record area is zero."""
    data = OVERLAY_FILE.read_bytes()
    head = struct.unpack_from('<8I', data)
    assert head[:4] == (0x336F574D, OVERLAY_ID, O.ARENA, 0x2BC0) and len(data) == 0x40 + 0x2BC0 + 0x1E00, head
    text_end = O.ARENA + 0x40 + head[3]
    images = {}
    host = refs.capture(HOST)
    sources = [(IMAGE, host, True)] + [('live/' + t, LIVE_ROOT / t, False) for t in live_takes()]
    for key, folder, place in sources:
        ram = bytearray((folder / 'eeMemory.bin').read_bytes())
        spad = (folder / 'scratchpad.bin').read_bytes()
        assert len(ram) == O.RAM_SIZE and len(spad) == O.SPAD_SIZE, key
        assert ram[0x100000:0x241000] == elf[0x300:0x300 + 0x141000], (key, 'boot text differs from the ELF')
        assert not any(ram[FREE:FREE + 0x40000]), (key, 'the designed-record area is not zero')
        if place:
            ram[O.ARENA:O.ARENA + len(data)] = data
            ram[0x810700] = 0x14
        assert ram[O.ARENA:text_end] == data[:text_end - O.ARENA], (key, 'overlay text differs')
        images[key] = (bytes(ram), spad)
    return images


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS}
    O.OVERLAY_FILES = {OVERLAY_ID: OVERLAY_FILE}
    O.OUT = ROOT / 'build/area20_overlay'
    O.SOURCES = ('em_area20_overlay.c',)
    O.BEATS = []
    O.BEAT_DIRS = {}
    O.RESIDENT = {}
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
    O.image_for = lambda entry: O.CAPTURES[IMAGE][0]
    O.CAPTURES.clear()
    O.CAPTURES.update(load_images(O.read_elf()))


fw, b, h, w, f = O.fw, O.b8, O.h16, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x40000000, -0x80000000, 0x000003F0, 0x5A5A5A5A, 0x00000210)


def ulp(x, d):
    """The float d units in the last place from x, as bytes."""
    return w(struct.unpack('<I', struct.pack('<f', x))[0] + d)


def common():
    """The node's method, its +0xD byte, a position and rotation, a player."""
    return [(A + 0x4C, w(CALLBACK)), (A + 0xD, b(32)), (A + 0xB0, fw(224., 85., 238., 1.)),
            (A + 0xC0, fw(.25, 1.5, -.5, 0.)), (0x810350, fw(160., 92., 267., 1.))]


def fields(rng):
    """The fields the entries branch on, each a random interesting value."""
    c = rng.choice
    return [
        (A, b(c((0, 1, 2)))), (A + 3, b(c((10, 12, 0x18, 0x2A, 0, 11)))), (A + 4, b(c((0, 1, 1, 1, 2, 2, 3, 4)))),
        (A + 5, b(c((0, 1, 2, 3)))), (A + 0x28, h(c((0, 1, 2, 0xFFFF)))), (A + 0x36, h(c((0, 1, 0x8000)))),
        (A + 0x38, f(c((0., 1., 2.5)))), (A + 0x94, h(c((10, 11, 12, 13)))), (A + 0x1F0, f(c((0., 1.19, 1.99, 2.)))),
        (A + 0x264, f(c((0., 1., -2.)))), (A + 0x268, f(c((.5, 0., -.03, -3.99, -4.)))),
        (C0C, b(c((0, 1, 2)))), (B92, b(c((0, 1)))), (B68, w(c((0, 5, 25, 64, 7)))), (B8A, h(c((0, 1, 0xFFC0)))),
    ]


def hook_values(rng):
    def ints(*values, n=4): return [rng.choice(values) for _ in range(n)]
    out = {a: ints(0, 1, -1, 2) for a in (0x1BA1C0, 0x1BA1F0, 0x1B1EA0, 0x1B0FD0, 0x19A570, 0x19AB20, 0x19AD00)}
    out[0x122BB8] = ints(*RANDS)
    out[0x1C6190] = ints(0, 150, 151, 449, 450, -1)
    out[0x1EFEB0] = ints(0, MADE)
    out[0x1C64F0] = ints(0, 7, -2)
    return out


def cases():
    out, labels = [], set()
    base = common()

    def add(label, e, patches=(), queues=None, run=MATH, args=None, image=IMAGE):
        assert label not in labels, label
        labels.add(label)
        all_patches = (base if args is None else []) + list(patches)
        out.append((label, image, e, tuple(args if args is not None else O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    def st(state, step=0):
        return [(A + 4, b(state)), (A + 5, b(step))]

    # ---- every live AREA20 owner node of the extracted states, as recorded
    for take in live_takes():
        key = 'live/' + take
        ram = O.CAPTURES[key][0]
        for entry in FUNCS:
            for node in O.owners(ram, entry):
                args = [node, O.STACK_TOP] if FUNCS[entry][2] == [U, S] else [node]
                add(f'live/{take}/{entry:x}/{node:x}', entry, args=args, image=key)
    # ---- the effect objects
    for e in (0x823580, 0x823600):
        for state in (0, 1, 2, 3, 4):
            add(f'{e:x}/{state}', e, st(state))
    for r in RANDS:
        add(f'3660/0/{r & 0xFFFFFFFF:x}', 0x823660, st(0), {0x122BB8: [r]})
        add(f'3840/0/{r & 0xFFFFFFFF:x}', 0x823840, st(0), {0x122BB8: [r]})
    for name, v in (('0', f(0.)), ('1.97', f(1.97)), ('1.98', f(1.98)), ('2-', ulp(2., -1)), ('2', f(2.)),
                    ('3', f(3.)), ('-1', f(-1.))):
        add(f'3660/1/{name}', 0x823660, st(1) + [(A + 0x1F0, v), (A + 0x1F4, f(.375))])
    for name, v in (('0', f(0.)), ('1.16', f(1.16)), ('1.17', f(1.17)), ('1.2-', ulp(1.2, -1)), ('1.2', f(1.2)),
                    ('2', f(2.))):
        for kind in (10, 11, 12, 13, 0, -1):
            add(f'3840/1/{name}/{kind}', 0x823840, st(1) + [(A + 0x1F0, v), (A + 0x94, h(kind))])
    for e in (0x823660, 0x823840):
        for state in (2, 3, 4):
            add(f'{e:x}/{state}', e, st(state))
    for v in (0, 150, 151, 449, 450, -1):
        for frame in (0, 5, 25, 10, -5, 7, -25, 50):
            for made in (0, MADE):
                add(f'3780/1/{v}/{frame}/{made:x}', 0x823780, st(1) + [(B68, w(frame)), (A + 0x94, h(0x1234)),
                                                                      (MADE + 0x94, h(0x5A5A))],
                    {0x1C6190: [v], 0x1EFEB0: [made]})
    for state in (0, 2, 3, 4):
        add(f'3780/{state}', 0x823780, st(state))
    add('3780/scribble', 0x823780, st(1) + [(B68, w(10))],
        {0x1C6190: [O.Scribble(0, [(B68, w(11))])],
         0x1EFEB0: [O.Scribble(MADE, [(A + 0x94, h(0x7777))])]})
    # ---- [32] and its two callees
    for r in (0, 1, -1):
        add(f'39b0/0/{r}', 0x8239B0, st(0) + [(C0C, b(1))], {0x1BA1C0: [r]})
    for c0c in (0, 1, 2, 0xFF):
        for step in (0, 1, 2, 3):
            for r in (0, 1, -1):
                add(f'39b0/1/{c0c}/{step}/{r}', 0x8239B0, st(1, step) + [(C0C, b(c0c))],
                    {0x1BA1F0: [r], 0x1B1EA0: [r], 0x1C64F0: [3]})
    for state in (2, 3, 4, 0xFF):
        add(f'39b0/{state}', 0x8239B0, st(state))
    add('39b0/scribble', 0x8239B0, st(1, 1) + [(C0C, b(0))],
        {0x1BA1F0: [O.Scribble(1, [(C0C, b(5)), (A + 0xB4, f(-7.))])]})
    for step in (0, 1, 2, 3):
        for r in (0, 1, -1):
            add(f'3b00/{step}/{r}', 0x823B00, st(1, step), {0x1BA1F0: [r], 0x1C64F0: [-1]})
    for r in (0, 1, 2, -1):
        add(f'3be0/0/{r}', 0x823BE0, st(1, 0), {0x1B1EA0: [r]})
    for r in (0, 1):
        for v in (0, 0x1234, -2):
            add(f'3be0/1/{r}/{v}', 0x823BE0, st(1, 1), {0x1BA1F0: [r], 0x1C64F0: [v]})
    for step in (2, 3):
        add(f'3be0/{step}', 0x823BE0, st(1, step))
    add('3be0/scribble', 0x823BE0, st(1, 0),
        {0x1B1EA0: [O.Scribble(1, [(0x8273B0, fw(1., 2., 3., 4.)), (A + 5, b(2))])]})
    # ---- the breakables
    for r in (0, 1, -1):
        add(f'3d30/0/{r}', 0x823D30, st(0), {0x1B0FD0: [r]})
    for shot in (0, 1, 0x8000):
        for hit in (0, 1):
            for flag in (0, 1):
                for c0c in (0, 2):
                    add(f'3d30/1/{shot}/{hit}/{flag}/{c0c}', 0x823D30,
                        st(1) + [(A + 0x36, h(shot)), (B92, b(flag)), (C0C, b(c0c))], {0x19A570: [hit]})
    for kind in (10, 12, 0x18, 0x2A, 0, 11):
        for r in RANDS:
            add(f'3d30/2/0/{kind}/{r & 0xFFFFFFFF:x}', 0x823D30, st(2, 0) + [(A + 3, b(kind))],
                {0x122BB8: [r, r ^ 0x100, r ^ 0x200]})
    for kind in (10, 0x18, 0x2A):
        for n in (1, 2, 0, 0x8000):
            add(f'3d30/2/1/{kind}/{n}', 0x823D30, st(2, 1) + [(A + 3, b(kind)), (A + 0x28, h(n)),
                                                             (A + 0x264, f(-1.25))])
    for name, lift in (('0.5', f(.5)), ('0.06', f(.06)), ('0.059', f(.059)), ('0', f(0.)), ('-0.03', f(-.03)),
                       ('-3.94', f(-3.94)), ('-3.95', f(-3.95)), ('-4', f(-4.)), ('-9', f(-9.))):
        for speed in (0., 1.5):
            for wall in (0, 1):
                for land in (0, 1):
                    add(f'3d30/2/2/{name}/{speed}/{wall}/{land}', 0x823D30,
                        st(2, 2) + [(A + 0x268, lift), (A + 0x38, f(speed)), (A + 0x264, f(.7)), (B68, w(64))],
                        {0x19AD00: [wall], 0x19AB20: [land]})
    for frame, extra in ((63, 1), (64, 0), (0, 0x40), (1, 0xFFFF), (5, 0)):
        add(f'3d30/2/2/frame/{frame}/{extra}', 0x823D30,
            st(2, 2) + [(A + 0x268, f(-1.)), (A + 0x38, f(0.)), (B68, w(frame)), (B8A, h(extra))])
    add('3d30/2/3', 0x823D30, st(2, 3))
    for state in (3, 4):
        add(f'3d30/{state}', 0x823D30, st(state))
    add('3d30/2/2/scribble', 0x823D30, st(2, 2) + [(A + 0x268, f(-1.)), (A + 0x38, f(2.))],
        {0x19AD00: [O.Scribble(0, [(A + 0x38, f(0.)), (A + 0x264, f(3.))])],
         0x19AB20: [O.Scribble(0, [(A + 0xB4, f(50.)), (B68, w(63))])]})
    add('3d30/1/scribble', 0x823D30, st(1) + [(A + 0x36, h(1))],
        {0x19A570: [O.Scribble(1, [(A + 0xC0, f(2.)), (A + 0xB0, f(-3.))])]})
    # ---- a seeded random sample over every entry
    rng = random.Random(0xA20)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(24, 3)):
            add(f'random/{entry:06X}/{n}', entry, fields(rng), hook_values(rng))
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area20_overlay.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area20_overlay_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a20_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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
    only = os.environ.get('EM_AREA20_OVERLAY_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen
        print(f'{entry:08X}: {len(words & seen)}/{len(words)} reachable words')
        if missing and (not only or os.environ.get('EM_AREA20_OVERLAY_MISSING')):
            failures.append(('coverage', [hex(entry), [hex(m) for m in sorted(missing)]]))
    if not only:
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
    if '--extract-live' in sys.argv[1:]:
        extract_live()
        return
    configure()
    O.NATIVE = O.build(os.environ.get('EM_AREA20_OVERLAY_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA20_OVERLAY_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA20_OVERLAY_TRACE'):
        trace(os.environ['EM_AREA20_OVERLAY_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA20_OVERLAY_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    live = len([c for c in selected if c[0].startswith('live/')])
    O.reference_mode.banner(f'{len(selected)} cases ({live} live nodes)')
    print(f'PASS {len(selected)} cases ({live} live nodes): calls, ordered accesses, whole memory, poisoned starts '
          f'and fail-stop')


if __name__ == '__main__':
    main()
