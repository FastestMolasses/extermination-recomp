#!/usr/bin/env python3
"""Export AREA06 sub 1 (docs/AREA06_ASSETS.md "Sub 1"): the room Claude's
guided run reaches from AREA01 (take g59) and leaves into AREA16 (g60), and
returns to from AREA16 (g67). The g16 chain's card g60 (g59's end) loads it.

AREA06 sub 0 (chunk10.n0) is export_area06_*'s. Sub 1 (chunk10.n1) takes the
AREA13 lane's per-(target, sub) exporters, imported and run unchanged, as
AREA00 sub 2 does (export_area00s2_assets) and AREA08's and AREA07's subs do
(export_area08_common): a Target for AREA06 (area 6, OVERLAY/AREA06.BIN =
MWo3 id 6, INDEX chunk10, nested), pointed at sub 1 by `install`.

  captures  g59_area01_area06 (g59's end: AREA06 sub 1 in play, area bytes
            06 01, after the real load from AREA01 sub 1) of the guided
            route set guided-20261117/s87/route_g16) and g67_area16_out
            (g67's end: AREA16 -> AREA06 sub 1, area bytes 06 01); one load
            map fits both
  previous  g58_area04_area01 (g58's end, AREA01 sub 1: the state g59's load
            ran from; route set guided-20261117/s87/route_g16pre, built by
            tools/capture_from_take.py)

001A2370 / 0019C6F0 rules, from AREA06's byte-identical overlay C (decomp
src/overlays/AREA06): only the beam 0x824560 (func_overlay_AREA06_00824520,
sub 0's) calls 001A2370, and only [9] 0x823580
(overlay_AREA06_func_00823540) calls 0019C6F0. Sub 1's overlay owners are
0x823580, 0x823B50 and the director 0x825E20 (placements 0x8283D0; decomp
tools/area_overview.py --area 6), so sub 1's 0019C6F0 rule is sub 0's [9]
rule (export_area06_level.flag_calls) for sub 1, and no sub-1 overlay owner
is a matrix-address caller (the pickup, the drum and the 0x219870 node are
the exporter's own boot rules).

Output (ignored, disc-derived, never committed): assets/area06/sub1/ (the
level, collision, cells, roster, world models, messages, sound, tables.json,
sub1.json). Group 3 bank 0 is the fitted attachment's bank in these
captures (export_second_half_common._attachment_aware: the attachment
scope holds it). The side files of this run stay under
build/area06s1/assets/tree/ only: AREA06's spawn table, door rows, scripts
and overlay data stay export_area06_tables' (assets/area06/), as sub 0's.

Usage (port root, macOS arm64, pure Python):
  python3 tools/export_area06s1_assets.py [--kind level|tables|sfx|place]
"""
from __future__ import annotations

import argparse
import hashlib
import json
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
import export_area06_level as E06  # noqa: E402  ([9]'s 0019C6F0 rule)
import export_second_half_common as SH  # noqa: E402  (the fitted attachment's group 3 bank 0)

C = A13.C
ROOT = A13.ROOT
SET = A13.DECOMP / 'build/fork_refs/guided-20261117/s87/route_g16'
SET_PRE = A13.DECOMP / 'build/fork_refs/guided-20261117/s87/route_g16pre'
SCRATCH = ROOT / 'build/area06s1/assets'
# export_area06_common.OVERLAY_SHA256 (a hash, not disc data)
AREA06_SHA256 = 'eb5bd82c38ad27084dd67bd8ed5b9f42830b6f13d57c1d1fe04f73097dadc997'
SUB = 1
TARGET = A13.Target('area06', 6, SUB, 'AREA06.BIN', 0x5A00, 6, AREA06_SHA256, 'chunk10', True,
                    ROOT / 'assets/area06/sub1', 'AREA06')
A13.TARGETS['area06'] = TARGET
PATHS = [SET / 'g59_area01_area06', SET / 'g67_area16_out']
PREVIOUS = SET_PRE / 'g58_area04_area01'
DOORS = (0x1BC350, 0x1BB860)            # export_area06_tables.DOOR_BEHAVIOURS
DOOR_ROW = 0x24E028                     # export_area06_tables.DOOR_ROW (D_0024E140[6])


def flag_calls(ram):
    """[9] 0x823580's 0019C6F0 rule (export_area06_level.flag_calls) for
    sub 1: a live [9] in state 1 made (0xD, +5) last. Outside AREA06 sub 1:
    no call."""
    if ram[0x810700] != TARGET.area or ram[0x810701] != SUB:
        return ()
    nodes = [a for _s, a in LV.E02.pool(ram) if C.u32(ram, a + 0x10) == E06.FLAG_OWNER]
    if len(nodes) != 1 or ram[nodes[0] + 4] != 1 or ram[nodes[0] + 5] not in (0, 1):
        raise ValueError(f'[9] (0x823580) is not one live node in state 1 with sub-state 0 or 1: '
                         f'{[(hex(a), ram[a + 4], ram[a + 5]) for a in nodes]}')
    return ((E06.FLAG_KEY, ram[nodes[0] + 5]),)


def pins():
    read = C.static_reader(C.read_elf(), A13.read_overlay(TARGET))
    groups, paddr, _placements = TB.R.walk_roster(read, TARGET.area, SUB)
    _table, entries, count = TB.spawn_rows(read)
    return paddr, tuple((a, len(r)) for a, r in groups), (entries, count)


def dynamic():
    """True when sub 1's nested relocation list holds id 0x45."""
    d = C.iso_descriptor(TARGET.area + 4)
    if d is None:
        raise SystemExit("the user's disc image is required (export_area01_common.ISO_PATH)")
    if C.u32(d, 0x18) != 2:
        raise SystemExit(f'AREA06: {C.u32(d, 0x18)} nested blocks, not 2')
    block = d[C.NESTED_BASE + C.NESTED_SIZE * SUB:C.NESTED_BASE + C.NESTED_SIZE * (SUB + 1)]
    return 0x45 in [i for i, _o in A13._files(block)]


def install():
    TARGET.sub = SUB
    A13.SCRATCH = SCRATCH / 'scratch'
    A13.OUT = SCRATCH / 'tree' / 'area06' / f'part{SUB}'
    TARGET.out = A13.OUT / f'sub{SUB}'
    A13.capture_paths = lambda: list(PATHS)
    A13.configure(TARGET)
    LV.PREVIOUS['area06'] = PREVIOUS
    LV.CREATURES['area06'] = None
    LV.FLAG_OWNERS['area06'] = E06.FLAG_OWNER
    LV.flag_calls = flag_calls
    # the two colour windows hold the last class-6 draw's tint, no load state
    # (export_area08_common.retained_colours, run over the capture)
    LV.E22.check_ctx_block = A08.check_ctx_block
    TB.SCRIPT_ENTRIES['area06'] = tuple(sorted(TB.c_script_entries(TARGET)))
    TB.WRITERS['area06'] = ()
    TB.DOOR_ROW['area06'] = DOOR_ROW
    TB.DOOR_BEHAVIOURS = DOORS
    TB.EXPLICIT_MODELS = {}
    TB.PLACEMENTS['area06'], TB.GROUPS['area06'], TB.SPAWN_SUB0['area06'] = pins()
    LV.install('area06')
    # group 3 bank 0 holds the fitted attachment's bank in the guided run's
    # captures (g59: global row 3's twin, from chunk50..53): the attachment
    # scope holds it, as in the second half's areas
    if not getattr(SX.S.bindings_from_captures, 'attachments', False):
        SX.S.bindings_from_captures, SX.E04.refusal_reasons = SH._attachment_aware(SX.S.bindings_from_captures,
                                                                                   SX.E04.refusal_reasons)
        SX.S.bindings_from_captures.attachments = True
    return C


def place():
    part = SCRATCH / 'tree' / 'area06' / f'part{SUB}'
    out = ROOT / 'assets/area06' / f'sub{SUB}'
    if out.is_symlink() or (ROOT / 'assets/area06').is_symlink():
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
    print(f'assets/area06/sub{SUB}/: {len(record)} files')


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
                L15.export_dynamic(el, elf, A13.OUT, iso, False, 'area06')
            else:
                LV.export_target('area06', el, elf, A13.OUT, iso, False)
        if args.kind in ('all', 'tables'):
            install()
            TB.export_target('area06', A13.OUT)
        if args.kind in ('all', 'sfx'):
            install()
            SX.export_target('area06', A13.OUT)
    if args.kind in ('all', 'place'):
        place()
    return 0


if __name__ == '__main__':
    sys.exit(main())
