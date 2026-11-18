#!/usr/bin/env python3
"""export_game_over.py - compose the game-over screen (screen module 0x27)
and the ending's two pictures (screen module 0x37, --module 0x37).

001AD4E0 (byte-matched; em_scene_task.c em_sf_001AD4E0) loads screen module
0x27 at its step 1 (001FF080(0, 0x27): DATA.DAT chunk 0x27, the user's
extract/chunk39) and, from step 3 on, hands 001ABF90 four TEX0 words every
tick: 0x2005C00621322A00, 0x2005C08621322A40, 0x2005C20621322C00 and
0x2005C28621322C40 (em_scene_task.c push_packet_001ABF90, the instructions'
immediates). 001ABF90 draws them as four 256 x 256 sprites, top-left,
top-right, bottom-left, bottom-right, in the order the decomp's
export_startup.py models for the logo screens (the same routine, the same
canvas): the module's one PSMCT32 upload holds both the indexed art and its
palettes, so the texels and the CLUTs come from the disc alone.

001AD740 (the ending; em_scene_task.c em_sf_001AD740) loads screen module
0x37 the same way at its state 3 (001FF080(0, 0x37), extract/chunk55) and
hands 001ABF90 two sets of four TEX0 words: 0x2006440621322E00,
0x2006448621322E40, 0x2006460621323000, 0x2006468621323040 (the first
picture, its subs 2 .. 4) and 0x2006400621322A00, 0x2006408621322A40,
0x2006420621322C00, 0x2006428621322C40 (the second, subs 5 and 6). They
compose as the game over's: assets/startup/ending_0.emui and ending_1.emui.

This tool composes that screen with the decomp's own composer
(../Extermination/tools/export_startup.py: upload_module, compose,
write_emui; imported read-only, nothing there is changed) and writes the
port's asset. It runs natively on arm64 macOS (Python stdlib).

Optional check (developers with the capture): --capture <folder> decodes
the four textures from that capture's GS local memory (gs.bin; the DAMAGE
lane's dmg_02_flame_death end snapshot shows the game-over screen) and
requires every texel and palette entry to equal the disc's decode.

Output (disc-derived: git-ignored assets/ only):
  assets/startup/game_over.emui   EMUI version 1, 512 x 448 (the startup
                                  screens' format, em_frontend.c)
  assets/startup/game_over.json   the source's SHA-256, the TEX0 words and
                                  the check result (no pixels)
  assets/startup/ending_0.emui, ending_1.emui (+ .json)   --module 0x37

Usage (port root):
  python3 tools/export_game_over.py [--module 0x27|0x37] [--extract ../Extermination/extract]
          [--capture ../Extermination/build/c10/damage/dmg_02_flame_death]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
sys.path.insert(0, str(DECOMP / 'tools'))

import export_startup as S  # noqa: E402

MODULE = 0x27
# 001AD4E0's 001ABF90 arguments (top-left, top-right, bottom-left,
# bottom-right), the immediates of its steps 3 and 4.
TEX0S = (0x2005C00621322A00, 0x2005C08621322A40, 0x2005C20621322C00, 0x2005C28621322C40)
# Per module: the screens it holds, by output name (001AD740's two sets for
# module 0x37, em_scene_task.c push_ending_001ABF90).
SCREENS = {
    0x27: {'game_over': TEX0S},
    0x37: {'ending_0': (0x2006440621322E00, 0x2006448621322E40, 0x2006460621323000, 0x2006468621323040),
           'ending_1': (0x2006400621322A00, 0x2006408621322A40, 0x2006420621322C00, 0x2006428621322C40)},
}
GS_LOCALMEM = 0x400000


def capture_local(folder: Path) -> bytes:
    blob = (folder / 'gs.bin').read_bytes()
    base = len(blob) - GS_LOCALMEM - 84       # the freeze layout (gs_vram.py localmem_base)
    if base < 0:
        raise SystemExit(f'{folder}/gs.bin: too short for GS local memory')
    return blob[base:base + GS_LOCALMEM]


def upload_all(path: Path) -> tuple[bytes, set[int]]:
    """Every PSMCT32 host-to-local transfer of the module, in order. Module
    0x27 has one (export_startup.upload_module's); module 0x37 has two: the
    pictures' texels from block 0x2A00 and, after them, a 256 x 64 transfer
    to block 0x3180 that holds the palettes (CBP 0x3200 .. 0x3234)."""
    data = path.read_bytes()
    local = bytearray(GS_LOCALMEM)
    covered: set[int] = set()
    transfers = 0
    for at in range(0, len(data) - 16, 16):
        if int.from_bytes(data[at + 8:at + 16], 'little') != 0x50:
            continue
        bb = int.from_bytes(data[at:at + 8], 'little')
        if (bb >> 56) & 63:
            raise SystemExit(f'{path}: a transfer at {at:#x} is not PSMCT32')
        shape = S.first_trxreg(data, at)
        if shape is None:
            raise SystemExit(f'{path}: the transfer at {at:#x} has no TRXREG')
        w, h = shape
        base, ppr = ((bb >> 32) & 16383) * 256, (bb >> 48) & 63
        start = S.find_image_block(data, w * h * 4, at)
        if start is None or not ppr:
            raise SystemExit(f'{path}: the transfer at {at:#x} has no IMAGE payload')
        for y in range(h):
            for x in range(w):
                dst = base + S.psmct32_word(x, y, ppr) * 4
                local[dst:dst + 4] = data[start + (y * w + x) * 4:start + (y * w + x) * 4 + 4]
                covered.add(dst // 256)
        transfers += 1
    if not transfers:
        raise SystemExit(f'{path}: no image transfer')
    return bytes(local), covered


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--extract', type=Path, default=DECOMP / 'extract')
    ap.add_argument('--module', type=lambda v: int(v, 0), default=MODULE, choices=sorted(SCREENS))
    ap.add_argument('--out-dir', type=Path, default=ROOT / 'assets/startup')
    ap.add_argument('--capture', type=Path, help='a capture folder whose gs.bin shows the screen')
    args = ap.parse_args(argv)
    module = args.module
    paths = sorted((args.extract / f'chunk{module:02d}').glob('*.bin'))
    if len(paths) != 1:
        raise SystemExit(f'expected one section in chunk{module:02d}; got {len(paths)}')
    source = paths[0].read_bytes()
    local, covered = S.upload_module(paths[0]) if module == 0x27 else upload_all(paths[0])
    captured = capture_local(args.capture) if args.capture else None
    args.out_dir.mkdir(parents=True, exist_ok=True)
    for name, tex0s in SCREENS[module].items():
        out = args.out_dir / f'{name}.emui'
        pixels = S.compose(local, covered, tex0s)
        report = {'module': module, 'source': str(paths[0]), 'source_sha256': hashlib.sha256(source).hexdigest(),
                  'tex0': [hex(t) for t in tex0s], 'canvas': [S.WIDTH, S.HEIGHT],
                  'pixels_sha256': hashlib.sha256(pixels).hexdigest()}
        if captured is not None:
            everything = set(range(GS_LOCALMEM // 256))
            for t in tex0s:
                if S.texture(captured, everything, t) != S.texture(local, covered, t):
                    raise SystemExit(f'{args.capture}: TEX0 {t:#x} decodes differently from the disc upload')
            report['capture_check'] = f'{args.capture}: the four textures and palettes equal the disc decode'
        for path in (out, out.with_suffix('.json')):
            if path.is_symlink():
                path.unlink()   # a worktree's link to a shared tree: write a file of its own
        S.write_emui(out, pixels)
        out.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
        print(f'wrote {out}: {S.WIDTH}x{S.HEIGHT} from module {module:#x}'
              + (f'; {report["capture_check"]}' if captured is not None else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
