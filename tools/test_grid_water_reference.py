#!/usr/bin/env python3
"""Execute the original host workers of 001E1AD0's flag-0x22 path (001E1760
with 001E10A0 and 001E13E0, and 001E17E0) and compare
src/game/em_grid_water.c (docs/AREA21_PORT.md "Grid").

The user's pinned ELF supplies every instruction; none are embedded here.
The oracle is test_far_light_reference.FarEE (the measured EE / VU0 model
with the clip test and the CLIP register). Every callee runs as original
code (001CD370, float_to_int 001281C0) but rand 00122BB8 and 001E1760's
translated callees 001D6E60, 001D1F80, 001D1FF0, 001D6F60 and 001D1F20
(proven by their own oracles), which are scripted identically on both
sides (their arguments compared, 001D6F60's doubleword included).

Each case designs a state at original addresses: the context pointer
D_00275670 (context 0x811CC0: the channel cursors +0x10.., the TEX0 word
+0x1E0, 001CD370(2)'s clip matrix +0x22C0), the camera rows 0x70003AC0, the
surface table D_0081E0F0 (0x18-byte records x, y, z, w, u, v: heights and
velocities from small to clamping magnitudes, signed zeros), the packet
window, and the VU0 registers (vf23 the fog ramp, CLIP's older flags).
Camera and clip matrix are built so the 16 x 16 grid of 001E17E0 falls
inside, across and outside the clip volume, in front of and behind the
camera.

Compared per case: the return value, every byte of every window (and no
original store outside them, the stack excepted), the scripted calls in
order, and the VU0 registers 001E17E0 writes (vf1, vf2, vf24..vf31, ACC,
Q, CLIP) with vf23 unchanged. A fail-stop case checks an unmapped window
latches and blocks later calls, and a NULL hook faults only when reached.

Default: a fixed-seed sample; EM_TEST_FULL=1: the larger sweep.
"""
import ctypes as C
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

OUT = ROOT / 'build' / 'grid_water'
MASK = 0xFFFFFFFF
CTX = 0x00811CC0
TABLE = 0x0081E0F0
PKT = 0x01E00000
SP = 0x01FFF000
WINDOWS = [(0x00275670, 4), (CTX, 0x2400), (0x70003AC0, 0x40), (TABLE, 0x1800), (PKT, 0x8000)]
COMPARED_VF = [1, 2] + list(range(23, 32))
SCRIPTED = (0x122BB8, 0x1D6E60, 0x1D1F80, 0x1D1FF0, 0x1D6F60, 0x1D1F20)

fb = FL.fb


# ---------------------------------------------------------------- native
MAPFN = FL.MAPFN
RANDFN = C.CFUNCTYPE(C.c_int, C.c_void_p, C.POINTER(C.c_int32))
F6E60 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_int32, C.c_int32, C.c_int32)
F1F80 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_int32, C.c_int32)
F1FF0 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_int32)
F6F60 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_uint64, C.c_int32)
F1F20 = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32)


class Grid(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('map', MAPFN), ('vu', C.POINTER(FL.Vu)), ('w_00122BB8', RANDFN),
                ('w_001D6E60', F6E60), ('w_001D1F80', F1F80), ('w_001D1FF0', F1FF0), ('w_001D6F60', F6F60),
                ('w_001D1F20', F1F20), ('fault', C.c_int32), ('fault_function', C.c_uint32),
                ('fault_address', C.c_uint32)]


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('grid_water' + ('.dylib' if sys.platform == 'darwin' else '.so'))
    srcs = ['src/game/em_grid_water.c', 'src/game/em_far_light.c', 'src/game/em_effect_original.c',
            'src/game/em_player_stage_workers.c']
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', *srcs, '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    P = C.POINTER(Grid)
    n.em_grid_water_001E1760.argtypes = [P, C.c_int32]
    n.em_grid_water_001E10A0.argtypes = [P]
    n.em_grid_water_001E13E0.argtypes = [P, C.c_int32]
    n.em_grid_water_001E17E0.argtypes = [P, C.c_int32, C.POINTER(C.c_uint32)]
    return n


class Script:
    """rand()'s words, replayed identically on both sides."""
    def __init__(self, seed):
        self.seed = seed
        self.reset()

    def reset(self):
        self.rng = random.Random(self.seed)

    def rand(self):
        return self.rng.choice((self.rng.getrandbits(15), self.rng.getrandbits(31), 0, 0x7FFF, 0x3C0, 0x40))


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
        self.cb = (MAPFN(self.map), RANDFN(self.rand), F6E60(self.c6e60), F1F80(self.c1f80),
                   F1FF0(self.c1ff0), F6F60(self.c6f60), F1F20(self.c1f20))
        self.g = Grid(None, self.cb[0], C.pointer(self.vu), *self.cb[1:], 0, 0, 0)
        self.fail_map = None

    def map(self, _ctx, address, size, _write):
        if self.fail_map is not None and address == self.fail_map:
            return None
        for base, buf in self.bufs:
            if base <= address and address + size <= base + len(buf):
                return C.addressof(buf) + address - base
        return None

    def rand(self, _ctx, out):
        self.calls.append(('00122BB8',))
        out[0] = self.script.rand()
        return 0

    def c6e60(self, _c, ch, a1, a2, a3):
        self.calls.append(('001D6E60', ch & MASK, a1 & MASK, a2 & MASK, a3 & MASK))
        return 0

    def c1f80(self, _c, ch, a1, a2):
        self.calls.append(('001D1F80', ch & MASK, a1 & MASK, a2 & MASK))
        return 0

    def c1ff0(self, _c, ch, a1):
        self.calls.append(('001D1FF0', ch & MASK, a1 & MASK))
        return 0

    def c6f60(self, _c, ch, tex0, a2):
        self.calls.append(('001D6F60', ch & MASK, tex0, a2 & MASK))
        return 0

    def c1f20(self, _c, ch):
        self.calls.append(('001D1F20', ch & MASK))
        return 0

    def windows(self):
        return {base: bytes(buf) for base, buf in self.bufs}

    def vu_state(self):
        return ({i: tuple(self.vu.vf[i][k] for k in range(4)) for i in COMPARED_VF},
                tuple(self.vu.acc[k] for k in range(4)), self.vu.q, self.vu.clip)


# ---------------------------------------------------------------- oracle
def oracle(ee, image, vu, script, entry, ints):
    for base, data in image.items():
        ee.write(base, data)
    ee.set_vu((tuple(tuple(v) for v in vu['vf']), tuple(vu['acc']), vu['q'], vu['clip']))
    calls = []
    U64 = 0xFFFFFFFFFFFFFFFF

    def h_rand(e):
        calls.append(('00122BB8',))
        e.r[2] = script.rand()

    def h(name, n, wide=()):
        def f(e):
            calls.append((name,) + tuple((e.r[4 + i] & U64) if i in wide else (e.r[4 + i] & MASK) for i in range(n)))
        return f
    ee.hooks = {0x122BB8: h_rand, 0x1D6E60: h('001D6E60', 4), 0x1D1F80: h('001D1F80', 3),
                0x1D1FF0: h('001D1FF0', 2), 0x1D6F60: h('001D6F60', 3, wide=(1,)), 0x1D1F20: h('001D1F20', 1)}
    stores = []
    save = ee.save

    def saved(address, value, size=4):
        stores.append((address & MASK, size))
        save(address, value, size)
    ee.save = saved
    try:
        ee.r[29] = SP
        for i, v in enumerate(ints):
            ee.r[4 + i] = v & U64
        ee.r[31] = RETURN
        ee.run(entry)
    finally:
        del ee.save
    v0 = ee.r[2] & MASK
    for a, size in stores:
        if SP - 0x400 <= a < SP:
            continue
        assert any(b <= a and a + size <= b + n for b, n in WINDOWS), ('original store outside the windows', hex(a))
    wins = {base: bytes(ee.read(base, size)) for base, size in WINDOWS}
    st = ee.vu_state()
    vus = ({i: tuple(st[0][i]) for i in COMPARED_VF}, tuple(st[1]), st[2], st[3])
    return v0, wins, calls, vus


# ---------------------------------------------------------------- states
def table(rng):
    """The surface table: positions around the grid, heights and velocities
    mostly small (the area load's values), sometimes huge or signed zero."""
    out = bytearray()
    scale = rng.choice((0.5, 2.0, 50.0, 1e30))
    for i in range(16):
        for j in range(16):
            x = rng.choice((j * 32.0 - 256, rng.uniform(-300, 300), rng.uniform(-1e8, 1e8)))
            y = rng.choice((i * 14.0 - 112, rng.uniform(-150, 150)))
            z = rng.choice((rng.uniform(-scale, scale), 0.0, -0.0, rng.uniform(-1, 1)))
            w = rng.choice((rng.uniform(-scale, scale), 0.0, -0.0, rng.uniform(-0.1, 0.1)))
            u, v = rng.uniform(-2, 2), rng.uniform(-2, 2)
            out += struct.pack('<6f', x, y, z, w, u, v)
    return bytes(out)


def camera(rng):
    """vf28..vf31: screen x = a x' + 2048 w, y = b y' + 2048 w, w = depth
    along a yawed axis plus an offset (points behind the camera too)."""
    import math
    t = rng.uniform(-math.pi, math.pi)
    c, s = math.cos(t), math.sin(t)
    a, b = rng.uniform(300, 900), rng.uniform(200, 600)
    dz = rng.choice((rng.uniform(100, 3000), rng.uniform(-800, 800), rng.uniform(1e4, 1e5)))
    ey = rng.uniform(-200, 200)
    zc = rng.uniform(1e5, 1e7)
    # columns: x -> (a c + 2048 s, ., ., s), y -> (0, b, 0, 0), z -> (-a s + 2048 c, ., ., c)
    rows = [(a * c + 2048 * s, 2048 * s, 0, s), (0, b, 0, 0), (-a * s + 2048 * c, 2048 * c, 0, c),
            (2048 * dz, b * ey + 2048 * dz, zc, dz)]
    return rows


def clip_matrix(rng):
    k = rng.choice((1.0, 0.5, 2.0, rng.uniform(0.2, 3)))
    dz = rng.choice((rng.uniform(100, 2000), rng.uniform(-500, 500)))
    return [(k, 0, 0, 0), (0, k, 0, 0), (0, 0, rng.choice((0.0, 0.5, k)), 1),
            (rng.uniform(-300, 300), rng.uniform(-50, 50), 0, dz)]


def state(rng):
    image = {base: bytearray(rng.getrandbits(8) for _ in range(size)) for base, size in WINDOWS}

    def put(a, data):
        for base, size in WINDOWS:
            if base <= a and a + len(data) <= base + size:
                image[base][a - base:a - base + len(data)] = data
                return
        raise AssertionError(('design outside the windows', hex(a)))
    put(0x00275670, struct.pack('<I', CTX))
    chan = rng.choice((3, 3, 0, 1, 2))
    for ch in range(4):
        put(CTX + 0x10 + 4 * ch, struct.pack('<I', PKT + 0x10 * rng.randrange(0, 8)))
    put(CTX + 0x22C0, b''.join(struct.pack('<4f', *r) for r in clip_matrix(rng)))
    put(0x70003AC0, b''.join(struct.pack('<4f', *r) for r in camera(rng)))
    put(TABLE, table(rng))
    fw = rng.choice((rng.uniform(-1, -0.001), 0.0, rng.uniform(-0.05, 0.05)))
    fog = (rng.choice((255.0, 128.0)), 2048.0, rng.uniform(-50, 400), fw)
    vf = [[rng.getrandbits(32) & 0x7F7FFFFF for _ in range(4)] for _ in range(32)]
    vf[0] = [0, 0, 0, 0x3F800000]
    vf[23] = [fb(x) for x in fog]
    vu = {'vf': vf, 'acc': [rng.getrandbits(32) & 0x7F7FFFFF for _ in range(4)],
          'q': fb(rng.uniform(-2, 2)), 'clip': rng.getrandbits(24)}
    return {k: bytes(v) for k, v in image.items()}, vu, chan


# ---------------------------------------------------------------- cases
NAMES = {0x1E1760: 'em_grid_water_001E1760', 0x1E10A0: 'em_grid_water_001E10A0',
         0x1E13E0: 'em_grid_water_001E13E0', 0x1E17E0: 'em_grid_water_001E17E0'}


def run_case(ee, lib, rng, entry, seed):
    image, vu, chan = state(rng)
    script = Script(seed)
    ints = () if entry == 0x1E10A0 else (chan,)
    want = oracle(ee, image, vu, script, entry, ints)
    script.reset()
    n = Native(lib, image, vu, script)
    fn = getattr(lib, NAMES[entry])
    out = C.c_uint32()
    if entry == 0x1E10A0:
        rc = fn(C.byref(n.g))
    elif entry == 0x1E17E0:
        rc = fn(C.byref(n.g), chan, C.byref(out))
    else:
        rc = fn(C.byref(n.g), chan)
    label = (hex(entry), seed)
    assert rc == 0, (label, 'native fault', n.g.fault, hex(n.g.fault_function), hex(n.g.fault_address))
    if entry == 0x1E17E0:
        assert out.value == want[0], (label, 'v0', hex(out.value), hex(want[0]))
    got = n.windows()
    for base, size in WINDOWS:
        if got[base] != want[1][base]:
            bad = [hex(base + i) for i in range(0, size, 4) if got[base][i:i + 4] != want[1][base][i:i + 4]]
            raise AssertionError((label, 'window', hex(base), len(bad), bad[:8]))
    assert n.calls == want[2], (label, 'calls', n.calls, want[2])
    if entry in (0x1E17E0,):
        assert n.vu_state() == want[3], (label, 'vu0', n.vu_state(), want[3])
    else:   # the scalar workers leave VU0 alone
        assert want[3] == Native(lib, image, vu, script).vu_state(), (label, 'original touched VU0')
        assert n.vu_state() == want[3], (label, 'native touched VU0')
    return want


def classify(entry, want, image_vu=None):
    if entry != 0x1E17E0:
        return 'ran'
    pkt = want[1][PKT]
    # count vertices marked no-draw (word +0xC == 0xFFFF) in the strips
    flagged = sum(1 for o in range(0, len(pkt) - 16, 16) if pkt[o + 12:o + 16] == b'\xff\xff\x00\x00')
    return 'some-adc' if flagged else 'no-adc'


def fail_stop(ee, lib):
    rng = random.Random(7)
    image, vu, chan = state(rng)
    n = Native(lib, image, vu, Script(1))
    n.fail_map = TABLE
    assert lib.em_grid_water_001E10A0(C.byref(n.g)) == -1
    assert n.g.fault == 1 and n.g.fault_function == 0x1E10A0 and n.g.fault_address == TABLE
    n.fail_map = None
    out = C.c_uint32()
    assert lib.em_grid_water_001E17E0(C.byref(n.g), chan, C.byref(out)) == -1, 'latched fault not kept'
    n2 = Native(lib, image, vu, Script(1))
    n2.g.w_001D6E60 = F6E60()
    # a NULL hook faults only when reached
    assert lib.em_grid_water_001E13E0(C.byref(n2.g), chan) == 0
    assert lib.em_grid_water_001E1760(C.byref(n2.g), chan) == -1
    assert n2.g.fault == 2 and n2.g.fault_function == 0x1D6E60
    assert n2.calls == [], 'a call ran before the NULL hook'


def main():
    lib = build()
    ee = FL.FarEE(read_elf())
    counts = {}
    total = RM.pick(240, 40)
    rng = random.Random(0x1E1AD0)
    for i in range(total):
        entry = (0x1E1760, 0x1E10A0, 0x1E13E0, 0x1E17E0, 0x1E17E0)[i % 5]
        want = run_case(ee, lib, rng, entry, i)
        key = '%06X %s' % (entry, classify(entry, want))
        counts[key] = counts.get(key, 0) + 1
    for need in ('1E17E0 some-adc', '1E1760 ran', '1E10A0 ran', '1E13E0 ran'):
        assert counts.get(need), ('outcome never reached', need, counts)
    fail_stop(ee, lib)
    RM.banner(*('%s %d' % kv for kv in sorted(counts.items())))
    print('grid water reference: PASS (original 001E1760 / 001E10A0 / 001E13E0 / 001E17E0)')


if __name__ == '__main__':
    main()
