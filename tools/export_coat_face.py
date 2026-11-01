#!/usr/bin/env python3
"""Add the player's other faces to the retained resources (lane M1).

001B81D0 picks D_0028A490 row 0x19 while the player's +0x2FF is 0x3E (the
coat), row 0x1A at 0x3F and row 0x1B at 0x40 (the infected models; a script
holds that face at infection 100, AREA04 sub 1's event a04c_00, read by address
through 001CB3C0): extract/chunk03/f17_id19.bin, f18_id1a.bin, f19_id1b.bin,
which equal RAM at the table's words in the fork capture of a04c_00. The
export's own table words say where they lie. Run after export_later_library.py;
each region is added once.

Output: assets/scene_snow/roger/resources.emrs, disc-derived and ignored.
Usage (port root; native arm64 macOS): python3 tools/export_coat_face.py
"""
import struct
from pathlib import Path

import export_area00_library as L
import export_area02_library as L2
import export_roger_banks as B

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/scene_snow/roger/resources.emrs'
# D_0028A490 row, the file it holds
ROWS = ((0x19, 'f17_id19.bin'), (0x1A, 'f18_id1a.bin'), (0x1B, 'f19_id1b.bin'))


def add_row(header, regions, row, name):
    address = struct.unpack_from('<I', header, 32 + 4 * row)[0]
    face = (B.DECOMP / 'extract/chunk03' / name).read_bytes()
    assert len(face) == 0x4C800, len(face)
    if any(a <= address and address + len(face) <= a + len(r) for a, _w, _f, r in regions):
        print(f'row {row:#x}: already present')
        return False
    assert not any(a < address + len(face) and address < a + len(r) for a, _w, _f, r in regions)
    # each row's file ends where the next starts: one region (the pose
    # host's table holds twelve, EM_POSE_REGION_MAX)
    for i, (a, w, f, r) in enumerate(regions):
        if a + len(r) == address and (w, f) == (0, 0):
            regions[i] = (a, 0, 0, r + face)
            break
    else:
        regions.append((address, 0, 0, face))
    print(f'row {row:#x}: {len(face):#x} bytes at {address:#x}, {len(regions)} regions')
    return True


def main():
    if any(p.is_symlink() for p in (OUT, *OUT.parents)):
        raise SystemExit(f'{OUT} is a symlink: unlink it first')
    data = OUT.read_bytes()
    header, regions = L.parse(data)
    if not any([add_row(header, regions, row, name) for row, name in ROWS]):
        return 0
    if len(regions) > 16:
        raise SystemExit(f'{len(regions)} regions: the provider holds 16')
    OUT.write_bytes(L2.rebuild(header[:16] + len(regions).to_bytes(4, 'little') + header[20:], regions))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
