#!/usr/bin/env python3
"""Capture folders from Claude's guided run (decomp build/guided_play/), offline.

The second half's areas have no route recording (CAP2 never ran); the guided
run's kept takes are recordings of the original playing through them. Each
take holds end.p2s (an exact fork save state at the main loop's
end-of-frame point, end.json's pc), take.rec (one row per game tick: the pad
that tick's input step read, the area bytes, the player position and
heading, the rand() word and the world clock; decomp
tools/record_original_play.py write_rec) and rows.tbl (the same ticks'
D_00810700..03 and the rest of its spans, pxtbl/1). This tool turns a list
of takes into a route set in the layout tools/route_capture.py writes, so
the exporters (export_area01_common.Capture), the start cards
(build_start_saves.py START_POINTS) and the beats (mid_route_beats.py) read
them unchanged:

  <set>/<take>/eeMemory.bin, scratchpad.bin, gs.bin
        the take's end.p2s entries eeMemory.bin, Scratchpad.bin and GS.bin
        (the files pcsx2_session.ForkSession.snapshot extracts from its own
        save: a capture folder is exactly a save state's three entries);
  <set>/<take>/state.p2s
        a hard link to the take's end.p2s (the take is never modified; the
        next beat's source_state then lies inside the set, as a route's
        does);
  <set>/<take>/snapshot.json
        where the bytes come from (the take, end.p2s's SHA-256, end.json's
        vsync / tick / pc) and ee_sha256 (build_start_saves checks it);
  <set>/<take>/trace.json
        the take as a beat: rows (f, counter, pos, yaw, area4 from take.rec
        and rows.tbl; row 0 also carries fade (D_0028A9A0, 16 bytes) and
        m1F0 (the player record's +0x1F0) read from the source state's RAM,
        the state row 0's tick starts from), inputs (each change of the pad
        take.rec says a tick read, at f = tick - 2: route_capture's rule, a
        command set at row f reaches the game's pad block two frames later,
        which EM_START_AT_PAD reproduces), first_counter, frames,
        source_state, no teleports;
  <set>/manifest.json
        the beats in order with their source states.

Nothing is invented: every byte of a folder is the original's RAM, GS memory
or scratchpad in the take's own save, and every row value is the take's
record. What a capture represents is the take's END: the state the next take
starts from (end.json: the main loop's end-of-frame pc), not a moment
chosen inside the take. A take ending inside a load (g45: the area bytes
already 0x11, the load still running) gives a mid-load capture; the
exporters' own checks say whether a capture serves them.

A take with a movie row (take.rec's M rows) is refused: its ticks are not
one row per game frame there.

Usage (native arm64 macOS; reads the decomp's build/guided_play/, writes the
decomp's ignored build/fork_refs/<set>/; the zstd entries need the decomp's
.venv, which the tool re-runs itself under when zstandard is missing):

  python3 tools/capture_from_take.py --set guided-20261117/s87/route_g17 \\
      g44_area18_area14 g45_area14_area17 g46_area17_ramp g47_area17_roger g48_area17_cable

An existing folder is rewritten file by file only where it differs.
Output is disc-derived (RAM, GS memory): never committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
GUIDED = DECOMP / 'build/guided_play'
FORK = DECOMP / 'build/fork_refs'
ENTRIES = (('eeMemory.bin', 'eeMemory.bin'), ('Scratchpad.bin', 'scratchpad.bin'), ('GS.bin', 'gs.bin'))
FADE, FADE_SIZE = 0x28A9A0, 0x10          # the transition machine (route_capture's "fade" span)
PLAYER = 0x810350 - 0xA0                   # the player record (+0xA0 = D_00810350, the position)
PAD_LAG = 2                                # route_capture: a pad set at row f is read at row f + 2


def zstd_entry(p2s: Path, name: str) -> bytes:
    import zstandard  # noqa: PLC0415  (the decomp's venv)
    with zipfile.ZipFile(p2s) as z:
        info = z.getinfo(name)
    with open(p2s, 'rb') as f:
        f.seek(info.header_offset)
        lfh = f.read(30)
        if struct.unpack('<I', lfh[:4])[0] != 0x04034B50:
            raise SystemExit(f'{p2s}: bad local header for {name}')
        nlen, elen = struct.unpack('<HH', lfh[26:30])
        f.seek(info.header_offset + 30 + nlen + elen)
        comp = f.read(info.compress_size)
    d = zstandard.ZstdDecompressor()
    try:
        out = d.decompress(comp, max_output_size=info.file_size)
    except zstandard.ZstdError:
        out = d.stream_reader(comp).read()
    if len(out) != info.file_size:
        raise SystemExit(f'{p2s}: {name} is {len(out)} bytes, the zip says {info.file_size}')
    return out


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def table(path: Path):
    """pxtbl/1 rows as dicts of raw span bytes (and the row header's u32s)."""
    meta = json.loads(path.with_name(path.name + '.json').read_text())
    if meta.get('format') != 'pxtbl/1':
        raise SystemExit(f'{path}: format {meta.get("format")!r}')
    data = path.read_bytes()
    hsize, rsize = meta['header_size'], meta['row_size']
    out = []
    for at in range(hsize, len(data) - rsize + 1, rsize):
        row = data[at:at + rsize]
        r = {h['name']: struct.unpack_from('<I' if h['type'] == 'u32' else '<Q', row, h['offset'])[0]
             for h in meta['row_header']}
        r.update((s['name'], row[s['offset']:s['offset'] + s['len']]) for s in meta['spans'])
        out.append(r)
    return out


def rec_rows(path: Path):
    cols, rows = None, []
    for line in path.read_text().splitlines():
        if line.startswith('#cols '):
            cols = line[6:].split()
        elif line and line[0] != '#':
            rows.append(dict(zip(cols, line.split())))
    return rows


def write(path: Path, data: bytes) -> None:
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def beat(take: str, folder: Path, source_state: str, source_ram: bytes) -> dict:
    src = GUIDED / take
    end = json.loads((src / 'end.json').read_text())
    p2s = src / 'end.p2s'
    folder.mkdir(parents=True, exist_ok=True)
    blobs = {dst: zstd_entry(p2s, name) for name, dst in ENTRIES}
    for dst, data in blobs.items():
        write(folder / dst, data)
    link = folder / 'state.p2s'
    if not (link.exists() and os.path.samefile(link, p2s)):
        if link.exists():
            raise SystemExit(f'{link} exists and is not {p2s}')
        os.link(p2s, link)
    rec = rec_rows(src / 'take.rec')
    if any(r['mv'] != '0' for r in rec):
        raise SystemExit(f'{take}: a movie row in take.rec (not one row per game frame): refused')
    tbl = table(src / 'rows.tbl')
    if len(tbl) != len(rec) or any(int(r['counter']) != struct.unpack('<I', t['counter'])[0]
                                   for r, t in zip(rec, tbl)):
        raise SystemExit(f'{take}: take.rec and rows.tbl differ in rows or counters')
    rows, inputs, sent = [], [], None
    for k, (r, t) in enumerate(zip(rec, tbl)):
        rows.append({'f': k, 'counter': int(r['counter']), 'pos': [float(r['x']), float(r['y']), float(r['z'])],
                     'yaw': float(r['yaw']), 'area4': t['area'].hex(), 'ph': r['ph'], 'rand': r['rand']})
        pad = (int(r['btn'], 16), int(r['lx']), int(r['ly']))
        if pad != sent:
            f = max(k - PAD_LAG, 0)
            if inputs and inputs[-1]['f'] == f:
                inputs.pop()
            inputs.append({'f': f, 'buttons': pad[0], 'lx': pad[1], 'ly': pad[2]})
            sent = pad
    rows[0]['fade'] = source_ram[FADE:FADE + FADE_SIZE].hex()
    rows[0]['m1F0'] = source_ram[PLAYER + 0x1F0]
    rows[0]['row0_from'] = 'fade, m1F0: the source state\'s RAM'
    snap = {'emulator': 'fork', 'derived_from': f'build/guided_play/{take}/end.p2s',
            'tool': 'extermination-port tools/capture_from_take.py', 'end_p2s_sha256': sha256(p2s.read_bytes()),
            'vsync': end.get('vsync'), 'tick': end.get('tick'), 'pc': end.get('pc'),
            'reason': end.get('reason'), 'frames_stepped': len(rows) - 1,
            'ee_sha256': sha256(blobs['eeMemory.bin'])}
    write(folder / 'snapshot.json', (json.dumps(snap, indent=2) + '\n').encode())
    trace = {'what': f'guided take {take} as a beat (tools/capture_from_take.py)', 'source': 'guided_play',
             'emulator': 'fork', 'source_state': source_state, 'first_counter': rows[0]['counter'],
             'last_counter': rows[-1]['counter'], 'frames': len(rows) - 1, 'inputs': inputs, 'teleports': [],
             'snapshot': snap, 'rows': rows}
    write(folder / 'trace.json', (json.dumps(trace) + '\n').encode())
    return {'folder': str(folder.relative_to(DECOMP)), 'take': take, 'source_state': source_state,
            'state_sha256': snap['end_p2s_sha256'], 'frames': trace['frames'],
            'first_counter': trace['first_counter'], 'last_counter': trace['last_counter'],
            'area_end': blobs['eeMemory.bin'][0x810700:0x810704].hex()}


def main() -> int:
    try:
        import zstandard  # noqa: F401,PLC0415
    except ModuleNotFoundError:
        venv = DECOMP / '.venv/bin/python'
        if not venv.exists() or os.environ.get('EM_CAPTURE_FROM_TAKE_VENV'):
            raise SystemExit('zstandard is missing (the decomp\'s .venv has it)')
        env = dict(os.environ, EM_CAPTURE_FROM_TAKE_VENV='1')
        return subprocess.run([str(venv), __file__, *sys.argv[1:]], env=env).returncode
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--set', required=True, help='the route set under the decomp\'s build/fork_refs/')
    ap.add_argument('takes', nargs='+')
    args = ap.parse_args()
    out = FORK / args.set
    if FORK.resolve() not in out.resolve().parents:
        raise SystemExit(f'{out} is not under {FORK}')
    beats, previous = {}, {}
    for take in args.takes:
        start = json.loads((GUIDED / take / 'start.json').read_text())
        after = start.get('after')
        if after in previous:
            source = previous[after]
        else:
            source = str(Path(start['state_abs']).resolve())
        src_folder = Path(source).parent
        ram = (src_folder / 'eeMemory.bin').read_bytes() if (src_folder / 'eeMemory.bin').exists() else \
            zstd_entry(Path(source), 'eeMemory.bin')
        beats[take] = beat(take, out / take, source, ram)
        previous[take] = str(out / take / 'state.p2s')
        print(f'{take}: {beats[take]["frames"]} frames, end area {beats[take]["area_end"]}', flush=True)
    manifest = {'what': 'guided takes as a route set (extermination-port tools/capture_from_take.py); every '
                        'folder is a take\'s end.p2s entries, every trace the take\'s take.rec / rows.tbl',
                'set': str(out.relative_to(DECOMP)), 'beats': beats}
    write(out / 'manifest.json', (json.dumps(manifest, indent=1) + '\n').encode())
    return 0


if __name__ == '__main__':
    sys.exit(main())
