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
the original's: where position or heading first leaves the recording in each segment.
A viewer's saves are not touched: the run uses its own empty card folder. A
take whose start.json has no "card" key was played on the fork's fresh,
unformatted cards, so until the chain's first formatted take the port answers
as that card did (EM_TEST_CARD_FRESH, docs/SAVE_GAME.md section 6.2): a save
attempt there fails as it failed on the original.

Close the window to stop; the comparison covers what played.

Usage (native arm64 macOS; build/extermination first, native):

  python3 tools/watch_takes.py                 # the chain from run1, in a window
  python3 tools/watch_takes.py test1           # another chain
  python3 tools/watch_takes.py --check test1   # no window, full speed, comparison only (EM_NO_RENDER=1;
                                               # --render draws the picture as well)
  python3 tools/watch_takes.py --root ../Extermination/build/guided_play g01_title_newgame
                                               # Claude's guided chain (docs FULL_RUN_PLAN 8.3)
  python3 tools/watch_takes.py --check --ticks 64000 --root ../Extermination/build/guided_play g01_title_newgame
                                               # a stop point: the chain's first 64000 game ticks

  python3 tools/watch_takes.py --root ../Extermination/build/guided_play g01_title_newgame --chapters
                                               # the guided chain's chapters, with their times
  python3 tools/watch_takes.py --root ../Extermination/build/guided_play g01_title_newgame --from g16_area08
  python3 tools/watch_takes.py --root ../Extermination/build/guided_play g01_title_newgame --at 2:41:37
                                               # fast-forward (no window, no sound, no pacing) to that take or
                                               # chapter, then the normal window, pace and sound, one process
                                               # (EM_FAST_UNTIL, docs/STARTUP.md); the point's (segment:offset)
                                               # is printed, and a warning when it is past the last --check's frontier

Disc timing: every playback runs with the PS2 disc-drive timing switch, and where a take of
the chain has a recorded drive table (tools/record_drive_replay_guided.py: the original's
per-read drive timing on the fork's exact replay of the take) each read takes the original's
time (EM_PS2_DRIVE_REPLAY; docs/IOP_STREAM.md "Guided takes"). A comparison aid only, never
game behaviour (docs/LAUNCHER_OPTIONS.md); --no-drive-replay plays with the drive model.

--root DIR plays the chains of another take folder; the default is the user's
own takes. A chain never mixes the two: under --root, a take whose start state or
card lies outside DIR is refused, and the output goes to build/guided_takes/.
--port DIR runs another checkout's built binary (its build/extermination and
assets), e.g. a worktree that has no build of its own.

A guided take's pad columns are re-read from its input.log at the vsync the
original read them (read_pads: the guided recorder's timeline rule is one vsync
off the user's); the user's takes play their take.rec as written.

rand()'s state follows the recording tick by tick (em_replay.c says why), and the
world clock too (take.rec's column, or tools/take_clock.py for an older take), so a
difference comes from the logic and not from a load's length.

Output: build/takes/<first take>/ (derived from the disc: never committed).
"""
import argparse
import bisect
import json
import math
import os
import shutil
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TAKES = ROOT.parent / 'Extermination/build/human_play'
USER_TAKES = TAKES
OUT = ROOT / 'build/takes'
BIN = ROOT / 'build/extermination'
COLS = ('step counter mv ph btn lx ly rx ry area sub entry bd8 s8d t9 tb x y z yaw seg off cap af').split()
HEADER = ['# EMREC 1', '# source ps2-takes', '# mode recording', '# area0 world', '# tick_hz 59.94',
          '#cols ' + ' '.join(COLS) + ' rand']
TICK_HZ = 59.94
SAME = 0.01             # position difference a float print can leave
TURNED = 0.001          # the same for the heading, in radians


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


def clocks(names):
    """The takes' world clocks by counter: take.rec's clock column, or for a take
    played before it the file tools/take_clock.py writes. None if a take has neither."""
    out = {}
    for name in names:
        rows = [line.split() for line in (TAKES / name / 'take.rec').read_text().splitlines() if line[0] != '#']
        played = [row for row in rows if row[2] == '0']
        if len(played[0]) == len(COLS) + 2:
            out.update((int(row[1]), row[-1]) for row in played)
            continue
        path = OUT / name / 'clock.txt'
        if not path.exists():
            print(f'{name} has no clock.txt (tools/take_clock.py): the world clock runs free', flush=True)
            return None
        for line in path.read_text().splitlines():
            counter, clock = line.split()
            out[int(counter)] = clock
    return out


def input_log(folder):
    """The take's input.log as {vsync: (buttons, lx, ly, rx, ry)} (the decomp's
    record_original_play.records: 32-byte records, a later record of a vsync wins)."""
    data = (folder / 'input.log').read_bytes()
    return {v: pad for v, _tick, *pad in
            (struct.unpack_from('<IIH4B', data, i) for i in range(0, len(data) - 31, 32))}


def read_pads(name, carried):
    """The pad each played tick of a guided take read, or None for a take whose
    take.rec pad columns already are (the user's takes).

    take.rec lists at row k the input.log record of row k's vsync. Both kinds of
    take replay exactly on the original, but with different timeline rules: a
    user's take puts record v at offset v - (first record) - 1 = v - start - 2
    (the decomp's record_original_play.timeline), a guided take at offset
    v - start + shift with shift -1 (record_guided_run.replay; verify.json
    'shift'). EM_INPUT_PLAY plays the user's rule (row k's pad is tick k's read:
    test1 and run1 play exact), so a guided take's tick k read the record of
    vsync(k) - 2 - shift, one vsync earlier than its take.rec row says: played
    as listed, every guided stick reached the port a tick early (the heading
    one pi/8 turn step ahead, lane PORT-WATCH-DRIFT). Before a take's first
    record the pad in effect is the one the previous take left ('carried').
    Returns (pads per game row, the take's last pad)."""
    folder = TAKES / name
    verify = folder / 'verify.json'
    shift = json.loads(verify.read_text()).get('shift') if verify.exists() else None
    if shift is None:
        return None, carried
    meta = json.loads((folder / 'rows.tbl.json').read_text())
    at = next(h['offset'] for h in meta['row_header'] if h['name'] == 'vsync')
    table = (folder / 'rows.tbl').read_bytes()[meta['header_size']:]
    size = meta['row_size']
    log = input_log(folder)
    keys = sorted(log)
    pads = []
    for i in range(len(table) // size):
        v = struct.unpack_from('<I', table, i * size + at)[0] - 2 - shift
        if v in log:
            pads.append(log[v])
        else:
            j = bisect.bisect_right(keys, v)
            pads.append(log[keys[j - 1]] if j else carried)
    return pads, log[keys[-1]] if keys else carried


def stitch(names, path, ticks=None, marks=None):
    """Write the stitched recording; returns its game ticks. `marks`, when given,
    becomes {take: [stitched row index of each game row of its take.rec, or None
    for a row the earlier take already had]}."""
    rows, last = [], -1
    clock = clocks(names)
    carried = (0, 128, 128, 128, 128)
    for name in names:
        take = TAKES / name / 'take.rec'
        if not take.exists():
            sys.exit(f'{take} is missing: run tools/record_original_play.py trace {name} in the decomp')
        pads, carried = read_pads(name, carried)
        played = -1
        for line in take.read_text().splitlines():
            if line.startswith('#'):
                continue
            parts = line.split()
            movie = parts[2] == '1'
            if not movie:
                played += 1
                if pads is not None:
                    if played >= len(pads):
                        sys.exit(f'{name}: take.rec has more game rows than rows.tbl')
                    btn, *axes = pads[played]
                    parts[4:9] = [f'0x{btn:04x}', *map(str, axes)]
                    line = ' '.join(parts)
            if not movie and int(parts[1]) <= last:
                if marks is not None:
                    marks.setdefault(name, []).append(None)
                continue            # the tick a take ended inside: the earlier take has it
            if not rows and not movie:
                rows = lead_in(int(parts[1]))
            if not movie:
                last = int(parts[1])
                if marks is not None:
                    marks.setdefault(name, []).append(len(rows))
            rows.append(line)
    if ticks is not None:
        played = [i for i, line in enumerate(rows) if line.split()[2] == '0']
        rows = rows[:played[ticks - 1] + 1] if ticks < len(played) else rows
    rows = [' '.join([str(i), *line.split()[1:]]) for i, line in enumerate(rows)]
    header = HEADER
    if clock:
        # The lead-in and the movie rows have neither column; the port reads only past
        # the first segment, and a row with no clock takes the one before it.
        kept, now = [], '0'
        for line in rows:
            parts = line.split()
            if len(parts) == len(COLS):
                parts.append('00000000')
            parts = parts[:len(COLS) + 1]
            now = clock.get(int(parts[1]), now) if parts[2] == '0' else now
            kept.append(' '.join([*parts, now]))
        rows = kept
        header = HEADER[:-1] + [HEADER[-1] + ' clock']
    path.write_text('\n'.join(header + rows) + '\n')
    if marks is not None:
        marks['_pos'] = positions(rows)
    return sum(line.split()[2] == '0' for line in rows)


def positions(rows):
    """(segment, offset) of each stitched row, None for a movie row: the pair the
    port's log writes and compare() reads (segments() groups the game rows)."""
    pos, seg, off, prev = [], -1, 0, None
    for line in rows:
        parts = line.split()
        if parts[2] != '0':
            pos.append(None)
            continue
        if parts[3] != prev:
            seg, off, prev = seg + 1, 0, parts[3]
        else:
            off += 1
        pos.append((seg, off))
    return pos


def chain_vsyncs(names):
    """The vsyncs before each take in the one continuous run, and the total: the
    decomp's guided_run.timeline.chain_offsets, whose seconds the chapters use."""
    off, total = {}, 0
    for name in names:
        start, end = (json.loads((TAKES / name / f).read_text()) for f in ('start.json', 'end.json'))
        off[name] = total
        total += end['vsync'] - start.get('vsync', 0)
    return off, total


def take_vsyncs(name):
    """The vsync (from the take's start) of each game row of the take."""
    folder = TAKES / name
    meta = json.loads((folder / 'rows.tbl.json').read_text())
    at = next(h['offset'] for h in meta['row_header'] if h['name'] == 'vsync')
    table = (folder / 'rows.tbl').read_bytes()[meta['header_size']:]
    size = meta['row_size']
    v0 = json.loads((folder / 'start.json').read_text()).get('vsync', 0)
    return [struct.unpack_from('<I', table, i * size + at)[0] - v0 for i in range(len(table) // size)]


def stitched_row(names, marks, vsync):
    """The stitched row that plays chain vsync `vsync`: its take's game row at or
    just before that time (the next one the earlier take did not already have)."""
    off, total = chain_vsyncs(names)
    vsync = max(0, min(vsync, total - 1))
    name = [n for n in names if off[n] <= vsync][-1]
    row = max(0, bisect.bisect_right(take_vsyncs(name), vsync - off[name]) - 1)
    mine = marks[name]
    while row < len(mine) and mine[row] is None:
        row += 1
    return mine[row] if row < len(mine) else len(marks['_pos']) - 1


def target_of(names, marks, vsync):
    """EM_FAST_UNTIL's "segment:offset" for chain vsync `vsync`, and its stitched row."""
    idx = stitched_row(names, marks, vsync)
    while idx < len(marks['_pos']) and marks['_pos'][idx] is None:
        idx += 1                    # a movie row: the first game tick after it
    seg, off = marks['_pos'][min(idx, len(marks['_pos']) - 1)]
    return seg, off, idx


def parse_clock(text):
    """H:MM:SS, M:SS or plain seconds, as `record_guided_run.py chapters` prints them."""
    seconds = 0.0
    for part in text.split(':'):
        seconds = seconds * 60 + float(part)
    return seconds


def chapter_list():
    """The decomp's chapters over the guided chain: [(seconds, label, derived)]
    (tools/guided_run/timeline.py, read as that script's `chapters` prints them)."""
    sys.path.insert(0, str(ROOT.parent / 'Extermination/tools'))
    from guided_run import timeline
    return timeline.chapters(), timeline.clock


def frontier_of(take):
    """The last full --check run's exact frontier: {segment, offset} of the first
    tick that left the recording, or None if there is no such run."""
    path = OUT / take / 'frontier.json'
    return json.loads(path.read_text()) if path.exists() else None


def beyond(frontier, seg, off):
    """True when (seg, off) lies past the first tick the port left the recording."""
    if not frontier or frontier.get('segment') is None:
        return False
    return (seg, off) >= (frontier['segment'], frontier['offset'])


def fresh_until(names):
    """EM_TEST_CARD_FRESH: the first counter of the chain's first take on a
    formatted card (its start.json has a "card" key), past every counter when
    all of them played on the fresh cards."""
    for name in names:
        if 'card' in json.loads((TAKES / name / 'start.json').read_text()):
            rows = (line.split() for line in (TAKES / name / 'take.rec').read_text().splitlines())
            return next(int(r[1]) for r in rows if r[0][0] != '#' and r[2] == '0')
    return 0xFFFFFFFF


def parse(path):
    rows = []
    for line in path.read_text(errors='replace').splitlines():
        if line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) >= len(COLS) and parts[2] == '0':
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
    compare.last = None
    for row in played:
        seg, off = int(row['seg']), int(row['off'])
        if seg < 0 or seg >= len(segs) or off >= len(segs[seg]) or row['ph'] == 'T':
            continue
        want = segs[seg][off]
        pos = [float(row[c]) for c in 'xyz']
        target = [float(want[c]) for c in 'xyz']
        away = math.dist(pos, target)
        turned = abs(math.remainder(float(row['yaw']) - float(want['yaw']), math.tau))
        same = away <= SAME and turned <= TURNED and row['area'] == want['area'] and row['sub'] == want['sub']
        ticks += 1
        exact += same
        compare.last = (seg, off)
        entry = per.setdefault(seg, dict(ticks=0, first=None, worst=0.0, phase=want['ph'], area=want['area'], sub=want['sub']))
        entry['ticks'] += 1
        entry['worst'] = max(entry['worst'], away)
        if entry['first'] is None and not same:
            entry['first'] = (off, int(want['counter']), away, turned)
    print(f'{ticks} ticks compared, {exact} on the recording ({100 * exact / max(ticks, 1):.1f}%)')
    bad = [(seg, e['first'][0]) for seg, e in sorted(per.items()) if e['first'] is not None]
    compare.frontier = dict(segment=bad[0][0], offset=bad[0][1]) if bad else dict(segment=None, offset=None)
    compare.frontier['segments'] = len(segs)
    compare.frontier['reached'] = max(per, default=-1)
    for seg, e in sorted(per.items()):
        state = 'exact' if e['first'] is None else \
            f'leaves at offset {e["first"][0]} (original counter {e["first"][1]}), {e["first"][2]:.2f} away, ' \
            f'heading {e["first"][3]:.3f} off, worst {e["worst"]:.2f}'
        print(f'  segment {seg} {e["phase"]} area {e["area"]} sub {e["sub"]}: {e["ticks"]} ticks, {state}')
    return ticks, exact


def use_root(root):
    """Read takes from another folder (Claude's guided chain); its playbacks
    go to their own output folder, never into the user's."""
    global TAKES, OUT
    TAKES = Path(root).resolve()
    OUT = ROOT / 'build/guided_takes'


def refuse_mixing(names):
    """Under --root, every take of the chain starts inside that folder: from the
    title state (no 'after') or from the end of a take in the same folder."""
    user = (ROOT.parent / 'Extermination/build/human_play').resolve()
    for name in names:
        start = json.loads((TAKES / name / 'start.json').read_text())
        for key in ('state', 'state_abs'):
            if key in start and str(user) in str(Path(ROOT.parent / 'Extermination' / start[key]).resolve()):
                sys.exit(f'{name}: starts from the user\'s own takes; a chain never mixes them')
        if start.get('after') and not (TAKES / start['after'] / 'end.json').exists():
            sys.exit(f'{name}: its previous take {start["after"]} is not in {TAKES}')


def progress(proc, log, until_row):
    """While the port fast-forwards (no window yet), say how far it is every few seconds."""
    if until_row is None:
        return
    began = time.monotonic()
    while proc.poll() is None:
        time.sleep(3)
        try:
            with log.open('rb') as f:
                f.seek(0, os.SEEK_END)
                f.seek(max(0, f.tell() - 600))
                last = f.read().decode(errors='replace').splitlines()
            step = int(last[-2].split()[0]) if len(last) > 2 else 0
        except (OSError, ValueError, IndexError):
            continue
        if step >= until_row:
            return
        print(f'fast-forwarding: {100 * step / max(until_row, 1):.0f}% ({step} of {until_row} steps, '
              f'{time.monotonic() - began:.0f} s)', flush=True)


def choose_start(args, names, marks):
    """The start the options name, as (segment, offset, stitched row); prints the
    chapters for --chapters (None). Warns when the point is past the port's exact frontier."""
    frontier = frontier_of(args.take)
    off, total = chain_vsyncs(names)
    if args.chapters:
        chapters, clock = chapter_list()
        for t, label, derived in chapters:
            seg, o, _ = target_of(names, marks, round(t * TICK_HZ))
            flag = '  (past the exact frontier)' if beyond(frontier, seg, o) else ''
            print(f'{clock(t):>8} {label}{" *" if derived else ""}{flag}')
        print(f'\n{len(chapters)} chapters over {clock(total / TICK_HZ)}; start at one with --at <time> (or --from <take>); '
              f'* = a derived time.' + ('' if frontier else ' No --check result yet: the exact frontier is unknown.'))
        return None
    if args.start:
        if args.start not in off:
            sys.exit(f'--from {args.start}: not a take of this chain ({", ".join(names)})')
        vsync, what = off[args.start], f'the start of {args.start}'
    else:
        seconds = parse_clock(args.at)
        try:                        # a chapter's own time is exact to the vsync
            chapters, clock = chapter_list()
            seconds = next((t for t, _, _ in chapters if clock(t) == args.at.lstrip('0:') or clock(t) == args.at), seconds)
        except Exception:
            pass
        vsync, what = round(seconds * TICK_HZ), f'chain time {args.at}'
    seg, o, idx = target_of(names, marks, vsync)
    print(f'starting at {what}: segment {seg} offset {o} (stitched row {idx} of {len(marks["_pos"])})', flush=True)
    if beyond(frontier, seg, o):
        print(f'WARNING: that point is past where the port stops matching the recording (segment {frontier["segment"]} '
              f'offset {frontier["offset"]}): the pad no longer fits the game there, and what plays can drift or stall.')
    elif frontier is None:
        print('No full --check result for this chain: whether the port is exact up to there is unknown '
              f'(python3 tools/watch_takes.py --check --root ... {args.take}).')
    return seg, o, idx


def drive_replay_env(names, out, bank=True):
    """The comparison-only drive replay (IOP_STREAM.md "Drive replay", "Guided
    takes"): the original's per-read drive timing, recorded on the fork's replay
    of each take (tools/record_drive_replay_guided.py), merged over the chain into
    out/drive_replay.txt and served through EM_PS2_DRIVE_REPLAY. The disc-drive
    timing switch is on in every playback here already. A take with no table takes
    the drive model. The takes' recorded loader reads ("L" lines) and sound-bank
    transfers ("B" lines; without them with bank=False, --no-bank-replay) come with
    it. Never game behaviour: LAUNCHER_OPTIONS.md "Not launcher options"."""
    import drive_replay
    table_dir = drive_replay.GUIDED_DIR if TAKES != USER_TAKES else drive_replay.TAKES_DIR
    info = drive_replay.guided_table(names, out / 'drive_replay.txt', table_dir, bank=bank)
    if not info['entries']:
        print('drive replay: no recorded tables for this chain (the drive model)', flush=True)
        return {}
    gaps = f'; {len(info["missing"])} with no table take the drive model ({", ".join(info["missing"][:4])}' \
        f'{", ..." if len(info["missing"]) > 4 else ""})' if info['missing'] else ''
    print(f'drive replay: {info["entries"]} recorded reads from {len(info["takes"])} of {len(names)} takes{gaps}',
          flush=True)
    if bank and info['bank_entries']:
        print(f'bank replay: {info["bank_entries"]} recorded sound-bank loads; {len(info["bank_missing"])} takes '
              'without them', flush=True)
    return dict(EM_PS2_DRIVE_REPLAY=info['table'])


def main():
    global BIN
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('take', nargs='?', default='run1')
    parser.add_argument('--check', action='store_true', help='no window, full speed, silent')
    parser.add_argument('--render', action='store_true', help='with --check: draw the picture too (default: EM_NO_RENDER=1, state only)')
    parser.add_argument('--root', help='the take folder (default: the user\'s own takes)')
    parser.add_argument('--port', help='run this port checkout\'s build (default: this one)')
    parser.add_argument('--ticks', type=int, help='play only the chain\'s first N game ticks (the lead-in\'s '
                        'included): a stop point inside a long chain')
    parser.add_argument('--from', dest='start', metavar='TAKE', help='start watching at the start of this take of the chain '
                        '(the port fast-forwards to it first: no window, no sound, no pacing)')
    parser.add_argument('--at', metavar='H:MM:SS', help='start watching at this chain time (as `--chapters` or the decomp\'s '
                        '`record_guided_run.py chapters` prints it)')
    parser.add_argument('--chapters', action='store_true', help='list the guided chain\'s chapters with their times and exit')
    parser.add_argument('--then', type=int, metavar='N', help='with --from/--at: stop N game ticks after the start point '
                        '(a test or a timing of the fast-forward)')
    parser.add_argument('--env', action='append', default=[], metavar='NAME=VALUE', help='one more variable for the port '
                        '(developer: e.g. EM_FRAME_TIMING=<file>)')
    parser.add_argument('--hidden', action='store_true', help='with --from/--at: no window even after the switch, '
                        'and no sound (a test of the switch)')
    parser.add_argument('--no-drive-replay', action='store_true', help='leave out the recorded drive timing (the '
                        'default uses it wherever a take of the chain has a table: a comparison aid, below)')
    parser.add_argument('--no-bank-replay', action='store_true', help='leave out the takes\' recorded sound-bank '
                        'transfers (the drive replay\'s "B" lines: the area loads\' 001FB370 answers)')
    args = parser.parse_args()
    if args.root:
        use_root(args.root)
    port = Path(args.port).resolve() if args.port else ROOT
    BIN = port / 'build/extermination'
    names = chain(args.take)
    if args.root:
        refuse_mixing(names)
    beat = json.loads((TAKES / names[0] / 'start.json').read_text()).get('beat')
    if beat:
        sys.exit(f'{names[0]} starts where route beat {beat} ended (record_original_play.py --beat), not at the '
                 f'title: the port cannot play it from New Game')
    out = OUT / args.take
    fast = args.start or args.at or args.chapters
    if fast:
        out = out / 'from'
    if (out / 'card').exists():
        shutil.rmtree(out / 'card')     # every run starts from an empty card, as the take did
    (out / 'card').mkdir(parents=True)
    rec, log = out / 'stitched.rec', out / 'played.rec'
    marks = {} if fast else None
    ticks = stitch(names, rec, args.ticks, marks)
    print(f'{" + ".join(names)}: {ticks} ticks, {ticks / 59.94 / 60:.1f} minutes', flush=True)
    until = None
    if fast:
        until = choose_start(args, names, marks)
        if until is None:
            return 0
        if args.then is not None:
            ticks = stitch(names, rec, sum(p is not None for p in marks['_pos'][:until[2] + 1]) + args.then)
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(EM_INPUT_PLAY=str(rec), EM_INPUT_RECORD=str(log), EM_PS2_DISC_DRIVE_TIMING='1',
               EM_REPLAY_MAX_OVERRUN='3600', EM_TEST_MEMCARD_ROOT=str(out / 'card'))
    env.update(EM_TEST_CARD_FRESH=str(fresh_until(names)))
    env.update(item.split('=', 1) for item in args.env)
    if until:
        env.update(EM_FAST_UNTIL=f'{until[0]}:{until[1]}')
    if not args.no_drive_replay:
        env.update(drive_replay_env(names, out, bank=not args.no_bank_replay))
    if args.hidden:
        env.update(EM_HEADLESS='1', EM_MUTE='1')
    if args.check:
        env.update(EM_HEADLESS='1', EM_UNCAPPED='1', EM_MUTE='1')
        if not args.render:
            env.update(EM_NO_RENDER='1')    # a comparison reads no picture (docs/LAUNCHER_OPTIONS.md)
    with (out / 'run.log').open('w') as text:
        proc = subprocess.Popen([str(BIN)], cwd=port, env=env, stdout=text, stderr=subprocess.STDOUT)
        progress(proc, log, until[2] if until else None)
        code = proc.wait()
    faulted = False
    for line in (out / 'run.log').read_text(errors='replace').splitlines():
        stopped = 'FAULT' in line or 'player stage worker' in line or ' fault code ' in line or \
            line.startswith('chain page:')
        if line.startswith(('replay:', 'stream drive: replay')) or stopped or 'fail-stop' in line:
            print(line[:200], flush=True)
            faulted = faulted or stopped
    print(f'port exit {code}', flush=True)
    compare(rec, log)
    if args.check and not args.ticks and not fast:
        if faulted and compare.frontier['segment'] is None and compare.last:
            # every compared tick on the recording, but the game stopped: the frontier is the stop
            compare.frontier.update(segment=compare.last[0], offset=compare.last[1] + 1, fault=True)
        (out / 'frontier.json').write_text(json.dumps(compare.frontier) + '\n')
    if faulted:
        print('The game stopped at the FAULT above: a difference after it is the stopped game, not a new one.')
    return code


if __name__ == '__main__':
    sys.exit(main())
