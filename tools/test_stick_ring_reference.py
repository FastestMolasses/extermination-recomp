#!/usr/bin/env python3
"""001B5DC0 (the stick ring of a pad axis byte) vs the original instructions.

em_effect_original_001B5DC0 is the runtime's translation (em_area01_live's
case 0x001B5DC0u; AREA21's Gatling 0x829310 reaches it through the area
hooks). The oracle executes the user's pinned boot ELF: 001B5DC0 and its
leaf 0011E860 (abs) run as original instructions (closed world: any other
call target fails the test). Inputs: every low byte 0..255 under several
high bit patterns (only the low byte may matter), so every ring boundary
(0x31, 0x59, 0x7B either side of 0x80) is taken. No original bytes are
embedded or printed.
"""
import ctypes as C
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
from test_player_slide_reference import read_elf, s32  # noqa: E402
import test_player_recovery_reference as recovery  # noqa: E402

OUT = ROOT / 'build' / 'stick_ring_reference'
RING, ABS = 0x1B5DC0, 0x11E860
HIGH = (0, 0x100, 0xFFFFFF00, 0x7FFFFF00, 0x80000000, 0x12345600)


def build_native():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('stick_ring' + ('.dylib' if sys.platform == 'darwin' else '.so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', 'src/game/em_effect_original.c', '-lm', '-o', str(lib)],
                   cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    n.em_effect_original_001B5DC0.argtypes = [C.c_int32]
    n.em_effect_original_001B5DC0.restype = C.c_int32
    return n


def main():
    started = time.time()
    ee = recovery.ModelEE(read_elf())
    ee.allowed = {ABS}
    ee.limit = 1000
    native = build_native()
    inputs = [(high | low) & 0xFFFFFFFF for high in HIGH for low in range(256)]
    rings = {}
    for a0 in inputs:
        ee.r, ee.rh = [0] * 32, [0] * 32
        ee.r[29] = 0x01F00000
        ee.call(RING, (a0,))
        want = s32(ee.r[2])
        got = native.em_effect_original_001B5DC0(s32(a0))
        assert want == got, ('001B5DC0', hex(a0), want, got)
        rings[want] = rings.get(want, 0) + 1
    assert sorted(rings) == [0, 1, 2, 3], ('a ring never reached', rings)
    reference_mode.banner('all %d cases (every low byte under %d high patterns)' % (len(inputs), len(HIGH)))
    print('stick ring 001B5DC0 vs original instructions: PASS %d cases, rings %s (%.1fs)' % (
        len(inputs), ', '.join('%d: %d' % kv for kv in sorted(rings.items())), time.time() - started))


if __name__ == '__main__':
    main()
