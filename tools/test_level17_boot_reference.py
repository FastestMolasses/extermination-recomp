#!/usr/bin/env python3
"""The second half's small boot behaviours (src/game/em_level17_boot.c)
against the ORIGINAL instructions: 001C2420 (AREA16, AREA17, AREA20 and
AREA21 placements) and 001C4A00 (AREA20's six placements).

The harness is tools/test_level14_port_reference.py (`O`), configured as
test_level16_boot_reference.py configures it. Each entry runs as original
code from the user's ELF over a fork recording's RAM (a03b_03_item2a: boot
code is the same in every image; its AREA03 overlay text is checked against
the user's extract/OVERLAY/AREA03.BIN and its boot text against the pinned
ELF). The node is designed in zero RAM at FREE; the scratchpad flag
0x70003B92 and the table D_008107D8 are patched per case. Every callee is a
hook with a scripted result; the node's +0x4C method is stubbed and logged.
Compared per case: every call and its arguments, memory at every call
entry, the memory accesses between calls one for one in order, memory after
the last store, a poisoned re-run, coverage of every reachable original
word, the callee inventory and the fail-stop contract.

EM_TEST_FULL=1 widens the random sample. EM_LEVEL17_BOOT_ONLY=<label prefix>
runs a subset (no coverage or contract); EM_LEVEL17_BOOT_SOURCE=<dir> tests
another copy of the module sources; EM_LEVEL17_BOOT_SHOW=<n> prints n
failures; EM_LEVEL17_BOOT_TRACE=<label> prints the original's calls and
accesses of matching cases (no instruction text). Reports hold addresses and
counts only.
"""
import ctypes as C
import os
import random
import re
from pathlib import Path
import gen_level17_boot as G
import test_level14_port_reference as O
import reference_root as refs
ROOT = Path(__file__).resolve().parents[1]
U, I, F, Q, S = O.U, O.I, O.F, O.Q, O.S
FUNCS = {
    0x001C2420: ('em_level17_boot_001C2420', 8, [U], None),
    0x001C4A00: ('em_level17_boot_001C4A00', 232, [U], None),
}
BEAT = 'a03b_03_item2a'
FREE = 0x1E61000
A = FREE + 0x1000            # the designed node
CALLBACK = 0x1CAA00          # a +0x4C method word (stubbed and logged)
FLAG = 0x70003B92            # the scratchpad flag 001C4A00 tests
TABLE = 0x8107D8             # D_008107D8, one byte per node byte +3
SELF_ENTRIES = {0x1C4A00}
DEAD = set()


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


def configure():
    O.FREE = FREE
    O.FUNCS, O.HOOKS = FUNCS, HOOKS
    O.OVERLAY_OF = {}
    O.OVERLAY_FILES = {4: ROOT.parent / 'Extermination/extract/OVERLAY/AREA03.BIN'}
    O.OUT = ROOT / 'build/levels/level17_boot'
    O.SOURCES = ('em_level17_boot.c',)
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
    O.DEFAULT_ARGS = {e: [A] for e in FUNCS}
    O.image_for = lambda entry: O.CAPTURES[BEAT][0]
    O.CAPTURES.clear()
    O.CAPTURES.update(O.load_captures(O.read_elf()))
    O.JUMP_TABLES = {}
    O.TABLE_BYTES = frozenset()


BYTES = (0, 1, 2, 3, 4, 5, 0x7F, 0x80, 0xFF)
HALVES = (0, 1, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFF80, 0xFFFF)
RESULTS = (0, 1, -1, 2, 0x100)


def cases():
    out, labels = [], set()
    b, h, w = O.b8, O.h16, O.w32
    base = [(A + 0x4C, w(CALLBACK))]

    def add(label, e, patches=(), queues=None):
        assert label not in labels, label
        labels.add(label)
        all_patches = base + list(patches)
        out.append((label, BEAT, e, tuple(O.DEFAULT_ARGS[e]),
                    [(a, v) for a, v in all_patches if a < 0x70000000],
                    [(a, v) for a, v in all_patches if a >= 0x70000000], O.Script(queues), set()))

    # ---- 001C2420: any node, any state
    for state in (0, 1, 2, 3, 0xFF):
        add(f'2420/{state:x}', 0x1C2420, [(A + 4, b(state))])
    # ---- 001C4A00 state 0: 001B0FD0's result
    for r in RESULTS:
        add(f'4a00/0/{r}', 0x1C4A00, [], {0x1B0FD0: [r]})
    # state 1: the scratchpad flag, the table byte against the halfword
    for flag in (0, 1, 0x80, 0xFF):
        for index in (0, 1, 0x1F, 0x80, 0xFF):
            for byte, half in ((0, 0), (5, 5), (5, 6), (0x80, 0x80), (0x80, 0xFF80), (0xFF, 0xFFFF),
                               (0xFF, 0xFF), (0, 0x100), (1, 0x8001)):
                add(f'4a00/1/{flag:x}/{index:x}/{byte:x}/{half:x}', 0x1C4A00,
                    [(A + 4, b(1)), (A + 3, b(index)), (A + 0x56, h(half)), (TABLE + index, b(byte)), (FLAG, b(flag))])
    # states 2, 3 and the rest
    for state in (2, 3, 4, 5, 0x7F, 0x80, 0xFF):
        add(f'4a00/state/{state:x}', 0x1C4A00, [(A + 4, b(state))])
    # a seeded random sample over both entries
    rng = random.Random(0x17B007)
    for entry in FUNCS:
        for n in range(O.reference_mode.pick(200, 24)):
            index = rng.choice(BYTES)
            patches = [(A + 4, b(rng.choice((0, 1, 1, 2, 3, 4, 0xFF)))), (A + 3, b(index)),
                       (A + 0x56, h(rng.choice(HALVES))), (TABLE + index, b(rng.choice(BYTES))),
                       (FLAG, b(rng.choice((0, 0, 1, 0xFF)))), (A + 5, b(rng.choice(BYTES)))]
            add(f'random/{entry:06X}/{n}', entry, patches, {0x1B0FD0: [rng.choice(RESULTS)]})
    return out


def header_checks():
    header = (ROOT / 'src/game/em_level17_boot.h').read_text()
    block = header.split('/* BEGIN GENERATED HOOKS */')[1].split('/* END GENERATED HOOKS */')[0]
    found = re.findall(r'int \(\*(w_[0-9A-F]{8})\)\(void \*ctx((?:, [^,)]+)*)\);', block)
    assert [n for n, _ in found] == [h[0] for h in HOOKS]
    types = {U: 'uint32_t', I: 'int32_t', F: 'float', Q: 'uint64_t'}
    for (name, params), (expected, address, args, result) in zip(found, HOOKS):
        actual = [re.sub(r'\s*\b\w+$', '', p.strip()).replace(' ', '') for p in params.split(',')[1:]]
        assert actual == [types[k] for k, _ in args] + ([types[result] + '*'] if result else [])
    internal = (ROOT / 'src/game/em_level17_boot_internal.h').read_text()
    for name, address, _, _ in HOOKS:
        bodies = re.findall(r'static inline int l17b_c_%s\(.*?\n}\n' % name[2:], internal, re.S)
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
            assert rc == -1 and not touched, (symbol, rc)
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
    only = os.environ.get('EM_LEVEL17_BOOT_ONLY')
    failures = [(name, errors) for name, _, errors, _, _, _ in results if errors]
    seen = set().union(*(r[3] for r in results)) if results else set()
    for entry, (_, size, _, _) in FUNCS.items():
        words = O.reachable_words(O.image_for(entry), entry, size)
        missing = words - seen - DEAD
        print(f'{entry:08X}: {len(words & seen)}/{len(words - DEAD)} reachable words')
        if missing and not only:
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
    configure()
    O.NATIVE = O.build(os.environ.get('EM_LEVEL17_BOOT_SOURCE'))
    header_checks()
    entry_guards()
    selected = cases()
    only = os.environ.get('EM_LEVEL17_BOOT_ONLY')
    if only:
        selected = [c for c in selected if c[0].startswith(only)]
    O.CASES[:] = selected
    if os.environ.get('EM_LEVEL17_BOOT_TRACE'):
        trace(os.environ['EM_LEVEL17_BOOT_TRACE'], selected)
        return
    results = O.run_cases(selected)
    failures = judge(results)
    for label, errors in failures[:int(os.environ.get('EM_LEVEL17_BOOT_SHOW', '8'))]:
        print('FAIL', label, errors)
    assert not failures, f'{len(failures)} failures'
    O.reference_mode.banner(f'{len(selected)} cases')
    print(f'PASS {len(selected)} cases: calls, ordered accesses, whole memory, poisoned starts and fail-stop')


if __name__ == '__main__':
    main()
