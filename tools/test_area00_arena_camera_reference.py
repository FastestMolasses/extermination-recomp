#!/usr/bin/env python3
"""Execute the original 00194DB0 (the AREA00 boss arena camera) and 0018C0C0
(the target copy) and compare em_area00_arena_camera.c.

The user's pinned ELF supplies every instruction (none are embedded here).
00194DB0 runs unmodified on the shared EE core (FallEE: COP1 through the
measured EE float model), with 0018C0C0 and its 00102948 executed as they
are. Every other callee (0011DE90, 0011DF78, 0011E2A8, 001B1470, 001B12B0,
0018D7B0, 0018C6A0, 0018C4B0) is hooked: scripted per case and recorded with
its arguments and a digest of the compared memory at the call; the native
routine gets the same script through its hooks. The test asserts that the
hooked set plus the executed set is exactly the jal targets of the two
routines.

Compared per case: the call sequence (callee, arguments, memory digest at
the call) and the final 0x810000..0x810FFF window (the camera block 0x8101E0,
the player record 0x8102B0, the eye D_008105D0 and target D_008105E0). Every
conditional branch of 00194DB0 is asserted to go both ways. Fail-stop cases:
each hook refusing in turn (the native stops with that callee's address and
the window as the original had it at the call) and an unmapped player record.

Default run ~2 s; EM_TEST_FULL=1 multiplies the random cases by ten.
"""
import ctypes as C
import hashlib
import os
import random
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from test_player_fall_reference import FallEE  # noqa: E402
from test_player_slide_reference import read_elf, RETURN, sx32  # noqa: E402

OUT = ROOT / 'build' / 'a00arena' / 'arena-camera'
CAM, ENT, EYE, TGT = 0x8101E0, 0x8102B0, 0x8105D0, 0x8105E0
WINDOW, SIZE = 0x810000, 0x1000
ARENA, ARENA_END = 0x194DB0, 0x195130
COPY, COPY_SIZE = 0x18C0C0, 0x10
EXECUTED = {COPY: COPY_SIZE, 0x102948: 0xC}
FULL = os.getenv('EM_TEST_FULL') == '1'

# callee -> (hook field, kind): 'f' float leaf, 'a' 001B12B0, 'c' 0018D7B0,
# 'x' 0018C6A0, 'y' 0018C4B0
HOOKED = {0x11DE90: 'w_0011DE90', 0x11DF78: 'w_0011DF78', 0x11E2A8: 'w_0011E2A8',
          0x1B1470: 'w_001B1470', 0x1B12B0: 'w_001B12B0', 0x18D7B0: 'w_0018D7B0',
          0x18C6A0: 'w_0018C6A0', 0x18C4B0: 'w_0018C4B0'}


def F(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def FL(b):
    return struct.unpack('<f', struct.pack('<I', b & 0xFFFFFFFF))[0]


def digest(data):
    return hashlib.blake2b(bytes(data), digest_size=8).hexdigest()


class Oracle(FallEE):
    def __init__(self, elf):
        super().__init__(elf)
        self.outcomes = set()

    def branch(self, word, pc):
        b = super().branch(word, pc)
        if b is not None and ARENA <= pc < ARENA_END:
            op, rs, rt = word >> 26, word >> 21 & 31, word >> 16 & 31
            if not (op in (4, 20) and rs == 0 and rt == 0):
                self.outcomes.add((pc, b[0]))
        return b


def jal_targets(ee):
    targets = set()
    for start, size in ((ARENA, ARENA_END - ARENA), (COPY, COPY_SIZE)):
        for pc in range(start, start + size, 4):
            w = ee.load(pc)
            if w >> 26 == 3 or (w >> 26 == 2 and start == COPY):    # 0018C0C0 tail-jumps
                targets.add((w & 0x3FFFFFF) << 2)
    return targets


def branch_sites(ee):
    sites = set()
    for pc in range(ARENA, ARENA_END, 4):
        w = ee.load(pc)
        op, rs, rt = w >> 26, w >> 21 & 31, w >> 16 & 31
        if op in (4, 20) and rs == 0 and rt == 0:
            continue
        if ee.branch(w, pc) is not None or op == 0x11 and (w >> 21 & 31) == 8:
            sites.add(pc)
    return sites


# ---- native ----------------------------------------------------------
VP, U32, I32, FLT = C.c_void_p, C.c_uint32, C.c_int32, C.c_float
BYTES = C.CFUNCTYPE(VP, VP, U32, U32)
LEAF = C.CFUNCTYPE(C.c_int, VP, FLT, C.POINTER(FLT))
APPROACH = C.CFUNCTYPE(C.c_int, VP, FLT, FLT, FLT, C.POINTER(FLT))
COPY2 = C.CFUNCTYPE(C.c_int, VP, U32, U32)
STYLE = C.CFUNCTYPE(C.c_int, VP, U32, I32)
EASE_XZ = C.CFUNCTYPE(C.c_int, VP, U32, U32, FLT)
EASE_Y = C.CFUNCTYPE(C.c_int, VP, U32, FLT, FLT)


class Hooks(C.Structure):
    _fields_ = [('ctx', VP), ('bytes', BYTES), ('w_0011DE90', LEAF), ('w_0011DF78', LEAF),
                ('w_0011E2A8', LEAF), ('w_001B1470', LEAF), ('w_001B12B0', APPROACH),
                ('w_00102948', COPY2), ('w_0018D7B0', STYLE), ('w_0018C6A0', EASE_XZ),
                ('w_0018C4B0', EASE_Y)]


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('arena-%d.%s' % (os.getpid(), 'dylib' if sys.platform == 'darwin' else 'so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wpedantic',
                    '-ffp-contract=off', '-shared', '-fPIC', '-Isrc', '-Isrc/game',
                    'src/game/em_area00_arena_camera.c', '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    n.em_a00_arena_camera_00194DB0.argtypes = [C.POINTER(Hooks), U32, U32, C.POINTER(U32)]
    n.em_a00_arena_camera_0018C0C0.argtypes = [C.POINTER(Hooks), U32, C.POINTER(U32)]
    lib.unlink()
    return n


# ---- one case ----------------------------------------------------------
def run_original(ee, window, script, entry, args):
    ee.write(WINDOW, window)
    log = []
    queue = list(script)

    def hook(address):
        def run(e):
            if address in (0x11DE90, 0x11DF78, 0x11E2A8, 0x1B1470):
                rec = (address, e.f[12] & 0xFFFFFFFF)
            elif address == 0x1B12B0:
                rec = (address, e.f[12] & 0xFFFFFFFF, e.f[13] & 0xFFFFFFFF, e.f[14] & 0xFFFFFFFF)
            elif address == 0x18D7B0:
                rec = (address, e.r[4] & 0xFFFFFFFF, sx32(e.r[5]) & 0xFFFFFFFF)
            elif address == 0x18C6A0:
                rec = (address, e.r[4] & 0xFFFFFFFF, e.r[5] & 0xFFFFFFFF, e.f[12] & 0xFFFFFFFF)
            else:
                rec = (address, e.r[4] & 0xFFFFFFFF, e.f[12] & 0xFFFFFFFF, e.f[13] & 0xFFFFFFFF)
            log.append(rec + (digest(e.read(WINDOW, SIZE)),))
            value = queue.pop(0)
            if address in (0x11DE90, 0x11DF78, 0x11E2A8, 0x1B1470, 0x1B12B0):
                e.f[0] = value
            else:
                e.r[2] = sx32(value)
            # scramble what the callee may clobber (it is not the routine's)
            for r in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25):
                if r != 2 or address in (0x11DE90, 0x11DF78, 0x11E2A8, 0x1B1470, 0x1B12B0):
                    e.r[r] = 0x5A5A0000 + r
            for r in range(0, 20):
                if r != 0 or address not in (0x11DE90, 0x11DF78, 0x11E2A8, 0x1B1470, 0x1B12B0):
                    e.f[r] = F(-77.25 - r)
        return run
    ee.hooks = {a: hook(a) for a in HOOKED}
    ee.r[29] = 0x7F0F0000
    for i, v in enumerate(args):
        ee.r[4 + i] = sx32(v)
    ee.r[6] = 8                      # 00195130 passes 8 in a2 (unused)
    ee.r[31] = RETURN
    ee.run(entry)
    return log, ee.read(WINDOW, SIZE)


def run_native(n, window, script, which, args, refuse=None, unmapped=None):
    mem = (C.c_uint8 * SIZE).from_buffer_copy(window)
    base = C.addressof(mem)
    log = []
    queue = list(script)
    keep = []

    def mapped(_, address, size):
        if unmapped is not None and address < unmapped[0] + unmapped[1] and address + size > unmapped[0]:
            return None
        if WINDOW <= address and address + size <= WINDOW + SIZE:
            return base + address - WINDOW
        return None

    def rec(entry):
        log.append(entry + (digest(C.string_at(base, SIZE)),))
        return refuse is not None and len(log) - 1 == refuse

    def leaf(address):
        def fn(_, x, out):
            if rec((address, F(x))):
                return -1
            out[0] = FL(queue.pop(0))
            return 0
        return LEAF(fn)

    def approach(_, g, c, r, out):
        if rec((0x1B12B0, F(g), F(c), F(r))):
            return -1
        out[0] = FL(queue.pop(0))
        return 0

    def copy(_, dst, src):
        # 00102948 is executed on the original side; the copy here is
        # checked by the window comparison.
        C.memmove(base + dst - WINDOW, base + src - WINDOW, 16)
        return 0

    def style(_, cam, s):
        if rec((0x18D7B0, cam, s & 0xFFFFFFFF)):
            return -1
        queue.pop(0)
        return 0

    def ease_xz(_, a, b, r):
        if rec((0x18C6A0, a, b, F(r))):
            return -1
        queue.pop(0)
        return 0

    def ease_y(_, a, y, r):
        if rec((0x18C4B0, a, F(y), F(r))):
            return -1
        queue.pop(0)
        return 0

    h = Hooks()
    for name, fn in (('bytes', BYTES(mapped)), ('w_0011DE90', leaf(0x11DE90)), ('w_0011DF78', leaf(0x11DF78)),
                     ('w_0011E2A8', leaf(0x11E2A8)), ('w_001B1470', leaf(0x1B1470)),
                     ('w_001B12B0', APPROACH(approach)), ('w_00102948', COPY2(copy)),
                     ('w_0018D7B0', STYLE(style)), ('w_0018C6A0', EASE_XZ(ease_xz)),
                     ('w_0018C4B0', EASE_Y(ease_y))):
        keep.append(fn)
        setattr(h, name, fn)
    fault = U32(0xDEAD)
    if which == ARENA:
        rc = n.em_a00_arena_camera_00194DB0(C.byref(h), args[0], args[1], C.byref(fault))
    else:
        rc = n.em_a00_arena_camera_0018C0C0(C.byref(h), args[0], C.byref(fault))
    return rc, fault.value, log, bytes(mem)


EDGES_Z = (-1610.0, -1497.0, -1657.0, -1577.0)


def near(rng, x):
    b = F(x)
    return FL(rng.choice((b, b + 1, b - 1)))


def make_case(rng, kind):
    w = bytearray(rng.getrandbits(8) for _ in range(SIZE))
    for off in range(0, SIZE, 4):
        struct.pack_into('<f', w, off, rng.uniform(-2000, 2000))

    def put(address, value):
        struct.pack_into('<f', w, address - WINDOW, value)
    z = near(rng, rng.choice(EDGES_Z)) if kind % 3 == 0 else rng.uniform(-1720, -1430)
    put(ENT + 0xA8, z)
    struct.pack_into('<i', w, ENT + 0x230 - WINDOW, 3 if kind % 2 else rng.choice((0, 2, 4, 44, 3)))
    put(CAM + 0xAC, rng.uniform(-7, 7))
    struct.pack_into('<H', w, CAM + 0x5A - WINDOW, rng.getrandbits(16))
    put(CAM + 0x20, near(rng, -90.0) if rng.random() < 0.3 else rng.uniform(-150, -30))
    put(CAM + 0x28, near(rng, rng.choice((-1617.0, -1537.0))) if rng.random() < 0.3 else rng.uniform(-1680, -1480))
    mag = near(rng, 0.7853982) if rng.random() < 0.3 else rng.uniform(0, 3.2)
    script = [F(rng.uniform(-3.2, 3.2)), F(mag), F(rng.uniform(-7, 7)), F(rng.uniform(-1, 1)),
              F(rng.uniform(-1, 1)), rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32)]
    return bytes(w), script


def main():
    elf = read_elf()
    ee = Oracle(elf)
    targets = jal_targets(ee)
    assert targets == set(HOOKED) | {0x18C0C0, 0x102948}, sorted(hex(t) for t in targets)
    sites = branch_sites(ee)
    n = build()
    rng = random.Random(0x194DB0)
    count = refusals = 0
    cases = 4000 if FULL else 400
    for i in range(cases):
        window, script = make_case(rng, i)
        # the hooks hand out the script in call order (state 3 with a small
        # difference skips 001B12B0; other states skip 001B1470 / 0011DF78)
        oseq, ofinal = run_original(ee, window, script, ARENA, (CAM, ENT))
        handed = script[:len(oseq)]
        rc, fault, nlog, nfinal = run_native(n, window, handed, ARENA, (CAM, ENT))
        assert rc == 0, (i, 'native failed', hex(fault))
        assert nlog == oseq, (i, 'calls', oseq, nlog)
        assert nfinal == ofinal, (i, 'window', next(k for k in range(SIZE) if nfinal[k] != ofinal[k]) + WINDOW)
        count += 1
        if i % 7 == 0:
            # each hook refusing in turn: stop there, window as at the call
            for k in range(len(oseq)):
                rc, fault, nlog, nfinal = run_native(n, window, handed, ARENA, (CAM, ENT), refuse=k)
                assert rc == -1 and fault == oseq[k][0], (i, k, rc, hex(fault))
                assert nlog == oseq[:k + 1] and digest(nfinal) == oseq[k][-1], (i, k, 'refusal')
                refusals += 1
    missing = {(pc, t) for pc in sites for t in (False, True)} - ee.outcomes
    assert not missing, ('branch outcomes not exercised', sorted((hex(p), t) for p, t in missing))
    # an unmapped player record: fault at its first read, nothing written
    window, script = make_case(rng, 1)
    rc, fault, nlog, nfinal = run_native(n, window, script, ARENA, (CAM, ENT), unmapped=(ENT + 0xA8, 4))
    assert rc == -1 and fault == ENT + 0xA8 and not nlog and nfinal == window
    # 0018C0C0 alone, over random windows
    for i in range(50):
        window, _ = make_case(rng, i)
        olog, ofinal = run_original(ee, window, [], COPY, (CAM,))
        rc, fault, nlog, nfinal = run_native(n, window, [], COPY, (CAM,))
        assert rc == 0 and not olog and not nlog and nfinal == ofinal, ('0018C0C0', i)
        count += 1
    print(f'PASS area00 arena camera: {count} original-instruction cases (00194DB0 + 0018C0C0), '
          f'{len(sites)} branch sites both ways, {refusals} hook refusals, 1 unmapped read')


if __name__ == '__main__':
    main()
