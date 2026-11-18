#!/usr/bin/env python3
"""Execute the original AREA21 Use classifier and compare em_area21_use.c.

docs/AREA21_PORT.md "Use". The user's pinned ELF supplies every instruction;
none are embedded here. Routines executed unmodified:

  001AAC00  the class-4 list walk (gate D_0028A9A0, D_00275B7C / D_00275B84)
  001AA8D0  the 0x4A placement test
  001AAA90  the 0x4B mount test

Every other callee is hooked, scripted per case and recorded on both sides:
0011DF78 (fabs), 0011E748 (sqrt), 001B1470 (the angle wrap) return scripted
floats chosen at and one ulp past each limit the tests compare against
(3.0, 3.5, 8.0, 4.0); 001026A0 writes a scripted quadword. The test asserts
that the hooked set is exactly the set of jal targets of the three routines.

Compared per case: the return value; every hook call with its argument
bits, and for 001026A0 its three addresses and the 16 bytes at its vector
argument; the low 8 MB of RAM and the scratchpad after the call (so a store
outside the expected windows fails). Worker faults: a hook failing at call
k must stop the native routine with -1 and memory as the original had it
at that call's entry. Coverage: every word of the three routines executes.

Default run: a fixed-seed sample (~1 s); EM_TEST_FULL=1 the whole sweep.
"""
import ctypes as C
import os
import random
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
from test_player_slide_reference import EE, read_elf  # noqa: E402

OUT = ROOT / 'build' / 'area21_use_reference'
WALK, PLACE, MOUNT = 0x1AAC00, 0x1AA8D0, 0x1AAA90
SIZES = {WALK: 0xF4, PLACE: 0x1B8, MOUNT: 0x164}
HOOKS = {0x11DF78: 'fabs', 0x11E748: 'sqrt', 0x1B1470: 'wrap', 0x1026A0: 'xform'}
GATE, COUNT, CURSOR = 0x28A9A0, 0x275B84, 0x275B7C
PLAYER, OBJECTS, LIST = 0x680000, 0x690000, 0x6A0000
LOW = 0x800000


def F(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]


def next_up(bits):
    return bits + 1


def check_callee_set(ee):
    targets = set()
    for start, size in SIZES.items():
        for pc in range(start, start + size, 4):
            word = ee.load(pc)
            if word >> 26 == 3:
                targets.add((word & 0x3FFFFFF) << 2)
    targets -= set(SIZES)
    assert targets == set(HOOKS), ('callee set', sorted(map(hex, targets)))


class CoverEE(EE):
    def __init__(self, elf):
        super().__init__(elf)
        self.seen = set()

    def execute(self, word, pc):
        self.seen.add(pc)
        return super().execute(word, pc)

    def branch(self, word, pc):
        b = super().branch(word, pc)
        if b is not None:
            self.seen.add(pc)
        return b


# ---- native ------------------------------------------------------------
BYTES = C.CFUNCTYPE(C.c_void_p, C.c_void_p, C.c_uint32, C.c_uint32, C.c_int)
XFORM = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint32)
FHOOK = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_float, C.POINTER(C.c_float))


class Hooks(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('bytes', BYTES), ('w_001026A0', XFORM),
                ('w_0011DF78', FHOOK), ('w_0011E748', FHOOK), ('w_001B1470', FHOOK)]


class Fault(C.Structure):
    _fields_ = [('address', C.c_uint32), ('code', C.c_int32)]


def build_native():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('area21_use.dylib' if sys.platform == 'darwin' else 'area21_use.so')
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                    '-shared', '-fPIC', '-Isrc', 'src/game/em_area21_use.c', '-o', str(lib)],
                   cwd=ROOT, check=True)
    native = C.CDLL(str(lib))
    native.em_area21_use_001AAC00.argtypes = [C.POINTER(Hooks), C.c_uint32, C.c_uint32, C.c_uint32,
                                              C.POINTER(C.c_int32), C.POINTER(Fault)]
    return native


# ---- cases -------------------------------------------------------------
# Every limit the two tests compare against, and one ulp past it.
FABS = [F(1.0), F(3.0), next_up(F(3.0)), F(4.0), next_up(F(4.0)), F(20.0)]
SQRT = [F(1.0), F(3.5), next_up(F(3.5)), F(8.0), next_up(F(8.0)), F(20.0)]
# Two words no path reaches: 001AAC00's compiler-placed copies of an
# argument move after an unconditional branch (its delay slot ran instead).
DEAD = {0x1AAC84, 0x1AACA4}


def make_case(seed):
    rng = random.Random(seed)
    n = rng.choice([0, 1, 1, 2, 3, 4])
    objs = []
    for i in range(n):
        objs.append({'tag': rng.choice([1, 1, 1, 0, 2]), 'kind': rng.choice([0x4A, 0x4A, 0x4B, 0x4B, 0x31, 0x4C]),
                     'f': [F(rng.uniform(-300, 300)) for _ in range(4)] + [F(rng.uniform(-7, 7))],
                     'm': [F(rng.uniform(-2, 2)) for _ in range(16)]})
    return {'seed': seed, 'gate': rng.choice([0, 0, 0, 0, 1, 0x8000]), 'objs': objs,
            # a far player now and then: dx * dx past the float range (the
            # MADD sum saturates; MUL then ADD would too: both products are
            # squares, so that rewrite is equivalent here)
            'player': [F(rng.choice([rng.uniform(-300, 300)] * 3 + [rng.choice([-1, 1]) * 2.6e19]))
                       for _ in range(3)],
            'scripts': [rng.randrange(1 << 30) for _ in range(64)],
            'fail': rng.choice([None] * 6 + list(range(4)))}


def scripted(case, name, k):
    """The k-th result of a hook: a limit for fabs / sqrt, random otherwise."""
    r = case['scripts'][k % 64]
    if name == 'fabs':
        return FABS[r % len(FABS)]
    if name == 'sqrt':
        return SQRT[r % len(SQRT)]
    return F(random.Random(r).uniform(-4, 4))


def seed_memory(case, put32, put16, put8):
    put16(GATE, case['gate'])
    put16(COUNT, len(case['objs']))
    put32(CURSOR, LIST)
    for i, o in enumerate(case['objs']):
        e = OBJECTS + i * 0x300
        put32(LIST + 4 * i, e)
        put8(e, o['tag'])
        put8(e + 3, o['kind'])
        for j, off in enumerate((0xB0, 0xB4, 0xB8, 0xBC, 0xC4)):
            put32(e + off, o['f'][j])
        for j in range(16):
            put32(e + 0xD0 + 4 * j, o['m'][j])
    for j, off in enumerate((0xA0, 0xA4, 0xA8)):
        put32(PLAYER + off, case['player'][j])
    for off in range(0x200, 0x2A0, 4):
        put32(PLAYER + off, 0xDEAD0000 | off)


def run_original(elf, case, cover):
    ee = CoverEE(elf) if cover else EE(elf)
    ee.r[29] = 0x7F0F0000
    put32 = lambda a, v: ee.save(a, v, 4)
    put16 = lambda a, v: ee.save(a, v, 2)
    put8 = lambda a, v: ee.save(a, v, 1)
    seed_memory(case, put32, put16, put8)
    calls, snaps = [], []

    def snap():
        snaps.append((bytes(ee.mem[:LOW]), bytes(ee.spad)))

    def fhook(name):
        def h(e):
            snap()
            arg = e.f[12] & 0xFFFFFFFF
            calls.append((name, arg))
            e.f[0] = scripted(case, name, len(calls) - 1)
        return h

    def xform(e):
        snap()
        out, m, v = (e.r[4] & 0xFFFFFFFF, e.r[5] & 0xFFFFFFFF, e.r[6] & 0xFFFFFFFF)
        calls.append(('xform', out, m, v, e.read(v, 16)))
        rng = random.Random(case['scripts'][(len(calls) - 1) % 64])
        for j in range(4):
            e.save(out + 4 * j, F(rng.uniform(-300, 300)))

    for addr, name in HOOKS.items():
        ee.hooks[addr] = xform if name == 'xform' else fhook(name)
    ee.call(WALK, (PLAYER, PLAYER + 0x290, PLAYER + 0x218))
    return ee, calls, snaps, s32(ee.r[2])


def s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def run_native(native, base_mem, base_spad, case, fail_at):
    mem = C.create_string_buffer(bytes(base_mem), LOW)
    spad = C.create_string_buffer(bytes(base_spad), 0x4000)
    calls = []
    mem_addr, spad_addr = C.addressof(mem), C.addressof(spad)

    def bytes_cb(ctx, address, size, write):
        if address < LOW and address + size <= LOW:
            return mem_addr + address
        if 0x70000000 <= address and address + size <= 0x70004000:
            return spad_addr + address - 0x70000000
        return None

    def fhook(name):
        def h(ctx, x, out):
            calls.append((name, F(x)))
            if fail_at == len(calls) - 1:
                return -1
            out[0] = struct.unpack('<f', struct.pack('<I', scripted(case, name, len(calls) - 1)))[0]
            return 0
        return h

    def xform(ctx, out, m, v):
        vec = bytes(C.string_at(spad_addr + v - 0x70000000, 16)) if v >= 0x70000000 \
            else bytes(C.string_at(mem_addr + v, 16))
        calls.append(('xform', out, m, v, vec))
        if fail_at == len(calls) - 1:
            return -1
        rng = random.Random(case['scripts'][(len(calls) - 1) % 64])
        data = b''.join(struct.pack('<I', F(rng.uniform(-300, 300))) for _ in range(4))
        dst = spad_addr + out - 0x70000000 if out >= 0x70000000 else mem_addr + out
        C.memmove(dst, data, 16)
        return 0

    cbs = [BYTES(bytes_cb), XFORM(xform), FHOOK(fhook('fabs')), FHOOK(fhook('sqrt')), FHOOK(fhook('wrap'))]
    hooks = Hooks(None, *cbs)
    result, fault = C.c_int32(0), Fault()
    rc = native.em_area21_use_001AAC00(C.byref(hooks), PLAYER, PLAYER + 0x290, PLAYER + 0x218,
                                       C.byref(result), C.byref(fault))
    return rc, result.value, calls, mem.raw[:LOW], spad.raw, fault


def fp_calls(calls):
    """Float hook arguments as bits (a C float round trip keeps them)."""
    return [c if c[0] == 'xform' else (c[0], c[1] & 0xFFFFFFFF) for c in calls]


def main():
    elf = read_elf()
    check_callee_set(EE(elf))
    native = build_native()
    total = reference_mode.pick(4000, 4000)
    cases = [make_case(s) for s in range(total)]
    chosen = reference_mode.select(cases, 300, 21, axes=(lambda c: len(c['objs']), lambda c: c['gate'],
                                                         lambda c: c['fail']))
    seen = set()
    results = {0: 0, 1: 0, 2: 0, 3: 0}
    faults = 0
    for case in chosen:
        base = EE(elf)
        base.r[29] = 0x7F0F0000
        seed_memory(case, lambda a, v: base.save(a, v, 4), lambda a, v: base.save(a, v, 2),
                    lambda a, v: base.save(a, v, 1))
        ee, calls, snaps, v0 = run_original(elf, case, cover=True)
        seen |= ee.seen
        fail = case['fail'] if case['fail'] is not None and case['fail'] < len(calls) else None
        rc, r, ncalls, nmem, nspad, fault = run_native(native, base.mem[:LOW], base.spad, case, fail)
        where = ('seed', case['seed'])
        if fail is None:
            assert rc == 0, (where, 'native fault', hex(fault.address), fault.code)
            assert r == v0, (where, 'result', r, v0)
            assert fp_calls(ncalls) == fp_calls(calls), (where, 'calls', ncalls, calls)
            assert nmem == bytes(ee.mem[:LOW]), (where, 'memory')
            assert nspad == bytes(ee.spad), (where, 'scratchpad')
            results[v0] = results.get(v0, 0) + 1
        else:
            assert rc == -1 and fault.code == 2, (where, 'fault contract', rc, fault.code)
            assert fp_calls(ncalls) == fp_calls(calls[:fail + 1]), (where, 'calls before the fault')
            assert (nmem, nspad) == snaps[fail], (where, 'memory at the failing call')
            faults += 1
    words = {pc for start, size in SIZES.items() for pc in range(start, start + size, 4)}
    # j / jal / jr / jalr go straight to their delay slot in EE.run: a jump
    # word ran when its delay slot ran (no delay slot here is a branch target).
    probe = EE(elf)
    for pc in words:
        w = probe.load(pc)
        if (w >> 26 in (2, 3) or (w >> 26 == 0 and w & 63 in (8, 9))) and pc + 4 in seen:
            seen.add(pc)
    missing = sorted(words - seen - DEAD)
    if reference_mode.FULL or len(chosen) >= 300:
        assert not missing, ('words never executed', [hex(p) for p in missing], results)
    reference_mode.banner(reference_mode.part(len(chosen), total, 'cases'),
                          f'results {results}', f'{faults} worker faults',
                          f'{len(words) - len(DEAD) - len(missing)} of {len(words) - len(DEAD)} reachable words')
    print('PASS: em_area21_use matches the original 001AAC00 / 001AA8D0 / 001AAA90')


if __name__ == '__main__':
    main()
