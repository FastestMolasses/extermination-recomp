#!/usr/bin/env python3
"""Execute the original effect node behaviour 001E76A0, its ring 001E0AC0
and the sprite 001E0750 and compare src/game/em_haze_ring.c
(docs/AREA21_PORT.md "Haze ring").

The user's pinned ELF supplies every instruction; none are embedded here.
The oracle is test_far_light_reference.FarEE (the measured EE / VU0 model
with the clip test and the CLIP register). 001E0AC0 runs 001E0750 as
original code and 001E76A0 runs 001E0AC0; 001CD370 and float_to_int
(001281C0) run as original code too. The hooks 001028B8 (it stores a
scripted quadword at its destination), 001AFC10, 001D1F80, 001D7510,
001D1FF0, sin 0011DE90, cos 0011E2A8, rand 00122BB8 and 001CAAC0 are
scripted identically on both sides; their arguments and the stack pointer
at each call are compared.

Each case designs a state at original addresses: D_00275670 (the context
0x811CC0: the channel-3 cursor +0x1C, 001CD370(0)'s clip matrix +0x2240),
the camera rows 0x70003AC0, the style words D_0026E9A0, the corner pairs
D_00253530 (the ELF's own or designed), the node (state 0..3 and others,
the timer at, around and far from 1.0 and 0, signed zeros), the packet
window and the VU0 registers (vf23 the fog ramp, CLIP's older flags, the
upper half of t0 that becomes vf3.zw). Camera and clip matrix put the
points inside, across and outside the clip volume and behind the camera;
the fog ramp covers both bounds; rand words and sin / cos results include
0, -0, extremes and the true values.

Compared per case: every byte of every window (and no original store
outside them, the stack excepted), the hook calls in order with their
arguments and stack pointer, and the VU0 registers (vf1, vf2, vf3.x / .y,
vf23..vf31, ACC, Q, CLIP; vf3.z / .w unchanged in the port). Every word
of the three functions must execute (coverage). Fail-stop cases check an
unmapped window latches, blocks later calls, and that a NULL hook faults
only when reached.

Default: a fixed-seed sample (about 10 s); EM_TEST_FULL=1: the wider
sweep. EM_HAZE_RING_MUTANTS=1 also builds source mutants and requires each
to be caught.
"""
import ctypes as C
import math
import os
import random
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode as RM  # noqa: E402
import test_far_light_reference as FL  # noqa: E402
from test_player_slide_reference import read_elf, RETURN  # noqa: E402

OUT = ROOT / 'build' / 'haze_ring'
MASK = 0xFFFFFFFF
U64 = 0xFFFFFFFFFFFFFFFF
CTX = 0x00811CC0
NODE = 0x007BCB00
PKT = 0x01E00000
SP = 0x01FFF000
POS = SP - 0x300            # 001E0750's argument vector in direct cases
STYLE, CORNER = 0x0026E9A0, 0x00253530
WINDOWS = [(0x00275670, 4), (CTX, 0x2300), (0x70003600, 0x10), (0x70003AC0, 0x40), (STYLE, 0x10),
           (CORNER, 0x30), (NODE, 0x100), (PKT, 0xA000)]
COMPARED_VF = [1, 2] + list(range(23, 32))
FUNCS = {0x1E76A0: 0xD8, 0x1E0AC0: 0x168, 0x1E0750: 0x36C}

fb = FL.fb


def flt(b):
    return struct.unpack('<f', struct.pack('<I', b & MASK))[0]


# ---------------------------------------------------------------- native
MAPFN = FL.MAPFN
F3 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint32, C.c_uint32)
F1 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_uint32)
FI3 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_int32, C.c_int32, C.c_int32)
FI2 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_int32, C.c_int32)
FMATH = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
FRAND = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_int32))
F2 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint32)


class Ring(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('map', MAPFN), ('vu', C.POINTER(FL.Vu)), ('w_001028B8', F3),
                ('w_001AFC10', F1), ('w_001D1F80', FI3), ('w_001D7510', FI3), ('w_001D1FF0', FI2),
                ('w_0011DE90', FMATH), ('w_0011E2A8', FMATH), ('w_00122BB8', FRAND), ('w_001CAAC0', F2),
                ('fault', C.c_int32), ('fault_function', C.c_uint32), ('fault_address', C.c_uint32)]


def build(src=None, tag='haze_ring'):
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / (tag + ('.dylib' if sys.platform == 'darwin' else '.so'))
    srcs = [src or 'src/game/em_haze_ring.c', 'src/game/em_far_light.c', 'src/game/em_effect_original.c',
            'src/game/em_player_stage_workers.c']
    warn = ['-Wall', '-Wextra', '-Werror'] if src is None else ['-w']   # a mutant may leave an argument unused
    subprocess.run(['cc', '-std=c11', '-O2', *warn, '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', *srcs, '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    P = C.POINTER(Ring)
    n.em_haze_ring_001E76A0.argtypes = [P, C.c_uint32, C.c_uint32]
    n.em_haze_ring_001E0AC0.argtypes = [P, C.c_uint32, C.c_uint32, C.c_uint32, C.c_uint32, C.c_uint32]
    n.em_haze_ring_001E0750.argtypes = [P, C.POINTER(C.c_uint32), C.c_uint32, C.c_uint32]
    return n


class Script:
    """The hooks' results, replayed identically on both sides."""
    def __init__(self, seed, mode):
        self.seed, self.mode = seed, mode
        self.reset()

    def reset(self):
        self.rng = random.Random(self.seed)

    def rand(self):
        r = self.rng
        return r.choice((r.getrandbits(31), r.getrandbits(31), 0, 0x7FFFFFFF, 1, r.getrandbits(15)))

    def trig(self, x, fn):
        r = self.rng
        true = fb(fn(flt(x))) if math.isfinite(flt(x)) else 0
        if self.mode == 'true':
            return true
        return r.choice((true, true, 0, 0x80000000, fb(1.0), fb(-1.0), fb(r.uniform(-1, 1)), fb(1e-30)))

    def quad(self):
        r = self.rng
        return [fb(r.uniform(-50, 50)) for _ in range(3)] + [r.choice((fb(1.0), r.getrandbits(32) & 0x7F7FFFFF))]


class Native:
    def __init__(self, lib, image, vu, script):
        self.lib, self.script, self.calls = lib, script, []
        self.bufs = [(base, (C.c_uint8 * size).from_buffer_copy(image[base])) for base, size in WINDOWS]
        self.vu = FL.Vu()
        for i in range(32):
            for k in range(4):
                self.vu.vf[i][k] = vu['vf'][i][k]
        for k in range(4):
            self.vu.acc[k] = vu['acc'][k]
        self.vu.q, self.vu.clip = vu['q'], vu['clip']
        self.cb = (MAPFN(self.map), F3(self.c28b8), F1(self.cfc10), FI3(self.c1f80), FI3(self.c7510),
                   FI2(self.c1ff0), FMATH(self.sin), FMATH(self.cos), FRAND(self.rand), F2(self.caac0))
        self.g = Ring(None, self.cb[0], C.pointer(self.vu), *self.cb[1:], 0, 0, 0)
        self.fail_map = None

    def map(self, _ctx, address, size, _write):
        if self.fail_map is not None and address == self.fail_map:
            return None
        for base, buf in self.bufs:
            if base <= address and address + size <= base + len(buf):
                return C.addressof(buf) + address - base
        return None

    def c28b8(self, _c, sp, d, a, b):
        self.calls.append(('001028B8', sp, d, a, b))
        q = self.script.quad()
        p = self.map(None, d, 16, 1)
        C.memmove(p, struct.pack('<4I', *q), 16)
        return 0

    def cfc10(self, _c, sp, a):
        self.calls.append(('001AFC10', sp, a))
        return 0

    def c1f80(self, _c, sp, a, b, c):
        self.calls.append(('001D1F80', sp, a & MASK, b & MASK, c & MASK))
        return 0

    def c7510(self, _c, sp, a, b, c):
        self.calls.append(('001D7510', sp, a & MASK, b & MASK, c & MASK))
        return 0

    def c1ff0(self, _c, sp, a, b):
        self.calls.append(('001D1FF0', sp, a & MASK, b & MASK))
        return 0

    def sin(self, _c, sp, x, out):
        self.calls.append(('0011DE90', sp, x))
        out[0] = self.script.trig(x, math.sin)
        return 0

    def cos(self, _c, sp, x, out):
        self.calls.append(('0011E2A8', sp, x))
        out[0] = self.script.trig(x, math.cos)
        return 0

    def rand(self, _c, sp, out):
        self.calls.append(('00122BB8', sp))
        out[0] = self.script.rand()
        return 0

    def caac0(self, _c, sp, a, b):
        self.calls.append(('001CAAC0', sp, a, b))
        return 0

    def windows(self):
        return {base: bytes(buf) for base, buf in self.bufs}

    def vu_state(self):
        return ({i: tuple(self.vu.vf[i][k] for k in range(4)) for i in COMPARED_VF},
                tuple(self.vu.vf[3][k] for k in range(4)), tuple(self.vu.acc[k] for k in range(4)),
                self.vu.q, self.vu.clip)


# ---------------------------------------------------------------- oracle
SEEN = set()


def oracle(ee, image, vu, script, entry, ints, floats, pos, t0_high):
    for base, data in image.items():
        ee.write(base, data)
    if pos is not None:
        ee.write(POS, struct.pack('<4I', *pos))
    ee.set_vu((tuple(tuple(v) for v in vu['vf']), tuple(vu['acc']), vu['q'], vu['clip']))
    ee.rh[8] = t0_high
    calls = []

    def sp(e):
        return e.r[29] & MASK

    def h28b8(e):
        d = e.r[4] & MASK
        calls.append(('001028B8', sp(e), d, e.r[5] & MASK, e.r[6] & MASK))
        e.write(d, struct.pack('<4I', *script.quad()))

    def hrand(e):
        calls.append(('00122BB8', sp(e)))
        e.r[2] = script.rand() & U64

    def htrig(name, fn):
        def f(e):
            x = e.f[12] & MASK
            calls.append((name, sp(e), x))
            e.f[0] = script.trig(x, fn)
        return f

    def h(name, n):
        def f(e):
            calls.append((name, sp(e)) + tuple(e.r[4 + i] & MASK for i in range(n)))
        return f
    ee.hooks = {0x1028B8: h28b8, 0x1AFC10: h('001AFC10', 1), 0x1D1F80: h('001D1F80', 3),
                0x1D7510: h('001D7510', 3), 0x1D1FF0: h('001D1FF0', 2), 0x11DE90: htrig('0011DE90', math.sin),
                0x11E2A8: htrig('0011E2A8', math.cos), 0x122BB8: hrand, 0x1CAAC0: h('001CAAC0', 2)}
    stores = []
    save, load = ee.save, ee.load

    def saved(address, value, size=4):
        stores.append((address & MASK, size))
        save(address, value, size)

    def loaded(address, size=4):
        a = address & MASK
        for f, n in FUNCS.items():
            if f <= a < f + n:
                SEEN.add(a)
        return load(address, size)
    ee.save, ee.load = saved, loaded
    try:
        ee.r[29] = SP
        for i, v in enumerate(ints):
            ee.r[4 + i] = v & U64
        for i, v in enumerate(floats):
            ee.f[12 + i] = v
        ee.r[31] = RETURN
        ee.run(entry)
    finally:
        del ee.save, ee.load
    for a, size in stores:
        if SP - 0x1000 <= a < SP:
            continue
        assert any(b <= a and a + size <= b + n for b, n in WINDOWS), ('original store outside the windows', hex(a))
    wins = {base: bytes(ee.read(base, size)) for base, size in WINDOWS}
    st = ee.vu_state()
    vus = ({i: tuple(st[0][i]) for i in COMPARED_VF}, tuple(st[0][3]), tuple(st[1]), st[2], st[3])
    return wins, calls, vus


# ---------------------------------------------------------------- states
def camera(rng):
    """vf28..vf31: screen x = a x' + 2048 w, y = b y' + 2048 w, w = depth
    along a yawed axis plus an offset (points behind the camera too)."""
    t = rng.uniform(-math.pi, math.pi)
    c, s = math.cos(t), math.sin(t)
    a, b = rng.uniform(300, 900), rng.uniform(200, 600)
    dz = rng.choice((rng.uniform(100, 3000), rng.uniform(-800, 800), rng.uniform(1e4, 1e5), 0.0))
    ey = rng.uniform(-200, 200)
    zc = rng.uniform(1e5, 1e7)
    return [(a * c + 2048 * s, 2048 * s, 0, s), (0, b, 0, 0), (-a * s + 2048 * c, 2048 * c, 0, c),
            (2048 * dz, b * ey + 2048 * dz, zc, dz)]


def clip_matrix(rng):
    k = rng.choice((1.0, 0.5, 2.0, rng.uniform(0.2, 3), 0.01))
    dz = rng.choice((rng.uniform(100, 2000), rng.uniform(-500, 500), 1e6))
    return [(k, 0, 0, 0), (0, k, 0, 0), (0, 0, rng.choice((0.0, 0.5, k)), 1),
            (rng.uniform(-300, 300), rng.uniform(-50, 50), 0, dz)]


def point(rng):
    return [fb(rng.choice((rng.uniform(-200, 200), rng.uniform(-5, 5), 0.0, -0.0, rng.uniform(-1e4, 1e4))))
            for _ in range(3)] + [rng.getrandbits(32) & 0x7F7FFFFF]


def state(rng, entry, elf_corner, seed=0):
    image = {base: bytearray(rng.getrandbits(8) for _ in range(size)) for base, size in WINDOWS}

    def put(a, data):
        for base, size in WINDOWS:
            if base <= a and a + len(data) <= base + size:
                image[base][a - base:a - base + len(data)] = data
                return
        raise AssertionError(('design outside the windows', hex(a)))
    put(0x00275670, struct.pack('<I', CTX))
    put(CTX + 0x1C, struct.pack('<I', PKT + 0x10 * rng.randrange(0, 8)))
    put(CTX + 0x2240, b''.join(struct.pack('<4f', *r) for r in clip_matrix(rng)))
    put(0x70003AC0, b''.join(struct.pack('<4f', *r) for r in camera(rng)))
    put(STYLE + 0xC, struct.pack('<I', rng.choice((0x80, 0x40, 0, rng.getrandbits(32)))))
    if rng.random() < 0.5:
        put(CORNER, elf_corner)
    else:
        put(CORNER, struct.pack('<12f', *(rng.choice((rng.uniform(-24, 24), 0.0, -0.0, 1e20))
                                         for _ in range(12))))
    st = rng.choice((0, 1, 1, 1, 2, 3, 4, 0xFF))
    if entry == 0x1E76A0:
        st = (0, 1, 1, 0, 2, 3, 4, 0xFF)[seed % 8]
    timers = (1.0, fb(1.0) + 1, fb(1.0) - 1, 0.0, 2.0, 300.0, -0.0, 0.5, -5.0, 1.0000001)
    timer = rng.choice(timers)
    if entry == 0x1E76A0:       # each state meets 1.0 and its neighbours first
        timer = timers[(seed // 8) % len(timers)]
    timer = timer if isinstance(timer, float) else flt(timer)
    put(NODE + 4, bytes([st]))
    put(NODE + 0x20, struct.pack('<f', timer))
    put(NODE + 0xB0, struct.pack('<4I', *point(rng)))
    put(NODE + 0xC0, struct.pack('<4I', *point(rng)))
    fw = rng.choice((rng.uniform(-1, -0.001), 0.0, rng.uniform(-0.05, 0.05), -1e-30, 1e6, -1e6))
    fog = (rng.choice((255.0, 128.0, -1.0)), 2048.0, rng.uniform(-50, 400), fw)
    vf = [[rng.getrandbits(32) & 0x7F7FFFFF for _ in range(4)] for _ in range(32)]
    vf[0] = [0, 0, 0, 0x3F800000]
    vf[23] = [fb(x) for x in fog]
    vu = {'vf': vf, 'acc': [rng.getrandbits(32) & 0x7F7FFFFF for _ in range(4)],
          'q': fb(rng.uniform(-2, 2)), 'clip': rng.getrandbits(24)}
    p = (rng.choice((NODE + 0xB0, NODE + 0xB0, NODE + 0x40)), STYLE, fb(rng.choice((75.0, 30.0, 0.0, -2.5, 1e30))),
         fb(rng.choice((30.0, 75.0, -0.0, 7.0))))
    return {k: bytes(v) for k, v in image.items()}, vu, point(rng), p, rng.getrandbits(64)


# ---------------------------------------------------------------- cases
def compare(lib, case):
    entry, seed, trig = case
    rng = random.Random(seed * 7919 + entry)
    image, vu, pos, args, t0h = state(rng, entry, ELF_CORNER, seed)
    script = Script(seed, trig)
    if entry == 0x1E76A0:
        ints, floats, opos = (NODE,), (), None
    elif entry == 0x1E0AC0:
        ints, floats, opos = (args[0], args[1]), (args[2], args[3]), None
    else:
        ints, floats, opos = (POS, STYLE), (), pos
    want = oracle(EE, image, vu, script, entry, ints, floats, opos, t0h)
    script.reset()
    n = Native(lib, image, vu, script)
    if entry == 0x1E76A0:
        rc = lib.em_haze_ring_001E76A0(C.byref(n.g), NODE, SP)
    elif entry == 0x1E0AC0:
        rc = lib.em_haze_ring_001E0AC0(C.byref(n.g), args[0], args[1], args[2], args[3], SP)
    else:
        rc = lib.em_haze_ring_001E0750(C.byref(n.g), (C.c_uint32 * 4)(*pos), STYLE, SP)
    label = (hex(entry), seed)
    if rc != 0:
        return (label, 'native fault', n.g.fault, hex(n.g.fault_function), hex(n.g.fault_address))
    got = n.windows()
    for base, size in WINDOWS:
        if got[base] != want[0][base]:
            bad = [hex(base + i) for i in range(0, size, 4) if got[base][i:i + 4] != want[0][base][i:i + 4]]
            return (label, 'window', hex(base), len(bad), bad[:8])
    if n.calls != want[1]:
        k = next((i for i, (a, b) in enumerate(zip(n.calls, want[1])) if a != b), min(len(n.calls), len(want[1])))
        return (label, 'calls', k, n.calls[k:k + 2], want[1][k:k + 2], len(n.calls), len(want[1]))
    nv = n.vu_state()
    if nv[0] != want[2][0] or nv[2:] != want[2][2:]:
        return (label, 'vu0', nv, want[2])
    if nv[1][:2] != want[2][1][:2]:
        return (label, 'vf3.xy', nv[1], want[2][1])
    drew = any(c[0] == '00122BB8' for c in want[1])
    if drew and nv[1][2:] != tuple(vu['vf'][3][2:]):
        return (label, 'vf3.zw changed in the port', nv[1])
    if not drew and nv[1] != tuple(vu['vf'][3]):
        return (label, 'vf3 changed without a draw', nv[1])
    return None, outcome(entry, want, image)


def outcome(entry, want, image):
    sprites = sum(1 for c in want[1] if c[0] == '00122BB8') // 12
    if entry == 0x1E76A0:
        st = image[NODE][4]
        after = want[0][NODE][4]
        return '76A0 state %d -> %d%s' % (st if st < 4 else 9, after if after < 4 else 9, ' drew' if sprites else '')
    if entry == 0x1E0AC0:
        return '0AC0 ' + ('none' if not sprites else 'all' if sprites == 64 else 'some')
    return '0750 ' + ('drew' if sprites else 'clipped')


def run_cases(lib, cases):
    return [compare(lib, c) for c in cases]


def worker(chunk):
    out = run_cases(LIB, chunk)
    return out, sorted(SEEN)


def fail_stop(lib):
    rng = random.Random(7)
    image, vu, pos, args, _ = state(rng, 0x1E0750, ELF_CORNER)
    n = Native(lib, image, vu, Script(1, 'true'))
    n.fail_map = 0x00275670
    assert lib.em_haze_ring_001E0750(C.byref(n.g), (C.c_uint32 * 4)(*pos), STYLE, SP) == -1
    assert n.g.fault == 1 and n.g.fault_function == 0x1E0750 and n.g.fault_address == 0x00275670
    n.fail_map = None
    assert lib.em_haze_ring_001E76A0(C.byref(n.g), NODE, SP) == -1, 'latched fault not kept'
    assert n.calls == []
    # a NULL hook faults only when reached: 001AFC10 is not reached in state 1
    img = dict(image)
    node = bytearray(img[NODE])
    node[4] = 1
    img[NODE] = bytes(node)
    n2 = Native(lib, img, vu, Script(1, 'true'))
    n2.g.w_001AFC10 = F1()
    n2.g.w_001CAAC0 = F2()
    assert lib.em_haze_ring_001E76A0(C.byref(n2.g), NODE, SP) == -1
    assert n2.g.fault == 2 and n2.g.fault_function == 0x1CAAC0, (n2.g.fault, hex(n2.g.fault_function))
    assert n2.calls and n2.calls[-1][0] != '001CAAC0'


MUTANTS = [
    ('the timer 300 (one ulp)', '#define F_300    0x43960000u', '#define F_300    0x43960001u'),
    ('the s jitter span (one ulp)', '#define F_S_SPAN 0x39CCCCCDu', '#define F_S_SPAN 0x39CCCCCEu'),
    ('the t jitter base (one ulp)', '#define F_T_BASE 0xB9EA0EA1u', '#define F_T_BASE 0xB9EA0EA2u'),
    ('the jitter scale', 'const u32 jitter_s = em_ee_mul_bits(scale, t);', 'const u32 jitter_s = t;'),
    ('a corner offset index', 'corner[2 * i + 1]', 'corner[2 * i]'),
    ('the closing tag 0x60', 'st8(h, cur + 3u, 0x60u, fn)', 'st8(h, cur + 3u, 0x70u, fn)'),
    ('the ring radius fy', 'em_ee_mul_bits(fy, c)', 'em_ee_mul_bits(fx, c)'),
    ('the step count 64', 'em_ee_div_bits(a, F_64)', 'em_ee_div_bits(a, F_TWO)'),
    ('the w reduction lane', 'em_vu_vec_bits(EM_VU_SUBBC, 1, 0,', 'em_vu_vec_bits(EM_VU_SUBBC, 1, 1,'),
    ('the timer compare (< for <=)', 'em_ee_c_le_bits(timer, F_ZERO)', 'em_ee_c_lt_bits(timer, F_ZERO)'),
    ('the fog max bound', 'em_vu_max_bits(v->vf[2][3], VF0[0])', 'em_vu_max_bits(v->vf[2][3], VF0[3])'),
    ('the scale factor 8 (one ulp)', '#define F_8      0x41000000u', '#define F_8      0x41000001u'),
]


def mutants(cases):
    src = (ROOT / 'src/game/em_haze_ring.c').read_text()
    caught = []
    for i, (name, old, new) in enumerate(MUTANTS):
        assert src.count(old) == 1, ('mutant site', name)
        path = OUT / ('mutant_%02d.c' % i)
        path.write_text(src.replace(old, new).replace('#include "game/em_haze_ring.h"',
                                                      '#include "game/em_haze_ring.h"\n/* mutant */'))
        lib = build(str(path), 'mutant_%02d' % i)
        bad = next((r[0] for r in run_cases(lib, cases) if r[0] is not None), None)
        assert bad is not None, ('mutant not caught', name)
        caught.append(name)
    return caught


EE = LIB = ELF_CORNER = None


def main():
    global EE, LIB, ELF_CORNER
    LIB = build()
    EE = FL.FarEE(read_elf())
    ELF_CORNER = EE.read(CORNER, 0x30)
    plan = [(0x1E0750, i, 'true' if i % 3 else 'any') for i in range(RM.pick(600, 120))]
    plan += [(0x1E0AC0, i, 'true' if i % 2 else 'any') for i in range(RM.pick(48, 10))]
    plan += [(0x1E76A0, i, 'true' if i % 2 else 'any') for i in range(RM.pick(96, 24))]
    chunks = [plan[k::8] for k in range(8)]
    results = RM.parallel_map(worker, chunks, cost=lambda ch: sum(64 if c[0] != 0x1E0750 else 1 for c in ch))
    counts, seen = {}, set()
    for res, s in results:
        seen.update(s)
        for bad, key in res:
            assert bad is None, bad
            counts[key] = counts.get(key, 0) + 1
    for need in ('0750 drew', '0750 clipped', '0AC0 some', '76A0 state 0 -> 1 drew', '76A0 state 1 -> 3 drew',
                 '76A0 state 1 -> 1 drew', '76A0 state 2 -> 2', '76A0 state 3 -> 3', '76A0 state 9 -> 9'):
        assert counts.get(need), ('outcome never reached', need, sorted(counts))
    total = sum(n // 4 for n in FUNCS.values())
    missing = [hex(f + 4 * k) for f, n in FUNCS.items() for k in range(n // 4) if f + 4 * k not in seen]
    assert not missing, ('words never executed', missing)
    fail_stop(LIB)
    line = ['%s %d' % kv for kv in sorted(counts.items())] + ['%d of %d words' % (total - len(missing), total)]
    if os.environ.get('EM_HAZE_RING_MUTANTS') == '1':
        caught = mutants([c for c in plan if c[0] != 0x1E0750] + [c for c in plan if c[0] == 0x1E0750][:40])
        line.append('%d of %d mutants caught' % (len(caught), len(MUTANTS)))
    RM.banner(*line)
    print('haze ring reference: PASS (original 001E76A0 / 001E0AC0 / 001E0750)')


if __name__ == '__main__':
    main()
