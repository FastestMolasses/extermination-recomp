#!/usr/bin/env python3
"""Original 001E7780 and the second half's area inits against em_area01_state.

em_area01_state selects AREA14, AREA16, AREA17, AREA18, AREA20 and AREA21
(docs/SECOND_HALF_AREAS.md): their overlay views are the delivered data and
BSS, the six boot globals the shared owner's, and 001E7780's keys 0xE00,
0x1000 / 0x1001, 0x1100, 0x1200, 0x1400 and 0x1500 run each area's init. No
route capture of these areas exists yet, so the machine is a host capture of
another area (AREA08's a08_01, only for the boot's RAM and scratchpad) with
the area's own overlay file written at 0x823500 and synthetic BSS and globals
(zeros, a poison pattern, and the host's leftover bytes). This runs the
ORIGINAL 001E7780 (subs 0..7 and 0xFF, so every key of the area and the
subs no key names) and compares the six globals, the overlay data and the
whole BSS; then the header checks and a wrong area. No capture bytes or
original instructions are exported.
"""
import ctypes as C
import hashlib
import struct
import subprocess
from pathlib import Path

import reference_mode as mode
from test_area01_state_reference import Oracle
from test_player_slide_reference import ELF_SHA256

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
OUT = ROOT / 'build/second_half/state'
BASE, GLOBALS = 0x823500, 0x275C18
HOST = 'levels-recordings-20261030-1/s87/route_a08/a08_01_magazine'
# area byte: (overlay, header id, text, data address, data size, BSS, BSS size, init, subs with a key)
AREAS = {
    0x0E: ('AREA14', 0x0B, 0x2C40, 0x826180, 0x2300, 0x828480, 0x140C00, 0x823740, (0,)),
    0x10: ('AREA16', 0x0D, 0x4F40, 0x828480, 0x4080, 0x82C500, 0x140C00, 0x824420, (0, 1)),
    0x11: ('AREA17', 0x0E, 0x2D40, 0x826280, 0x2C00, 0x828E80, 0x140C80, 0x823B80, (0,)),
    0x12: ('AREA18', 0x0F, 0x00C0, 0x823600, 0x0780, 0x823D80, 0x140C00, 0x823580, (0,)),
    0x14: ('AREA20', 0x11, 0x2BC0, 0x826100, 0x1E00, 0x827F00, 0x140C00, 0x823990, (0,)),
    0x15: ('AREA21', 0x12, 0x6E40, 0x82A380, 0x3900, 0x82DC80, 0x140C00, 0x826180, (0,)),
}


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / 'bridge.dylib'
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', '-Isrc',
                    'tests/later_state_bridge.c', 'src/game/em_area01_state.c', 'src/game/em_area00_state.c',
                    'src/game/em_area02_state.c', 'src/game/em_area02_overlay.c', 'src/game/em_area04_state.c',
                    'src/game/em_area22_port.c', 'src/game/em_area06_port.c', 'src/game/em_area06_port_node.c',
                    'src/game/em_area06_port_overlay.c', 'src/game/em_area06_port_strip.c', 'src/game/em_level10_port_boot.c',
                    'src/game/em_status_scene_original.c', 'src/game/em_task.c',
                    '-o', str(target)], cwd=ROOT, check=True)
    lib = C.CDLL(str(target))
    lib.later_state_test_seed.argtypes = [C.c_uint, C.c_char_p, C.c_uint32, C.c_char_p, C.c_uint32, C.c_uint32,
                                          C.c_char_p]
    lib.later_state_test_view.argtypes = [C.c_uint32, C.c_uint32]
    lib.later_state_test_view.restype = C.c_void_p
    lib.later_state_test_dispatch.argtypes = [C.c_uint8, C.c_uint8]
    lib.later_state_test_fault.restype = C.c_uint32
    return lib


def main():
    elf = (DECOMP / 'config/SCUS_971.12').read_bytes()
    assert hashlib.sha256(elf).hexdigest() == ELF_SHA256
    host = DECOMP / 'build/fork_refs' / HOST
    host_ram, host_spad = (host / 'eeMemory.bin').read_bytes(), (host / 'scratchpad.bin').read_bytes()
    lib = build()
    compared = cases = called = 0
    kinds = ('zero', 'poison', 'host')
    for area, (name, ident, text, data, data_size, bss_at, bss_size, init, keyed) in AREAS.items():
        overlay = (DECOMP / f'extract/OVERLAY/{name}.BIN').read_bytes()
        assert struct.unpack_from('<8I', overlay) == (0x336F574D, ident, BASE, text, data_size, bss_size, bss_at,
                                                      bss_at), name
        assert BASE + 0x40 + text == data and data + data_size == bss_at and len(overlay) == bss_at - BASE
        runs = [(s, k) for s in (0, 1, 2, 3, 4, 5, 6, 7, 255) for k in kinds]
        selected = runs if mode.FULL else [r for r in runs if r[0] in keyed + (2, 255)]
        for sub, kind in selected:
            o = Oracle(elf, host_ram, host_spad)
            o.write(BASE, overlay)
            if kind == 'zero':
                bss, globals_ = bytes(bss_size), bytes(24)
            elif kind == 'poison':
                bss, globals_ = bytes([0xA5]) * bss_size, bytes([0x5A]) * 24
            else:
                bss, globals_ = host_ram[bss_at:bss_at + bss_size], host_ram[GLOBALS:GLOBALS + 24]
            o.write(bss_at, bss)
            o.write(GLOBALS, globals_)
            o.save(0x810700, area, 1)
            o.save(0x810701, sub, 1)
            assert lib.later_state_test_seed(area, overlay, len(overlay), bss, bss_at, bss_size, globals_) == 0
            o.call(0x1E7780)
            rc = lib.later_state_test_dispatch(area, sub)
            assert rc == 0, (name, sub, kind, hex(lib.later_state_test_fault()))
            for address, size in ((GLOBALS, 24), (data, data_size), (bss_at, bss_size)):
                p = lib.later_state_test_view(address, size)
                assert p and C.string_at(p, size) == o.read(address, size), (name, sub, kind, hex(address))
                compared += size
            # the key ran the init exactly when the original stored the BSS into D_00275C1C
            ran = struct.unpack('<I', o.read(0x275C1C, 4))[0] == bss_at
            assert ran == (sub in keyed), (name, sub, ran)
            called += ran
            cases += 1
        zero = bytes(bss_size)
        for offset in range(0, 0x20, 4):
            bad = bytearray(overlay)
            bad[offset] ^= 1
            assert lib.later_state_test_seed(area, bytes(bad), len(bad), zero, bss_at, bss_size, bytes(24)) == -1
            assert lib.later_state_test_fault() == BASE
        assert lib.later_state_test_seed(area, overlay, len(overlay), zero, bss_at, bss_size, bytes(24)) == 0
        assert lib.later_state_test_dispatch(1, 0) == -1 and lib.later_state_test_fault() == 0x1E7780
    mode.banner(mode.part(cases, len(AREAS) * 9 * len(kinds), 'area / sub / input runs'))
    print(f'second-half state: PASS {cases} original 001E7780 runs ({called} through an init), {compared:,} '
          f'compared bytes, {8 * len(AREAS)} header refusals, {len(AREAS)} wrong-area refusals')


if __name__ == '__main__':
    main()
