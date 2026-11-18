#!/usr/bin/env python3
"""AREA16's water grid (src/game/em_area16_water.c) against the ORIGINAL
instructions: 001DAFA0, 001DB240, 001DB250, 001DB040 and 001DB480 with its
vertex emit 001DAE10 and vertex record 001DAC90 (these two run nested in
001DB480's cases, as original code at the top level of the oracle; their
words count in the coverage). Lane PORT-WATER, docs/AREA16_PORT.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_area17_creature_reference.py configures it, over Claude's guided-run
captures of AREA16 sub 0 (decomp build/fork_refs/guided-20261117/s87/
route_g16: g60 at the arrival, the grid at stage 0; g61 after the water
barrier, stage 6, the shade words eased to 0x81): before any case the
test checks that each image's overlay is AREA16 with its text equal to the
user's extract/OVERLAY/AREA16.BIN and that its boot text equals the pinned
ELF, so every executed instruction is original. Every callee is a hook
with a scripted result, except the pure helpers in MATH, which run as
original code nested in the oracle (their writes replayed to the native
side). The designed render context, matrices and packet buffer live in
zero RAM at FREE. Compared per case: every call and its arguments, memory
at every call entry, the memory accesses between calls one for one in
order, memory after the last store, a poisoned re-run, coverage of every
reachable original word (001DAE10 and 001DAC90 included), the callee
inventory and the fail-stop contract. 001DB250's jump-table load is left
out of the access comparison (TABLE_BYTES): the translation is a switch.

EM_TEST_FULL=1 adds random grids and stages. EM_AREA16_WATER_ONLY=<label
prefix> runs a subset (no coverage or contract); EM_AREA16_WATER_SOURCE=
<dir> tests another copy of the module source (the mutation check);
EM_AREA16_WATER_SHOW=<n> prints n failures; EM_AREA16_WATER_MISSING=1
lists unexecuted words. Reports hold addresses and counts only.
"""
import ctypes as C
import os
import random
import re
import struct
from pathlib import Path
import gen_area16_water as G
import test_level14_port_reference as O
import test_area17_creature_reference as A17
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
FUNCS = {
    0x001DAFA0: ('em_area16_water_001DAFA0', 0xA0, [], None),
    0x001DB240: ('em_area16_water_001DB240', 0xC, [I], None),
    0x001DB040: ('em_area16_water_001DB040', 0x200, [U], None),
    0x001DB250: ('em_area16_water_001DB250', 0x228, [], None),
    0x001DB480: ('em_area16_water_001DB480', 0x378, [S], None),
}
# translated functions reached only from 001DB480 (no entry of their own:
# 001DAE10 reads the VU0 registers vf24..vf27 001DB480 loads)
NESTED = {0x001DAE10: 0x18C, 0x001DAC90: 0x180}
SET = 'guided-20261117/s87/route_g16/'
BEATS = ['g60_area06_area16', 'g61_area16_water']
G60, G61 = BEATS
FREE = 0x1E61000
CTX = FREE + 0x1000           # a designed render context (0x275670 points here)
PKT = FREE + 0x10000          # its packet cursor
MAT = FREE + 0x8000           # a designed VU0 matrix (001CD370's result)
GRID2 = FREE + 0x30000        # a designed grid for 001DB040 (0x4040 bytes)
GRID, BLOCK, STAGE = 0x818000, 0x81C000, 0x81C011
SCREEN = 0x70003AC0           # 001DAE10's screen matrix (scratchpad)
MATH = frozenset((0x1026A0, 0x102948, 0x1281C0, 0x1CD370))
JUMPS = {0x1DB250: (0x26E6D0, 7)}
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


def jump_tables(ram):
    out = {}
    for entry, (table, count) in JUMPS.items():
        size = FUNCS[entry][1]
        sites = [pc for pc in range(entry, entry + size, 4)
                 if (O.u32(ram, pc) & 0xFC1FFFFF) == 8 and (O.u32(ram, pc) >> 21 & 31) != 31]
        assert len(sites) == 1, hex(entry)
        out[sites[0]] = (table, count)
    return out


def default_args(entry):
    return {0x1DB240: [1], 0x1DB040: [GRID], 0x1DB480: [O.STACK_TOP]}.get(entry, [])


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {13: ROOT.parent / 'Extermination/extract/OVERLAY/AREA16.BIN'}
    O.OUT = ROOT / 'build/levels/area16_water'
    O.SOURCES = ('em_area16_water.c',)
    O.BEATS = list(BEATS)
    O.BEAT_DIRS = {b: refs.capture(SET + b) for b in BEATS}
    O.RESIDENT = {b: 13 for b in BEATS}
    O.BY_ADDRESS = {a: (n, args, res) for n, a, args, res in HOOKS}
    O.BY_NAME = {n: a for n, a, _, _ in HOOKS}
    O.HOOK_NAMES = [h[0] for h in HOOKS]
    O.Hooks = type('Hooks', (C.Structure,), {'_fields_':
        [('ctx', C.c_void_p), ('bytes', O.BYTES_FN)] +
        [(name, O.hook_proto(args, res)) for name, _, args, res in HOOKS]})
    O.SELF_ENTRIES = set()
    O.HEAVY = {0x1DB480}
    O.RETURN_SITES = O.RETURN_SITES_FULL = ()
    O.CONTRACT_UNREACHABLE = {}
    O.LOCAL_STORES = set()
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[G60][0]
    O.Replay = A17.FastReplay
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    O.JUMP_TABLES = jump_tables(O.CAPTURES[G60][0])
    O.TABLE_BYTES = frozenset(a for table, n in O.JUMP_TABLES.values() for a in range(table, table + 4 * n))
    for beat in BEATS:
        assert not any(O.CAPTURES[beat][0][FREE:FREE + 0x40000]), 'the designed area is not zero in the capture'


fw, b, w, f = O.fw, O.b8, O.w32, O.f32b
RANDS = (0, 1, 0x7FFFFFFF, -1, 0x12345678, 0x001F0000, -0x10000, -0x1F0000, 0x80000000, 0x00200000,
         0x5A5A5A5A, 0x000F8000)


def designed_context(cursor=PKT):
    """0x275670 -> CTX, its cursor, its matrix slot 3 (001CD370(3) = ctx +
    0x2300) and the screen matrix."""
    return [(0x275670, w(CTX)), (CTX + 0x1C, w(cursor)), (CTX + 0x2300, fw(1., 0., 0., 0., 0., 1., 0., 0.,
                                                                          0., 0., 1., 0., -306., -184., -460., 10.)),
            (SCREEN, fw(100., 0., 0., 0., 0., 100., 0., 0., 0., 0., 1., 1., 2048., 2048., 0., 400.))]


def screens(k):
    """001DAE10's screen points for the designed case, cycled: inside,
    w < 0, x < 0, x > 65520, y < 0, y > 65520, the two edges, x NaN-free
    extremes, and w 0 (16 / 0)."""
    table = [(100., 200., 1., 16.), (100., 200., 1., -16.), (-1., 200., 1., 16.), (65521., 2., 1., 16.),
             (5., -1., 1., 16.), (5., 65521., 1., 16.), (0., 0., 1., 16.), (65520., 65520., 2., 16.),
             (4096., 70000., 3., 16.), (-0., 5., 1., 16.), (100., 200., 1., 0.), (3e38, 1., 1., 16.)]
    return table[k % len(table)]


def scribble_screen(k):
    def make(e):
        return O.Scribble(0, [(e.r[4] & O.MASK, fw(*screens(k)))])
    return make


def cases():
    out, labels = [], set()

    def add(label, beat, e, patches=(), queues=None, run=MATH, args=None):
        assert label not in labels, label
        labels.add(label)
        out.append((label, beat, e, tuple(args if args is not None else O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in patches if a < 0x70000000],
                    [(a, v) for a, v in patches if a >= 0x70000000], O.Script(queues), set(run)))

    # ---- 001DAFA0 / 001DB240
    for beat in BEATS:
        add(f'afa0/{beat[:3]}', beat, 0x1DAFA0)
    for value in (0, 1, 3, 6, 0xFF, -1, 0x1234):
        add(f'b240/{value:x}', G60, 0x1DB240, args=[value])
    # ---- 001DB040: the live grids, the random cell's sign, a designed grid
    for beat in BEATS:
        for r1, r2 in ((0, 0), (0x7FFFFFFF, -1), (-0x1F0000, 0x001F0000), (0x80000000, 0x12345678)):
            add(f'b040/{beat[:3]}/{r1 & 0xFFFFFFFF:x}/{r2 & 0xFFFFFFFF:x}', beat, 0x1DB040, (), {0x122BB8: [r1, r2]})
    rng = random.Random(0xB040)
    grid = []
    for i in range(32):
        for j in range(32):
            h = rng.choice((0., 69.9, 70., 70.1, -3., 1e-39, 3e38, -3e38, rng.uniform(-80, 80)))
            grid.append(fw(rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-5, 5), h))
    designed = [(GRID2 + 0x10 * k, v) for k, v in enumerate(grid)]
    designed.append((GRID2 + 0x4000, fw(.991, .0176, .05, .2)))
    add('b040/designed', G60, 0x1DB040, designed, {0x122BB8: [0x00050000, 0x00090000]}, args=[GRID2])
    # ---- 001DB250: every stage, out of range, stage 5's threshold
    for beat in BEATS:
        for stage in (0, 1, 2, 3, 4, 5, 6, 7, 0xFF):
            add(f'b250/{beat[:3]}/{stage}', beat, 0x1DB250, [(STAGE, b(stage))], {0x122BB8: [0x00030000, 0x00110000]})
    for impulse in (.505, .6, .5050505, -1.):
        add(f'b250/5/{impulse}', G60, 0x1DB250, [(STAGE, b(5)), (BLOCK + 0xC, f(impulse))])
    for words in ((0, 0xC0, 0x100, -5), (0x71, 0x53, 0x71, 0)):
        add(f'b250/ease/{words[0]:x}', G61, 0x1DB250, [(STAGE, b(3)), (BLOCK + 0x30, struct.pack('<4i', *words))])
    # ---- 001DB480 (with 001DAE10 / 001DAC90): the captured context and
    # matrices; then the designed context with scripted screen points
    # (g60's fog word is 0: 001DAC90's computed fog; g61's is 0x81, kept)
    add('b480/g60/live', G60, 0x1DB480)
    if O.reference_mode.FULL:
        add('b480/g61/live', G61, 0x1DB480)
    queues = {0x1026A0: [scribble_screen(k) for k in range(31 * 64)]}
    add('b480/designed', G61, 0x1DB480, designed_context(), queues, run=MATH - {0x1026A0})
    if O.reference_mode.FULL:
        rng = random.Random(0x1DB480)
        for n in range(4):
            beat = BEATS[n & 1]
            patches = designed_context() + [(BLOCK + 0x3C, w(rng.choice((0, 0x81, 0xC0)))),
                                             (BLOCK, f(rng.choice((.991, .9249, 2.))))]
            for k in range(0, 1024, rng.choice((7, 13))):
                patches.append((GRID + 0x10 * k, fw(rng.uniform(-3, 3), rng.uniform(-3, 3), 0.,
                                                    rng.uniform(-90, 90))))
            add(f'b480/random/{n}', beat, 0x1DB480, patches)
        for n in range(16):
            beat = BEATS[n & 1]
            add(f'b250/random/{n}', beat, 0x1DB250,
                [(STAGE, b(rng.choice(range(9)))), (BLOCK + 0xC, f(rng.uniform(-1, 3)))],
                {0x122BB8: [rng.choice(RANDS), rng.choice(RANDS)]})
            add(f'b040/random/{n}', beat, 0x1DB040, (), {0x122BB8: [rng.choice(RANDS), rng.choice(RANDS)]})
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area16_water.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area16_water_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a16w_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
        assert len(bodies) == 1 and f'o->h->{name}(' in bodies[0] and f'0x{address:08X}u' in bodies[0]
    for entry, (symbol, _, kinds, result) in FUNCS.items():
        assert re.search(r'^int %s\(.*\);$' % symbol, header, re.M), symbol


def entry_guards():
    """A latched fault, a NULL hook table and a NULL fault pointer: every
    entry refuses with no hook or memory call."""
    touched = []

    def refuse(*_):
        touched.append(1)
        return -1
    keep = [O.BYTES_FN(refuse)]
    fields_ = {'ctx': None, 'bytes': keep[0]}
    for name, _, args, result in HOOKS:
        fields_[name] = O.hook_proto(args, result)(refuse)
        keep.append(fields_[name])
    hooks = O.Hooks(**fields_)
    for entry, (symbol, _, _, _) in FUNCS.items():
        for table, fault in ((C.byref(hooks), O.Fault(0x1234, 7)), (None, O.Fault()), (C.byref(hooks), None)):
            touched.clear()
            args = [table] + O.native_args(entry, O.DEFAULT_ARGS[entry])
            rc = getattr(O.NATIVE, symbol)(*args, C.byref(fault) if fault is not None else None)
            assert rc == -1 and not touched, symbol
            if fault is not None:
                assert (fault.address, fault.code) == ((0x1234, 7) if table is not None else (0, 0))


def callee_checks():
    """Every jal the static walk of an entry or nested function reaches is
    a hook, an entry or a nested translation."""
    problems = []
    ram = O.CAPTURES[G60][0]
    known = set(O.BY_ADDRESS) | set(FUNCS) | set(NESTED)
    sizes = {e: v[1] for e, v in FUNCS.items()}
    sizes.update(NESTED)
    for entry, size in sorted(sizes.items()):
        words = O.reachable_words(ram, entry, size)
        for pc in sorted(words | {p - 4 for p in words}):
            if not entry <= pc < entry + size:
                continue
            word = O.u32(ram, pc)
            if word >> 26 in (2, 3):
                target = (pc & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
                if word >> 26 == 2 and entry <= target < entry + size:
                    continue
                if target not in known:
                    problems.append('%06X calls %06X (not a hook, entry or nested translation)' % (entry, target))
    return sorted(set(problems))


def contract_site(site):
    """O.hook_contract_site; on a 001DB480 case (about 8000 calls) its
    sampled form also under EM_TEST_FULL=1."""
    heavy = O.CASES[site[0]][2] in O.HEAVY
    keep = O.reference_mode.FULL
    try:
        if heavy:
            O.reference_mode.FULL = False
        return O.hook_contract_site(site)
    finally:
        O.reference_mode.FULL = keep


def contract_sites(results):
    """O.hook_sites. The quick run leaves 001DB480's cases out of the
    fail-stop contract (each of its native runs makes about 8000 hook calls
    and 100000 memory requests; its contract alone is some 20 s): the hooks
    only 001DB480 reaches and its own refusals are contracted under
    EM_TEST_FULL=1. The wrappers are generated alike (tools/gen_area16_water.py)
    and the other entries' sites contract the same design."""
    if O.reference_mode.FULL:
        return O.hook_sites(results)
    light = [r if r[1] not in O.HEAVY else r[:5] + (None,) for r in results]
    reached = set().union(*(set(r[5]) for r in light if r[5] is not None and not r[2]))
    keep = O.CONTRACT_UNREACHABLE
    O.CONTRACT_UNREACHABLE = {name: 'only 001DB480 (EM_TEST_FULL=1)' for name in O.HOOK_NAMES if name not in reached}
    try:
        sites, missing = O.hook_sites(light)
    finally:
        O.CONTRACT_UNREACHABLE = keep
    return sites, [m for m in missing if not (isinstance(m, tuple) and m[1] in O.HEAVY)]


def judge(results):
    only = os.environ.get('EM_AREA16_WATER_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    ram = O.CAPTURES[G60][0]
    sizes = {e: v[1] for e, v in FUNCS.items()}
    sizes.update(NESTED)
    for entry, size in sorted(sizes.items()):
        words = O.reachable_words(ram, entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA16_WATER_MISSING')):
            failures.append(('coverage', [hex(entry), [hex(m) for m in sorted(missing)]]))
    if not only:
        failures += [('callee inventory', p) for p in callee_checks()]
        if not failures:
            sites, missing = contract_sites(results)
            if missing:
                failures.append(('unreached hooks', missing))
            for problems, runs in O.reference_mode.parallel_map(contract_site, sites):
                if problems:
                    failures.append(('fail-stop contract', problems))
    return failures


def main():
    configure()
    O.NATIVE = O.build(os.environ.get('EM_AREA16_WATER_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_AREA16_WATER_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA16_WATER_SHOW', '8'))]:
        print('FAIL', label, str(errors)[:2000])
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
