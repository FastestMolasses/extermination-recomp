#!/usr/bin/env python3
"""AREA16's own overlay owners (src/game/em_area16_overlay*.c) against the
ORIGINAL instructions: every behaviour its placements and groups name
(0x823580, 0x824440, 0x824690, 0x824E10, 0x825310, 0x825800, 0x8261A0,
0x8263D0, 0x826C50, 0x827030, 0x8273F0, 0x827B70, 0x827CE0, 0x827FA0), the
two children they spawn (0x825260, 0x828200), the scripts' op09 callbacks
(0x825630, 0x826C20) and the overlay helpers the owners call (0x824540,
0x825660, 0x825670, 0x8256E0, 0x825730, 0x825780, 0x826A50) and the five
effect behaviours AREA16's effect ids 0..4 select (0x8237A0, 0x8239B0,
0x823D20, 0x823F70, 0x824110). docs/AREA16_PORT.md.

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_level17_boot_reference.py configures it for AREA21's twins: no AREA16
capture exists, so the user's extract/OVERLAY/AREA16.BIN (header, text and
data) is loaded over the arena of capture a03b_03_item2a (its AREA03 text
and boot text first checked against the user's files) and the start of
AREA16's BSS is zeroed; only those words run. Every callee is a hook with a
scripted result, except the pure helpers in MATH, which run as original
code nested in the oracle (their writes replayed to the native side). A
node's +0x4C method is stubbed and logged. The designed nodes and records
live in zero RAM at FREE. Compared per case: every call and its arguments,
memory at every call entry, the memory accesses between calls one for one
in order, memory after the last store, the result, a poisoned re-run,
coverage of every reachable original word (DEAD excepted, each with its
reason), the callee inventory and the fail-stop contract.

The cases live in tools/area16_overlay_cases_*.py, one module per group of
entries (each defines SOURCE, its module file under src/game; FUNCS; DEAD;
SELF_ENTRIES; cases(add, K); optionally LOCAL_STORES, the pcs where a
callee-saved register is stored as a frame local, and extra_checks).

EM_TEST_FULL=1 widens the random samples. EM_AREA16_OVERLAY_ONLY=<label
prefix> runs a subset (no coverage or contract);
EM_AREA16_OVERLAY_SOURCE=<dir> tests another copy of the module source;
EM_AREA16_OVERLAY_SHOW=<n> prints n failures; EM_AREA16_OVERLAY_MISSING=1
lists unexecuted words; EM_AREA16_OVERLAY_TRACE=<label> prints the
original's calls and accesses of matching cases (no instruction text);
EM_AREA16_OVERLAY_GROUPS=a,b runs only those case modules.
Reports hold addresses and counts only.
"""
import ctypes as C
import importlib
import os
import re
import struct
import types
from pathlib import Path
import gen_area16_overlay as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
OVERLAY_ID = 0x0D
AREA16 = ROOT.parent / 'Extermination/extract/OVERLAY/AREA16.BIN'
BEAT = 'a03b_03_item2a'
DESIGNED = 'designed-a16/' + BEAT   # the capture with AREA16's file loaded over its arena
FREE = 0x1E61000                    # zero in the capture
A = FREE + 0x1000                   # the designed node (0x300 bytes)
B = FREE + 0x2000                   # a second node (a parent, a target, a child)
REC = FREE + 0x3000                 # an op09 record / a spare record
WORK = FREE + 0x3100                # a script work block
SPAWN = FREE + 0x4000               # nodes a spawn hook returns (0x400 apart)
CALLBACK = 0x1CAA00                 # a +0x4C method word (stubbed and logged)
BSS = 0x82C500                      # AREA16's BSS (runtime); its first 0x1000 bytes are zeroed
GROUPS = ('a', 'b', 'c', 'd', 'e', 'f')
# the pure helpers run as original code (their writes replayed): vector
# copies, differences and scales, the matrix identity / product /
# translation / rotation, the sine, the colour byte 001281C0
MATH = frozenset((0x1026A0, 0x1026D0, 0x102918, 0x102948, 0x102958, 0x1029C0, 0x102B08, 0x102BB0,
                  0x102C58, 0x103230, 0x11E2A8, 0x1281C0))


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


def group_modules():
    names = os.environ.get('EM_AREA16_OVERLAY_GROUPS')
    wanted = names.split(',') if names else GROUPS
    out = []
    for g in wanted:
        try:
            out.append(importlib.import_module(f'area16_overlay_cases_{g}'))
        except ModuleNotFoundError as e:
            if e.name != f'area16_overlay_cases_{g}':
                raise
    return out


MODULES = group_modules()
FUNCS, DEAD = {}, {}
for _m in MODULES:
    assert not set(_m.FUNCS) & set(FUNCS), _m.__name__
    FUNCS.update(_m.FUNCS)
    DEAD.update(getattr(_m, 'DEAD', {}))
SELF_ENTRIES = set().union(*(getattr(m, 'SELF_ENTRIES', set()) for m in MODULES)) if MODULES else set()


def default_args(entry):
    kinds = FUNCS[entry][2]
    if kinds[:3] == [U, U, U] and FUNCS[entry][3] is not None:   # an op09 callback
        return [A, REC, WORK]
    out, words = [], 0
    for k in kinds:
        if k == S:
            out.append(O.STACK_TOP)
        elif k == F:
            out.append(O.fb(1.0))
        else:
            out.append(A if words == 0 else (B if k == U else 0))
            words += 1
    return out


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {e: OVERLAY_ID for e in FUNCS if e >= O.ARENA}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN', OVERLAY_ID: AREA16}
    # a private library per group selection (lanes may run subsets at once)
    O.OUT = ROOT / 'build/area16_overlay' / '-'.join(m.__name__[-1] for m in MODULES)
    O.SOURCES = tuple(m.SOURCE for m in MODULES)
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
    O.LOCAL_STORES = set().union(*(getattr(m, 'LOCAL_STORES', set()) for m in MODULES))
    O.JUMP_TABLES = {}
    O.TABLE_BYTES = frozenset()
    O.DEFAULT_ARGS = {e: default_args(e) for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[DESIGNED][0]
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    # no AREA16 capture exists: the overlay file (header, text, data) is
    # loaded over the capture's arena, and the start of its BSS zeroed
    data = AREA16.read_bytes()
    assert data[4] == OVERLAY_ID and len(data) == BSS - O.ARENA
    ram, spad = O.CAPTURES[BEAT]
    designed = bytearray(ram)
    designed[O.ARENA:O.ARENA + len(data)] = data
    designed[BSS:BSS + 0x1000] = bytes(0x1000)
    O.CAPTURES[DESIGNED] = (bytes(designed), spad)


K = types.SimpleNamespace(O=O, DESIGNED=DESIGNED, U=U, I=I, F=F, S=S, A=A, B=B, REC=REC, WORK=WORK, SPAWN=SPAWN, FREE=FREE,
                          CALLBACK=CALLBACK, MATH=MATH, fw=O.fw, b=O.b8, h=O.h16, w=O.w32, f=O.f32b,
                          ulp=lambda x, d: O.w32(struct.unpack('<I', struct.pack('<f', x))[0] + d))


def cases():
    out, labels = [], set()

    def add(label, e, patches=(), queues=None, run=MATH, args=None, base=True):
        """One case: entry `e` over the designed image with `patches`
        ((address, bytes); scratchpad addresses go to the scratchpad),
        hook results `queues` ({address: [values]}), the helpers in `run`
        executed as original code, the entry's `args` (default
        default_args), the node's +0x4C method set unless base=False."""
        assert label not in labels, label
        assert e in FUNCS, hex(e)
        labels.add(label)
        all_patches = ([(A + 0x4C, O.w32(CALLBACK)), (B + 0x4C, O.w32(CALLBACK))] if base else []) + list(patches)
        out.append((label, DESIGNED, e, tuple(args if args is not None else O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set(run)))

    for m in MODULES:
        m.cases(add, K)
    return out


def header_checks():
    header = (ROOT / 'src/game/em_area16_overlay.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h_[0] for h_ in HOOKS]
    types_ = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types_[k] for k, _ in args] + ([types_[result] + '*'] if result else []), name
    internal = (ROOT / 'src/game/em_area16_overlay_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int a16_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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
    only = os.environ.get('EM_AREA16_OVERLAY_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - set(DEAD)
        print(f'{entry:08X}: {len(words & seen)}/{len(words - set(DEAD))} reachable words')
        if missing and (not only or os.environ.get('EM_AREA16_OVERLAY_MISSING')):
            failures.append(('coverage', [hex(entry), [hex(m) for m in sorted(missing)]]))
    if not only:
        assert not set(DEAD) & seen, 'a word called dead ran'
        failures += [('callee inventory', p) for p in O.callee_checks()]
        if not failures:
            sites, missing = O.hook_sites(results)
            if missing and len(MODULES) == len(GROUPS):   # a group subset leaves the others' hooks unreached
                failures.append(('unreached hooks', missing))
            for problems, runs in O.reference_mode.parallel_map(O.hook_contract_site, sites):
                if problems:
                    failures.append(('fail-stop contract', problems))
    return failures


def main():
    configure()
    O.NATIVE = O.build(os.environ.get('EM_AREA16_OVERLAY_SOURCE'))
    header_checks()
    entry_guards()
    for m in MODULES:
        if hasattr(m, 'extra_checks'):
            m.extra_checks(K, O.NATIVE)
    selected = cases()
    only = os.environ.get('EM_AREA16_OVERLAY_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_AREA16_OVERLAY_TRACE'):
        trace(os.environ['EM_AREA16_OVERLAY_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_AREA16_OVERLAY_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
