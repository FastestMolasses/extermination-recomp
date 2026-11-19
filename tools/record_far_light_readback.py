#!/usr/bin/env python3
"""Record the original's step K Z read-backs (001D7410) and far-light flare calls
over Claude's kept guided takes (docs/AREA21_PORT.md "Far light").

Each named take of the decomp's build/guided_play is replayed hidden on the PCSX2
fork through the decomp's tools/guided_take_probe.py (its own start state, pad and
card events; the decomp's run lock build/.pcsx2.lock; the session is shut down when
the take ends) with:
  - log probes at 001CEFD0 (the flare), 001D73A0 (the read-back word's reader),
    001D72D0 (the far point's queue) and 001D7410 (step K);
  - a watch table at every 001D7410 entry (step K, before its drain): the count
    D_00275C08, the handle D_0027569C, the bank word ctx +0x9C (0x00811D5C), the
    read-back list D_008172C0 (both banks, 0x900 bytes) and the draw environment
    D_00810F00 (FRAME_1, ZBUF_1).
A frame whose step K drains entry 0 (count 1) has its point at the bank's +0 / +4;
the next row (the other bank by then) holds the 192 bytes the transfer wrote.

Output (decomp build/fork_refs/far_light_readback/<take>.json; disc-derived RAM
values, never committed, never deleted: a recording rule of the user's):
  {"take", "vsyncs", "env" (D_00810F00 hex, one value over the take),
   "readbacks": [[vsync, tick, x, y, "192 bytes hex", bank], ...]  (step K's vsync
   and main-loop tick: the take's row; the bank ctx +0x9C, which in the original
   equals D_00810E80 and the field D_00810E88), "flares": [[vsync, tick], ...] (001CEFD0's
   calls), "readers": [[vsync, tick], ...] (001D73A0's), "max_count"}
The raw probe output stays in decomp build/far_light_readback_scratch/<take>/
(removable).

The renderer (--renderer, default 13): the takes were played and replay exactly
under the fork's Metal renderer (17; decomp record_guided_run.py take_renderer),
but a local -> host transfer under Metal reads the GPU's depth, not GS local
memory: past the buffer's width it reads 0 where the GS addresses the next pages,
and its Z differs in the low bits and in the field's half-line phase. The
software renderer (13) is the exact GS (docs/GS_EXACT.md); its replay of these
takes differs from the take only in that list and in IOP timestamps (decomp
record_guided_run.py: "a buffer list's order", nothing shown).

Usage (native arm64 macOS):
  python3 tools/record_far_light_readback.py g76_area21_prep g77_area21_phase1
  python3 tools/record_far_light_readback.py --extract g76_area21_prep   # from the scratch probe again
"""
import argparse, json, os, struct, subprocess, sys
from pathlib import Path

DECOMP = Path('/Users/abe/Documents/Extermination.nosync/Extermination')
GUIDED = DECOMP / 'build/guided_play'
OUT = DECOMP / 'build/fork_refs/far_light_readback'
SCRATCH = DECOMP / 'build/far_light_readback_scratch'
PCS = '0x1cefd0,0x1d73a0,0x1d72d0,0x1d7410'
TABLE = '0x275C08:4:count,0x27569C:4:handle,0x811D5C:4:bank,0x8172C0:2304:list,0x810F00:32:env'


def vsyncs_of(take):
    end = json.loads((GUIDED / take / 'end.json').read_text())
    start = json.loads((GUIDED / take / 'start.json').read_text())
    if 'vsync' in end and 'vsync' in start:
        return end['vsync'] - start['vsync']
    sys.exit(f'{take}: no vsync span in start.json / end.json')


def probe(take, vsyncs, renderer):
    out = SCRATCH / f'{take}{suffix(renderer)}'
    env = dict(os.environ, GUIDED_REPLAY_RENDERER=str(renderer))
    cmd = [str(DECOMP / '.venv/bin/python'), str(DECOMP / 'tools/guided_take_probe.py'), take, str(vsyncs), str(out),
           '--pcs', PCS, '--table', TABLE, '--trigger', '0x1d7410']
    subprocess.run(cmd, cwd=DECOMP, check=True, env=env)
    return out


def suffix(renderer):
    return '' if renderer == 13 else f'.renderer{renderer}'


def extract(take, raw, vsyncs, renderer):
    meta = json.loads((raw / 'x.tbl.json').read_text())
    off = {s['name']: s['offset'] for s in meta['spans']}
    size, head = meta['row_size'], meta['header_size']
    data = (raw / 'x.tbl').read_bytes()
    rows = []
    for i in range((len(data) - head) // size):
        r = data[head + i * size: head + (i + 1) * size]
        vs, tick = struct.unpack_from('<II', r, 4)
        count = struct.unpack_from('<i', r, off['count'])[0]
        bank = struct.unpack_from('<I', r, off['bank'])[0]
        rows.append((vs, count, bank, r[off['list']: off['list'] + 0x900], r[off['env']: off['env'] + 0x20], tick))
    envs = sorted({r[4].hex() for r in rows})
    if len(envs) != 1:
        sys.exit(f'{take}: D_00810F00 changes over the take ({len(envs)} values): not modelled')
    readbacks, max_count = [], 0
    for (vs, count, bank, lst, _, tick), nxt in zip(rows, rows[1:]):
        max_count = max(max_count, count)
        if count == 0:
            continue
        if count != 1 or nxt[2] == bank or nxt[5] != tick + 1:
            sys.exit(f'{take}: tick {tick}: count {count}, next row bank {nxt[2]} tick {nxt[5]}: not this shape')
        x, y = struct.unpack_from('<ff', lst, bank * 0x480)
        readbacks.append([vs, tick, int(x), int(y), nxt[3][bank * 0x480: bank * 0x480 + 192].hex(), bank])
    calls = (raw / 'calls.bin').read_bytes()
    by_pc = {}
    for i in range(0, len(calls), 64):
        vs, tick = struct.unpack_from('<II', calls, i + 4)
        pc = struct.unpack_from('<I', calls, i + 0x18)[0]
        by_pc.setdefault(pc, []).append([vs, tick])
    doc = dict(take=take, vsyncs=vsyncs, renderer=renderer, env=envs[0], readbacks=readbacks,
               flares=by_pc.get(0x1CEFD0, []), readers=by_pc.get(0x1D73A0, []), max_count=max_count)
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / f'{take}{suffix(renderer)}.json').write_text(json.dumps(doc))
    print(f'{take}: {len(readbacks)} read-backs, {len(doc["flares"])} flares, {len(doc["readers"])} 001D73A0 calls '
          f'-> {OUT / (take + suffix(renderer) + ".json")}', flush=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('takes', nargs='+')
    ap.add_argument('--extract', action='store_true', help='derive the JSON from the scratch probe again')
    ap.add_argument('--vsyncs', type=int, help='vsyncs to run (default: the take\'s span)')
    ap.add_argument('--renderer', type=int, default=13, help='the fork\'s GS renderer for the replay (default 13, '
                    'software: the exact GS, whose local memory holds the drawn Z; 17 Metal, the takes\' own '
                    'renderer, reads back its GPU depth: <take>.renderer17.json)')
    a = ap.parse_args()
    for take in a.takes:
        vs = a.vsyncs or vsyncs_of(take)
        raw = SCRATCH / f'{take}{suffix(a.renderer)}' if a.extract else probe(take, vs, a.renderer)
        extract(take, raw, vs, a.renderer)


if __name__ == '__main__':
    main()
