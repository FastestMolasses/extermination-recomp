#!/usr/bin/env python3
"""001B5BC0 (the attract demo's recorded-pad step) vs the original instructions.

em_pad_record_001B5BC0 (src/em_input.c) is the port's translation; the
scene binding (em_scene_bindings' w_001B5BC0) finds record a0 of demo
D_00275BD4 in the table D_0028A6AC[0] (0x4800 bytes per demo, 5 per record)
and hands it, with the record before it (none for a0 = 0), to that core
(docs/ATTRACT_DEMO.md).

The oracle executes the user's pinned boot ELF: 001B5BC0 runs as original
instructions in a closed world (it calls nothing). Every store it makes is
logged; it must store exactly D_00810E70 / E72 / E74 / E76 (halfwords) and
E64 / E65 / E57 (bytes), and the native core must give the same seven
values. E74's old value (E76's source) and the rest of the pad block are
set to a fresh pattern before each call.

Inputs: the three demos' recorded tables as the original's capture holds
them (the decomp's build/demo_ref/run1/snap_0_sub1, module 0x36 at
D_0028A6AC; records 0..3599 of each, plus a0 = 0x0E0F, the last whole
record), and random tables (every byte value, a0 = 0 and the last record,
demo indices 0..2). The quick run takes every 16th recorded record and 300
random cases; EM_TEST_FULL=1 takes every record and 3000 random cases. No
original bytes are embedded or printed.
"""
import ctypes as C
import random
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
from test_player_slide_reference import read_elf  # noqa: E402
import test_player_recovery_reference as recovery  # noqa: E402

OUT = ROOT / 'build' / 'pad_record_reference'
SNAP = ROOT.parent / 'Extermination/build/demo_ref/run1/snap_0_sub1/eeMemory.bin'
STEP = 0x1B5BC0
D_BD4, D_TABLE = 0x275BD4, 0x28A6AC
PAD = 0x810E50                      # the pad block D_00810E50..D_00810E7F
DEMO_SIZE, RECORDS = 9 * 2048, 0xE10
RANDOM_TABLE = 0x01E00000           # a free RAM span for the random tables
WANT = {0x810E70: 2, 0x810E72: 2, 0x810E74: 2, 0x810E76: 2, 0x810E64: 1, 0x810E65: 1, 0x810E57: 1}


class PadRecord(C.Structure):
    _fields_ = [('held', C.c_uint16), ('prev_held', C.c_uint16), ('pressed', C.c_uint16),
                ('prev_pressed', C.c_uint16), ('repeat', C.c_uint16), ('repeat_timer', C.c_int16),
                ('lx', C.c_uint8), ('ly', C.c_uint8), ('rx', C.c_uint8), ('ry', C.c_uint8), ('gait', C.c_uint8)]


class LogEE(recovery.ModelEE):
    """The oracle with every store logged (address, size)."""

    def __init__(self, elf):
        super().__init__(elf)
        self.stores = []

    def save(self, address, value, size=4):
        self.stores.append((address & 0xFFFFFFFF, size))
        super().save(address, value, size)


def build_native():
    OUT.mkdir(parents=True, exist_ok=True)
    lib = OUT / ('pad_record' + ('.dylib' if sys.platform == 'darwin' else '.so'))
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', 'src/em_input.c', '-lm', '-o', str(lib)], cwd=ROOT, check=True)
    n = C.CDLL(str(lib))
    n.em_pad_record_001B5BC0.argtypes = [C.POINTER(PadRecord), C.c_char_p, C.c_char_p]
    n.em_pad_record_001B5BC0.restype = None
    return n


def one(ee, native, rng, table, demo, a0, where):
    """001B5BC0(a0) over `table` (the address D_0028A6AC[0] holds) and demo
    index `demo`: the oracle's seven stores against the native core's."""
    block = bytes(rng.getrandbits(8) for _ in range(0x30))
    ee.write(PAD, block)
    ee.save(D_BD4, demo)
    ee.save(D_TABLE, table)
    ee.stores = []
    ee.r, ee.rh = [0] * 32, [0] * 32
    ee.r[28] = 0x27D370
    ee.r[29] = 0x01F00000
    ee.call(STEP, (a0,))
    stored = {}
    for address, size in ee.stores:
        assert WANT.get(address) == size, (where, 'a store outside the pad words', hex(address), size)
        stored[address] = ee.load(address, size)
    assert set(stored) == set(WANT), (where, 'stores missing', sorted(hex(a) for a in set(WANT) - set(stored)))
    at = table + demo * DEMO_SIZE + a0 * 5
    rec = ee.read(at, 5)
    prev = ee.read(at - 5, 5) if a0 else None
    u = PadRecord()
    u.pressed = struct.unpack_from('<H', block, 0x24)[0]       # E74 before the call
    native.em_pad_record_001B5BC0(C.byref(u), rec, prev)
    got = {0x810E70: u.held, 0x810E72: u.prev_held, 0x810E74: u.pressed, 0x810E76: u.prev_pressed,
           0x810E64: u.lx, 0x810E65: u.ly, 0x810E57: u.gait}
    for address in WANT:
        assert got[address] == stored[address], (where, hex(address), 'native', hex(got[address]),
                                                 'original', hex(stored[address]))


def main():
    started = time.time()
    full = reference_mode.FULL
    rng = random.Random(0x1B5BC0)
    ee = LogEE(read_elf())
    ee.allowed = set()
    ee.limit = 1000
    native = build_native()
    recorded = 0
    if SNAP.is_file():
        snap = SNAP.read_bytes()
        table = struct.unpack_from('<I', snap, D_TABLE)[0]
        assert 0x100000 <= table < 0x2000000 - 3 * DEMO_SIZE, ('D_0028A6AC in the snapshot', hex(table))
        ee.write(table, snap[table:table + 3 * DEMO_SIZE])
        step = 1 if full else 16
        for demo in range(3):
            for a0 in sorted(set(range(0, RECORDS, step)) | {1, RECORDS - 1, DEMO_SIZE // 5 - 1}):
                one(ee, native, rng, table, demo, a0, ('recorded', demo, a0))
                recorded += 1
    else:
        print(f'test_pad_record_reference: no capture snapshot {SNAP}; the recorded tables are skipped')
    count = 3000 if full else 300
    for i in range(count):
        ee.write(RANDOM_TABLE, bytes(rng.getrandbits(8) for _ in range(3 * DEMO_SIZE)) if i % 100 == 0
                 else ee.read(RANDOM_TABLE, 3 * DEMO_SIZE))
        demo = rng.randrange(3)
        a0 = rng.choice((0, 1, DEMO_SIZE // 5 - 1, rng.randrange(DEMO_SIZE // 5)))
        one(ee, native, rng, RANDOM_TABLE, demo, a0, ('random', i, demo, a0))
    reference_mode.banner(reference_mode.part(recorded, 3 * (RECORDS + 1), 'recorded records'),
                          reference_mode.part(count, 3000, 'random cases'))
    print('pad record 001B5BC0 vs original instructions: PASS %d recorded and %d random cases, '
          'seven stores each (%.1fs)' % (recorded, count, time.time() - started))


if __name__ == '__main__':
    main()
