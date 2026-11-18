#!/usr/bin/env python3
"""export_area11_player_variants.py - AREA11's world texture catalogs for the
player's other texture packets (tools/export_player_texture_variants.py's
pairs, for the first level's catalogs).

00200890 uploads the player texture packet D_0028A490[slot] chosen by
D_00810707 (mode) and D_00810C60 (costume) at every area load (001FFCD0
state 7) and at a page close (00200970(1)); export_disc_textures_gs.
player_texture_slot maps the pairs. AREA11 was first exported for (0, 0)
only (a New Game reaches no other pair there), but the guided run comes back
to AREA11 on the cable from AREA17 with costume 2 (g48's end: D_00810C60 2,
the AREA11 overlay resident), so the area's load uploads slot 0xA.

For each pair the object catalog is the GS memory the disc builds for AREA11
with that packet (export_disc_textures_gs.FirstLevel.world(11, 0, mode,
costume): the same replay object_textures.emot comes from), decoded for
object_textures.emot's keys and the pair's player model keys. Checks:
  * the (0, 0) decode of the object keys equals the installed
    assets/scene_snow/object_textures.emot byte for byte;
  * every object key a pair's packet changes is a player key (the packet
    writes the player's blocks only), and every page key
    (export_page_textures.tex0_set) decodes as in page_textures.emot, so the
    page catalog serves every pair unchanged;
  * every key reads only blocks the disc uploads write (disc_texels);
  * reference check: every guided capture of AREA11 sub 0 with the area's
    overlay resident (0x823504 == 9) whose pair is one of these (route_g49's
    g48_area17_cable: (0, 2)) decodes every key of that pair's catalog from
    its GS memory (gs.bin) to the same texels as the disc replay.

Output (disc-derived, git-ignored): assets/scene_snow/
object_textures_mode<m>_costume<c>.emot for the six pairs and
player_texture_variants.json (sizes, SHA-256s, the checks). Read by
src/game/em_world_textures_live.c with page_textures.emot.

Usage (port root, native arm64 macOS):
  python3 tools/export_area11_player_variants.py [--iso FILE | --disc DIR]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(DECOMP / 'tools'))

import export_object_textures as O  # noqa: E402
import export_page_textures as PT  # noqa: E402
import export_world_models as ewm  # noqa: E402
import reference_root as refs  # noqa: E402
from export_player_texture_variants import PAIRS, MODEL_FILES, read_emot  # noqa: E402

AREA, SUB, OVERLAY_ID = 11, 0, 0x09
ROUTES = ('guided-20261117/s87/route_g49',)   # tools/capture_from_take.py


def recorded_captures():
    out = {}
    for route in ROUTES:
        root = refs.capture(route)
        if not root.is_dir():
            continue
        for ee in sorted(root.glob('*/eeMemory.bin')):
            ram = ee.read_bytes()
            if (ram[0x810700], ram[0x810701], ram[0x823504]) != (AREA, SUB, OVERLAY_ID):
                continue
            pair = (ram[0x810707], ram[0x810C60])
            if pair in PAIRS:
                out.setdefault(pair, []).append(ee.parent / 'gs.bin')
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--iso', type=Path)
    ap.add_argument('--disc', type=Path)
    ap.add_argument('--extract', type=Path, default=DECOMP / 'extract')
    ap.add_argument('--elf', type=Path, default=DECOMP / 'config/SCUS_971.12')
    ap.add_argument('--out', type=Path, default=ROOT / 'assets/scene_snow')
    args = ap.parse_args(argv)
    import export_disc_textures_gs as G
    elf = args.elf.read_bytes()
    if hashlib.sha256(elf).hexdigest() != PT.ELF_SHA256:
        raise SystemExit(f'{args.elf}: not the pinned SCUS-97112 boot ELF')
    texes, _left_out, _static = O.tex0_set(args.extract)
    fixed = set(texes)
    base = read_emot((args.out / 'object_textures.emot').read_bytes())
    page_keys = PT.tex0_set(elf, (args.extract / 'OVERLAY/AREA11.BIN').read_bytes())
    page_base = read_emot((args.out / 'page_textures.emot').read_bytes())
    loader = G.FirstLevel(G.Disc(args.iso, args.disc), args.extract)
    if O.disc_texels(loader.world(AREA, SUB, 0, 0), fixed) != {k: base[k] for k in fixed} or set(base) != fixed:
        raise SystemExit('AREA11: the (0, 0) decode differs from object_textures.emot')
    recorded = recorded_captures()
    rows = []
    for (mode, costume), kind in PAIRS.items():
        model = (args.extract / MODEL_FILES[kind]).read_bytes()
        mkeys = {}
        O.block_tex0(model, 0, ewm.model_record(model, 0, kind)[0], f'player {kind:#x}', mkeys)
        world = loader.world(AREA, SUB, mode, costume)
        texels = O.disc_texels(world, fixed | set(mkeys))
        changed = sorted(k for k in fixed if texels[k] != base[k])
        if any(not any(lbl.startswith('player') for lbl in texes[k]) for k in changed):
            raise SystemExit(f'AREA11: pair {mode, costume} changes a non-player texture')
        pages = PT.texels_of(world, page_keys)
        if any(pages[k] != page_base[k] for k in pages) or set(pages) != set(page_base):
            raise SystemExit(f'AREA11: pair {mode, costume} changes a page texture')
        for k, data in texels.items():
            if k in page_base and page_base[k] != data:
                raise SystemExit(f'AREA11: pair {mode, costume}: TEX0 {k:#x} aliases a page texture')
        O.cross_check(texels, recorded.get((mode, costume), []))
        data = O.emot(texels)
        path = args.out / f'object_textures_mode{mode}_costume{costume}.emot'
        if path.is_symlink():
            raise SystemExit(f'{path}: a symlink (write a real file)')
        path.write_bytes(data)
        rows.append(dict(mode=mode, costume=costume, model=hex(kind), file=path.name, textures=len(texels),
                         player_keys_added=len(set(mkeys) - fixed), changed_player_keys=len(changed),
                         cross_checked=[str(g.parent.name) for g in recorded.get((mode, costume), [])],
                         bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        print(f'AREA11/0: {path.name}: {len(texels)} textures ({rows[-1]["player_keys_added"]} player keys added, '
              f'{len(changed)} changed); cross-checked: {rows[-1]["cross_checked"] or "none"}')
    (args.out / 'player_texture_variants.json').write_text(json.dumps(dict(
        area=AREA, sub=SUB, base='object_textures.emot', pages='page_textures.emot', variants=rows,
        limitation='pairs without a cross_checked recording: the texels are the disc replay of 00200890 only'),
        indent=1) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
