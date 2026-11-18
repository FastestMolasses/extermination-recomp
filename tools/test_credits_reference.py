#!/usr/bin/env python3
"""0022D360 / 0022D380 (the ending's credits) vs the original instructions.

em_credits_original is the translation (001AD740 states 0 and 1 run it,
em_scene_task.c). The oracle executes the user's pinned boot ELF: 0022D360
(its memset 00121A28 a hook whose arguments are checked), and 0022D380 with float_to_int 001281C0 (and the
soft-float unpacker below it) and fabsf 0011DF78 as original instructions.
Its three draw callees are hooks on both sides: 001CC170 answers a width
that depends on the text address (odd and even widths, so the arithmetic
shift of the centring is taken both ways), 001CC1E0 and 00207F80 record
their arguments (001CC1E0's colour record as its 8 bytes, the byte +6 the
original never writes masked). Every frame compares the 0x20 state bytes
D_00822CD0, the return value and the draw calls in order. The row table
D_00268B40 and its texts are the ELF's own .data on both sides.

Cases (quick): the reset, state 0 from garbage bytes, the first frames of
the scroll, frames around every 8th row entering and leaving (both halves
of the half-line), the end row reaching 0x70 (the hold starts), the hold's
last frames (the return 1) and a state byte outside 0..2. EM_TEST_FULL=1
also runs the whole credits from state 0 to the return (about 11,600
frames, minutes). No original bytes are embedded or printed.
"""
import ctypes as C
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
from test_player_slide_reference import read_elf, s32  # noqa: E402
import test_player_recovery_reference as recovery  # noqa: E402

OUT = ROOT / 'build' / 'credits_reference'
RESET, SCROLL = 0x22D360, 0x22D380
CC170, CC1E0, F80, MEMSET = 0x1CC170, 0x1CC1E0, 0x207F80, 0x121A28
STATE, TABLE = 0x822CD0, 0x268B40
DATA_LO, DATA_HI = 0x268B40, 0x275300      # the table and its texts

BYTES = C.CFUNCTYPE(C.c_void_p, C.c_void_p, C.c_uint32, C.c_uint32)
CC170_T = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_int32))
CC1E0_T = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_int32, C.c_int32, C.c_int32, C.c_int32,
                      C.c_uint32, C.POINTER(C.c_uint8))
F80_T = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int32, C.c_int32, C.c_int32, C.c_int32, C.c_int32, C.c_uint32)


class Hooks(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('bytes', BYTES), ('cc170', CC170_T), ('cc1e0', CC1E0_T), ('f80', F80_T)]


def width_of(text):
    return (text * 7 + 3) % 301


def build_native():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('credits' + ('.dylib' if sys.platform == 'darwin' else '.so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', 'src/game/em_credits_original.c', 'src/game/em_effect_original.c',
                    '-lm', '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    n.em_credits_0022D360.argtypes = [C.c_void_p]
    n.em_credits_0022D380.argtypes = [C.c_void_p, C.POINTER(Hooks), C.POINTER(C.c_int32)]
    n.em_credits_0022D380.restype = C.c_int
    return n


class Native:
    def __init__(self, lib, data):
        self.lib, self.events = lib, []
        self.data = C.create_string_buffer(data, len(data))
        base = C.addressof(self.data)

        def bytes_(ctx, address, size):
            if DATA_LO <= address and address + size <= DATA_HI:
                return base + address - DATA_LO
            return None

        def cc170(ctx, text, out):
            self.events.append(('170', text))
            out[0] = width_of(text)
            return 0

        def cc1e0(ctx, slot, x, y, w, h, text, colour):
            col = bytes(colour[i] for i in range(8))
            self.events.append(('1e0', slot, x, y, w, h, text, col[:6] + b'\0' + col[7:]))
            return 0

        def f80(ctx, slot, x0, y0, x1, y1, rgba):
            self.events.append(('f80', slot, x0, y0, x1, y1, rgba))
            return 0

        self.keep = (BYTES(bytes_), CC170_T(cc170), CC1E0_T(cc1e0), F80_T(f80))
        self.hooks = Hooks(None, *self.keep)

    def frame(self, state):
        buf = (C.c_uint8 * 32)(*state)
        result = C.c_int32(0)
        assert self.lib.em_credits_0022D380(buf, C.byref(self.hooks), C.byref(result)) == 0
        return bytes(buf), result.value


class Oracle:
    def __init__(self, elf):
        ee = recovery.ModelEE(elf)
        ee.limit = 5_000_000
        self.ee, self.events = ee, []

        def cc170(e):
            text = e.arg(0)
            self.events.append(('170', text))
            e.ret_int(width_of(text))

        def cc1e0(e):
            col = e.read(e.arg(6), 8)
            self.events.append(('1e0', s32(e.arg(0)), s32(e.arg(1)), s32(e.arg(2)), s32(e.arg(3)), s32(e.arg(4)),
                                e.arg(5), col[:6] + b'\0' + col[7:]))
            e.ret_int(0)

        def f80(e):
            self.events.append(('f80', s32(e.arg(0)), s32(e.arg(1)), s32(e.arg(2)), s32(e.arg(3)), s32(e.arg(4)),
                                e.arg(5)))
            e.ret_int(0)

        def memset(e):
            # 00121A28 (the library memset; its MMI stores are outside the
            # interpreter): the call's arguments are checked, the fill done.
            self.memsets.append((e.arg(0), e.arg(1), e.arg(2)))
            e.write(e.arg(0), bytes([e.arg(1) & 0xFF]) * e.arg(2))
            e.ret_int(e.arg(0))

        self.memsets = []
        ee.hooks.update({CC170: cc170, CC1E0: cc1e0, F80: f80, MEMSET: memset})

    def frame(self, state):
        ee = self.ee
        ee.write(STATE, state)
        ee.r[29] = 0x01F00000
        ee.call(SCROLL)
        return ee.read(STATE, 32), s32(ee.r[2])

    def reset(self, state):
        self.ee.write(STATE, state)
        self.ee.r[29] = 0x01F00000
        self.ee.call(RESET)
        return self.ee.read(STATE, 32)


def state_of(st, y, cnt=0, timer=0, frac=None, garbage=0):
    b = bytearray((garbage * (i + 1)) & 0xFF for i in range(32))
    b[0] = st
    struct.pack_into('<f', b, 8, y)
    struct.pack_into('<i', b, 0x14, cnt)
    struct.pack_into('<i', b, 0x18, timer)
    struct.pack_into('<f', b, 0x1C, frac if frac is not None else abs(y - int(y)))
    return bytes(b)


def run_case(oracle, native, start, frames):
    a = b = start
    for f in range(frames):
        oracle.events.clear()
        native.events.clear()
        a, ra = oracle.frame(a)
        b, rb = native.frame(b)
        assert a == b, ('state', f, a.hex(), b.hex())
        assert ra == rb, ('result', f, ra, rb)
        assert oracle.events == native.events, ('draws', f, oracle.events[:4], native.events[:4])
        yield a, ra, len(oracle.events)


def main():
    started = time.time()
    elf = read_elf()
    oracle = Oracle(elf)
    data = oracle.ee.read(DATA_LO, DATA_HI - DATA_LO)
    native = Native(build_native(), data)
    rows = 0
    while struct.unpack_from('<i', data, 8 * rows)[0] != 4:
        rows += 1
    end_y = 0x15 * rows                      # the end row's line at y = 0

    # 0022D360
    for g in (0, 0x5A, 0xFF):
        start = state_of(1, -3.5, 7, 9, garbage=g)
        want = oracle.reset(start)
        buf = (C.c_uint8 * 32)(*start)
        native.lib.em_credits_0022D360(buf)
        assert want == bytes(buf) == bytes(32), ('0022D360', g)
        assert oracle.memsets[-1] == (STATE, 0, 32), ('0022D360 memset', oracle.memsets[-1])

    cases = [('state 0 from the reset', bytes(32), 4),
             ('state 0 from garbage', state_of(0, 9.0, 5, 5, garbage=0x33), 2),
             ('a state byte 3', state_of(3, -10.0, garbage=0x11), 1),
             ('the hold: last frames', state_of(2, float(0x70 - end_y) - 0.5, timer=1), 3),
             ('the hold: its start', state_of(2, float(0x70 - end_y) - 0.5, timer=200), 2),
             ('the end row reaching 0x70', state_of(1, float(0x70 - end_y) + 1.0), 4)]
    # The window's edges: a line exactly at 0x91 / 0x90 
    # and at -0x90 / -0x8F (float_to_int truncates toward zero), on drawn rows.
    drawn = [r for r in range(rows) if struct.unpack_from('<i', data, 8 * r)[0] in (1, 2, 3)]
    for r in (drawn[0], drawn[5]):
        for line in (0x91, 0x90, -0x90, -0x8F):
            base = line - 0x15 * r
            y = base + (0.25 if base >= 0 else -0.25) + 0.5
            cases.append(('edge: row %d at %d' % (r, line), state_of(1, y), 1))
    for row in range(0, rows, 8):
        for half in (0.0, 0.5):
            y = -float(0x15 * row) + 0x90 + half
            cases.append(('row %d enters' % row, state_of(1, y, cnt=row), 2))
            cases.append(('row %d leaves' % row, state_of(1, -float(0x15 * row) - 0x8F + half, cnt=row), 2))
    picked = reference_mode.select(cases, 60, 0x22D380, keep=lambda i, c: not c[0].startswith('row'))
    frames = draws = 0
    seen_hold = seen_end = False
    for name, start, n in picked:
        for state, result, d in run_case(oracle, native, start, n):
            frames += 1
            draws += d
            seen_hold |= state[0] == 2
            seen_end |= result == 1
    assert seen_hold and seen_end, ('the hold or its end never ran', seen_hold, seen_end)
    if os.environ.get('EM_TEST_FULL') == '1':
        state, total = bytes(32), 0
        for state, result, d in run_case(oracle, native, state, 20000):
            total += 1
            if result == 1:
                break
        assert result == 1, 'the full credits never returned 1'
        print('full run: %d frames to the return 1' % total)
    reference_mode.banner('%d of %d cases' % (len(picked), len(cases)))
    print('credits 0022D360 / 0022D380 vs original instructions: PASS %d cases, %d frames, %d draw calls, '
          '%d table rows (%.1fs)' % (len(picked), frames, draws, rows, time.time() - started))


if __name__ == '__main__':
    main()
