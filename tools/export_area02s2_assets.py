#!/usr/bin/env python3
"""Export AREA02 sub 2 (docs/AREA02_ASSETS.md, its section on sub 2): the
south tunnel and its rails room, the state AREA02 loads once the car has
stopped. Claude's guided run reaches it by the duct from AREA02 sub 1 (take
g51 row 3333, area bytes 02 02 05) and leaves it by door [25] into AREA04
sub 1 entry 0 (g51 row 5546); the g11 chain's duct load at g51 row 3333
needs it.

AREA02 sub 0 / sub 1 (chunk06.n0 / n1) are export_area02_*'s. Sub 2
(chunk06.n2, placements 0x8283D0) takes the AREA13 lane's per-(target, sub)
exporters, imported and run unchanged, as AREA01 sub 1
(export_area01s1_assets) and AREA06 sub 1 (export_area06s1_assets) do: a
Target for AREA02 (area 2, OVERLAY/AREA02.BIN = MWo3 id 3, INDEX chunk06,
nested), pointed at sub 2 by `install`.

  captures  a04e_01_door41 (AREA04 sub 1's slider [41] -> AREA02 sub 2
            entry 4, after the real load, area bytes 02 02 04 02) and
            a02c_00_seal (the same load later: the seal [21] broken) of the
            fork route sets levels-recordings-20261102-1/s87/route_a04e and
            route_a02c; one load map fits both (D_0028A740 0x13E38C0 in both)
  previous  a04e_00_hall (AREA04 sub 1: the state a04e_01's load ran from)

No take of the guided run ends in sub 2 (g51 passes through it), so the
duct's own load (from AREA02 sub 1; g50's end has D_0028A73C 0x1349D00, the
AREA04 load 0x133C1C0) has no capture: the export is the AREA04 load's.
AREA01 sub 1's two loads (export_area01s1_assets --cross-check) showed
every file the port reads is byte-identical across load bases but the raw
address-bearing banks and the EMWM header address.

001A2370 / 0019C6F0 rules, from AREA02's byte-identical overlay C (decomp
src/overlays/AREA02; export_area02_level's reading of the same owners):
sub 2's only overlay owner is the turntable 0x825100
(func_overlay_AREA02_008250C0, placements [27] and [28]), which passes
self + 0xD0 to 001A2370; the pickup 0x219550 (deferred group 0x826080 [0])
is the exporter's own node + 0xD0 rule. The 0019C6F0 callers (0x823D70,
0x823930's kinds 8 / 10 / 11, the switch 0x823980) are sub 0's placements
only (decomp tools/area_overview.py --area 2: owner_subs), so no sub-2
owner decides a uid word: no flag call.

Output (ignored, disc-derived, never committed): assets/area02/sub2/ (the
level, collision, cells, roster, world models, messages, sound, tables.json,
sub2.json). The side files of this run stay under
build/area02s2/assets/tree/ only: AREA02's spawn table, door rows, scripts
and overlay data stay export_area02_tables' (assets/area02/), as sub 0's.

Usage (port root, macOS arm64, pure Python):
  python3 tools/export_area02s2_assets.py [--kind level|tables|sfx|place]
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
import export_area01_tables as T01  # noqa: E402  (D_0024E140)
import export_second_half_common as SH  # noqa: E402  (the fitted attachment's group 3 bank 0)

C = A13.C
ROOT = A13.ROOT
SETS = A13.DECOMP / 'build/fork_refs/levels-recordings-20261102-1/s87'
SCRATCH = ROOT / 'build/area02s2/assets'
# export_area02_common.OVERLAY_SHA256 (a hash, not disc data)
AREA02_SHA256 = '10740d88df2a328c7e966a94525e9ddc1dbbb35f4d6b08aa46e629dbf6587f93'
SUB = 2
TARGET = A13.Target('area02', 2, SUB, 'AREA02.BIN', 0x5D80, 3, AREA02_SHA256, 'chunk06', True,
                    ROOT / 'assets/area02/sub2', 'AREA02')
A13.TARGETS['area02'] = TARGET
PATHS = [SETS / 'route_a04e/a04e_01_door41', SETS / 'route_a02c/a02c_00_seal']
PREVIOUS = SETS / 'route_a04e/a04e_00_hall'
DOORS = (0x1BC350, 0x1BB860)            # sub 2's door behaviours ([22], [25])
TURNTABLE = 0x825100                    # placements [27], [28] (module docstring)
_LV_OWNER_MATRIX = LV.owner_matrix


def owner_matrix(ram, node):
    """export_area13_level.owner_matrix (the pickup, the drum) plus the
    turntable 0x825100's self + 0xD0 (module docstring)."""
    if C.u32(ram, node + 0x10) == TURNTABLE:
        return node + 0xD0
    return _LV_OWNER_MATRIX(ram, node)


def flag_calls(ram):
    """No sub-2 owner calls 0019C6F0 (module docstring)."""
    return ()


def pins():
    read = C.static_reader(C.read_elf(), A13.read_overlay(TARGET))
    groups, paddr, _placements = TB.R.walk_roster(read, TARGET.area, SUB)
    _table, entries, count = TB.spawn_rows(read)
    door_row = C.u32(bytes(read(T01.D_0024E140 + 4 * TARGET.area, 4)), 0)   # D_0024E140[2]
    return paddr, tuple((a, len(r)) for a, r in groups), (entries, count), door_row


def dynamic():
    """True when sub 2's nested relocation list holds id 0x45."""
    d = C.iso_descriptor(TARGET.area + 4)
    if d is None:
        raise SystemExit("the user's disc image is required (export_area01_common.ISO_PATH)")
    if C.u32(d, 0x18) != 3:
        raise SystemExit(f'AREA02: {C.u32(d, 0x18)} nested blocks, not 3')
    block = d[C.NESTED_BASE + C.NESTED_SIZE * SUB:C.NESTED_BASE + C.NESTED_SIZE * (SUB + 1)]
    return 0x45 in [i for i, _o in A13._files(block)]


def install():
    TARGET.sub = SUB
    A13.SCRATCH = SCRATCH / 'scratch'
    A13.OUT = SCRATCH / 'tree' / 'area02' / f'part{SUB}'
    TARGET.out = A13.OUT / f'sub{SUB}'
    A13.capture_paths = lambda: list(PATHS)
    A13.configure(TARGET)
    LV.PREVIOUS['area02'] = PREVIOUS
    LV.CREATURES['area02'] = None
    LV.FLAG_OWNERS['area02'] = None
    LV.owner_matrix = owner_matrix      # LV.install hands it to export_area01_level by name
    LV.flag_calls = flag_calls
    # the two colour windows hold the last class-6 draw's tint, no load state
    # (export_area08_common.retained_colours, run over the capture)
    LV.E22.check_ctx_block = A08.check_ctx_block
    # AREA02's C casts the chain argument (func_001BA1A0(talk, (unsigned
    # char *)D_overlay_AREA02_…)), which export_area13_tables' pattern does
    # not take: the same pattern with the cast (twelve entries, as
    # export_area02_tables.SCRIPT_ENTRIES)
    TB.C_CALL = re.compile(r'func_001BA1A0\s*\(\s*\w+\s*,\s*(?:\(\s*unsigned\s+char\s*\*\s*\))?\s*'
                           r'D_overlay_AREA(\d\d)_([0-9A-F]{8})\s*\)')
    TB.SCRIPT_ENTRIES['area02'] = tuple(sorted(TB.c_script_entries(TARGET)))
    TB.WRITERS['area02'] = ()
    # sub 2's nested list holds no id 0x41: no area message bank, as sub 1
    # (AREA02_ASSETS.md finding 2); *D_0028A594 keeps the previous load's word
    TB.NO_AREA_BANK['area02'] = LV.previous_word('area02', LV.D_0028A594)
    TB.DOOR_BEHAVIOURS = DOORS
    TB.EXPLICIT_MODELS = {}
    TB.PLACEMENTS['area02'], TB.GROUPS['area02'], TB.SPAWN_SUB0['area02'], TB.DOOR_ROW['area02'] = pins()
    LV.install('area02')
    if not getattr(SX.S.bindings_from_captures, 'attachments', False):
        SX.S.bindings_from_captures, SX.E04.refusal_reasons = SH._attachment_aware(SX.S.bindings_from_captures,
                                                                                   SX.E04.refusal_reasons)
        SX.S.bindings_from_captures.attachments = True
    return C


def place():
    part = SCRATCH / 'tree' / 'area02' / f'part{SUB}'
    out = ROOT / 'assets/area02' / f'sub{SUB}'
    if out.is_symlink() or (ROOT / 'assets/area02').is_symlink():
        raise SystemExit(f'{out} is a symlink: unlink it first')
    for p in sorted((part / f'sub{SUB}').rglob('*')):
        if p.is_file():
            dest = out / p.relative_to(part / f'sub{SUB}')
            if dest.is_symlink():
                raise SystemExit(f'{dest} is a symlink: unlink it first')
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(p, dest)
    shutil.copyfile(part / 'tables.json', out / 'tables.json')
    record = {p.relative_to(out).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(out.rglob('*')) if p.is_file() and p.name != f'sub{SUB}.json'}
    (out / f'sub{SUB}.json').write_text(json.dumps(record, indent=1) + '\n')
    print(f'assets/area02/sub{SUB}/: {len(record)} files')


def run(kinds):
    el = LV.L.load_export_level()
    elf = C.read_elf()
    iso = C.ISO_PATH.resolve()
    install()
    A13.OUT.mkdir(parents=True, exist_ok=True)
    TARGET.out.mkdir(parents=True, exist_ok=True)
    if 'level' in kinds:
        if dynamic():
            L15.export_dynamic(el, elf, A13.OUT, iso, False, 'area02')
        else:
            LV.export_target('area02', el, elf, A13.OUT, iso, False)
    if 'tables' in kinds:
        install()
        TB.export_target('area02', A13.OUT)
    if 'sfx' in kinds:
        install()
        SX.export_target('area02', A13.OUT)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--kind', choices=('all', 'level', 'tables', 'sfx', 'place'), default='all')
    args = ap.parse_args(argv)
    if args.kind != 'place':
        run(('level', 'tables', 'sfx') if args.kind == 'all' else (args.kind,))
    if args.kind in ('all', 'place'):
        place()
    return 0


if __name__ == '__main__':
    sys.exit(main())
