#!/usr/bin/env python3
"""Original-instruction oracle for the radar of D_00810CA5 == 6 (the UPPER
U.R.S., item 0x06): 0022E7F0 and its blip 0022E630 as src/game/em_sight.c
translates them (docs/CAMERA_LIVE.md section 7, docs/WHOLE_GAME_LEDGER.md
section 4.3).

The user's pinned ELF supplies every instruction; none are embedded here.
Each case designs memory at original addresses over the ELF's image: the
render context's slot cursors (*D_00275670 + 0x10 + 4 slot) into a packet
buffer of random bytes, the radar list D_00275B54 / D_00275B58 and its
entries (+0 used byte, +0xB0 / +0xB8 inside, on and outside the 200-unit
range), the player (+0xA0 / +0xA8, the yaw +0xC4, the heading +0xF0, the
camera record behind +0x12C with its +0x90 heading), the scratchpad
0x700036A0..0x7000371F and 0x700038A0..0x700038CF (random), the caller's
stack (random), and the ELF's .data corners D_0026A990.. and ring records
D_0026A9C0 / D_0026AA20 / D_0026AA80 (or random ones). The original runs
on one image (test_player_fall_reference.FallEE), the native module on a
copy through its map / call / store host.

Callees: the SDK leaves (001029C0, 00102760, 00102BB0, 001026D0, 001031E0,
00102948, 001026A0, 0011E620 with f12 and f13, 001B1470, float_to_int
001281C0, and 002082B0's sin, cos, 00102850, 001028D0) execute their
ORIGINAL instructions on both sides; 0022E630 and 002082B0 are the
translation's own (the original runs its instructions, the native its
translation). 00207D00, 00207E40 and 00208040 are scripted (they advance the
slot cursor by a packet size; their translations have their own oracles).
Compared: every callee entry in order (function, stack pointer, argument
registers with 00207E40's 64-bit tex0, f12 / f13, the bytes behind each
pointer argument), and at the end the whole RAM and scratchpad and the
stack below the caller (every byte but the original's register-save
slots). Every instruction word of the two executes and every conditional
branch goes both ways. Fail-stop cases: each callee failing in turn, an
unmapped range, no map, no call, a latched fault.

Default run ~10 s (mode quick); EM_TEST_FULL=1 runs the wider sweep.
"""
import ctypes as C
import math
import os
import random
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode as RM  # noqa: E402
import test_player_slide_reference as S  # noqa: E402
import test_sight_reference as T  # noqa: E402
from test_player_fall_reference import FallEE  # noqa: E402

OUT = ROOT / 'build/radar'
MASK = 0xFFFFFFFF
FUNCS = {0x22E7F0: 0x3E8, 0x22E630: 0x1C0}
SIG = {0x207D00: (2, 0), 0x207E40: (7, 0), 0x208040: (5, 0),
       0x1029C0: (1, 0), 0x102760: (2, 0), 0x102BB0: (2, 1), 0x1026D0: (3, 0), 0x1031E0: (2, 0),
       0x102948: (2, 0), 0x1026A0: (3, 0), 0x11E620: (0, 2), 0x1B1470: (0, 1), 0x1281C0: (0, 1),
       0x11E2A8: (0, 1), 0x11DE90: (0, 1), 0x102850: (2, 1), 0x1028D0: (3, 0), 0x1028B8: (3, 0)}
REAL = {0x1029C0, 0x102760, 0x102BB0, 0x1026D0, 0x1031E0, 0x102948, 0x1026A0, 0x11E620, 0x1B1470, 0x1281C0,
        0x11E2A8, 0x11DE90, 0x102850, 0x1028D0, 0x1028B8}
ADVANCE = {0x207D00: 0x20, 0x207E40: 0x60, 0x208040: 0x60}
CTX = 0x811CC0
PKT = 0x01E00000
PLAYER, CAMREC, LIST, ACTORS = 0x8102B0, 0x01D00000, 0x01D10000, 0x01D20000
SP = S.STACK_TOP
SAVED_GPR = set(range(16, 24)) | {30, 31}
CODE = set()
for _f, _n in FUNCS.items():
    CODE.update(range(_f, _f + _n, 4))
# The wrap loops' back edges: 0011E620 returns within [-pi, pi], so the
# heading difference lies in [-2 pi, 2 pi] and one turn of 2 pi always ends
# either loop; their loop-back branches are never taken.
NEVER_TAKEN = {0x22E924, 0x22E970}
ENTRIES = {0x22E7F0: ('0022E7F0', 'u'), 0x22E630: ('0022E630', 'uu')}
ELF = LIB = None


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    source = os.environ.get('EM_SIGHT_SOURCE', str(ROOT / 'src/game/em_sight.c'))
    lib = OUT / (Path(source).stem + ('.dylib' if sys.platform == 'darwin' else '.so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-ffp-contract=off',
                    '-shared', '-fPIC', '-Isrc', source, '-o', str(lib)], cwd=ROOT, check=True,
                   capture_output=True)
    dll = C.CDLL(str(lib))
    for entry, (name, kinds) in ENTRIES.items():
        f = getattr(dll, 'em_sight_' + name)
        f.argtypes = [C.POINTER(T.Host)] + [C.c_uint32 for _ in kinds]
        f.restype = C.c_int
    dll.em_sight_call.argtypes = [C.POINTER(T.Host), C.POINTER(T.Call)]
    dll.em_sight_call.restype = C.c_int
    return dll


class Oracle(FallEE):
    """FallEE recording the two functions' instruction words, the stack
    bytes register saves store, and branch outcomes."""

    def __init__(self, ram=None, spad=None):
        super().__init__(ELF, ram, spad)
        self.seen = set()
        self.saves = set()
        self.branches = set()
        self.word = 0

    def load(self, address, size=4):
        if address in CODE:
            self.seen.add(address)
        return super().load(address, size)

    def execute(self, word, pc):
        self.word = word
        super().execute(word, pc)

    def save(self, address, value, size=4):
        a = address & MASK
        if 0x7F000000 <= a < 0x7F100000:
            w = self.word
            op, rs, rt = w >> 26, w >> 21 & 31, w >> 16 & 31
            if rs == 29 and ((op in (0x1F, 0x3F) and rt in SAVED_GPR) or (op == 0x39 and rt >= 20)):
                self.saves.update(range(a, a + size))
        super().save(address, value, size)

    def branch(self, w, pc):
        b = super().branch(w, pc)
        if b is not None and pc in CODE:
            self.branches.add((pc, b[0]))
        return b


def pointers(e, fn, a):
    if fn == 0x208040:
        return (e.read(a[1], 12), e.read(a[2], 12), e.read(a[3], 12))
    if fn in (0x102948, 0x102760, 0x1031E0, 0x102BB0, 0x102850):
        return (e.read(a[1] & ~15, 16),)
    if fn in (0x1028D0, 0x1028B8):
        return (e.read(a[1] & ~15, 16), e.read(a[2] & ~15, 16))
    if fn == 0x1026A0:
        return (e.read(a[1] & ~15, 64), e.read(a[2] & ~15, 16))
    if fn == 0x1026D0:
        return (e.read(a[1] & ~15, 64), e.read(a[2] & ~15, 64))
    return ()


def policy(e, fn):
    a = [int(e.r[4 + i]) & MASK for i in range(7)]
    if fn in REAL:
        T.nested(e, fn)
        return
    e.r[2] = 0
    e.f[0] = 0
    cell = e.load(0x275670) + ((a[0] << 2) & MASK) + 0x10
    e.save(cell, (e.load(cell) + ADVANCE[fn]) & MASK)


def F(x):
    return S.bits(x)


def unit(rng):
    t = rng.uniform(-math.pi, math.pi)
    return (math.cos(t) * rng.choice((1.0, 3.0, 0.2)), rng.uniform(-2, 2), math.sin(t) * rng.choice((1.0, 3.0)),
            rng.choice((0.0, 1.0)))


def design(case):
    rng = random.Random(case['seed'])
    e = Oracle()
    e.write(0x275670, struct.pack('<I', CTX))
    for k in range(4):
        e.write(CTX + 0x10 + 4 * k, struct.pack('<I', PKT + 0x8000 * k + 16 * rng.randrange(0, 8)))
    e.write(PKT, rng.randbytes(0x20000))
    e.write(0x70003600, struct.pack('<16I', *(F(rng.uniform(-300, 300)) for _ in range(16))))
    e.write(0x700036A0, rng.randbytes(0x80))
    e.write(0x700038A0, rng.randbytes(0x30))
    e.write(SP - 0x1000, rng.randbytes(0x1000))
    if case.get('data') == 'random':
        e.write(0x26A990, b''.join(struct.pack('<4f', rng.uniform(-200, 200), rng.uniform(-5, 5),
                                               rng.uniform(-200, 200), 1.0) for _ in range(3)))
    px, pz = float(rng.randint(-2000, 2000)), float(rng.randint(-2000, 2000))
    e.write(PLAYER + 0xA0, struct.pack('<3f', px, rng.uniform(-50, 50), pz))
    e.write(PLAYER + 0xC4, struct.pack('<f', rng.choice((0.0, math.pi, -math.pi, rng.uniform(-7, 7)))))
    e.write(PLAYER + 0xF0, struct.pack('<4f', *unit(rng)))
    e.write(PLAYER + 0x12C, struct.pack('<I', CAMREC))
    e.write(CAMREC + 0x90, struct.pack('<4f', *unit(rng)))
    count = case.get('count', rng.choice((0, 1, 3, 6)))
    e.write(0x275B58, struct.pack('<h', count))
    e.write(0x275B54, struct.pack('<I', LIST))
    for i in range(max(count, 0)):
        actor = ACTORS + 0x400 * i
        e.write(LIST + 4 * i, struct.pack('<I', actor))
        e.write(actor, bytes([rng.choice((0, 1, 1, 1, 0x40))]))
        d = rng.choice((10.0, 100.0, 199.0, 200.0, 201.0, 2000.0))
        t = rng.uniform(-math.pi, math.pi)
        if rng.random() < 0.2:
            t = 0.0                       # exactly 200 away on one axis (the <= 40000 bound)
        e.write(actor + 0xB0, struct.pack('<3f', px + d * math.cos(t), rng.uniform(-50, 50), pz + d * math.sin(t)))
    return e


def run_case(case):
    e = design(case)
    n = Oracle(e.mem, e.spad)
    n.stack[:] = e.stack
    oevents, nevents, errors, calls = [], [], [], [0]

    def args64(regs, fn, na):
        a = [int(v) & MASK for v in regs[:7]]
        if fn == 0x207E40:
            a[6] = int(regs[6]) & 0xFFFFFFFFFFFFFFFF
        return tuple(a[:na])

    def hook(fn):
        def inner(x):
            na, nf = SIG[fn]
            oevents.append(('call', fn, x.r[29] & MASK, args64(x.r[4:11], fn, na),
                            tuple(x.f[12 + i] & MASK for i in range(nf)),
                            pointers(x, fn, [int(v) & MASK for v in x.r[4:11]])))
            policy(x, fn)
        return inner
    e.hooks = {fn: hook(fn) for fn in SIG}
    steps = case['steps']
    for fn, args in steps:
        e.r[29] = SP
        e.r[31] = S.RETURN
        for i, v in enumerate(args):
            e.r[4 + i] = S.sx32(v & MASK) & 0xFFFFFFFFFFFFFFFF
        e.run(fn)
    arrays = [(C.c_uint8 * len(buf)).from_buffer(buf) for buf in (n.mem, n.spad, n.stack)]

    def mapping(_, a, size, write):
        try:
            if case.get('unmapped') is not None and a <= case['unmapped'] < a + size:
                return None
            if a < 0x2000000:
                b, i = 0, a
            elif 0x70000000 <= a < 0x70004000:
                b, i = 1, a - 0x70000000
            elif 0x7F000000 <= a < 0x7F100000:
                b, i = 2, a - 0x7F000000
            else:
                return None
            if i + size > len(arrays[b]):
                return None
            return C.addressof(arrays[b]) + i
        except Exception as ex:          # noqa: BLE001
            errors.append(ex)
            return None

    def observe(_, a, size):
        pass

    def worker(_, p):
        c = p.contents
        try:
            fn = c.function
            na, nf = SIG[fn]
            assert (c.na, c.nf) == (na, nf), (hex(fn), 'signature', c.na, c.nf)
            assert all(int(v) == 0 for v in c.a[na:7]), (hex(fn), 'unused argument registers set')
            assert all(int(v) == 0 for v in c.f[nf:8]), (hex(fn), 'unused float arguments set')
            regs = [int(v) for v in c.a[:7]]
            nevents.append(('call', fn, c.sp, args64(regs, fn, na), tuple(int(c.f[i]) for i in range(nf)),
                            pointers(n, fn, [v & MASK for v in regs])))
            k = len(nevents)
            assert nevents == oevents[:k], (case['name'], 'callee entries', T.first_diff(nevents, oevents))
            calls[0] += 1
            if calls[0] == case.get('failat'):
                return -1
            n.r[29] = c.sp
            for i in range(7):
                n.r[4 + i] = regs[i] & 0xFFFFFFFFFFFFFFFF
            for i in range(nf):
                n.f[12 + i] = c.f[i]
            policy(n, fn)
            c.v0 = n.r[2] & 0xFFFFFFFFFFFFFFFF
            c.f0 = n.f[0] & MASK
            return 0
        except Exception as ex:          # noqa: BLE001
            errors.append(ex)
            return -1
    callbacks = (T.MAP(mapping), T.CALL(worker), T.STORE(observe))
    h = T.Host(None, *callbacks, SP, 0, 0, 0)
    status = 0
    for fn, args in steps:
        name, kinds = ENTRIES[fn]
        h.sp = SP
        status = getattr(LIB, 'em_sight_' + name)(C.byref(h), *args)
        if status:
            break
    if errors:
        raise errors[0]
    if 'failat' in case or case.get('unmapped') is not None:
        code = 3 if 'failat' in case else 2
        assert status == -1 and h.fault == code and h.fault_function == steps[0][0], \
            (case['name'], 'failure latch', status, h.fault, hex(h.fault_function))
        assert nevents == oevents[:len(nevents)], (case['name'], 'failure prefix')
        before = list(nevents)
        assert getattr(LIB, 'em_sight_' + ENTRIES[steps[0][0]][0])(C.byref(h), *steps[0][1]) == -1
        assert nevents == before, (case['name'], 'latched call had effects')
        return set(), set(), len(nevents)
    assert status == 0 and h.fault == 0, (case['name'], status, h.fault, hex(h.fault_function),
                                          hex(h.fault_address))
    assert nevents == oevents, (case['name'], 'callee entries', T.first_diff(nevents, oevents), len(nevents),
                                len(oevents))
    if n.mem != e.mem or n.spad != e.spad:
        where = [hex(i) for i in range(len(e.mem)) if n.mem[i] != e.mem[i]][:8]
        where += [hex(0x70000000 + i) for i in range(len(e.spad)) if n.spad[i] != e.spad[i]][:8]
        raise AssertionError((case['name'], 'memory differs', where))
    saves = e.saves | n.saves
    for i in range(SP - 0x1000 - 0x7F000000, SP - 0x7F000000):
        if e.stack[i] != n.stack[i] and 0x7F000000 + i not in saves:
            raise AssertionError((case['name'], 'stack local differs', hex(0x7F000000 + i)))
    return e.seen, e.branches, len(oevents)


def cases():
    rng = random.Random(0x4ADA)
    out = []
    for k in range(RM.pick(160, 400)):
        fn = rng.choice((0x22E7F0, 0x22E7F0, 0x22E630))
        args = [PLAYER] if fn == 0x22E7F0 else [PLAYER, ACTORS]
        out.append({'name': f'{fn:06X}/{k}', 'seed': rng.getrandbits(32), 'steps': [(fn, args)],
                    'data': rng.choice(('elf', 'elf', 'random')), 'count': rng.choice((0, 1, 3, 6, 12))})
    return out


def worker(case):
    seen, branches, ev = run_case(case)
    return sorted(seen), sorted(branches), ev


def fail_cases(rng):
    out = []
    for fn in (0x22E7F0, 0x22E630):
        base = {'name': f'fail {fn:06X}', 'seed': 7, 'data': 'elf', 'count': 3,
                'steps': [(fn, [PLAYER] if fn == 0x22E7F0 else [PLAYER, ACTORS])]}
        # a design where the entries are in range (the blips run)
        for seed in range(1, 200):
            row = run_case(dict(base, seed=seed))
            if row[2] > (40 if fn == 0x22E7F0 else 4):
                base['seed'] = seed
                break
        n = row[2]
        ks = set(range(1, n + 1)) if RM.FULL else set(range(1, min(n, 6) + 1)) | set(range(1, n + 1, max(1, n // 8))) | {n}
        for k in sorted(ks):
            out.append(dict(base, failat=k, name=f'{base["name"]} at {k}'))
    out.append({'name': 'unmapped list', 'seed': 3, 'data': 'elf', 'count': 2, 'steps': [(0x22E7F0, [PLAYER])],
                'unmapped': LIST + 4})
    out.append({'name': 'unmapped stack', 'seed': 4, 'data': 'elf', 'count': 0, 'steps': [(0x22E7F0, [PLAYER])],
                'unmapped': SP - 0xB0 + 0x68})
    return out


def refusals():
    count = 0
    mem = (C.c_uint8 * 0x2000000)()
    stack = (C.c_uint8 * 0x100000)()

    def mp(_, a, size, write):
        if a + size <= 0x2000000:
            return C.addressof(mem) + a
        if 0x7F000000 <= a and a + size <= 0x7F100000:
            return C.addressof(stack) + a - 0x7F000000
        return None
    mapfn = T.MAP(mp)
    for fn, (name, kinds) in ENTRIES.items():
        args = [PLAYER] if fn == 0x22E7F0 else [PLAYER, ACTORS]
        for absent in ('map', 'call', 'latched'):
            h = T.Host(None, T.MAP() if absent == 'map' else mapfn, T.CALL(), T.STORE(), SP,
                       7 if absent == 'latched' else 0, 0, 0)
            rc = getattr(LIB, 'em_sight_' + name)(C.byref(h), *args)
            # 0022E630 over zeroed memory: the entry is at the player (within 200), so a callee is reached
            assert rc == -1 and h.fault == (7 if absent == 'latched' else 1), (hex(fn), absent, rc, h.fault)
            assert absent == 'latched' or h.fault_function == fn, (hex(fn), hex(h.fault_function))
            count += 1
    c = T.Call(0x22E7F0, SP, (T.U64 * 7)(PLAYER))
    h = T.Host(None, mapfn, T.CALL(), T.STORE(), 0, 0, 0, 0)
    assert LIB.em_sight_call(C.byref(h), C.byref(c)) == -1 and h.sp == SP and h.fault == 1
    return count + 1


def main():
    global ELF, LIB
    start = time.monotonic()
    ELF = S.read_elf()
    T.ELF = ELF
    LIB = build()
    ee = FallEE(ELF)
    targets = set()
    for fn, size in FUNCS.items():
        for pc in range(fn, fn + size, 4):
            word = ee.load(pc)
            if word >> 26 == 3:
                targets.add((word & 0x3FFFFFF) << 2)
    expect = {0x207D00, 0x1029C0, 0x102948, 0x102760, 0x11E620, 0x102BB0, 0x1026D0, 0x1031E0, 0x1026A0,
              0x208040, 0x2082B0, 0x1B1470, 0x1281C0, 0x207E40, 0x22E630}
    assert targets == expect, (sorted(hex(t) for t in targets ^ expect))
    all_cases = cases()
    selected = RM.select(all_cases, 120, 0x4ADA, axes=(lambda c: c['steps'][0][0], lambda c: c['data']))
    RM.banner(RM.part(len(selected), len(all_cases), 'radar cases'))
    rows = RM.parallel_map(worker, selected, cost=lambda c: 1 + c['count'])
    seen = set().union(*(set(r[0]) for r in rows))
    branches = set().union(*(set(tuple(b) for b in r[1]) for r in rows))
    missing = sorted(CODE - seen)
    assert not missing, ('instruction words never executed', [hex(a) for a in missing[:24]], len(missing))
    sites = set()
    for pc in sorted(CODE):
        word = ee.load(pc)
        op, rs, rt = word >> 26, word >> 21 & 31, word >> 16 & 31
        if op in (4, 20) and rs == 0 and rt == 0:
            continue
        if ee.branch(word, pc) is not None:
            sites.add(pc)
    gaps = sorted((hex(pc), b) for pc in sites for b in (False, True) if (pc, b) not in branches and not (pc in NEVER_TAKEN and b))
    assert not gaps, ('branch outcomes never exercised', gaps)
    fails = fail_cases(random.Random(0xFA11))
    fail_rows = RM.parallel_map(run_case, fails)
    refused = refusals()
    assert not any((pc, True) in branches for pc in NEVER_TAKEN), 'a back edge named never taken ran'
    print('radar vs original instructions: PASS %d cases, %d callee entries compared; %d of %d words executed, '
          '%d branch outcomes (2 back edges unreachable, named); %d callee-failure / unmapped cuts; %d host refusals (%.1fs)' %
          (len(selected), sum(r[2] for r in rows), len(CODE), len(CODE), len(branches), len(fail_rows), refused,
           time.monotonic() - start))


if __name__ == '__main__':
    main()
