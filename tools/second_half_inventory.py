#!/usr/bin/env python3
"""Generate the checklist tables of docs/SECOND_HALF_AREAS.md.

Inputs: the decomp's static area census (`tools/area_overview.py --area N`,
run in ../Extermination; it reads the user's ELF and overlays and writes the
ignored build/s87/areaNN/overview.json, or the folder --overviews names),
the static export's receipts (assets/areaNN/static.json, from
tools/export_second_half_assets.py --kind static) and this repo's src/.
Output: the Markdown between the "second_half:<name>" markers of the doc
(--doc rewrites them in place, --check compares). Addresses and counts only:
no original bytes, no instruction text.

"Port" gives a boot address's rows in the first-level census ledgers
(FIRST/SECOND/THIRD_LEVEL_CENSUS.md: live, verified-unbound, ...), and the
files that DEFINE a function whose name ends in the address (a body, not a
comment, a prototype or a literal; tools/level_readiness.py's reader and
its reviewed aliases). A definition is the port's claim of a translation;
its oracle is named in that module's doc, not here. Owners bound through
macro tables (e.g. em_area02_math.c's VOID1 rows) and not named in a ledger
show as missing: an area lane confirms each one. Overlay addresses mean another
function in every area, so an overlay behaviour of these areas counts as
translated only through a port file of its own area: a body of
em_areaNN_overlay_<address> in src/game/em_areaNN_overlay*.c (own_overlay).

Usage (port root): python3 tools/second_half_inventory.py [--overviews DIR] [--doc docs/SECOND_HALF_AREAS.md | --check ...]
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
AREAS = (14, 16, 17, 18, 20, 21)
ARENA_LO, ARENA_HI = 0x823500, 0x980000


def port_index():
    """({address: [file]} of boot definitions, {address: [(level, status, doc)]} of
    the census ledgers), by tools/level_readiness.py's readers: a definition is a
    function body whose name ends in the address (comments, prototypes and
    literals excluded), plus its reviewed ALIASES; the ledgers are
    FIRST/SECOND/THIRD_LEVEL_CENSUS.md."""
    import level_readiness as LR
    _by_symbol, by_key = LR.source_definitions(ROOT)
    defs = defaultdict(set)
    for key, items in by_key.items():
        if key.startswith('boot:'):
            defs[int(key[5:], 16)] |= {i['file'] for i in items}
    for key, (path, _sym) in LR.ALIASES.items():
        if key.startswith('boot:'):
            defs[int(key[5:], 16)].add(path)
    ledgers = defaultdict(list)
    for key, rows in LR.census_ledgers(ROOT).items():
        if key.startswith('boot:'):
            ledgers[int(key[5:], 16)] += [(r['level'], r['status'], r['document']) for r in rows]
    return defs, ledgers


def own_overlay(n):
    """{address: file} of the overlay functions the area's own module
    defines (a body `int em_areaNN_overlay_XXXXXXXX(` at a line start)."""
    out = {}
    for path in sorted((ROOT / 'src/game').glob(f'em_area{n:02d}_overlay*.c')):
        for m in re.finditer(rf'^int em_area{n:02d}_overlay_([0-9A-F]{{8}})\(', path.read_text(), re.M):
            out[int(m.group(1), 16)] = path.name
    return out


def port_cell(a, defs, ledgers, overlay):
    if overlay:
        return 'missing (overlay)'
    led = sorted(set(ledgers.get(a, ())))
    d = sorted(defs.get(a, ()))
    parts = []
    if led:
        parts.append('census: ' + ', '.join(f'{st} (level {lv})' for lv, st, _doc in led))
    if d:
        parts.append(', '.join(f'`{x.split("/")[-1]}`' for x in d[:3]) + (f' (+{len(d) - 3})' if len(d) > 3 else ''))
    return '; '.join(parts) if parts else '**missing**'


PORTED = (0, 1, 2, 3, 4, 6, 7, 8, 11, 13, 15, 19, 22)
STORE_LOAD = set(range(0x20, 0x40)) | {0x1E, 0x1F, 0x09, 0x0D}


def masked(words):
    """A function's words with its overlay-relocated fields zeroed, so the
    same code linked into another overlay compares equal: lui immediates,
    the low halves an addiu / ori / load / store adds to a lui-loaded
    register (tracked from the function start until the register is
    written again), and j / jal targets inside the arena."""
    out, hi = [], set()
    for w in words:
        op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        m = w
        if op == 0x0F:
            m = w & 0xFFFF0000
            hi.add(rt)
        elif op in (2, 3) and ARENA_LO <= ((w & 0x3FFFFFF) << 2) < ARENA_HI:
            m = w & 0xFC000000
        elif op in STORE_LOAD and rs in hi:
            m = w & 0xFFFF0000
            if op in (0x09, 0x0D) or 0x20 <= op <= 0x27:
                hi.discard(rt)
        elif op == 0:
            hi.discard(rd)
        elif op in (0x09, 0x0D, 0x0C, 0x0E, 0x0A, 0x0B) or 0x20 <= op <= 0x27:
            hi.discard(rt)
        out.append(m)
    return tuple(out)


def overlay_functions(overviews, n):
    p = overviews / f'area{n:02d}' / 'overview.json'
    if not p.exists():
        return None
    ov = json.loads(p.read_text())
    blob = (DECOMP / f'extract/OVERLAY/AREA{n:02d}.BIN').read_bytes()
    out = {}
    for f in ov['overlay_functions']:
        lo = f['addr'] - ARENA_LO
        words = [int.from_bytes(blob[lo + k:lo + k + 4], 'little') for k in range(0, f['size'], 4)]
        if len(words) >= 12 and any(words):
            out[f['addr']] = masked(words)
    return out


def twin_index(overviews):
    """{masked words: [(area, address)]} over the ported areas' overlays."""
    idx = defaultdict(list)
    for n in PORTED:
        fns = overlay_functions(overviews, n)
        for a, sig in (fns or {}).items():
            idx[sig].append((n, a))
    return idx


def twin_cell(n, a, overviews, twins, cache={}):
    if n not in cache:
        cache[n] = overlay_functions(overviews, n) or {}
    sig = cache[n].get(a)
    hits = twins.get(sig, []) if sig else []
    return ', '.join(f'AREA{m:02d} 0x{b:06X}' for m, b in hits[:3]) if hits else ''


def hx(v):
    return f'0x{v:06X}' if v >= ARENA_LO else f'{v:08X}'


def load(overviews, n):
    return json.loads((overviews / f'area{n}' / 'overview.json').read_text())


def static_receipt(n):
    p = ROOT / f'assets/area{n}/static.json'
    return json.loads(p.read_text()) if p.exists() else None


def area_blocks(n, ov, st, defs, cases):
    out = []
    overlay_fns = {f['addr']: f for f in ov['overlay_functions']}
    # behaviour -> [where]
    users = defaultdict(list)
    for s, pl in ov['placements'].items():
        for r in pl['records']:
            if r['behavior']:
                users[r['behavior']].append(f's{s}[{r["index"]}]')
    for g in ov['groups']:
        for r in ov['group_records'].get(f'0x{g["item"]:x}', []):
            if r['behavior']:
                users[r['behavior']].append(f'{g["label"].replace("deferred group", "g")} [{r["index"]}]'
                                            if 'deferred' in g['label'] else f'{g["label"]} [{r["index"]}]')
    rows = ['| Behaviour | Kind | Used by | Port |', '|---|---|---|---|']
    own = own_overlay(n)
    for b in sorted(users):
        ovl = ARENA_LO <= b < ARENA_HI
        where = users[b]
        shown = ', '.join(where[:6]) + (f' (+{len(where) - 6})' if len(where) > 6 else '')
        cell = port_cell(b, defs, cases, ovl)
        if ovl and b in own:
            cell = f'`{own[b]}`'
        elif ovl:
            tw = twin_cell(n, b, OVERVIEWS[0], TWINS[0])
            cell = f'missing; twin of {tw}' if tw else 'missing'
        rows.append(f'| {hx(b)} | {"overlay" if ovl else "boot"} | {shown} | {cell} |')
    out.append('\n'.join(rows))
    # scripts
    cb = defaultdict(list)
    for f in ov['overlay_functions']:
        for r in f['reached_by']:
            m = re.match(r'script (0x[0-9a-f]+) op09 record', r)
            if m:
                cb[int(m.group(1), 16)].append(f['addr'])
    if ov['scripts']:
        rows = ['| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |',
                '|---|---:|---|---|---|']
        for s in ov['scripts']:
            calls = ', '.join(hx(a) for a in sorted(set(cb.get(s['entry'], [])))) or '—'
            rows.append(f'| {hx(s["entry"])} | {len(s["records"])} | '
                        f'{", ".join(hx(a) for a in s["started_by"]) or "data word"} | {calls} | '
                        f'{", ".join(s["not_admitted"]) or "—"} |')
        out.append('\n'.join(rows))
    else:
        out.append('No script chain.')
    # doors and area changes
    rows = ['| Sub | Placement | Behaviour | Door id | Meaning |', '|---|---|---|---:|---|']
    for r in (st or {}).get('subs', []):
        for d in r['doors']['used']:
            rec = d['record']
            meaning = (f'area change: area {rec[0]:#04x} entry {rec[1]}' + (f' sub {rec[3]}' if rec[2] else ' sub kept')
                       if d['area_change'] else f'room move: entries {rec[0]} / {rec[1]}')
            rows.append(f'| {r["sub"]} | [{d["placement"]}] | {d["behaviour"]} | {d["door_id"]} | {meaning} |')
    if len(rows) == 2:
        rows.append('| — | — | — | — | no door placement |')
    out.append('\n'.join(rows))
    # boot reach
    delta = ov['census_delta']
    missing = [f for f in delta['functions'] if not (defs.get(f['addr']) or cases.get(f['addr']))]
    have = [f for f in delta['functions'] if defs.get(f['addr']) or cases.get(f['addr'])]
    named = [f for f in missing if f'{f["addr"]:08X}' in SOURCE_TEXT[0]]
    absent = [f for f in missing if f not in named]
    out.append(f'Boot functions reachable from this area\'s owners and not in the first-level census: '
               f'{delta["delta"]} ({delta["delta_bytes"]:,} bytes); the port defines or ledgers {len(have)} of '
               f'them; {len(named)} more are named in the sources only (a comment, a table or another name: '
               'read each): ' + (', '.join(f'{f["addr"]:08X}' for f in sorted(named, key=lambda f: f['addr'])) or 'none')
               + f'; {len(absent)} appear nowhere: '
               + (', '.join(f'{f["addr"]:08X}' for f in sorted(absent, key=lambda f: f['addr'])) or 'none') + '.')
    stats = defaultdict(int)
    for f in ov['overlay_functions']:
        stats[f['status']] += 1
    done = sorted(a for a in own if a in {f['addr'] for f in ov['overlay_functions']})
    out.append(f'Overlay functions: {len(overlay_fns)} ({", ".join(f"{k} {v}" for k, v in sorted(stats.items()))}; '
               'decomp status at the census commit); '
               + (f'{len(done)} have a translation in the area\'s own module ('
                  + ', '.join(sorted({own[a] for a in done})) + ').' if done else 'none has a port translation.'))
    return out


def summary(overviews, defs, cases):
    rows = ['| Area | Subs | Placements | Behaviours (boot / overlay) | Boot behaviours in a ledger or defined | '
            'Chains | Doors used |', '|---|---|---|---|---|---:|---:|']
    for n in AREAS:
        ov, st = load(overviews, n), static_receipt(n)
        beh = set()
        for pl in ov['placements'].values():
            beh |= {r['behavior'] for r in pl['records'] if r['behavior']}
        for g in ov['groups']:
            beh |= {r['behavior'] for r in ov['group_records'].get(f'0x{g["item"]:x}', []) if r['behavior']}
        boot = sorted(b for b in beh if not ARENA_LO <= b < ARENA_HI)
        ovl = [b for b in beh if ARENA_LO <= b < ARENA_HI]
        have = [b for b in boot if defs.get(b) or cases.get(b)]
        doors = sum(len(r['doors']['used']) for r in st['subs']) if st else 0
        rows.append(f'| AREA{n} | {", ".join(ov["placements"])} | '
                    f'{" + ".join(str(len(p["records"])) for p in ov["placements"].values())} | '
                    f'{len(boot)} / {len(ovl)} | {len(have)} of {len(boot)} | {len(ov["scripts"])} | {doors} |')
    return '\n'.join(rows)


OVERVIEWS, TWINS, SOURCE_TEXT = [None], [None], ['']


def blocks(overviews):
    defs, cases = port_index()
    OVERVIEWS[0], TWINS[0] = overviews, twin_index(overviews)
    SOURCE_TEXT[0] = '\n'.join(p.read_text(errors='replace').upper() for p in sorted((ROOT / 'src').rglob('*'))
                                if p.is_file() and p.suffix in ('.c', '.h', '.inc', '.m'))
    out = {'summary': summary(overviews, defs, cases)}
    for n in AREAS:
        parts = area_blocks(n, load(overviews, n), static_receipt(n), defs, cases)
        out[f'area{n}'] = '\n\n'.join(parts)
    return out


def apply(doc, generated, write):
    text = doc.read_text()
    changed = []
    for name, body in generated.items():
        pat = re.compile(rf'(<!-- second_half:{name} begin -->\n).*?(<!-- second_half:{name} end -->)', re.S)
        if not pat.search(text):
            raise SystemExit(f'{doc}: no second_half:{name} markers')
        new = pat.sub(lambda m: m.group(1) + body + '\n' + m.group(2), text)
        if new != text:
            changed.append(name)
        text = new
    if write:
        doc.write_text(text)
    return changed


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--overviews', type=Path, default=DECOMP / 'build/s87')
    ap.add_argument('--doc', type=Path)
    ap.add_argument('--check', type=Path)
    args = ap.parse_args(argv)
    gen = blocks(args.overviews)
    if args.doc:
        print('rewrote', apply(args.doc, gen, True) or 'nothing')
    elif args.check:
        changed = apply(args.check, gen, False)
        if changed:
            print('differs:', changed)
            return 1
        print('equal')
    else:
        for k, v in gen.items():
            print(f'## {k}\n\n{v}\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
