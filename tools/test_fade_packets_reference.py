#!/usr/bin/env python3
"""The screen effects' GS packets against the original instructions.

src/game/em_fade_packets.c (with em_fade.c's state machines), GS_EXACT.md
section 9 "Slot 1". Input (user-local, nothing of it embedded): the pinned
boot ELF ../Extermination/config/SCUS_971.12.

The EE interpreter of test_player_slide_reference.py executes, over the
ELF's own memory image:
A. 001AEA50 (which calls 001AE900) once: the two letterbox blocks
   D_0028A7B0 (2 x 0x90) and the two fade blocks D_0028A8E0 (2 x 0x60)
   must equal em_fpk_001AEA50's, the bars' corners read from D_0024A8C0.
B. 001AEBE0 over its state words D_0028A8D0 / D2 / D4, the gate
   (0x70003B90, D_008106C4) and D_00810E80, with the render context's
   +0x14 cursor pointed into a filled scratch quadword: em_screen_fade_tick
   then em_fpk_001AEBE0 must leave the same state, the same blocks, the same
   16 bytes at the cursor and the same cursor.
C. 001AEE70 (with 001AED80 / DB0 / DE0 / E10) over +0xC0..+0xC7 of
   D_0028A8E0, likewise with em_transition_fade_tick and em_fpk_001AEE70.
The original may store only into the blocks and their state words, the
cursor word, the cursor's quadword and its stack.

Quick (default): a sample of B and C; EM_TEST_FULL=1: every case.
"""
from __future__ import annotations

import ctypes as C
import hashlib
import random
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from reference_mode import MODE, select  # noqa: E402
from test_player_slide_reference import EE  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
ELF = DECOMP / 'config/SCUS_971.12'
ELF_SHA256 = 'ee052236783e7d3e865754d3ff9fee71290addeb7d146c86caa7ff2724d1e17a'
OUT = ROOT / 'build/fade_packets_reference'

LETTERBOX, FADE, CORNERS = 0x0028A7B0, 0x0028A8E0, 0x0024A8C0
D_00275670, D_008106C4, D_00810E80, SPAD_3B90 = 0x00275670, 0x008106C4, 0x00810E80, 0x70003B90
CONTEXT, CURSOR = 0x01E00000, 0x01F00000           # a free context and quadword
F_001AEA50, F_001AEBE0, F_001AEE70 = 0x001AEA50, 0x001AEBE0, 0x001AEE70


class Screen(C.Structure):
    _fields_ = [('state', C.c_int16), ('step', C.c_int16), ('level', C.c_int32)]


class Transition(C.Structure):
    _fields_ = [('substate', C.c_int16), ('colour', C.c_uint8), ('mode', C.c_int8), ('level', C.c_int16),
                ('step', C.c_int16)]


class Packets(C.Structure):
    _fields_ = [('letterbox', C.c_uint8 * 0x120), ('fade', C.c_uint8 * 0xC0)]


def native():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / 'libfade_packets.dylib'
    subprocess.run(['clang', '-shared', '-O1', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Isrc',
                    'src/game/em_fade.c', 'src/game/em_fade_packets.c', '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    n.em_fpk_001AEA50.argtypes = [C.POINTER(Packets), C.POINTER(C.c_int16)]
    n.em_fpk_001AEBE0.argtypes = [C.POINTER(Packets), C.c_int, C.c_int32, C.c_int, C.c_char_p]
    n.em_fpk_001AEE70.argtypes = [C.POINTER(Packets), C.c_int, C.c_int16, C.c_int8, C.c_int, C.c_char_p]
    n.em_screen_fade_tick.argtypes = [C.POINTER(Screen), C.c_uint8, C.c_uint8]
    n.em_transition_fade_tick.argtypes = [C.POINTER(Transition)]
    return n


class Original:
    def __init__(self, elf):
        self.ee = ee = EE(elf)
        ee.save(D_00275670, CONTEXT)
        ee.save(CONTEXT + 0x14, CURSOR)
        self.stores = set()
        save = ee.save

        def guarded(address, value, size=4):
            address &= 0xFFFFFFFF
            if not 0x7F000000 <= address < 0x7F100000:
                self.stores.update(range(address, address + size))
            save(address, value, size)
        ee.save = guarded
        self.allowed = (set(range(LETTERBOX, FADE + 0xC8)) | set(range(CONTEXT + 0x14, CONTEXT + 0x18)) |
                        set(range(CURSOR, CURSOR + 16)))

    def call(self, entry):
        self.stores.clear()
        self.ee.call(entry, [])
        bad = sorted(a for a in self.stores if a not in self.allowed)
        assert not bad, ('the original stored outside the modelled bytes', hex(bad[0]))


def blocks(ee):
    return ee.read(LETTERBOX, 0x120), ee.read(FADE, 0xC0)


def check_blocks(ee, pk, what):
    lb, fd = blocks(ee)
    assert lb == bytes(pk.letterbox), (what, 'letterbox blocks differ',
                                       next(i for i in range(0x120) if lb[i] != pk.letterbox[i]))
    assert fd == bytes(pk.fade), (what, 'fade blocks differ', next(i for i in range(0xC0) if fd[i] != pk.fade[i]))


def main():
    elf = ELF.read_bytes()
    assert hashlib.sha256(elf).hexdigest() == ELF_SHA256, f'{ELF}: not the pinned boot ELF'
    n = native()
    o = Original(elf)
    ee = o.ee
    # A. boot
    o.call(F_001AEA50)
    pk = Packets()
    corners = (C.c_int16 * 8)(*struct.unpack('<8h', ee.read(CORNERS, 16)))
    n.em_fpk_001AEA50(C.byref(pk), corners)
    check_blocks(ee, pk, '001AEA50')
    assert ee.read(0x0028A8D0, 8) == bytes(8) and ee.read(0x0028A9A0, 8) == bytes(8), '001AEA50 state words'
    checks = 1

    rng = random.Random(0x1AEBE0)
    levels = (-0x80000000, -300, -1, 0, 1, 4, 64, 128, 251, 252, 254, 255, 256, 0x7FFFFFFF)
    steps = (-0x8000, -255, -4, 0, 4, 255, 0x7FFF)

    # B. 001AEBE0
    cases = [(st, lv, sp, gate, c4, par) for st in range(-1, 5) for lv in levels for sp in steps
             for gate in (0, 1, 2, 3) for c4 in (0, 1, 0x80) for par in (0, 1)]
    for st, lv, sp, gate, c4, par in select(cases, 600, 0xB):
        fill = bytes(rng.randrange(256) for _ in range(16))
        ee.write(CURSOR, fill)
        ee.save(CONTEXT + 0x14, CURSOR)
        ee.write(0x0028A8D0, struct.pack('<hhi', st, sp, lv))
        ee.spad[SPAD_3B90 - 0x70000000] = gate
        ee.write(D_008106C4, bytes([c4]))
        ee.write(D_00810E80, struct.pack('<h', par))
        o.call(F_001AEBE0)
        f = Screen(st, sp, lv)
        draw = n.em_screen_fade_tick(C.byref(f), gate, c4)
        tag = C.create_string_buffer(fill, 16)
        n.em_fpk_001AEBE0(C.byref(pk), par, f.level, draw, tag)
        what = ('001AEBE0', st, lv, sp, gate, c4, par)
        assert ee.read(0x0028A8D0, 8) == struct.pack('<hhi', f.state, f.step, f.level), (what, 'state')
        check_blocks(ee, pk, what)
        assert ee.read(CURSOR, 16) == tag.raw[:16], (what, 'the cursor quadword')
        assert ee.load(CONTEXT + 0x14) == CURSOR + (16 if draw else 0), (what, 'the cursor', draw)
        checks += 1

    # C. 001AEE70
    cases = [(mode, sub, lv, sp, col, par) for mode in range(-1, 8) for sub in range(-1, 5)
             for lv in (-0x8000, -1, 0, 1, 4, 128, 251, 252, 255, 0x7FFF) for sp in steps
             for col in (0, 1, 0xFF) for par in (0, 1)]
    for mode, sub, lv, sp, col, par in select(cases, 900, 0xC):
        fill = bytes(rng.randrange(256) for _ in range(16))
        ee.write(CURSOR, fill)
        ee.save(CONTEXT + 0x14, CURSOR)
        ee.write(0x0028A9A0, struct.pack('<hBbhh', sub, col, mode, lv, sp))
        ee.write(D_00810E80, struct.pack('<h', par))
        o.call(F_001AEE70)
        f = Transition(sub, col, mode, lv, sp)
        draw = n.em_transition_fade_tick(C.byref(f))
        tag = C.create_string_buffer(fill, 16)
        n.em_fpk_001AEE70(C.byref(pk), par, f.level, C.c_int8(f.colour).value, draw, tag)
        what = ('001AEE70', mode, sub, lv, sp, col, par)
        assert ee.read(0x0028A9A0, 8) == struct.pack('<hBbhh', f.substate, f.colour, f.mode, f.level, f.step), \
            (what, 'state')
        check_blocks(ee, pk, what)
        assert ee.read(CURSOR, 16) == tag.raw[:16], (what, 'the cursor quadword')
        assert ee.load(CONTEXT + 0x14) == CURSOR + (16 if draw else 0), (what, 'the cursor', draw)
        checks += 1
    print(f'mode {MODE}: fade packets: PASS ({checks} original-instruction comparisons: 001AEA50, '
          f'001AEBE0, 001AEE70)')


if __name__ == '__main__':
    main()
