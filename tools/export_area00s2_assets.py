#!/usr/bin/env python3
"""Export AREA00 sub 2 (docs/AREA14_PORT.md): the room the constructed start
k01 loads (EIGHTEENTH_LEVEL_ROUTE.md section 7c).

AREA00's subs 0 and 1 (chunk04.n0 / n1) are export_area00_*'s: two nested
blocks that differ only in their static bank. Sub 2's nested block
(chunk04.n2) is another level: sixteen files, none equal to n0's. So sub 2
takes the AREA13 lane's per-(target, sub) exporters, imported and run
unchanged, as AREA08's and AREA07's subs do (export_area08_common): a Target
for AREA00 (area 0, OVERLAY/AREA00.BIN = MWo3 id 1, INDEX chunk04, nested),
pointed at sub 2 by `install`.

  captures  k01_00_load (the arrival from the card's LOAD), k01_01_slab
            (route_k01 of levels-recordings-20261102-1); k01_02_item26 ends
            in AREA14 and is left out by its area byte
  previous  a02c_01_door22 (the state the card's LOAD ran from)

001A2370 / 0019C6F0 rules: sub 2's overlay owners (0x825D70, 0x8260F0,
0x826790, 0x826BE0, 0x826CC0; decomp tools/area_overview.py --area 0) call
neither (their byte-identical C: only 0x825600 and 0x8263C0, sub 0's owners,
call 001A2370; only the shaft door 0x823580, sub 0's, calls 0019C6F0).
The boot owners' rules (the pickup and the drum) are the exporter's own.

Output (ignored, disc-derived, never committed): assets/area00/sub2/ (the
level, collision, cells, roster, world models, messages, sound), and the
side files of this run under build/area00s2/assets/tree/ only: AREA00's
spawn table, door rows, scripts window and overlay data stay
export_area00_tables' (assets/area00/), and the port's AREA00 scripts window
is widened in em_area01_script_live.c to the chains sub 2's owners start.

Usage (port root, macOS arm64, pure Python):
  python3 tools/export_area00s2_assets.py [--kind level|tables|sfx|place]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_area13_common as A13  # noqa: E402  (first: it keeps AREA01's nested load-map builder)
import export_area13_level as LV  # noqa: E402
import export_area13_tables as TB  # noqa: E402
import export_area13_sfx as SX  # noqa: E402
import export_area15_level as L15  # noqa: E402
import export_area08_common as A08  # noqa: E402  (its retained-colour room check)

C = A13.C
ROOT = A13.ROOT
SET = A13.DECOMP / 'build/fork_refs/levels-recordings-20261102-1/s87'
SCRATCH = ROOT / 'build/area00s2/assets'
# export_area00_common.OVERLAY_SHA256 (a hash, not disc data)
AREA00_SHA256 = '86fb5d594ca8963d13404933cfd7bd2fbe2268b2468a83aa399fd2c1558b8784'
TARGET = A13.Target('area00', 0, 2, 'AREA00.BIN', 0x9F80, 1, AREA00_SHA256, 'chunk04', True,
                    ROOT / 'assets/area00/sub2', 'AREA00')
A13.TARGETS['area00'] = TARGET
SUB = 2
PATHS = (SET / 'route_k01/k01_00_load', SET / 'route_k01/k01_01_slab', SET / 'route_k01/k01_02_item26')
PREVIOUS = SET / 'route_a02c/a02c_01_door22'
# every func_001BA1A0 script argument of the committed AREA00 overlay C
# (re-read by export_area13_tables.c_script_entries)
SCRIPT_ENTRIES = None
DOORS = (0x1BC350, 0x1BB860, 0x1BD9F0, 0x1BD560)
DOOR_ROW = 0x24DF80                         # D_0024E140[0] (export_area00_tables)


def flag_calls(ram):
    """No sub-2 owner calls 0019C6F0 (module docstring)."""
    return ()


def pins():
    read = C.static_reader(C.read_elf(), A13.read_overlay(TARGET))
    groups, paddr, _placements = TB.R.walk_roster(read, TARGET.area, SUB)
    _table, entries, count = TB.spawn_rows(read)
    return paddr, tuple((a, len(r)) for a, r in groups), (entries, count)


def dynamic():
    """True when sub 2's nested relocation list holds id 0x45."""
    d = C.iso_descriptor(TARGET.area + 4)
    if d is None:
        raise SystemExit("the user's disc image is required (export_area01_common.ISO_PATH)")
    if C.u32(d, 0x18) != 3:
        raise SystemExit(f'AREA00: {C.u32(d, 0x18)} nested blocks, not 3')
    block = d[C.NESTED_BASE + C.NESTED_SIZE * SUB:C.NESTED_BASE + C.NESTED_SIZE * (SUB + 1)]
    return 0x45 in [i for i, _o in A13._files(block)]


def install():
    TARGET.sub = SUB
    A13.SCRATCH = SCRATCH / 'scratch'
    A13.OUT = SCRATCH / 'tree' / 'area00' / f'part{SUB}'
    TARGET.out = A13.OUT / f'sub{SUB}'
    A13.capture_paths = lambda: list(PATHS)
    A13.configure(TARGET)
    LV.PREVIOUS['area00'] = PREVIOUS
    LV.CREATURES['area00'] = None
    LV.FLAG_OWNERS['area00'] = None
    LV.flag_calls = flag_calls
    # the two colour windows hold the last class-6 draw's tint, no load state
    # (export_area08_common.retained_colours, run over the capture)
    LV.E22.check_ctx_block = A08.check_ctx_block
    # AREA00's C casts the chain argument ((unsigned char *)D_overlay_AREA00_...),
    # which export_area13_tables' pattern does not take: the same pattern with
    # the cast allowed
    TB.C_CALL = re.compile(r'func_001BA1A0\s*\(\s*\w+\s*,\s*(?:\(\s*unsigned char\s*\*\s*\)\s*)?'
                           r'D_overlay_AREA(\d\d)_([0-9A-F]{8})\s*\)')
    TB.SCRIPT_ENTRIES['area00'] = tuple(sorted(TB.c_script_entries(TARGET)))
    TB.WRITERS['area00'] = ()
    TB.DOOR_ROW['area00'] = DOOR_ROW
    TB.DOOR_BEHAVIOURS = DOORS
    TB.EXPLICIT_MODELS = {}
    TB.PLACEMENTS['area00'], TB.GROUPS['area00'], TB.SPAWN_SUB0['area00'] = pins()
    LV.install('area00')
    return C


def place():
    part = SCRATCH / 'tree' / 'area00' / f'part{SUB}'
    out = ROOT / 'assets/area00' / f'sub{SUB}'
    if out.is_symlink() or (ROOT / 'assets/area00').is_symlink():
        raise SystemExit(f'{out} is a symlink: unlink it first')
    for p in sorted((part / f'sub{SUB}').rglob('*')):
        if p.is_file():
            dest = out / p.relative_to(part / f'sub{SUB}')
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(p, dest)
    shutil.copyfile(part / 'tables.json', out / 'tables.json')
    record = {p.relative_to(out).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(out.rglob('*')) if p.is_file() and p.name != 'sub2.json'}
    (out / 'sub2.json').write_text(json.dumps(record, indent=1) + '\n')
    print(f'assets/area00/sub2/: {len(record)} files')


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--kind', choices=('all', 'level', 'tables', 'sfx', 'place'), default='all')
    args = ap.parse_args(argv)
    if args.kind != 'place':
        el = LV.L.load_export_level()
        elf = C.read_elf()
        iso = C.ISO_PATH.resolve()
        install()
        A13.OUT.mkdir(parents=True, exist_ok=True)
        TARGET.out.mkdir(parents=True, exist_ok=True)
        if args.kind in ('all', 'level'):
            if dynamic():
                L15.export_dynamic(el, elf, A13.OUT, iso, False, 'area00')
            else:
                LV.export_target('area00', el, elf, A13.OUT, iso, False)
        if args.kind in ('all', 'tables'):
            install()
            TB.export_target('area00', A13.OUT)
        if args.kind in ('all', 'sfx'):
            install()
            SX.export_target('area00', A13.OUT)
    if args.kind in ('all', 'place'):
        place()
    return 0


if __name__ == '__main__':
    sys.exit(main())
