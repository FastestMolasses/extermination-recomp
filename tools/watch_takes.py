#!/usr/bin/env python3
"""Play the user's own recorded takes in the port, stitched into one run.

The takes come from the decomp's tools/record_original_play.py (a person playing
the original; `trace` writes each take's take.rec). A chain of takes (run1, then
every take recorded --after it) becomes one EMREC recording, so the port plays
straight through from the title with no restart and no save load between takes.
Playback is em_replay.c's EM_INPUT_PLAY: tick for tick inside a segment, and at a
load, a cutscene or a menu each side keeps its own timing and the next segment
starts on the port's first tick of that phase.

The run writes a log of what the port did, and every tick is then compared with
the original's: where the position first leaves the recording in each segment.
A viewer's saves are not touched: the run uses its own empty card folder.

Close the window to stop; the comparison covers what played.

Usage (native arm64 macOS; build/extermination first, native):

  python3 tools/watch_takes.py                 # the chain from run1, in a window
  python3 tools/watch_takes.py test1           # another chain
  python3 tools/watch_takes.py --check test1   # no window, full speed, comparison only

Output: build/takes/<first take>/ (derived from the disc: never committed).
"""
import argparse
import json
import math
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TAKES = ROOT.parent / 'Extermination/build/human_play'
OUT = ROOT / 'build/takes'
BIN = ROOT / 'build/extermination'
COLS = ('step counter mv ph btn lx ly rx ry area sub entry bd8 s8d t9 tb x y z yaw seg off cap af').split()
HEADER = ['# EMREC 1', '# source ps2-takes', '# mode recording', '# tick_hz 59.94', '#cols ' + ' '.join(COLS)]
SAME = 0.01             # position difference a float print can leave


def chain(first):
    """The take and the takes recorded after it, in order."""
    names, after = [first], {}
    for path in TAKES.glob('*/start.json'):
        parent = json.loads(path.read_text()).get('after')
        if parent:
            after[parent] = path.parent.name
    while names[-1] in after:
        names.append(after[names[-1]])
    return names


def lead_in(counter):
    """The port boots through the logos and the attract movie to the title
    state the takes start from: the neutral pad, the movie skipped."""
    rows = [f'0 {c} 0 T 0x0000 128 128 128 128 0 0 0 0 0 0 0 0 0 0 0 -1 -1 -1 -1' for c in range(counter)]
    return rows + ['0 0 1 M 0x0008 128 128 128 128 0 0 0 0 0 0 0 0 0 0 0 -1 -1 -1 -1']


def stitch(names, path):
    rows, last = [], -1
    for name in names:
        take = TAKES / name / 'take.rec'
        if not take.exists():
            sys.exit(f'{take} is missing: run tools/record_original_play.py trace {name} in the decomp')
        for line in take.read_text().splitlines():
            if line.startswith('#'):
                continue
            parts = line.split()
            movie = parts[2] == '1'
            if not movie and int(parts[1]) <= last:
                continue            # the tick a take ended inside: the earlier take has it
            if not rows and not movie:
                rows = lead_in(int(parts[1]))
            if not movie:
                last = int(parts[1])
            rows.append(line)
    rows = [' '.join([str(i), *line.split()[1:]]) for i, line in enumerate(rows)]
    path.write_text('\n'.join(HEADER + rows) + '\n')
    return sum(line.split()[2] == '0' for line in rows)


def parse(path):
    rows = []
    for line in path.read_text(errors='replace').splitlines():
        if line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) == len(COLS) and parts[2] == '0':
            rows.append(dict(zip(COLS, parts)))
    return rows


def segments(rows):
    segs = []
    for row in rows:
        if not segs or segs[-1][0]['ph'] != row['ph']:
            segs.append([])
        segs[-1].append(row)
    return segs


def compare(rec, log):
    """Each played tick against the recording's tick at the same segment and
    offset. Prints the segments' first differences; returns (ticks, exact)."""
    segs, played = segments(parse(rec)), parse(log)
    per, ticks, exact = {}, 0, 0
    for row in played:
        seg, off = int(row['seg']), int(row['off'])
        if seg < 0 or seg >= len(segs) or off >= len(segs[seg]) or row['ph'] == 'T':
            continue
        want = segs[seg][off]
        pos = [float(row[c]) for c in 'xyz']
        target = [float(want[c]) for c in 'xyz']
        away = math.dist(pos, target)
        same_area = row['area'] == want['area'] and row['sub'] == want['sub']
        ticks += 1
        exact += away <= SAME and same_area
        entry = per.setdefault(seg, dict(ticks=0, first=None, worst=0.0, phase=want['ph'], area=want['area'], sub=want['sub']))
        entry['ticks'] += 1
        entry['worst'] = max(entry['worst'], away)
        if entry['first'] is None and (away > SAME or not same_area):
            entry['first'] = (off, int(want['counter']), away, row['area'], row['sub'])
    print(f'{ticks} ticks compared, {exact} on the recording ({100 * exact / max(ticks, 1):.1f}%)')
    for seg, e in sorted(per.items()):
        state = 'exact' if e['first'] is None else \
            f'leaves at offset {e["first"][0]} (original counter {e["first"][1]}), {e["first"][2]:.2f} away, ' \
            f'worst {e["worst"]:.2f}'
        print(f'  segment {seg} {e["phase"]} area {e["area"]} sub {e["sub"]}: {e["ticks"]} ticks, {state}')
    return ticks, exact


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('take', nargs='?', default='run1')
    parser.add_argument('--check', action='store_true', help='no window, full speed, silent')
    args = parser.parse_args()
    names = chain(args.take)
    out = OUT / args.take
    (out / 'card').mkdir(parents=True, exist_ok=True)
    rec, log = out / 'stitched.rec', out / 'played.rec'
    ticks = stitch(names, rec)
    print(f'{" + ".join(names)}: {ticks} ticks, {ticks / 59.94 / 60:.1f} minutes', flush=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(EM_INPUT_PLAY=str(rec), EM_INPUT_RECORD=str(log), EM_PS2_DISC_DRIVE_TIMING='1',
               EM_REPLAY_MAX_OVERRUN='3600', EM_TEST_MEMCARD_ROOT=str(out / 'card'))
    if args.check:
        env.update(EM_HEADLESS='1', EM_UNCAPPED='1', EM_MUTE='1')
    with (out / 'run.log').open('w') as text:
        result = subprocess.run([str(BIN)], cwd=ROOT, env=env, stdout=text, stderr=subprocess.STDOUT)
    for line in (out / 'run.log').read_text(errors='replace').splitlines():
        if line.startswith('replay:') or 'FAULT' in line or 'fail-stop' in line:
            print(line[:200], flush=True)
    print(f'port exit {result.returncode}', flush=True)
    compare(rec, log)
    return result.returncode


if __name__ == '__main__':
    sys.exit(main())
