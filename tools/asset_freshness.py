#!/usr/bin/env python3
"""Asset freshness: which installed assets were made by an older exporter.

assets/ is generated locally by many exporters (tools/export_*.py and a few
others), and an exporter that changes does not re-run itself: AREA15's and the
per-area sound registries' files stayed older than their exporters for weeks
(lane ASSET-FRESH, 2026-11-18). This tool keeps the record.

Two files:
  tools/asset_exporters.json (committed; paths only, no data): every export
    step's command and the asset paths it writes, measured by running each
    step in a worktree on a real copy of assets/ (`map` rebuilds it from such
    a run's step reports).
  assets/.freshness.json (ignored, local): per installed asset, the step that
    made it, the sha256 of that step's code (its tool and every tools module
    it imports, port and decomp, transitively), the last commit touching that
    code, and the input hashes (the pinned ELF; the disc image's size and
    mtime), stamped when it was made (`record`) or confirmed (`stamp`).

check (default, ~2 s; run by tools/test_sweep.py as a warning):
  an asset whose stamp matches the file is STALE when its step's code
  hash or an input changed, or when an earlier step of a chain made it last
  (e.g. export_level_world_textures without export_player_texture_variants
  after it). An asset with no stamp, or changed since its stamp (exported by
  hand), is counted UNSTAMPED: its age says nothing (a file older than its
  exporter's last commit is usually still byte-equal: on 2026-11-18 a date
  rule flagged 1,224 files where a full re-export changed 4), so `stamp` it
  once its export is checked. Exit 0 unless --strict.

Usage (port root, macOS arm64, pure Python):
  python3 tools/asset_freshness.py [check] [--strict] [--verbose]
  python3 tools/asset_freshness.py record <step> [-- <command ...>]   # run the step, stamp what it wrote
  python3 tools/asset_freshness.py stamp [<step> ...]    # the installed files are this code's output
  python3 tools/asset_freshness.py map <steps dir>       # rebuild asset_exporters.json from a run
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
MAP = ROOT / 'tools' / 'asset_exporters.json'
STAMPS = ROOT / 'assets' / '.freshness.json'
ELF = DECOMP / 'config' / 'SCUS_971.12'
ISO = DECOMP / 'Extermination-rebuilt.iso'


def module_file(name: str) -> Path | None:
    for base in (ROOT / 'tools', DECOMP / 'tools'):
        p = base / f'{name}.py'
        if p.is_file():
            return p
    return None


_IMPORTS: dict = {}


def imports_of(p: Path) -> list[Path]:
    """The tools modules one file imports (cached per run)."""
    if p not in _IMPORTS:
        found = []
        try:
            tree = ast.parse(p.read_text(errors='replace'))
        except SyntaxError:
            tree = None
        for node in ast.walk(tree) if tree else ():
            names = []
            if isinstance(node, ast.Import):
                names = [a.name.split('.')[0] for a in node.names]
            elif isinstance(node, ast.ImportFrom) and node.module and not node.level:
                names = [node.module.split('.')[0]]
            for n in names:
                m = module_file(n)
                if m:
                    found.append(m)
        _IMPORTS[p] = found
    return _IMPORTS[p]


def closure(script: Path) -> list[Path]:
    """The script and every tools module it imports, transitively."""
    seen, todo = set(), [script]
    while todo:
        p = todo.pop()
        if p in seen or not p.is_file():
            continue
        seen.add(p)
        todo += [m for m in imports_of(p) if m not in seen]
    return sorted(seen)


_SHA: dict = {}


def file_sha(p: Path) -> bytes:
    if p not in _SHA:
        _SHA[p] = hashlib.sha256(p.read_bytes()).digest()
    return _SHA[p]


def step_script(cmd: list[str]) -> Path | None:
    for a in cmd:
        if a.endswith('.py'):
            p = (ROOT / a) if not os.path.isabs(a) else Path(a)
            return p.resolve()
    return None


class Code:
    """Per-step code hash and last commit, cached for one run."""

    def __init__(self):
        self.cache = {}

    def of(self, cmd: list[str]) -> dict:
        key = tuple(cmd)
        if key not in self.cache:
            script = step_script(cmd)
            files = closure(script) if script else []
            h = hashlib.sha256()
            for f in files:
                h.update(str(f.relative_to(ROOT.parent)).encode() + b'\0' + file_sha(f))
            self.cache[key] = dict(code=h.hexdigest()[:16], files=files)
        return self.cache[key]

    def commit(self, cmd: list[str]) -> tuple[int, str]:
        """The newest commit touching the step's code (time, 'hash subject'); git is slow, so lazily."""
        c = self.of(cmd)
        if 'commit' not in c:
            last = (0, '')
            for repo in (ROOT, DECOMP):
                mine = [str(f) for f in c['files'] if repo in f.parents]
                if mine:
                    r = subprocess.run(['git', '-C', str(repo), 'log', '-1', '--format=%ct %h %s', '--', *mine],
                                       capture_output=True, text=True)
                    if r.stdout.strip():
                        t, rest = r.stdout.strip().split(' ', 1)
                        last = max(last, (int(t), rest))
            c['commit_time'], c['commit'] = last
        return c['commit_time'], c['commit']


def inputs() -> dict:
    d = {}
    if ELF.is_file():
        d['elf_sha256'] = hashlib.sha256(ELF.read_bytes()).hexdigest()[:16]
    if ISO.is_file():
        st = ISO.stat()
        d['iso'] = f'{st.st_size}:{int(st.st_mtime)}'
    return d


def content(rel: str) -> str:
    h = hashlib.sha256()
    with open(ROOT / rel, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()[:16]


def file_key(rel: str):
    st = (ROOT / rel).stat()
    return [st.st_size, st.st_mtime_ns]


def load(path: Path, default):
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError):
        return default


def snap() -> dict:
    d = {}
    for dp, _dn, fn in os.walk(ROOT / 'assets'):
        for f in fn:
            p = Path(dp) / f
            if p.name != STAMPS.name:
                st = p.lstat()
                d[str(p.relative_to(ROOT))] = (st.st_size, st.st_mtime_ns)
    return d


def stamp_files(stamps: dict, step: str, cmd: list[str], files, code: Code, ins: dict):
    c = code.of(cmd)
    commit = code.commit(cmd)[1]
    for rel in files:
        if (ROOT / rel).is_file():
            stamps[rel] = dict(step=step, code=c['code'], commit=commit, inputs=ins, file=file_key(rel), sha=content(rel),
                               stamped=time.strftime('%Y-%m-%d %H:%M'))


def owners(steps: dict) -> dict:
    """asset path -> the steps that write it, in run order (the last one made the file)."""
    o = {}
    for name, s in steps.items():
        for rel in s['outputs']:
            o.setdefault(rel, []).append(name)
    return o


def check(args) -> int:
    steps = load(MAP, {}).get('steps', {})
    if not steps:
        print('asset_freshness: no tools/asset_exporters.json')
        return 0
    stamps, code, ins = load(STAMPS, {}), Code(), inputs()
    stale, unstamped, missing, checked = {}, {}, 0, 0
    for rel, names in owners(steps).items():
        p = ROOT / rel
        if not p.is_file():
            missing += 1
            continue
        checked += 1
        st, why = stamps.get(rel), None
        # a file rewritten with the same bytes (a test that re-exports in place) keeps its stamp
        if st and (st.get('file') == file_key(rel) or st.get('sha') == content(rel)):
            c = code.of(steps[st['step']]['cmd']) if st['step'] in steps else None
            if c is None:
                why = (st['step'], 'step no longer in the map')
            elif st['step'] != names[-1] and st['step'] in names:
                why = (names[-1], f"last made by {st['step']}; {names[-1]} must run after it")
            elif c['code'] != st['code']:
                why = (st['step'], f"code changed since {st['commit'] or 'its stamp'} "
                                   f"(now {code.commit(steps[st['step']]['cmd'])[1]})")
            elif st.get('inputs') != ins:
                why = (st['step'], 'an input changed (ELF or disc image)')
        else:
            unstamped.setdefault(names[-1], []).append(rel)
        if why:
            stale.setdefault(why[0], []).append((rel, why[1]))
    n = sum(len(v) for v in stale.values())
    print(f'asset_freshness: {checked} installed assets in {len(steps)} export steps, '
          f'{n} stale in {len(stale)} steps, {sum(len(v) for v in unstamped.values())} unstamped'
          + (f', {missing} mapped paths not installed' if args.verbose else ''))
    for step, rows in sorted(stale.items()):
        print(f'  STALE {step:34} {len(rows):4} files, e.g. {rows[0][0]}: {rows[0][1][:90]}')
        if args.verbose:
            for rel, why in rows[1:]:
                print(f'        {rel}: {why[:100]}')
    if args.verbose:
        for step, rows in sorted(unstamped.items()):
            print(f'  UNSTAMPED {step:30} {len(rows):4} files (made by hand or before the stamps), e.g. {rows[0]}')
    if stale:
        print('  re-export: python3 tools/asset_freshness.py record <step> (into a worktree copy, then mv; '
              'decomp orchestrator skill section 4)')
    return 1 if (stale and args.strict) else 0


def record(args) -> int:
    data = load(MAP, {'steps': {}})
    steps = data['steps']
    cmd = args.cmd[1:] if args.cmd[:1] == ['--'] else args.cmd
    cmd = cmd or steps.get(args.step, {}).get('cmd')
    if not cmd:
        sys.exit(f'asset_freshness: no command for step {args.step}')
    before = snap()
    rc = subprocess.run(cmd, cwd=ROOT).returncode
    after = snap()
    wrote = sorted(k for k in after if before.get(k) != after[k])
    if rc != 0:
        print(f'asset_freshness: {args.step} failed (rc {rc}); nothing stamped')
        return rc
    stamps = load(STAMPS, {})
    stamp_files(stamps, args.step, cmd, wrote, Code(), inputs())
    STAMPS.write_text(json.dumps(stamps, indent=0, sort_keys=True) + '\n')
    new = sorted(set(wrote) - set(steps.get(args.step, {}).get('outputs', [])))
    print(f'asset_freshness: {args.step} wrote {len(wrote)} files, stamped'
          + (f'; {len(new)} not in the map (e.g. {new[0]}): run `map` or add them' if new else ''))
    return 0


def stamp(args) -> int:
    steps = load(MAP, {}).get('steps', {})
    stamps, code, ins = load(STAMPS, {}), Code(), inputs()
    want = set(args.steps) or set(steps)
    for rel, names in owners(steps).items():
        if names[-1] in want:
            stamp_files(stamps, names[-1], steps[names[-1]]['cmd'], [rel], code, ins)
    STAMPS.write_text(json.dumps(stamps, indent=0, sort_keys=True) + '\n')
    print(f'asset_freshness: {len(stamps)} assets stamped')
    return 0


def build_map(args) -> int:
    steps = {}
    # run order: a report's 'order' when it has one, else when it was written
    paths = sorted(Path(args.dir).glob('*.json'), key=lambda p: p.stat().st_mtime)
    reports = [load(p, None) for p in paths]
    for r in sorted((r for r in reports if r and r.get('rc') == 0 and r.get('wrote')),
                    key=lambda r: r.get('order', 0)):
        cmd = [('python3' if c.endswith('/python') or c == sys.executable else c) for c in r['cmd']]
        steps[r['name']] = dict(cmd=cmd, outputs=sorted(r['wrote']))
    MAP.write_text(json.dumps({'what': __doc__.splitlines()[0], 'steps': steps}, indent=1) + '\n')
    print(f'asset_freshness: {MAP.relative_to(ROOT)}: {len(steps)} steps, '
          f'{sum(len(s["outputs"]) for s in steps.values())} outputs')
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest='what')
    c = sub.add_parser('check')
    for p in (ap, c):
        p.add_argument('--strict', action='store_true', help='exit 1 when an asset is stale')
        p.add_argument('--verbose', action='store_true')
    r = sub.add_parser('record')
    r.add_argument('step')
    r.add_argument('cmd', nargs=argparse.REMAINDER)
    s = sub.add_parser('stamp')
    s.add_argument('steps', nargs='*')
    m = sub.add_parser('map')
    m.add_argument('dir')
    a = ap.parse_args(argv)
    return {'record': record, 'stamp': stamp, 'map': build_map}.get(a.what, check)(a)


if __name__ == '__main__':
    sys.exit(main())
