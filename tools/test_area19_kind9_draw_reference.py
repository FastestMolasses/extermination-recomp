#!/usr/bin/env python3
"""The kind-9 draw method 001CB130 -> 001CB070 (the AREA19 worms' model)
against the ORIGINAL instructions, EE side.

The original runs in full from the captured AREA19 RAM (the decomp's
fork_refs a19c beats, AREA19 sub 0): 001CB130, 001CB070, 001D8C20 (light
modes 4 and 0), 001C7420 with 001D89D0 (object and bone matrices),
001D40E0 with 001D6930 / 001D1F80 / 001D6580 / 001D1F20 / 001D6BA0 /
001D1FF0 and the skin tail (the program CALL, the frame copy, the highlight
strip, the GS state, the TEX0, the model REFs), the RET tag, 001CAAC0 and
001CB760 (the chain-table insertion at the owner's depth). Nothing is hooked.
The native side is the production path: em_area01_model_draw_call(0x001CB130)
-> em_owner_draw_live_method (mode 4) over tests/area01_model_draw_bridge.c,
whose render-context views are the same captured memory (the veil module's
builders and the packet chain bound as em_render_context_live binds them).
After each case the whole 32 MiB EE RAM image and the 16 KiB scratchpad must
be byte-equal: the record's +0x80..+0x88 stores, the channel-3 unit (every
packet word), the cursors, the chain table and the scratch matrices. The
original's executed addresses must include every callee named above.

No capture holds a live worm: the five a19c beats keep the last worm's
record at 0x7BD710 freed (bytes +0x00..+0x0F and the bone pointers cleared;
its model 0x11699C0, colour, position and matrix kept). Designed states,
every changed byte named:
  worm     the freed record with +0x00..+0x0F (active header, bone count)
           and +0x110..+0x16F (24 bone pointers) copied from a live 24-bone
           record of the same capture (the worm model's own count is 24)
  donor    a live record (each bone count the capture has) with +0x44 :=
           0x11699C0 (the worm model) and +0x4C := 0x001CB130
  collapse one donor case with +0x94 := 1, the bone 001C7420 collapses to
           its translation row (every captured record has +0x94 = -1)
The rows 001D40E0 reads (0x26E470, 0x251BD0, 0x251BEC) are checked equal to
the ELF's before the bridge serves them from the captured RAM (production
reads them from effect_tables.emet).

EM_TEST_FULL=1: every a19c beat and every donor; quick: one beat, the worm
and one donor per bone count.
"""
import struct
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode as mode  # noqa: E402
import reference_root as refs  # noqa: E402
import test_area01_model_draw_reference as MD  # noqa: E402
from test_module_loader_reference import read_elf_bytes  # noqa: E402

ELF_ROW = None
WORM, WORM_MODEL, METHOD = 0x7BD710, 0x11699C0, 0x1CB130
POOL, POOL_END, RECORD = 0x7A5640, 0x7D4640, 0x2F0
BEATS = ('a19c_01_bar841', 'a19c_02_use7', 'a19c_03_slide', 'a19c_04_walkway', 'a19c_05_door27')
CAPTURE = 'levels-recordings-20261018-17/s87/route_a19c'
CALLEES = (0x1CB070, 0x1D8C20, 0x1C7420, 0x1D89D0, 0x1D40E0, 0x1D6930, 0x1D1F80, 0x1D6580,
           0x1D1F20, 0x1D6BA0, 0x1D1FF0, 0x1CAAC0, 0x1CB760)
ROWS = ((0x26E470, 0x50), (0x251BD0, 0x10), (0x251BEC, 0x24))   # not +0x10..+0x1B: 001D40E0 stores there


def u32(b, a):
    return struct.unpack_from('<I', b, a)[0]


def put(ram, address, value):
    struct.pack_into('<I', ram, address, value)


def cases_for(ram):
    """(label, designed RAM) for one capture."""
    live = [a for a in range(POOL, POOL_END, RECORD) if ram[a] and ram[a + 0xC] and 0 < u32(ram, a + 0x44) < 0x2000000]
    out = []
    donors24 = [a for a in live if ram[a + 0xC] == 24]
    assert u32(ram, WORM + 0x4C) == METHOD and u32(ram, WORM + 0x44) == WORM_MODEL and not any(ram[WORM:WORM + 0x10])
    assert u32(ram, WORM_MODEL + 8) == 24, 'the worm model is not the 24-bone one'
    for donor in donors24 if mode.FULL else donors24[:1]:
        r = bytearray(ram)
        r[WORM:WORM + 0x10] = ram[donor:donor + 0x10]
        r[WORM + 0x110:WORM + 0x170] = ram[donor + 0x110:donor + 0x170]
        out.append(('worm+bones of %X' % donor, WORM, r))
    seen = set()
    for donor in live:
        count = ram[donor + 0xC]
        if not mode.FULL and count in seen:
            continue
        seen.add(count)
        r = bytearray(ram)
        put(r, donor + 0x44, WORM_MODEL)
        put(r, donor + 0x4C, METHOD)
        out.append(('donor %X (%d bones)' % (donor, count), donor, r))
    r = bytearray(out[0][2])
    r[0x251BE0:0x251BEC] = ELF_ROW                      # the row as the ELF has it (no worm drawn yet)
    out.append(('worm, first draw (D_00251BE0 as in the ELF)', WORM, r))
    donor = donors24[0]
    r = bytearray(ram)
    put(r, donor + 0x44, WORM_MODEL)
    put(r, donor + 0x4C, METHOD)
    struct.pack_into('<h', r, donor + 0x94, 1)
    out.append(('collapsed bone 1 of %X' % donor, donor, r))
    return out


def main():
    start = time.time()
    lib = MD.build()
    elf = read_elf_bytes()
    paths = [refs.FORK / CAPTURE / beat / 'eeMemory.bin' for beat in BEATS]
    paths = [p for p in paths if p.exists()]
    if not paths:
        raise SystemExit('missing the a19c captures (%s)' % (refs.FORK / CAPTURE))
    paths = paths if mode.FULL else [p for p in paths if 'a19c_02' in str(p)] or paths[:1]
    executed = set()
    cases = unit_bytes = 0
    counts = set()
    original_ee = MD.EE

    class Recording(original_ee):
        def __init__(self, *a, **k):
            super().__init__(*a, **k)
            Recording.last = self

    MD.EE = Recording
    try:
        for path in paths:
            ram = path.read_bytes()
            spad = (path.parent / 'scratchpad.bin').read_bytes()
            assert ram[0x810700] == 0x13, (path, 'not AREA19')
            for address, size in ROWS:
                at = address - 0x100000 + 0x300
                assert ram[address:address + size] == elf[at:at + size], ('captured rows differ from the ELF', hex(address))
            ctx = u32(ram, 0x275670)
            global ELF_ROW
            ELF_ROW = elf[0x251BE0 - 0x100000 + 0x300:0x251BEC - 0x100000 + 0x300]
            for label, node, designed in cases_for(ram):
                ch3 = u32(designed, ctx + 0x1C)
                units, _packets, image, _cross = MD.check(lib, elf, bytes(designed), spad, node)
                executed |= Recording.last.pcs
                # The unit: 001CB070 moved the channel-3 cursor past its RET tag.
                end = u32(image, ctx + 0x1C)
                assert end > ch3 and image[end - 0x10 + 3] == 0x60, (label, hex(ch3), hex(end))
                assert image[node + 0x80:node + 0x8C] == struct.pack('<3f', 1.0, 1.0, 1.0), label
                unit_bytes += end - ch3
                counts.add(designed[node + 0xC])
                cases += 1
    finally:
        MD.EE = original_ee
    missing = [hex(a) for a in CALLEES if a not in executed]
    assert not missing, ('original callees never executed', missing)
    mode.banner('%d kind-9 cases over %d AREA19 capture(s)' % (cases, len(paths)))
    print('AREA19 kind-9 draw reference: PASS -- %d cases (bone counts %s, a collapsed bone, the worm '
          'record): the whole EE RAM and scratchpad byte-equal after the original 001CB130 and '
          'em_area01_model_draw_call; %d unit bytes; %d original callees executed; %.1f s'
          % (cases, sorted(counts), unit_bytes, len(CALLEES), time.time() - start))


if __name__ == '__main__':
    main()
