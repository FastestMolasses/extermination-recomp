#!/usr/bin/env python3
"""Export the camera timeline of the scene after AREA17's cable ride.

AREA11 entered from AREA17 (entry 3): the flag-0x30 manager 00823CE0 starts
the script 0x828C70 (tools/export_area11_opening.py writes its image). Its
op00 sub 6 starts the camera timeline 0022EC30 on bank D_0028A490[0x97]'s
clip 0, scene 0x1A (26). This writes, from the user's own extract and ELF:

  assets/scene_snow/cable_scene/camera.emcc  bank 0x97's clip 0 (the camera
      track; extract/chunk15/f12_id44.bin +0x60000, the bank directory's
      first entry), the EMCC format export_opening_camera.py writes
  assets/scene_snow/cable_scene/events.emfx  the ELF's event words
      0x26AB98..0x26ADB0 as one EMFX v1 window: 0022EC30's scene-26 tables
      (+0x84 the letterbox D_0026AB98, +0x80 the fades D_0026ADA0; the
      window ends with the latter's 0.0 terminator)
  assets/scene_snow/roger/resources.emrs  (rewritten in place, run it after
      export_roger_banks.py and the library exporters): the scene's two
      actors' models D_0028A490[0x50] and [0x5A] (op14's list 0x828BE0;
      001BAD40's 001CA6E0 / 001C6150 read them) are AREA11 resident bytes
      just below the region of f18_id94 (model 0x5A runs into it), so that
      region is extended down to [0x50] with the bytes the area load leaves
      there (export_disc_textures_gs.ResourceTable.resident_bytes; the
      region's own bytes are checked unchanged; the table words are the
      first level's, as for every other region)

With the guided g48 end state present (decomp build/fork_refs/guided-
20261117/s87/route_g49/g48_area17_cable/eeMemory.bin, or --verify-ram FILE;
--no-verify skips it) the bank's bytes are checked against RAM at that
run's D_0028A490[0x97], and the event window against RAM.

Output is private disc-derived data under ignored assets/ (never committed).
Runs natively on arm64 macOS (pure Python).
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from export_opening_camera import export as export_camera

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
BANK = 0x60000            # bank 0x97 in f12_id44 (bank 0x96 at +0x41000)
EVENTS = (0x26AB98, 0x26ADB0)
RAM = DECOMP / 'build/fork_refs/guided-20261117/s87/route_g49/g48_area17_cable/eeMemory.bin'
ELF_SHA256 = 'ee052236783e7d3e865754d3ff9fee71290addeb7d146c86caa7ff2724d1e17a'


def u32(b, a):
    return struct.unpack_from('<I', b, a)[0]


def extend_models(path, ram):
    """resources.emrs with the region holding [0x47]'s file extended down
    to D_0028A490[0x50] (see the docstring). Returns (bytes, report)."""
    import sys
    sys.path.insert(0, str(ROOT / 'tools'))
    import export_disc_textures_gs as G
    data = path.read_bytes()
    if data[:4] != b'EMRS' or u32(data, 4) != 2:
        raise SystemExit(f'{path}: not an EMRS v2 export (run export_roger_banks.py first)')
    words, count = u32(data, 12), u32(data, 16)
    table = [u32(data, 32 + 4 * i) for i in range(words)]
    lo = table[0x50]
    at, regions = 32 + 4 * words, []
    for _ in range(count):
        a, n, w, lib = struct.unpack_from('<4I', data, at)
        regions.append([a, w, lib, data[at + 16:at + 16 + n]])
        at += 16 + n
    if at != len(data):
        raise SystemExit(f'{path}: trailing bytes')
    model = G.ResourceTable(G.Disc(None, None))
    for i in (0x50, 0x5A, 0x47):
        if model.words[i] != table[i]:
            raise SystemExit(f'D_0028A490[{i:#x}] differs from the disc model')
    host = [r for r in regions if r[0] <= table[0x47] < r[0] + len(r[3])]
    if len(host) != 1 or host[0][1] or host[0][2]:
        raise SystemExit('no single read-only region holds model 0x47')
    r = host[0]
    end = r[0] + len(r[3])
    if r[0] > lo:
        if r[0] > table[0x5A] + u32(model.resident_bytes(table[0x5A], 16), 12):
            raise SystemExit('model 0x5A does not reach the region')
        below = model.resident_bytes(lo, r[0] - lo)
        if model.resident_bytes(r[0], len(r[3])) != r[3]:
            raise SystemExit('the region differs from the resident bytes')
        r[0], r[3] = lo, below + r[3]
    elif r[0] != lo or model.resident_bytes(lo, end - lo) != r[3]:
        raise SystemExit('the extended region differs from the resident bytes')
    for i in (0x50, 0x5A):
        a = table[i]
        size = u32(r[3], a - lo + 12)
        if a + size > end:
            raise SystemExit(f'model {i:#x} leaves the region')
    for other in regions:
        if other is not r and other[0] < end and lo < other[0] + len(other[3]):
            raise SystemExit(f'the extended region overlaps the region at {other[0]:#x}')
    checked = None
    if ram is not None:
        shift = u32(ram, 0x28A490 + 0x50 * 4) - lo
        for i in (0x50, 0x5A):
            a = table[i]
            size = u32(r[3], a - lo + 12)
            if u32(ram, 0x28A490 + 4 * i) != a + shift or ram[a + shift:a + shift + size] != r[3][a - lo:a - lo + size]:
                raise SystemExit(f'model {i:#x} differs from RAM (the load at +{shift:#x})')
        checked = f'+{shift:#x}'
    out = bytearray(data[:32 + 4 * words])
    for a, w, lib, raw in regions:
        out += struct.pack('<4I', a, len(raw), w, lib) + raw
    return bytes(out), dict(region=f'{lo:08X}..{end:08X}', models={f'{i:#x}': f'{table[i]:08X}' for i in (0x50, 0x5A)},
                            ram_shift=checked)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--verify-ram', type=Path, help='optional: an EE RAM image after the AREA11 entry-3 load')
    ap.add_argument('--no-verify', action='store_true')
    ap.add_argument('--out', type=Path, default=ROOT / 'assets/scene_snow/cable_scene')
    args = ap.parse_args()
    source = DECOMP / 'extract/chunk15/f12_id44.bin'
    data = source.read_bytes()
    count = struct.unpack_from('<I', data, BANK)[0]
    if count != 4:
        raise SystemExit('bank 0x97: expected four clips (the camera, the player and the two actors)')
    elf = (DECOMP / 'config/SCUS_971.12').read_bytes()
    if hashlib.sha256(elf).hexdigest() != ELF_SHA256:
        raise SystemExit('not the pinned SCUS-97112 ELF')
    lo, hi = EVENTS
    window = elf[lo - 0x100000 + 0x300:hi - 0x100000 + 0x300]
    words = struct.unpack(f'<{len(window) // 4}f', window)
    if words[-1] != 0.0 or any(w != w or abs(w) == float('inf') for w in words):
        raise SystemExit('the event window is not finite words ending with 0.0')
    checked = ram = None
    ram_path = args.verify_ram or RAM
    if not args.no_verify and ram_path.is_file():
        ram = ram_path.read_bytes()
        runtime = struct.unpack_from('<I', ram, 0x28A490 + 0x97 * 4)[0]
        size = struct.unpack_from('<I', data, BANK + 8)[0] & ~3   # through clip 0
        if data[BANK:BANK + size] != ram[runtime:runtime + size]:
            raise SystemExit(f'{ram_path}: bank 0x97 differs from RAM at {runtime:#x}')
        if window != ram[lo:hi]:
            raise SystemExit(f'{ram_path}: the event window differs from RAM')
        checked = dict(ram=str(ram_path), runtime_bank=f'{runtime:08X}')
    out = args.out
    for name in ('camera.emcc', 'camera.json', 'events.emfx'):
        if (out / name).is_symlink():
            (out / name).unlink()
    out.mkdir(parents=True, exist_ok=True)
    camera = export_camera(source, BANK, out / 'camera.emcc')
    emrs = ROOT / 'assets/scene_snow/roger/resources.emrs'
    blob, models = extend_models(emrs, ram if checked else None)
    if emrs.is_symlink():
        emrs.unlink()   # a worktree link: write this tree's own copy
    emrs.write_bytes(blob)
    emfx = struct.pack('<4sII', b'EMFX', 1, len(words)) + window
    (out / 'events.emfx').write_bytes(emfx)
    report = dict(camera=camera, models=models, events=dict(base=lo, end=hi, words=len(words),
                                             sha256=hashlib.sha256(emfx).hexdigest()),
                  ram_check=checked)
    folder = ROOT / 'build/cable_scene'
    folder.mkdir(parents=True, exist_ok=True)
    (folder / 'export.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'cable scene: bank 0x97 camera {camera["duration"]:.0f} frames, {len(words)} event words'
          + (' (checked against RAM)' if checked else ''))


if __name__ == '__main__':
    main()
