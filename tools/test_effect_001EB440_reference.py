#!/usr/bin/env python3
"""Execute the original effect handlers 001EB440, 001EB250 and 001EB600 and compare
em_effect_001EB440.c (docs/AREA21_PORT.md "Use and the effect 001EB440").

The user's pinned ELF supplies every instruction. 001EB440 runs unmodified;
its callees 0021B9A0, 001CFB50 and 001CFBE0 are hooked and recorded on both
sides (the test asserts that the hooked set is exactly its jal targets).
Designed states: the work block pointer D_00275C34, its seed word +4 over
the whole 16-bit range that the fraction reads (0, 1, 0x7FFF.., 0xFFFF..,
random) and its +0x54 scale; a0 / a1 random. Compared: every hook call with
its integer and float-bit arguments, RAM and scratchpad after the call
(the seed's two stores), and the fault contract (a failing hook at call k
stops the native with -1 and memory as the original had it at that call).
001EB250 also lowers a0 + 0x38 by 2 unless +0x54 <= 0.8: its scale is
drawn at 0.8, one ulp past it and away from it, and a0 points at a
designed matrix. 001EB600 (lane PORT-A21-BOSS) lowers it by 3 unless
+0x54 <= 0 (drawn at +0, -0 and the least positive float) and stores 20 at
a0 + 0x34. 001EE4E0 (lane PORT-A21-GATLING, the Gatling's hit spray) has
no fog call, rewrites the parameter rows of D_002576C0 / D_00257750 /
D_002577E0 (compared in RAM), and eases the work block's +8 toward 0.02
(drawn at 0.02, one ulp either side, above and below). Coverage: every word
of the four.
"""
import ctypes as C
import random
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
from test_player_slide_reference import EE, read_elf  # noqa: E402
from test_area21_use_reference import CoverEE  # noqa: E402

OUT = ROOT / 'build' / 'effect_001EB440_reference'
ENTRIES = {0x1EB440: 0x1B8, 0x1EB250: 0x1F0, 0x1EB600: 0x1F0, 0x1EE4E0: 0x350}
HOOKS = {0x21B9A0: 'fog', 0x1CFB50: 'xform', 0x1CFBE0: 'queue'}
WORK = 0x275C34
LOW = 0x800000


def F(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]


BYTES = C.CFUNCTYPE(C.c_void_p, C.c_void_p, C.c_uint32, C.c_uint32, C.c_int)
FOG = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_uint32, C.c_uint32)
XF = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_int32, C.c_uint32, C.c_uint32, C.c_uint32,
                 C.c_uint32, C.c_uint32, C.c_uint32)
QUEUE = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_int32, C.c_uint32, C.c_uint32, C.c_int32)


class Hooks(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('bytes', BYTES), ('w_0021B9A0', FOG), ('w_001CFB50', XF),
                ('w_001CFBE0', QUEUE)]


class Fault(C.Structure):
    _fields_ = [('address', C.c_uint32), ('code', C.c_int32)]


def build_native():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('effect.dylib' if sys.platform == 'darwin' else 'effect.so')
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                    '-shared', '-fPIC', '-Isrc', 'src/game/em_effect_001EB440.c', '-o', str(lib)],
                   cwd=ROOT, check=True)
    native = C.CDLL(str(lib))
    for name in ('em_effect_001EB440', 'em_effect_001EB250', 'em_effect_001EB600', 'em_effect_001EE4E0'):
        getattr(native, name).argtypes = [C.POINTER(Hooks), C.c_uint32, C.c_int32, C.POINTER(Fault)]
    return native


def make_case(seed):
    case = draw_case(seed)
    if case['entry'] == 0x1EE4E0 and case['fail'] == 6:     # 001EE4E0 makes six calls, not seven
        case['fail'] = None
    return case


def draw_case(seed):
    rng = random.Random(seed)
    work = 0x6B0000 + rng.randrange(0, 0x100) * 0x10
    s = rng.choice([0, 1, 0x7FFF0000, 0x7FFFFFFF, 0x80000000, 0xFFFF0000, 0xFFFFFFFF, 0x00010000,
                    rng.randrange(1 << 32), rng.randrange(1 << 32)])
    return {'seed': seed, 'entry': sorted(ENTRIES)[seed % 4], 'work': work, 'rng': s,
            'step': rng.choice([F(0.02), F(0.02) + 1, F(0.02) - 1, F(0.5), F(0.01), 0, 0x80000000,
                                F(rng.uniform(-1, 1))]),
            'scale': rng.choice([F(rng.uniform(-50, 50)), F(0.8), F(0.8) + 1, F(0.5), F(3.0), 0, 1, 0x80000000]),
            'a0': 0x6C0000 + rng.randrange(0x100) * 0x40, 'z': F(rng.uniform(-500, 500)), 'a1': rng.randrange(-5, 1 << 16),
            'fail': rng.choice([None] * 14 + list(range(7)))}


def seed_mem(case, save):
    save(WORK, case['work'])
    save(case['work'] + 4, case['rng'])
    save(case['work'] + 0x54, case['scale'])
    save(case['work'] + 8, case['step'])
    save(case['a0'] + 0x38, case['z'])


def run_original(elf, case):
    ee = CoverEE(elf)
    ee.r[29] = 0x7F0F0000
    seed_mem(case, lambda a, v: ee.save(a, v, 4))
    calls, snaps = [], []

    def hook(name):
        def h(e):
            snaps.append((bytes(e.mem[:LOW]), bytes(e.spad)))
            r = [e.r[i] & 0xFFFFFFFF for i in range(4, 9)]
            f = [e.f[i] & 0xFFFFFFFF for i in range(12, 17)]
            if name == 'fog':
                calls.append(('fog', s32(r[0]), f[0], f[1]))
            elif name == 'xform':
                calls.append(('xform', r[0], s32(r[1]), r[2], *f))
            else:
                calls.append(('queue', s32(r[0]), s32(r[1]), r[2], r[3], s32(r[4])))
            e.r[2] = 0
        return h
    for a, n in HOOKS.items():
        ee.hooks[a] = hook(n)
    ee.call(case['entry'], (case['a0'], case['a1']))
    return ee, calls, snaps


def s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def run_native(native, base, case, fail):
    mem = C.create_string_buffer(bytes(base.mem[:LOW]), LOW)
    spad = C.create_string_buffer(bytes(base.spad), 0x4000)
    ma, sa = C.addressof(mem), C.addressof(spad)
    calls = []

    def by(ctx, a, n, w):
        if a + n <= LOW:
            return ma + a
        if 0x70000000 <= a and a + n <= 0x70004000:
            return sa + a - 0x70000000
        return None

    def done():
        return -1 if fail == len(calls) - 1 else 0

    def fog(ctx, mode, f12, f13):
        calls.append(('fog', mode, f12, f13)); return done()

    def xf(ctx, a0, a1, a2, *f):
        calls.append(('xform', a0, a1, a2, *f)); return done()

    def queue(ctx, a0, a1, a2, a3, a4):
        calls.append(('queue', a0, a1, a2, a3, a4)); return done()
    cbs = [BYTES(by), FOG(fog), XF(xf), QUEUE(queue)]
    fault = Fault()
    fn = getattr(native, 'em_effect_%08X' % case['entry'])
    rc = fn(C.byref(Hooks(None, *cbs)), case['a0'], case['a1'], C.byref(fault))
    return rc, calls, mem.raw[:LOW], spad.raw, fault


def main():
    elf = read_elf()
    probe = EE(elf)
    words = [pc for e, n in ENTRIES.items() for pc in range(e, e + n, 4)]
    for e, n in ENTRIES.items():
        targets = {(probe.load(pc) & 0x3FFFFFF) << 2 for pc in range(e, e + n, 4) if probe.load(pc) >> 26 == 3}
        assert targets == (set(HOOKS) - {0x21B9A0} if e == 0x1EE4E0 else set(HOOKS)), sorted(map(hex, targets))
    native = build_native()
    cases = [make_case(s) for s in range(1200)]
    chosen = reference_mode.select(cases, 100, 7, axes=(lambda c: c['entry'], lambda c: c['scale'] in (F(0.8), F(0.8) + 1, 0, 1, 0x80000000), lambda c: c['fail'],
                                                         lambda c: c['step'] in (F(0.02), F(0.02) + 1, F(0.02) - 1)))
    seen, faults = set(), 0
    for case in chosen:
        base = EE(elf)
        seed_mem(case, lambda a, v: base.save(a, v, 4))
        ee, calls, snaps = run_original(elf, case)
        seen |= ee.seen
        rc, ncalls, nmem, nspad, fault = run_native(native, base, case, case['fail'])
        where = ('seed', case['seed'])
        if case['fail'] is None:
            assert rc == 0, (where, hex(fault.address), fault.code)
            assert ncalls == calls, (where, ncalls, calls)
            assert nmem == bytes(ee.mem[:LOW]) and nspad == bytes(ee.spad), (where, 'memory')
        else:
            k = case['fail']
            assert rc == -1 and fault.code == 2, (where, rc, fault.code)
            assert ncalls == calls[:k + 1], (where, 'calls')
            assert (nmem, nspad) == snaps[k], (where, 'memory at the failing call')
            faults += 1
    for pc in words:
        w = probe.load(pc)
        if (w >> 26 in (2, 3) or (w >> 26 == 0 and w & 63 in (8, 9))) and pc + 4 in seen:
            seen.add(pc)
    missing = [hex(pc) for pc in words if pc not in seen]
    assert not missing, ('words never executed', missing)
    reference_mode.banner(reference_mode.part(len(chosen), len(cases), 'cases'), f'{faults} worker faults',
                          f'{len(words)} of {len(words)} words')
    print('PASS: em_effect_001EB440 matches the original 001EB440, 001EB250, 001EB600 and 001EE4E0')


if __name__ == '__main__':
    main()
