#!/usr/bin/env python3
"""Export the second half's areas: AREA14, AREA16 (subs 0, 1), AREA17,
AREA18, AREA20 and AREA21 (docs/SECOND_HALF_AREAS.md).

--kind static (no capture needed; the default until the route captures land)
  writes, per target, under assets/<target>/ (ignored, disc-derived, never
  committed): sub<s>/roster.emro (EMRO v1, the original's roster walk over the
  pinned ELF and overlay), spawn_table.emsp (the global spawn export carrying
  the target's sub-0 rows), door_destinations.emsp (D_0024E140 and the area's
  row), scripts.emsc (the chains' window; AREA18 has none), overlay_data.emsc
  (the overlay data section) and static.json (pins, layout, chains, doors,
  hashes). It checks, from the user's disc image, ELF and overlays: each
  overlay's pinned hash and MWo3 header, the INDEX.IDX layout each Target
  states (flat or nested, one nested block per sub), SUBS' `dynamic` against
  the relocation lists, the chain entries against the committed overlay C's
  001BA1A0 calls, and the scripts window against SCRIPT_WINDOWS.

--kind level|tables|sfx|place|all (the route captures)
  run the AREA13 lane's level, table and sound exports over
  export_second_half_common's targets, as export_area08_assets.py does for
  AREA08 and AREA07, then place each part under assets/<target>/. They refuse
  a (target, sub) whose SUBS row has no capture yet.

Usage (port root, macOS arm64, pure Python):
  python3 tools/export_second_half_assets.py [--only area16:1 ...] [--kind static|level|tables|sfx|place|all]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_second_half_common as A  # noqa: E402  (first)
import export_area15_level as L15  # noqa: E402

SIDE = ('spawn_table.emsp', 'door_destinations.emsp', 'scripts.emsc', 'overlay_data.emsc')
C, TB = A.C, A.TB


def out_dir(target):
    out = A.ROOT / 'assets' / target
    if out.is_symlink():
        raise SystemExit(f'{out} is a symlink: unlink it first')
    return out


def header_check(target, ov):
    t = A.TARGETS[target]
    ident, base, text, data, bss_size, bss, bss2 = struct.unpack_from('<7I', ov, 4)
    if ov[:4] != b'MWo3' or ident != t.overlay_id or base != C.OVERLAY_ARENA or bss != bss2 or \
            len(ov) != 0x40 + text + data or bss != base + len(ov):
        raise SystemExit(f'{target}: not the MWo3 header the state table pins')
    return dict(id=hex(ident), text=hex(text), data=hex(data), bss=hex(bss), bss_size=hex(bss_size))


def export_doors(target, elf, ov, placements):
    """export_area01_tables.export_doors without captures, with one more
    bound: AREA20's row 0x27560C is the highest D_0024E140 names, so no next
    row ends it; there it ends after the highest door id a door placement of
    the area uses (001BC150 reads row + 4 * (id & 0x7F) and nothing else)."""
    read = C.static_reader(elf, ov)
    D_0024E140 = TB.T.D_0024E140
    rows = [C.u32(read(D_0024E140 + 4 * k, 4), 0) for k in range(0x17)]
    row = rows[C.AREA]
    later = [p for p in set(rows) | {D_0024E140} if p > row]
    if later:
        return TB.T.export_doors(elf, ov, [], placements)
    ids = [rec[3] & 0x7F for sub in A.subs_of(target)
           for rec in TB.R.walk_roster(read, C.AREA, sub)[2] if C.u32(rec, 0x24) in A.DOORS[target]]
    if not ids:
        raise SystemExit(f'{target}: the last door row {row:#x} and no door placement bounds it')
    data = read(row, 4 * (max(ids) + 1))
    ptrs = read(D_0024E140, 4 * 0x17)
    ranges = sorted([(row, data), (D_0024E140, ptrs)])
    blob = struct.pack('<4sIII', b'EMSP', 1, len(ranges), 0)
    blob += b''.join(struct.pack('<II', a, len(d)) for a, d in ranges) + b''.join(d for _a, d in ranges)
    return blob, dict(row=hex(row), bytes=len(data), records=len(data) // 4, bound='the highest door id used',
                      windows=[[hex(a), len(d)] for a, d in ranges], sha256=C.sha(data))


def static_one(target, sub, elf):
    """The capture-free part of (target, sub): returns its receipt and the
    side files {name: bytes} (the roster goes to sub<s>/)."""
    A.install(target, sub, need_captures=False)
    t = A.TARGETS[target]
    ov = A.A13.read_overlay(t)
    rep = dict(target=target, sub=sub, overlay=header_check(target, ov), overlay_sha256=t.overlay_sha256)
    top, nested = A.layout(target)
    rep['layout'] = dict(chunk=t.chunk, nested_blocks=len(nested), flat=not nested,
                         top_sections=[C.u16(top, 0x0C), C.u16(top, 0x0E)], top_resident=hex(C.u32(top, 0x14)))
    dyn = A.dynamic_rule(target, sub)
    if dyn != A.SUBS[(target, sub)]['dynamic']:
        raise SystemExit(f'{target} sub {sub}: the relocation list says dynamic={dyn}, SUBS says otherwise')
    rep['dynamic'] = dyn
    read = C.static_reader(elf, ov)
    side = {}
    roster, (_groups, placements), rep['roster'] = TB.export_roster(read, [])
    spawn, rep['spawn'] = TB.export_spawn(elf, read, [])
    side['spawn_table.emsp'] = spawn
    doors, rep['doors'] = export_doors(target, elf, ov, placements)
    if int(rep['doors']['row'], 16) != A.DOOR_ROW[target]:
        raise SystemExit(f'{target}: door row {rep["doors"]["row"]}, not {A.DOOR_ROW[target]:#x}')
    row = bytes(read(A.DOOR_ROW[target], rep['doors']['bytes']))
    rep['doors']['used'] = TB.doors_used(placements, row)
    side['door_destinations.emsp'] = doors
    olo, ohi = TB.E02.overlay_data_window(ov)
    entries = set(A.SCRIPT_ENTRIES[target])
    from_c = TB.c_script_entries(t)
    if not from_c <= entries:
        raise SystemExit(f'{target}: the C starts {sorted(hex(e) for e in from_c - entries)} outside SCRIPT_ENTRIES')
    rep['scripts'] = dict(entries=[hex(e) for e in sorted(entries)],
                          not_in_c_001BA1A0_calls=[hex(e) for e in sorted(entries - from_c)])
    records = set()
    if entries:
        records = TB.chain_records(ov[olo - C.OVERLAY_ARENA:ohi - C.OVERLAY_ARENA], olo)
        slo, shi = TB.scripts_window(ov)
        if (slo, shi) != A.SCRIPT_WINDOWS[target]:
            raise SystemExit(f'{target}: scripts window {slo:#x}..{shi:#x}, not SCRIPT_WINDOWS')
        sdata, _changed = TB.window_export(ov, [], slo, shi, records)
        chains = {f'{e:08X}': len(TB.T.walk_chain(sdata, slo, e)) for e in sorted(entries)}
        rep['scripts'].update(window=[hex(slo), hex(shi)], chains=chains, distinct_records=len(records),
                              sha256=C.sha(sdata))
        side['scripts.emsc'] = C.emsc(slo, sdata)
    elif A.SCRIPT_WINDOWS[target] is not None:
        raise SystemExit(f'{target}: no chains, but SCRIPT_WINDOWS names a window')
    odata, _changed = TB.window_export(ov, [], olo, ohi, records)
    rep['overlay_data'] = dict(window=[hex(olo), hex(ohi)], sha256=C.sha(odata))
    side['overlay_data.emsc'] = C.emsc(olo, odata)
    return rep, roster, side


def export_static(todo):
    elf = C.read_elf()
    for target in sorted({t for t, _s in todo}):
        out = out_dir(target)
        reports, first = [], None
        for sub in sorted(s for t, s in todo if t == target):
            rep, roster, side = static_one(target, sub, elf)
            if first is None:
                first = side
            elif any(side[n] != first[n] for n in side):
                raise SystemExit(f'{target}: the side files differ between the subs')
            (out / f'sub{sub}').mkdir(parents=True, exist_ok=True)
            (out / f'sub{sub}' / 'roster.emro').write_bytes(roster)
            reports.append(rep)
        for name, blob in first.items():
            (out / name).write_bytes(blob)
        files = {p.relative_to(out).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in sorted(out.rglob('*')) if p.is_file() and p.suffix in ('.emro', '.emsp', '.emsc')}
        (out / 'static.json').write_text(json.dumps(dict(subs=reports, files=files), indent=1) + '\n')
        r = reports[0]
        print(f"assets/{target}/: static, subs {[x['sub'] for x in reports]}; {r['layout']['chunk']} "
              f"{'flat' if r['layout']['flat'] else str(r['layout']['nested_blocks']) + ' nested'}; roster "
              f"{[x['roster']['placements'] for x in reports]} placements; door row {r['doors']['row']} "
              f"x{r['doors']['records']}, used {[(d['placement'], d['door_id']) for d in r['doors']['used']]}; "
              f"{len(r['scripts']['entries'])} chains {r['scripts'].get('window', '')}")


def export_one(target, sub, kind, el, elf, iso):
    A.install(target, sub)
    A.A13.OUT.mkdir(parents=True, exist_ok=True)
    A.TARGETS[target].out.mkdir(parents=True, exist_ok=True)
    if kind in ('all', 'level'):
        if A.SUBS[(target, sub)]['dynamic']:
            L15.export_dynamic(el, elf, A.A13.OUT, iso, False, target)
        else:
            A.LV.export_target(target, el, elf, A.A13.OUT, iso, False)
    if kind in ('all', 'tables'):
        A.install(target, sub)
        A.TB.export_target(target, A.A13.OUT)
    if kind in ('all', 'sfx'):
        A.install(target, sub)
        A.SX.export_target(target, A.A13.OUT)


def place(target):
    tree = A.SCRATCH / 'tree' / target
    out = out_dir(target)
    subs = A.subs_of(target)
    first = None
    for s in subs:
        part = tree / f'part{s}'
        if first is None:
            first = part
            out.mkdir(parents=True, exist_ok=True)
            for n in SIDE:
                if (part / n).exists():
                    shutil.copyfile(part / n, out / n)
        else:
            differ = [n for n in SIDE if (part / n).exists() and (part / n).read_bytes() != (first / n).read_bytes()]
            if differ:
                raise SystemExit(f'{target}: the side files {differ} differ between the subs')
        for p in sorted((part / f'sub{s}').rglob('*')):
            if p.is_file():
                dest = out / f'sub{s}' / p.relative_to(part / f'sub{s}')
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(p, dest)
        shutil.copyfile(part / 'tables.json', out / f'sub{s}' / 'tables.json')
    record = {p.relative_to(out).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(out.rglob('*')) if p.is_file() and p.name != f'{target}.json'}
    (out / f'{target}.json').write_text(json.dumps(record, indent=1) + '\n')
    print(f'assets/{target}/: {len(record)} files, subs {subs}')


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--only', action='append', help='target:sub, default every sub')
    ap.add_argument('--kind', choices=('static', 'all', 'level', 'tables', 'sfx', 'place'), default='static')
    args = ap.parse_args(argv)
    todo = [(o.split(':')[0], int(o.split(':')[1])) for o in args.only] if args.only else list(A.SUBS)
    for t, s in todo:
        if (t, s) not in A.SUBS:
            raise SystemExit(f'{t}:{s} is not a second-half sub')
    if args.kind == 'static':
        export_static(todo)
        return 0
    if args.kind != 'place':
        el = A.LV.L.load_export_level()
        elf = C.read_elf()
        for target, sub in todo:
            export_one(target, sub, args.kind, el, elf, C.ISO_PATH.resolve())
    if args.kind in ('all', 'place'):
        for target in sorted({t for t, _s in todo}):
            place(target)
    return 0


if __name__ == '__main__':
    sys.exit(main())
