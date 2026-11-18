#!/usr/bin/env python3
"""Compare the native flamethrower burst node 001E3630 (with its empty
contact callback 001E3620) and the effect-driver handler 001ECE70
(src/game/em_effect_001E3630.c) with the original code.

The oracle is the bounded EE interpreter of test_effect_original_reference.py
with the EE accumulator forms (ADDA / MADD) routed through tools/ee_cop1.py
(the measured EE float model). It executes the ORIGINAL instructions of the
pinned boot ELF (the user's config/SCUS_971.12) over a captured AREA11 RAM
image (s87 route beat 08). The native module runs over a second copy of the
same bytes through tests/effect_001E3630_bridge.c.

Callees, one policy on both sides, every call recorded with its argument
bits and the bytes its pointer arguments name:
  run   the vector / matrix leaves 00102948, copy_qw4 00102958, 001026A0,
        001028D0, 00102918, 001CD390 and the square root 0011E748: the
        ORIGINAL routine runs on a nested stack over that side's memory;
  stub  rand 00122BB8 (scripted), the probes 0019A570 / 0019B6C0 (scripted
        results; a hit writes a scripted point at 0x700031B0 and the hit
        record's address at 0x700031D0), 001CCF70 (a key from a counter),
        001CFA60 / 001CFB50 (a 0x60-byte block derived from their inputs),
        001CFBE0, 001F0190, 001F0290, 001EFD20, 001F0460, 001B17A0, 001AFC10.
Per case: the call lists are equal, the native returns 0, and all of RAM
(the stack window excepted: the original's register saves) and the whole
scratchpad are equal afterwards; the 0x60-byte stack block 001CFBE0 reads is
compared inside the call records.

Cases (001E3630): state 0 with the first probe hit, the second hit, the
grabbed-record path (+0x1A 0x5B), a hit on another record kind with the
re-probe hitting and missing, both misses; state 1 below 1.0; the 1.0..1.7
window with both side probes hitting (the 0x8000001A spawn), the first or
second missing, the flight time 1.0, w+0x1C other than 4, the window
already used; past 1.7; states 2 and 3 (free), 4 and 0xFF (nothing); a
random sweep of window times; whole lifetimes from state 0 to the free.
001ECE70: random accumulators / fractions and keys. 001E3620: the original
returns at once, storing nothing. Refusals: a NULL worker faults at its
address; an unmapped node faults.

No original instruction bytes, disassembly or data are written by this
file; build/effect_001E3630_reference/report.json holds counts only.
EM_TEST_FULL=1 runs the long sweeps (tools/reference_mode.py).
"""
import ctypes as C
import hashlib
import json
import random
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ee_cop1  # noqa: E402
import test_effect_original_reference as EO  # noqa: E402
from reference_mode import banner, pick  # noqa: E402

ROOT, DECOMP, ELF_SHA = EO.ROOT, EO.DECOMP, EO.ELF_SHA
M32 = EO.M32
OUT = ROOT / 'build/effect_001E3630_reference'
BEAT = '08_truck_crossing'
ENTRY, CONTACT, HANDLER = 0x1E3630, 0x1E3620, 0x1ECE70
NODE, TARGET, RECORD, CTXW, MATRIX = 0x01E00000, 0x01E01000, 0x01E02000, 0x01E03000, 0x01E04000
NESTED = 0x01F40000
STACK_WINDOW = (0x01F30000, EO.STACK)
RUN = {0x102948: 2, 0x102958: 2, 0x1026A0: 3, 0x1028D0: 3, 0x102918: 3, 0x1CD390: 2, 0x11E748: 0}
# stubs: integer argument count, float argument count, the bytes recorded behind (arg, size)
STUB = {0x122BB8: (0, 0, ()), 0x19A570: (4, 0, ((0, 12), (1, 12))), 0x19B6C0: (2, 0, ((0, 12), (1, 12))),
        0x1CCF70: (1, 0, ((0, 16),)), 0x1CFA60: (2, 2, ((1, 64),)), 0x1CFB50: (3, 5, ((2, 64),)),
        0x1CFBE0: (5, 0, ((2, 16), (3, 0x60))), 0x1F0190: (0, 2, ()), 0x1F0290: (0, 0, ()),
        0x1EFD20: (2, 0, ((1, 16),)), 0x1F0460: (2, 0, ((1, 64),)), 0x1B17A0: (1, 0, ()),
        0x1AFC10: (1, 0, ())}
F = EO.fbits
PYCALL = C.CFUNCTYPE(C.c_int, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint64), C.c_uint32,
                     C.POINTER(C.c_uint32), C.POINTER(C.c_uint64), C.POINTER(C.c_uint32))


class XEE(EO.EE):
    """EO.EE with every COP1 arithmetic op on the measured model (ee_cop1),
    the accumulator included."""

    def __init__(self, *a):
        super().__init__(*a)
        self.facc = 0

    def cop1(self, w, pc):
        if (w >> 21 & 31) in (16, 20):
            out = ee_cop1.step(w, self.f, self.facc)
            if out is None:
                raise EO.Unsupported(('cop1', hex(pc)))
            self.facc, cond = out
            if cond is not None:
                self.cond = int(cond)
            return
        super().cop1(w, pc)


class Mem:
    def __init__(self, ram, spad): self.ram, self.spad = ram, spad

    def where(self, a):
        a &= M32
        if 0x70000000 <= a < 0x70004000: return self.spad, a - 0x70000000
        assert a < 0x02000000, hex(a)
        return self.ram, a

    def get(self, a, n):
        b, o = self.where(a)
        return bytes(b[o:o + n])

    def put(self, a, data):
        b, o = self.where(a)
        b[o:o + len(data)] = data


class Script:
    """The stubs' answers, consumed in call order (one copy per side)."""

    def __init__(self, rands=(), probes=(), point=(1.5, -2.25, 40.0, 1.0)):
        self.rands, self.probes, self.point = list(rands), list(probes), point
        self.keys = 0

    def copy(self):
        s = Script(self.rands, self.probes, self.point)
        return s


def derived(tag, data, n):
    out = b''
    k = 0
    while len(out) < n:
        out += hashlib.sha256(tag + data + bytes([k])).digest()
        k += 1
    # keep the words finite floats: clear each word's top exponent bit
    words = list(struct.unpack(f'<{n // 4}I', out[:n]))
    return struct.pack(f'<{n // 4}I', *[w & 0xBFFFFFFF for w in words])


def stub(fn, args, floats, mem, script, calls):
    """One stub call on either side; returns (v0, f0)."""
    na, nf, behind = STUB[fn]
    rec = [fn, tuple(a & M32 for a in args[:na]), tuple(f & M32 for f in floats[:nf])]
    rec += [mem.get(args[i] & (M32 if size != 64 else 0xFFFFFFF0), size) for i, size in behind]
    calls.append(tuple(rec))
    if fn == 0x122BB8:
        return script.rands.pop(0) & M32, 0
    if fn in (0x19A570, 0x19B6C0):
        v = script.probes.pop(0)
        if v:
            mem.put(0x700031B0, struct.pack('<4f', *script.point))
            mem.put(0x700031D0, struct.pack('<I', RECORD))
        return v & M32, 0
    if fn == 0x1CCF70:
        script.keys += 1
        return (0x30 + script.keys) & M32, 0
    if fn in (0x1CFA60, 0x1CFB50):
        src = mem.get(args[1 if fn == 0x1CFA60 else 2], 64)
        mem.put(args[0] & M32, derived(struct.pack('<I', fn), src + struct.pack(f'<{nf}I', *floats[:nf]), 0x60))
    return 0, 0


def run_leaf(elf, fn, args, floats, mem, journal=None):
    sub = XEE(elf, mem.ram, mem.spad)
    if journal is not None:
        sub.journal = journal
    outer, EO.STACK = EO.STACK, NESTED
    try:
        sub.call(fn, args=tuple(a & M32 for a in args[:RUN[fn]]), floats=tuple(floats[:1]))
    finally:
        EO.STACK = outer
    return sub.r[2] & M32, sub.f[0] & M32


def original(elf, ram, spad, entry, args, script):
    ee, calls, mem = XEE(elf, ram, spad), [], Mem(ram, spad)

    def make(fn):
        def go(e):
            a = [e.r[4], e.r[5], e.r[6], e.r[7], e.r[8]]
            f = [e.f[12 + i] for i in range(5)]
            if fn in RUN:
                calls.append((fn, tuple(x & M32 for x in a[:RUN[fn]]), tuple(f[:1]) if fn == 0x11E748 else ()))
                v0, f0 = run_leaf(elf, fn, a, f, mem)
            else:
                v0, f0 = stub(fn, a, f, mem, script, calls)
            e.set64(2, EO.sx(v0, 32))
            e.f[0] = f0
        return go
    ee.stubs = {fn: make(fn) for fn in (*RUN, *STUB)}
    ee.call(entry, args=args)
    return calls, ee


def native(lib, elf, ram, spad, entry, args, script):
    calls, mem = [], Mem(ram, spad)
    errors = []

    def cb(fn, na, a, nf, f, v0, f0):
        try:
            ints = [a[i] for i in range(na)] + [0] * 5
            fl = [f[i] for i in range(nf)] + [0] * 5
            if fn in RUN:
                calls.append((fn, tuple(x & M32 for x in ints[:RUN[fn]]), tuple(fl[:1]) if fn == 0x11E748 else ()))
                r0, r1 = run_leaf(elf, fn, ints, fl, mem)
            else:
                r0, r1 = stub(fn, ints, fl, mem, script, calls)
            v0[0], f0[0] = r0, r1
            return 0
        except Exception as e:  # noqa: BLE001 - reported below
            errors.append(e)
            return -1
    keep = PYCALL(cb)
    rb = (C.c_uint8 * len(ram)).from_buffer(ram)
    sb = (C.c_uint8 * len(spad)).from_buffer(spad)
    fault = (C.c_uint32 * 2)()
    if entry == ENTRY:
        rc = lib.b_run(rb, sb, keep, args[0], EO.STACK, fault)
    else:
        rc = lib.b_ece70(rb, sb, keep, args[0], args[1], fault)
    del rb, sb
    if errors:
        raise errors[0]
    return calls, rc, (fault[0], fault[1])


def build_lib():
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / 'effect_001E3630.dylib'
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared', '-fPIC',
                    '-Isrc', 'tests/effect_001E3630_bridge.c', 'src/game/em_effect_001E3630.c', '-o', str(path)],
                   cwd=ROOT, check=True)
    lib = C.CDLL(str(path))
    lib.b_run.argtypes = [C.c_void_p, C.c_void_p, PYCALL, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32)]
    lib.b_ece70.argtypes = [C.c_void_p, C.c_void_p, PYCALL, C.c_uint32, C.c_int32, C.POINTER(C.c_uint32)]
    lib.b_null.argtypes = [C.c_uint32]
    return lib


def first_diff(a, b, skip=None):
    if skip is None:
        if a == b: return None
        lo, hi = 0, len(a)
    else:
        lo, hi = skip
        if a[:lo] == b[:lo] and a[hi:] == b[hi:]: return None
    for i in range(len(a)):
        if (skip is None or not lo <= i < hi) and a[i] != b[i]:
            return hex(i)
    return None


class Bench:
    def __init__(self, lib, elf, ram, spad):
        self.lib, self.elf, self.ram, self.spad = lib, elf, ram, spad
        self.cases = self.checks = 0
        self.seen = set()

    def compare(self, label, entry, args, ram, spad, script):
        r2, s2 = bytearray(ram), bytearray(spad)
        ocalls, _ = original(self.elf, ram, spad, entry, args, script.copy())
        ncalls, rc, fault = native(self.lib, self.elf, r2, s2, entry, args, script.copy())
        assert rc == 0, (label, 'native fault', fault)
        if ocalls != ncalls:
            k = next((i for i, (x, y) in enumerate(zip(ocalls, ncalls)) if x != y), min(len(ocalls), len(ncalls)))
            raise AssertionError((label, 'call', k, ocalls[k:k + 1], ncalls[k:k + 1], len(ocalls), len(ncalls)))
        d = first_diff(ram, r2, STACK_WINDOW)
        assert d is None, (label, 'RAM differs at', d)
        d = first_diff(spad, s2)
        assert d is None, (label, 'scratchpad differs at', d)
        self.cases += 1
        self.checks += 2 + len(ocalls)
        self.seen.update(c[0] for c in ocalls)
        return ocalls


def fresh_node(ram, rng, state, w=None):
    mem = Mem(ram, None)
    mem.put(NODE, bytes(0x2F0))
    ram[NODE + 4] = state
    struct.pack_into('<I', ram, NODE + 0x10, ENTRY)
    struct.pack_into('<I', ram, NODE + 0x14, NODE)
    # the launch frame +0xD0 (rows 0..2 a rotation-like set, row 3 = +0x100)
    m = [rng.uniform(-1.0, 1.0) for _ in range(12)] + [rng.uniform(-400, 400), rng.uniform(-50, 50),
                                                       rng.uniform(-400, 400), 1.0]
    for i in (3, 7, 11):
        m[i] = 0.0
    struct.pack_into('<16f', ram, NODE + 0xD0, *m)
    struct.pack_into('<4f', ram, NODE + 0xB0, rng.uniform(-9, 9), rng.uniform(-9, 9), rng.uniform(-9, 9), 1.0)
    struct.pack_into('<f', ram, NODE + 0x20, rng.uniform(0.0, 1.7))
    for k, v in (w or {}).items():
        if isinstance(v, float):
            struct.pack_into('<f', ram, NODE + 0x1F0 + k, v)
        else:
            struct.pack_into('<I', ram, NODE + 0x1F0 + k, v & M32)
    for off in (0x20, 0x50, 0x60, 0x90):
        if off not in (w or {}):
            vals = [rng.uniform(-300, 300) for _ in range(3)] + [1.0]
            if off == 0x20:   # a 4x4 launch frame copy in the work block
                vals = m
            struct.pack_into(f'<{len(vals)}f', ram, NODE + 0x1F0 + off, *vals)


def world(base_ram, base_spad, rng, grabbed):
    ram, spad = bytearray(base_ram), bytearray(base_spad)
    struct.pack_into('<I', ram, 0x8102D0, TARGET)
    struct.pack_into('<4f', ram, TARGET + 0xA0, rng.uniform(-300, 300), rng.uniform(-20, 20),
                     rng.uniform(-300, 300), 1.0)
    ram[RECORD + 0x1A] = 0x5B if grabbed else rng.choice((0x00, 0x05, 0x5A, 0x5C))
    struct.pack_into('<3f', ram, RECORD + 0x24, *[rng.uniform(-1, 1) for _ in range(3)])
    return ram, spad


def burst_cases(b, counts):
    rng = random.Random(0x1E3630)
    rands = [0, 0x7FFFFFFF, 0x40000000, 0x12345678, 1, 0x6FFFFFFF]
    # state 0: (probe script, grabbed record)
    zero = [('first probe hit', [1, 0], False), ('second probe hit', [0, 3], False),
            ('grabbed 0x5B', [0, 0, 2], True), ('other record, re-probe hit', [0, 0, 2, 4], False),
            ('other record, re-probe miss', [0, 0, 2, 0], False), ('both miss', [0, 0, 0, 0], False),
            ('miss, re-probe hit', [0, 0, 0, 4], False)]
    reps = pick(6, 2)
    for label, probes, grabbed in zero:
        for rep in range(reps):
            ram, spad = world(b.ram, b.spad, rng, grabbed)
            fresh_node(ram, rng, 0)
            calls = b.compare(f'state 0 {label} {rep}', ENTRY, (NODE,), ram, spad,
                              Script(rands=[rands[(rep + i) % 6] for i in range(2)], probes=probes))
            if label.startswith(('first', 'second')):
                assert calls[-1][0] == 0x19B6C0, label
    # state 1: (label, w fields, probes)
    one = [('below 1.0', {0x14: 0.5, 0x18: 0.8, 0x1C: 4}, []),
           ('window, both sides hit', {0x14: 0.98, 0x18: 0.8, 0x1C: 4, 0xA8: 0}, [1, 1]),
           ('window, first side misses', {0x14: 0.99, 0x18: 0.7, 0x1C: 4, 0xA8: 0}, [0]),
           ('window, second side misses', {0x14: 1.3, 0x18: 2.5, 0x1C: 4, 0xA8: 0}, [1, 0]),
           ('window, flight time 1.0', {0x14: 1.2, 0x18: 1.0, 0x1C: 4, 0xA8: 0}, []),
           ('window, kind not 4', {0x14: 1.2, 0x18: 0.3, 0x1C: 0x63, 0xA8: 0}, []),
           ('window used', {0x14: 1.4, 0x18: 0.3, 0x1C: 4, 0xA8: 1}, []),
           ('past 1.7', {0x14: 1.68, 0x18: 0.9, 0x1C: 4, 0xA8: 0}, []),
           ('at 1.7', {0x14: 1.66, 0x18: 0.9, 0x1C: 4, 0xA8: 1}, []),
           # w+0x14 + 0.04 lands exactly on 1.0 / on 1.7 (EE sums): both tests are "not <="
           ('exactly 1.0', {0x14: 0x3F75C290, 0x18: 0.8, 0x1C: 4, 0xA8: 0}, [1, 1]),
           ('exactly 1.7', {0x14: 0x3FD47AE2, 0x18: 0.8, 0x1C: 4, 0xA8: 0}, [1, 1])]
    for label, w, probes in one:
        for rep in range(reps):
            ram, spad = world(b.ram, b.spad, rng, False)
            fresh_node(ram, rng, 1, w)
            calls = b.compare(f'state 1 {label} {rep}', ENTRY, (NODE,), ram, spad, Script(probes=list(probes)))
            if label == 'window, both sides hit':
                assert [c[0] for c in calls if c[0] in (0x1EFD20, 0x1F0460)] == [0x1EFD20, 0x1F0460]
                assert calls[[c[0] for c in calls].index(0x1EFD20)][1][0] == 0x8000001A
            if label == 'past 1.7':
                assert ram[NODE] == 2 and ram[NODE + 4] == 3
    for state in (2, 3, 4, 0xFF):
        ram, spad = world(b.ram, b.spad, rng, False)
        fresh_node(ram, rng, state)
        calls = b.compare(f'state {state:#x}', ENTRY, (NODE,), ram, spad, Script())
        assert [c[0] for c in calls] == ([0x1AFC10] if state in (2, 3) else []), state
    # random window sweep
    for k in range(pick(300, 40)):
        ram, spad = world(b.ram, b.spad, rng, False)
        t = rng.uniform(0.9, 1.75)
        kind = rng.choice((4, 4, 4, 0, 0x63, 5))
        fresh_node(ram, rng, 1, {0x14: t, 0x18: rng.choice((1.0, rng.uniform(0.0, 4.0))), 0x1C: kind,
                                 0xA8: rng.choice((0, 0, 1))})
        b.compare(f'sweep {k}', ENTRY, (NODE,), ram, spad, Script(probes=[rng.choice((0, 1, 4)) for _ in range(2)]))
    counts['burst_cases'] = b.cases


def lifetimes(b, counts):
    """From state 0 to the free, original and native stepping in lockstep
    over their own memory, one node tick per compare."""
    rng = random.Random(0x3630)
    start = b.cases
    ticks = 0
    for life in range(pick(8, 2)):
        ram, spad = world(b.ram, b.spad, rng, life % 3 == 1)
        fresh_node(ram, rng, 0)
        first = [0, 0, 2, 4] if life % 3 != 1 else [0, 0, 2]
        script = Script(rands=[rng.randrange(0, 0x80000000) for _ in range(2)], probes=first)
        for tick in range(60):
            state = ram[NODE + 4]
            calls = b.compare(f'life {life} tick {tick}', ENTRY, (NODE,), ram, spad, script)
            # the oracle advanced `ram`; consume what it used
            used = sum(c[0] in (0x19A570, 0x19B6C0) for c in calls)
            script.probes = script.probes[used:] or [1, 1]
            script.rands = script.rands[sum(c[0] == 0x122BB8 for c in calls):]
            ticks += 1
            if state == 3:
                break
        else:
            raise AssertionError(('no free', life))
    counts['lifetime_ticks'] = ticks
    counts['lifetime_cases'] = b.cases - start


def handler_cases(b, counts):
    rng = random.Random(0x1ECE70)
    start = b.cases
    for k in range(pick(64, 12)):
        ram, spad = bytearray(b.ram), bytearray(b.spad)
        struct.pack_into('<I', ram, 0x275C34, CTXW)
        struct.pack_into('<f', ram, CTXW + 0x54, rng.uniform(-4, 4))
        struct.pack_into('<f', ram, CTXW + 0x5C, rng.uniform(0, 1))
        struct.pack_into('<16f', ram, MATRIX, *[rng.uniform(-200, 200) for _ in range(16)])
        key = rng.choice((0, 1, 0x7F, -1, rng.randrange(-0x8000, 0x8000)))
        calls = b.compare(f'001ECE70 {k}', HANDLER, (MATRIX, key & M32), ram, spad, Script())
        assert [c[0] for c in calls] == [0x1CFB50, 0x1CFBE0]
        assert calls[0][2][2:] == (EO.ONE, F(0.1), F(9.0)), calls[0][2]
        assert calls[1][1] == (key & M32, 1, 0x257000, 0x81F8F0, 0)
    counts['handler_cases'] = b.cases - start


def contact_and_refusals(b, counts):
    ram, spad = bytearray(b.ram), bytearray(b.spad)
    ee = XEE(b.elf, ram, spad)
    ee.call(CONTACT)
    assert not ee.journal and ee.steps <= 2, '001E3620 is not empty'
    assert b.lib.b_contact() == 0
    rng = random.Random(1)
    refusals = 0
    for fn, state in ((0x122BB8, 0), (0x1CFBE0, 1), (0x1AFC10, 3)):
        ram, spad = world(b.ram, b.spad, rng, False)
        fresh_node(ram, rng, state, {0x14: 0.5})
        b.lib.b_null(fn)
        try:
            calls, rc, fault = native(b.lib, b.elf, ram, spad, ENTRY, (NODE,), Script(rands=[1, 2], probes=[1, 0]))
        finally:
            b.lib.b_null(0)
        assert rc < 0 and fault == (2, fn), (hex(fn), rc, fault)
        refusals += 1
    ram, spad = bytearray(b.ram), bytearray(b.spad)
    calls, rc, fault = native(b.lib, b.elf, ram, spad, ENTRY, (0x03000000,), Script())
    assert rc < 0 and fault == (1, 0x03000004), fault
    refusals += 1
    counts['refusals'] = refusals


def main():
    elf = (DECOMP / 'config/SCUS_971.12').read_bytes()
    assert hashlib.sha256(elf).hexdigest() == ELF_SHA, 'not the pinned boot ELF'
    lib = build_lib()
    ram, spad = EO.load_beat(BEAT)
    b = Bench(lib, elf, bytes(ram), bytes(spad))
    counts = {}
    burst_cases(b, counts)
    lifetimes(b, counts)
    handler_cases(b, counts)
    contact_and_refusals(b, counts)
    missing = (set(RUN) | set(STUB)) - b.seen
    assert not missing, ('callees never exercised', sorted(hex(m) for m in missing))
    counts['cases'], counts['checks'] = b.cases, b.checks
    (OUT / 'report.json').write_text(json.dumps(counts, indent=1) + '\n')
    banner(f'{counts["burst_cases"]} 001E3630 cases', f'{counts["lifetime_ticks"]} lifetime ticks',
           f'{counts["handler_cases"]} 001ECE70 cases', f'{counts["refusals"]} refusals',
           f'{b.checks} checks')
    print('PASS test_effect_001E3630_reference')
    return 0


if __name__ == '__main__':
    sys.exit(main())
