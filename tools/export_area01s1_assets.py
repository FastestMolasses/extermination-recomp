#!/usr/bin/env python3
"""Export AREA01 sub 1 (docs/AREA01_ASSETS.md, its section on sub 1): the room Claude's
guided run reaches by AREA11's exit fan (take g49), crosses to AREA02 (g50),
comes back to from AREA04 sub 1 through AREA22 (g58) and leaves into AREA06
sub 1 (g59). The g11 chain's area change at g49 row 3148 and the g16 chain's
card g59 (g58's end) load it.

AREA01 sub 0 (chunk05.n0) is export_area01_*'s. Sub 1 (chunk05.n1, the other
nested block of the same top block chunk05) takes the AREA13 lane's
per-(target, sub) exporters, imported and run unchanged, as AREA06 sub 1
(export_area06s1_assets) and AREA00 sub 2 (export_area00s2_assets) do: a
Target for AREA01 (area 1, OVERLAY/AREA01.BIN = MWo3 id 2, INDEX chunk05,
nested), pointed at sub 1 by `install`.

  captures  g58_area04_area01 (g58's end: entry 6 after the real load from
            AREA22, area bytes 01 01 06 01) of the guided route set
            guided-20261117/s87/route_g49 (tools/capture_from_take.py);
            --cross-check reruns everything over g49_area11_area01 (g49's
            end: entry 4 after the real load from AREA11, 01 01 04 01),
            whose load sits 0x15C0 lower (see PATHS)
  previous  g57_area19_area04 (g57's end, AREA04 sub 1: the state g58's
            load ran from; only a report row: sub 1's nested list relocates
            id 0x45, so D_0028A5A4 is not stale)

001A2370 / 0019C6F0 rules, from AREA01's byte-identical overlay C (decomp
src/overlays/AREA01) and export_area01_level.owner_matrix (sub 0's reading
of the same owners): the matrix-address callers among sub 1's owners are
0x8261A0 (its states 0x826200 / 0x826440, func_overlay_AREA01_008261C0.c /
_00826400.c: self + 0xD0; sub 1's placements [17] and [18]) and 0x826D40
(func_overlay_AREA01_00826D00.c, four call sites: *(D_00275B40 + 0xC) +
0x90, so *(node + 0x11C) + 0x90 while it runs; group 0x829440), beside the
exporter's own pickup and drum rules. Only 0x825950
(func_overlay_AREA01_00825910.c, sub 0's group 0x828A00) calls 0019C6F0,
so no sub-1 owner decides a uid word: no flag call.

Output (ignored, disc-derived, never committed): assets/area01/sub1/ (the
level, collision, cells, roster, world models, messages, sound, tables.json,
sub1.json). The side files of this run stay under
build/area01s1/assets/tree/ only: AREA01's spawn table, door rows, scripts
and overlay data stay export_area01_tables' (assets/area01/), as sub 0's.

Usage (port root, macOS arm64, pure Python):
  python3 tools/export_area01s1_assets.py [--kind level|tables|sfx|place] [--cross-check]
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
import export_area01_tables as T01  # noqa: E402  (AREA01's fourteen script entries)
import export_second_half_common as SH  # noqa: E402  (the fitted attachment's group 3 bank 0)

C = A13.C
ROOT = A13.ROOT
SET = A13.DECOMP / 'build/fork_refs/guided-20261117/s87/route_g49'
SCRATCH = ROOT / 'build/area01s1/assets'
# export_area01_common's AREA01.BIN (sha256 of the user's extract; a hash, not disc data)
AREA01_SHA256 = '91269b993bba3a98c6608bc9013a2e2a177d92c2202869489f0de6fd1776f63a'
SUB = 1
TARGET = A13.Target('area01', 1, SUB, 'AREA01.BIN', 0x9800, 2, AREA01_SHA256, 'chunk05', True,
                    ROOT / 'assets/area01/sub1', 'AREA01')
A13.TARGETS['area01'] = TARGET
# The two loads sit 0x15C0 apart (D_0028A73C 0x1349D00 after AREA11, 0x134B2C0
# after AREA22: what the earlier areas left resident), so no one load map fits
# both. The export runs over g58's end; `--cross-check` runs it over g49's
# end alone and requires every file the port reads to be byte-identical
# (only the raw address-bearing banks static_bank.emsc / dynamic_objects.emsc,
# which src/ does not read, and the reports may differ).
PATHS = [SET / 'g58_area04_area01']
PREVIOUS = SET / 'g57_area19_area04'
OTHER = [SET / 'g49_area11_area01']
ADDRESS_BEARING = ('level/static_bank.emsc', 'level/dynamic_objects.emsc')
# EMWM's header word +8 is the table address the export saw; the port seats the
# bank at the loader's D_0028A59C (em_area11_boxes.c: the span is position-
# independent), so the cross-check masks that one word
MASKED = {'world_models.emwm': (8, 12)}
DOORS = (0x1BC350, 0x1BB860)            # sub 1's door behaviours (placements 0x82C5F0)
# the two matrix-address callers among sub 1's overlay owners (module docstring)
SELF_D0, CREATURE = 0x8261A0, 0x826D40
_LV_OWNER_MATRIX = LV.owner_matrix


def owner_matrix(ram, node):
    """export_area13_level.owner_matrix (the pickup, the drum, the target's
    creature 0x826D40) plus 0x8261A0's self + 0xD0 (module docstring)."""
    if C.u32(ram, node + 0x10) == SELF_D0:
        return node + 0xD0
    return _LV_OWNER_MATRIX(ram, node)


def flag_calls(ram):
    """No sub-1 owner calls 0019C6F0 (module docstring)."""
    return ()


def pins():
    read = C.static_reader(C.read_elf(), A13.read_overlay(TARGET))
    groups, paddr, _placements = TB.R.walk_roster(read, TARGET.area, SUB)
    _table, entries, count = TB.spawn_rows(read)
    door_row = C.u32(bytes(read(T01.D_0024E140 + 4 * TARGET.area, 4)), 0)   # D_0024E140[1]
    return paddr, tuple((a, len(r)) for a, r in groups), (entries, count), door_row


def dynamic():
    """True when sub 1's nested relocation list holds id 0x45."""
    d = C.iso_descriptor(TARGET.area + 4)
    if d is None:
        raise SystemExit("the user's disc image is required (export_area01_common.ISO_PATH)")
    if C.u32(d, 0x18) != 2:
        raise SystemExit(f'AREA01: {C.u32(d, 0x18)} nested blocks, not 2')
    block = d[C.NESTED_BASE + C.NESTED_SIZE * SUB:C.NESTED_BASE + C.NESTED_SIZE * (SUB + 1)]
    return 0x45 in [i for i, _o in A13._files(block)]


def install():
    TARGET.sub = SUB
    A13.SCRATCH = SCRATCH / 'scratch'
    A13.OUT = SCRATCH / 'tree' / 'area01' / f'part{SUB}'
    TARGET.out = A13.OUT / f'sub{SUB}'
    A13.capture_paths = lambda: list(PATHS)
    A13.configure(TARGET)
    LV.PREVIOUS['area01'] = PREVIOUS
    LV.CREATURES['area01'] = CREATURE
    LV.FLAG_OWNERS['area01'] = None
    LV.owner_matrix = owner_matrix      # LV.install hands it to export_area01_level by name
    LV.flag_calls = flag_calls
    # the two colour windows hold the last class-6 draw's tint, no load state
    # (export_area08_common.retained_colours, run over the capture)
    LV.E22.check_ctx_block = A08.check_ctx_block
    # AREA01's C passes a third argument to some chain starts
    # (func_001BA1A0(talk, D_overlay_AREA01_00829E60, 2)), which
    # export_area13_tables' pattern does not take: the same pattern with it
    TB.C_CALL = re.compile(r'func_001BA1A0\s*\(\s*\w+\s*,\s*D_overlay_AREA(\d\d)_([0-9A-F]{8})\s*(?:,\s*\w+\s*)?\)')
    TB.SCRIPT_ENTRIES['area01'] = tuple(T01.SCRIPT_ENTRIES)
    TB.WRITERS['area01'] = ()
    TB.DOOR_BEHAVIOURS = DOORS
    TB.EXPLICIT_MODELS = {}
    TB.PLACEMENTS['area01'], TB.GROUPS['area01'], TB.SPAWN_SUB0['area01'], TB.DOOR_ROW['area01'] = pins()
    LV.install('area01')
    # group 3 bank 0 holds the fitted attachment's bank when the player wears
    # one (the guided run after AREA17): the attachment scope holds it, as in
    # the second half's areas
    if not getattr(SX.S.bindings_from_captures, 'attachments', False):
        SX.S.bindings_from_captures, SX.E04.refusal_reasons = SH._attachment_aware(SX.S.bindings_from_captures,
                                                                                   SX.E04.refusal_reasons)
        SX.S.bindings_from_captures.attachments = True
    return C


def place():
    part = SCRATCH / 'tree' / 'area01' / f'part{SUB}'
    out = ROOT / 'assets/area01' / f'sub{SUB}'
    if out.is_symlink() or (ROOT / 'assets/area01').is_symlink():
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
    print(f'assets/area01/sub{SUB}/: {len(record)} files')


def _masked(name, data):
    if name in MASKED:
        lo, hi = MASKED[name]
        return data[:lo] + bytes(hi - lo) + data[hi:]
    return data


def cross_check(kinds):
    """Run `kinds` over OTHER alone (g49's end, the load from AREA11) into
    build/area01s1/cross and compare with the main run's tree: every file
    but the reports (*.json) and ADDRESS_BEARING must be byte-identical."""
    global PATHS, SCRATCH
    main_part = SCRATCH / 'tree' / 'area01' / f'part{SUB}' / f'sub{SUB}'
    saved = PATHS, SCRATCH
    PATHS, SCRATCH = OTHER, ROOT / 'build/area01s1/cross'
    try:
        run(kinds)
    finally:
        other_part = SCRATCH / 'tree' / 'area01' / f'part{SUB}' / f'sub{SUB}'
        PATHS, SCRATCH = saved
        install()
    names = sorted(p.relative_to(main_part).as_posix() for p in main_part.rglob('*') if p.is_file())
    other = sorted(p.relative_to(other_part).as_posix() for p in other_part.rglob('*') if p.is_file())
    if names != other:
        raise SystemExit(f'cross-check: the two runs wrote different files: {sorted(set(names) ^ set(other))}')
    same, skipped, bad = 0, 0, []
    for n in names:
        if n.endswith('.json') or n in ADDRESS_BEARING:
            skipped += 1
        elif _masked(n, (main_part / n).read_bytes()) == _masked(n, (other_part / n).read_bytes()):
            same += 1
        else:
            bad.append(n)
    if bad:
        raise SystemExit(f'cross-check: {OTHER[0].name} gives different bytes for {bad}')
    print(f'cross-check {OTHER[0].name}: {same} files byte-identical, {skipped} reports / address-bearing banks skipped')


def run(kinds):
    el = LV.L.load_export_level()
    elf = C.read_elf()
    iso = C.ISO_PATH.resolve()
    install()
    A13.OUT.mkdir(parents=True, exist_ok=True)
    TARGET.out.mkdir(parents=True, exist_ok=True)
    if 'level' in kinds:
        if dynamic():
            L15.export_dynamic(el, elf, A13.OUT, iso, False, 'area01')
        else:
            LV.export_target('area01', el, elf, A13.OUT, iso, False)
    if 'tables' in kinds:
        install()
        TB.export_target('area01', A13.OUT)
    if 'sfx' in kinds:
        install()
        SX.export_target('area01', A13.OUT)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--kind', choices=('all', 'level', 'tables', 'sfx', 'place'), default='all')
    ap.add_argument('--cross-check', action='store_true',
                    help="also run over g49's end alone and require the same bytes (cross_check)")
    args = ap.parse_args(argv)
    if args.kind != 'place':
        kinds = ('level', 'tables', 'sfx') if args.kind == 'all' else (args.kind,)
        run(kinds)
        if args.cross_check:
            cross_check(kinds)
    if args.kind in ('all', 'place'):
        place()
    return 0


if __name__ == '__main__':
    sys.exit(main())
