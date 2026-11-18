#!/usr/bin/env python3
"""Execute the original workers of 001E1E60's flag-0x23 branch (001D73A0,
001D72D0 with 001D7200, 001D75E0, 001CEFD0 with 001CF320) and compare
src/game/em_far_light.c (docs/AREA21_PORT.md "Far light").

The user's pinned ELF supplies every instruction and the .data tables
D_00251370 / D_00251390; none are embedded here. The oracle is
test_area00_hud_reference.HudEE (the measured EE / VU0 model with the clip
test and the CLIP register). Every callee runs as original code but the
sqrt 0011E748 and the packet chain 001CB5F0 / 001CB900, which are scripted
identically on both sides (their arguments compared).

Each case designs a state at original addresses: the context pointer
D_00275670 (context 0x811CC0: the bank word +0x9C, the channel cursors
+0x10.., the reference matrix +0x2340, 001CD370(2)'s clip matrix +0x22C0),
the count D_00275C08, the read-back list D_008172C0 (both banks), the
camera rows 0x70003AC0, the scratchpad 0x70003600.. and 0x70003A20.., the
argument vectors, the packet windows, and the VU0 registers (vf23 the fog
ramp the caller left, CLIP's older flags). Camera, clip matrix and fog are
built so that points fall inside, at the edges of and outside the window,
behind the camera, outside the clip volume and on both sides of the fog
range; lists full / not full, both banks, a0 inside and outside 0..15.

Compared per case: the return value, every byte of every window (and no
original store outside them, the stack excepted), the scripted calls and
their arguments in order, and the VU0 registers the functions write (vf1,
vf2, vf24..vf31, ACC, Q, CLIP; the callees' own temporaries vf4..vf9 are
not part of the translation's contract, em_far_light.h). A fail-stop case
checks an unmapped window latches and blocks later calls.

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
import test_area00_hud_reference as HUD  # noqa: E402
from test_player_slide_reference import read_elf, RETURN, sx32  # noqa: E402

OUT = ROOT / 'build' / 'far_light'
MASK = 0xFFFFFFFF
CTX = 0x00811CC0
POS, COL = 0x01F00000, 0x01F00010              # argument vectors (the caller's stack)
CHAN_PKT, FLARE_PKT = 0x01E00000, 0x01E10000   # channel list, effect-page packets
SP = 0x01FFF000
WINDOWS = [(0x00275670, 4), (0x00275C08, 4), (0x008172C0, 0xA00), (CTX, 0x2400),
           (0x70003600, 0x100), (0x70003A00, 0x100), (0x00251370, 0x40),
           (CHAN_PKT, 0x400), (FLARE_PKT, 0x800), (POS, 0x20)]
COMPARED_VF = [1, 2] + list(range(23, 32))


def fb(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


class FarEE(HUD.HudEE):
    """HudEE with VFTOI0 (00102990)."""
    def macro(self, word):
        op, fs, ft, fd = word & 63, word >> 11 & 31, word >> 16 & 31, word >> 6 & 31
        if op >= 60 and (fd << 2 | (op & 3)) == 0x14:
            mask = word >> 21 & 15
            s = [x & MASK for x in self.vf[fs]]
            if ft:
                for i in range(4):
                    if mask & (8 >> i):
                        self.vf[ft][i] = HUD.FM.vu_ftoi(s[i], 0)
            return
        super().macro(word)


# ---------------------------------------------------------------- native
MAPFN = C.CFUNCTYPE(C.c_void_p, C.c_void_p, C.c_uint32, C.c_uint32, C.c_int)
SQRTFN = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
CB5F0FN = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_int32, C.c_int32, C.POINTER(C.c_uint32))
CB900FN = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.c_int32, C.c_int32)


class Vu(C.Structure):
    _fields_ = [('vf', (C.c_uint32 * 4) * 32), ('acc', C.c_uint32 * 4), ('q', C.c_uint32),
                ('clip', C.c_uint32)]


class Far(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('map', MAPFN), ('vu', C.POINTER(Vu)), ('w_0011E748', SQRTFN),
                ('w_001CB5F0', CB5F0FN), ('w_001CB900', CB900FN), ('fault', C.c_int32),
                ('fault_function', C.c_uint32), ('fault_address', C.c_uint32)]


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('far_light' + ('.dylib' if sys.platform == 'darwin' else '.so'))
    srcs = ['src/game/em_far_light.c', 'src/game/em_effect_original.c', 'src/game/em_player_stage_workers.c']
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', *srcs, '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    P = C.POINTER(Far)
    U4 = C.POINTER(C.c_uint32)
    n.em_far_light_001D73A0.argtypes = [P, C.c_int32, C.POINTER(C.c_int32)]
    n.em_far_light_001D72D0.argtypes = [P, U4, C.POINTER(C.c_int32)]
    n.em_far_light_001D75E0.argtypes = [P, C.c_int32, U4, C.c_uint64, C.c_uint32, C.c_uint32, C.c_uint32, U4]
    n.em_far_light_001CEFD0.argtypes = [P, U4, U4]
    return n


class Native:
    def __init__(self, lib, image, vu, script):
        self.lib, self.script, self.calls = lib, script, []
        self.bufs = [(base, (C.c_uint8 * size).from_buffer_copy(image[base])) for base, size in WINDOWS]
        self.vu = Vu()
        for i in range(32):
            for k in range(4):
                self.vu.vf[i][k] = vu['vf'][i][k]
        for k in range(4):
            self.vu.acc[k] = vu['acc'][k]
        self.vu.q, self.vu.clip = vu['q'], vu['clip']
        self.cb = (MAPFN(self.map), SQRTFN(self.sqrt), CB5F0FN(self.cb5f0), CB900FN(self.cb900))
        self.far = Far(None, self.cb[0], C.pointer(self.vu), self.cb[1], self.cb[2], self.cb[3], 0, 0, 0)
        self.fail_map = None

    def map(self, _ctx, address, size, _write):
        if self.fail_map is not None and address == self.fail_map:
            return None
        for base, buf in self.bufs:
            if base <= address and address + size <= base + len(buf):
                return C.addressof(buf) + address - base
        return None

    def sqrt(self, _ctx, x, out):
        self.calls.append(('0011E748', x))
        out[0] = self.script.sqrt(x)
        return 0

    def cb5f0(self, _ctx, table, ident, count, out):
        self.calls.append(('001CB5F0', table, ident & MASK, count))
        out[0] = self.script.packet()
        return 0

    def cb900(self, _ctx, table, ident, mode):
        self.calls.append(('001CB900', table, ident & MASK, mode))
        return 0

    def windows(self):
        return {base: bytes(buf) for base, buf in self.bufs}

    def vu_state(self):
        return ({i: tuple(self.vu.vf[i][k] for k in range(4)) for i in COMPARED_VF},
                tuple(self.vu.acc[k] for k in range(4)), self.vu.q, self.vu.clip)


class Script:
    """The scripted callees' results, replayed identically on both sides."""
    def __init__(self, seed):
        self.seed = seed
        self.reset()

    def reset(self):
        self.rng = random.Random(self.seed)
        self.next_packet = FLARE_PKT

    def sqrt(self, x):
        # a plausible root, scaled by a scripted factor (the sqrt itself is
        # 0011E748's own oracle's business; here only its result is used)
        v = abs(struct.unpack('<f', struct.pack('<I', x))[0]) ** 0.5
        pick = self.rng.randrange(6)
        if pick == 5:   # at the clamp: 393.84616 and its neighbours
            return 0x43C4EC4F + self.rng.choice((-1, 0, 1))
        return fb(v * (1.0, 1.0, 0.5, 3.0, 0.999)[pick])

    def packet(self):
        p = self.next_packet
        self.next_packet += 0x80
        return p


# ---------------------------------------------------------------- oracle
def oracle(ee, image, vu, script, entry, ints, fbits):
    for base, data in image.items():
        ee.write(base, data)
    ee.set_vu((tuple(tuple(v) for v in vu['vf']), tuple(vu['acc']), vu['q'], vu['clip']))
    calls = []

    def ret(e):
        pass

    def h_sqrt(e):
        x = e.f[12] & MASK
        calls.append(('0011E748', x))
        e.f[0] = script.sqrt(x)

    def h_5f0(e):
        calls.append(('001CB5F0', e.r[4] & MASK, e.r[5] & MASK, e.r[6] & MASK))
        e.r[2] = script.packet()

    def h_900(e):
        calls.append(('001CB900', e.r[4] & MASK, e.r[5] & MASK, e.r[6] & MASK))
    ee.hooks = {0x11E748: h_sqrt, 0x1CB5F0: h_5f0, 0x1CB900: h_900}
    stores = []
    save = ee.save

    def saved(address, value, size=4):
        stores.append((address & MASK, size))
        save(address, value, size)
    ee.save = saved
    try:
        ee.r[29] = SP
        for i, v in enumerate(ints):
            ee.r[4 + i] = v & 0xFFFFFFFFFFFFFFFF
        for i, v in enumerate(fbits):
            ee.f[12 + i] = v & MASK
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
def rotation(rng):
    """Camera rows: screen x = a x / z + 2048, y = b y / z + 2048, w = z,
    with a small random skew; z lane zc / z."""
    a, b = rng.uniform(300, 900), rng.uniform(200, 600)
    s = rng.uniform(-0.05, 0.05)
    zc = rng.uniform(1e5, 1e7)
    return [(a, s * b, 0, 0), (s * a, b, 0, 0), (2048, 2048, 0, 1), (0, 0, zc, 0)]


def clip_rows(rng):
    k = rng.choice((1.0, 1.0, 0.6, 1.4))
    return [(k, 0, 0, 0), (0, k, 0, 0), (0, 0, rng.choice((0.0, 0.5, 1.2)), 1), (0, 0, 0, 0)]


def state(rng, entry):
    image = {base: bytearray(rng.getrandbits(8) for _ in range(size)) for base, size in WINDOWS}
    image[0x00251370] = None                      # the ELF's tables, set below
    put = lambda a, data: image_put(image, a, data)
    put(0x00275670, struct.pack('<I', CTX))
    count = rng.choice((0, 0, 0, 1, 2, 15, 16, 17, rng.randrange(0, 18)))
    put(0x00275C08, struct.pack('<I', count))
    bank = rng.choice((0, 1))
    put(CTX + 0x9C, struct.pack('<I', bank))
    for ch in range(8):
        put(CTX + 0x10 + 4 * ch, struct.pack('<I', CHAN_PKT + 0x40 * rng.randrange(0, 4)))
    # the reference matrix at ctx+0x2340: a projection whose (16, 16, 480, 1)
    # image is near (2048 + 16 k, 2048 + 16 k, ., w)
    wref = rng.choice((1.0, 2.0, 0.5, rng.uniform(0.3, 3)))
    ref = [(rng.uniform(300, 700) * wref / 480, 0, 0, 0), (0, rng.uniform(200, 500) * wref / 480, 0, 0),
           (2048 * wref / 480, 2048 * wref / 480, rng.uniform(-1, 1), wref / 480), (0, 0, rng.uniform(-5, 5), 0)]
    put(CTX + 0x2340, b''.join(struct.pack('<4f', *r) for r in ref))
    put(CTX + 0x22C0, b''.join(struct.pack('<4f', *r) for r in clip_rows(rng)))
    cam = rotation(rng)
    put(0x70003AC0, b''.join(struct.pack('<4f', *r) for r in cam))
    # a point in front (or behind) with a screen offset from inside to well
    # outside the 512 x 224 window
    z = rng.choice((rng.uniform(50, 2000), rng.uniform(-500, -1), rng.uniform(1e5, 1e9)))
    # (001D7200's clamps: pixel x - 0x700 at 511 / 512, y - 0x790 at 223 / 224)
    sx = rng.choice((0, rng.uniform(-200, 200), rng.uniform(-400, 400), rng.uniform(-3000, 3000),
                     255, 255.5, 256, 256.5, -256, -256.5))
    sy = rng.choice((0, rng.uniform(-90, 90), rng.uniform(-200, 200), rng.uniform(-3000, 3000),
                     111, 111.5, 112, 112.5, -112, -112.5))
    px, py = sx * z / cam[0][0], sy * z / cam[1][1]
    if rng.random() < 0.15:   # a clip lane exactly at |w| (clip rows scale 1)
        px = rng.choice((z, -z))
    pos = (px, py, z, rng.choice((0.0, 1.0)))
    put(POS, struct.pack('<4f', *pos))
    scale = rng.choice((255, 255, 1e7))
    put(POS + 0x10, struct.pack('<4f', *(rng.uniform(0, scale) for _ in range(4))))
    # fog ramp: F = fz + fw * z, clamped to [0, fx]; aimed at F in 0..15 often
    fw = rng.choice((rng.uniform(-1, -0.01), rng.uniform(-1, -0.01), 0.0))
    target = rng.choice((rng.uniform(0, 15.9), rng.uniform(-10, 0), rng.uniform(16, 300)))
    if fw == 0.0:   # F exactly at 001CEFD0's bounds (12.4: 0x100 and 0xFF, 0 and -1)
        target = rng.choice((16.0, 15.9375, 0.0, -0.0625))
    fog = (rng.choice((255.0, 8.0)), 2048.0, target - fw * z, fw)
    vf = [[rng.getrandbits(32) & 0x7F7FFFFF for _ in range(4)] for _ in range(32)]
    vf[0] = [0, 0, 0, 0x3F800000]
    vf[23] = [fb(x) for x in fog]
    vu = {'vf': vf, 'acc': [rng.getrandbits(32) & 0x7F7FFFFF for _ in range(4)],
          'q': fb(rng.uniform(-2, 2)), 'clip': rng.getrandbits(24)}
    a0 = rng.choice((0, 1, 15, 16, -1, rng.randrange(-5, 25)))
    return image, vu, a0, pos


def image_put(image, address, data):
    for base, size in WINDOWS:
        if base <= address and address + len(data) <= base + size and image.get(base) is not None:
            image[base][address - base:address - base + len(data)] = data
            return
    raise AssertionError(('design outside the windows', hex(address)))


# ---------------------------------------------------------------- cases
def run_case(ee, lib, rng, entry, seed):
    image, vu, a0, pos = state(rng, entry)
    image[0x00251370] = bytes(ee.read(0x00251370, 0x40))
    image = {k: bytes(v) for k, v in image.items()}
    script = Script(seed)
    tex0 = rng.getrandbits(64)
    rgba = rng.getrandbits(32)
    w, h = fb(rng.choice((384.0, 16.0, rng.uniform(1, 2000)))), fb(rng.choice((384.0, rng.uniform(1, 2000))))
    idx = rng.randrange(0, 8)
    if entry == 0x1D73A0:
        ints, fbits = (a0,), ()
    elif entry == 0x1D72D0:
        ints, fbits = (POS,), ()
    elif entry == 0x1D75E0:
        ints, fbits = (idx, POS, tex0, rgba), (w, h)
    else:
        ints, fbits = (POS, POS + 0x10), ()
    want = oracle(ee, image, vu, script, entry, ints, fbits)
    script.reset()
    n = Native(lib, image, vu, script)
    posw = (C.c_uint32 * 4)(*struct.unpack_from('<4I', image[POS], 0))
    colw = (C.c_uint32 * 4)(*struct.unpack_from('<4I', image[POS], 0x10))
    if entry == 0x1D73A0:
        out = C.c_int32()
        rc = lib.em_far_light_001D73A0(C.byref(n.far), a0, C.byref(out))
        v0 = out.value & MASK
    elif entry == 0x1D72D0:
        out = C.c_int32()
        rc = lib.em_far_light_001D72D0(C.byref(n.far), posw, C.byref(out))
        v0 = out.value & MASK
    elif entry == 0x1D75E0:
        out = C.c_uint32()
        rc = lib.em_far_light_001D75E0(C.byref(n.far), idx, posw, tex0, rgba, w, h, C.byref(out))
        v0 = out.value
    else:
        rc = lib.em_far_light_001CEFD0(C.byref(n.far), posw, colw)
        v0 = None
    label = (hex(entry), seed)
    assert rc == 0, (label, 'native fault', n.far.fault, hex(n.far.fault_function), hex(n.far.fault_address))
    if v0 is not None:
        assert v0 == want[0], (label, 'v0', hex(v0), hex(want[0]))
    got = n.windows()
    for base, size in WINDOWS:
        if got[base] != want[1][base]:
            bad = [hex(base + i) for i in range(0, size, 4) if got[base][i:i + 4] != want[1][base][i:i + 4]]
            raise AssertionError((label, 'window', hex(base), bad[:8]))
    assert n.calls == want[2], (label, 'calls', n.calls, want[2])
    assert n.vu_state() == want[3], (label, 'vu0', n.vu_state(), want[3])
    return want


def classify(entry, want):
    v0, _, calls, _ = want
    if entry == 0x1CEFD0:
        return 'drawn' if calls else 'skipped'
    if entry == 0x1D75E0:
        return 'packet' if v0 else 'rejected'
    return 'minus' if v0 == MASK else 'value'


def fail_stop(ee, lib):
    rng = random.Random(7)
    image, vu, _, _ = state(rng, 0x1D72D0)
    image[0x00251370] = bytes(ee.read(0x00251370, 0x40))
    image = {k: bytes(v) for k, v in image.items()}
    n = Native(lib, image, vu, Script(1))
    n.fail_map = 0x00275C08
    out = C.c_int32()
    posw = (C.c_uint32 * 4)()
    assert lib.em_far_light_001D72D0(C.byref(n.far), posw, C.byref(out)) == -1
    assert n.far.fault == 1 and n.far.fault_function == 0x1D72D0 and n.far.fault_address == 0x275C08
    n.fail_map = None
    assert lib.em_far_light_001D73A0(C.byref(n.far), 0, C.byref(out)) == -1, 'latched fault not kept'
    n2 = Native(lib, image, vu, Script(1))
    n2.far.w_0011E748 = SQRTFN()
    # a NULL hook faults only when reached; 001D73A0 does not reach it
    assert lib.em_far_light_001D73A0(C.byref(n2.far), 0, C.byref(out)) == 0


def main():
    lib = build()
    ee = FarEE(read_elf())
    counts = {}
    total = RM.pick(1600, 200)
    rng = random.Random(0x1E1E60)
    for i in range(total):
        entry = (0x1D73A0, 0x1D72D0, 0x1D75E0, 0x1CEFD0, 0x1CEFD0)[i % 5]
        want = run_case(ee, lib, rng, entry, i)
        key = '%06X %s' % (entry, classify(entry, want))
        counts[key] = counts.get(key, 0) + 1
    for need in ('1CEFD0 drawn', '1CEFD0 skipped', '1D75E0 packet', '1D75E0 rejected',
                 '1D72D0 minus', '1D72D0 value', '1D73A0 value'):
        assert counts.get(need), ('outcome never reached', need, counts)
    fail_stop(ee, lib)
    RM.banner(*('%s %d' % kv for kv in sorted(counts.items())))
    print('far light reference: PASS (original 001D73A0 / 001D72D0 / 001D7200 / 001D75E0 / 001CEFD0 / 001CF320)')


if __name__ == '__main__':
    main()
