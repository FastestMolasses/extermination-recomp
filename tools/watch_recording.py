#!/usr/bin/env python3
"""Watch the recorded playthrough in the port: the recorded pad, live, not a video.

Everything recorded from the original, in the order it was played:

  1. New Game through a13_05, one continuous run: the route smoke's pads and
     its drive and rand() anchors, so it stays row-exact to the recording.
  2. From AREA19's first beat to AREA07: one run per section, each from a
     start card (tools/mid_route_beats.py). A loaded game is not the
     recording's machine state, so these can drift from the recording; the
     pad and the steering hold the route, but the creatures can still kill
     the player. When a run stops on a fail-stop or a death, the next beat
     starts from its own card.

Each run is paced at 59.94 Hz, with drawing and sound. Nothing here writes to
a viewer's saves: part 1 uses an empty card folder, part 2's cards are
read-only. Keep hands off the pad; a run does not take live input.

Close the window to skip to the next section; Ctrl+C here stops it all.

Usage (native arm64 macOS; build/extermination and make start-saves first):

  python3 tools/watch_recording.py                 # everything, about an hour and a quarter
  python3 tools/watch_recording.py --boss          # part 1 through the first boss's fight instead
  python3 tools/watch_recording.py --from a19c     # part 2 from a section or a beat
  python3 tools/watch_recording.py --only a15b     # one section, or a section from a beat
  python3 tools/watch_recording.py --list          # the sections
"""
import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mid_route_beats as beats_tool  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/watch_recording'
RAND = ROOT / 'build/rand_replay_bed'
# The two New Game routes the level smoke passes: (last phase, rand() anchors).
ROUTE = ('a13_05', RAND / 'route_a13_20261030/route.txt')
BOSS_ROUTE = ('a04b_s7', RAND / 'route_a13s_20261030/route.txt')
# (name, card, beats) in recorded order. The groups' chains, cut where the next
# section's first beat replays a beat (a19_02, a13d_07), and without the takes
# a later recording superseded (a03c_02, a03c_03, a04c, a07_00: AREA03X_PORT.md).
SECTIONS = (
    ('a19', 'area19', ('a19_00', 'a19_01')),
    ('a13b_arrival', 'a19_02', ('a19_02', 'a13b_00')),
    ('a13b', 'a13b_01', ('a13b_01', 'a13b_02', 'a13b_03', 'a13b_04', 'a13b_05')),
    ('a13c', 'a13c_00', beats_tool.CHAINS['a13c'][1]),
    ('a13d', 'a13d_00', beats_tool.CHAINS['a13d'][1][:-1]),
    ('a13d_hatch', 'a13d_07', ('a13d_07', 'a19b_00')),
    ('a19b', 'a19b_01', ('a19b_01', 'a19b_02')),
    ('a19c', 'a19c_00', beats_tool.CHAINS['a19c'][1]),
    ('a19d', 'a19d_00', beats_tool.CHAINS['a19d'][1]),
    ('a15a', 'area15', ('a15a_00',)),
    ('a15', 'a15_00', ('a15_00',)),
    ('a15b', 'a15b_00', beats_tool.CHAINS['a15b'][1]),
    ('a03', 'a03_00', ('a03_00',)),
    ('a03b', 'a03b_00', beats_tool.CHAINS['a03b'][1]),
    ('a08', 'a08_00', beats_tool.CHAINS['a08'][1]),
    ('a03c', 'a03c_00', ('a03c_00', 'a03c_01')),
    ('a03d', 'a03d_00', beats_tool.CHAINS['a03d'][1]),
    ('a04d', 'a04d_00', beats_tool.CHAINS['a04d'][1]),
    ('a07b', 'a07b_00', beats_tool.CHAINS['a07b'][1]),
)


def seconds_of(beats):
    """The beats' recorded length at 59.94 Hz."""
    first, last = beats_tool.trace(beats[0]), beats_tool.trace(beats[-1])
    return (last['first_counter'] + len(last['rows']) - first['first_counter']) / 59.94


def play_route(route, mode):
    until, rand = route
    print(f'New Game through {until}, one continuous run', flush=True)
    return subprocess.run([sys.executable, str(ROOT / 'tools/test_level_smoke_area01r.py'), '--run', '--until', until,
                           '--rand-replay', str(rand), '--out', str(OUT / 'route'), '--watch', mode], cwd=ROOT).returncode


def closed(results, seconds, timeout, log):
    """The port left on its own before the script's end: the viewer closed it
    (the runner stops a fail-stop's or a stalled run's port itself, and those
    have no shutdown lines)."""
    last = results[-1][1] or ['']
    return 'no end report' in last[0] and seconds < timeout - 1 and ' play(s), ' in log.read_text(errors='replace')


def play_section(name, card, beats):
    """Returns after the section's last beat, or when the viewer closed the
    window. A fail-stop, a death or a timeout goes on from the next beat's own
    card."""
    i = 0
    while i < len(beats):
        part = beats[i:]
        print(f'{name}: {part[0]}..{part[-1]} from EM_START_AT={card}', flush=True)
        # the boot, a load's drive, and a beat that runs long
        timeout = seconds_of(part) * 1.25 + 240
        results, seconds, log = beats_tool.run(name, card, part, timeout)
        for beat, problems in results:
            print(f'  {beat}: {"as recorded" if not problems else problems[0][:200]}', flush=True)
        if closed(results, seconds, timeout, log):
            print(f'{name}: the window closed', flush=True)
            return
        i += len(results)
        if i < len(beats):
            card = beats_tool.BEATS[beats[i]][0]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--boss', action='store_true', help='part 1 through the first boss fight (a04b_s7)')
    parser.add_argument('--from', dest='start', metavar='NAME', help='a section or a beat of part 2; skips part 1')
    parser.add_argument('--only', metavar='NAME', help='just this section or from this beat to its end')
    parser.add_argument('--no-window', action='store_true', help='draw offscreen, silent (a check of the runs)')
    args = parser.parse_args()
    if args.list:
        print(f'part 1: New Game through {ROUTE[0]} (--boss: through {BOSS_ROUTE[0]})')
        for name, card, beats in SECTIONS:
            print(f'{name:13} {seconds_of(beats) / 60:5.1f} min  EM_START_AT={card:9} {beats[0]}..{beats[-1]}')
        return 0
    if not beats_tool.BIN.exists() or not (ROOT / 'build/start_saves').is_dir():
        print('build/extermination or build/start_saves is missing: run make and make start-saves', file=sys.stderr)
        return 2
    beats_tool.OUT = OUT
    beats_tool.STALL = 90
    beats_tool.HOST = dict(EM_HEADLESS='1' if args.no_window else '0', EM_UNCAPPED='0', EM_HEADLESS_RASTER='1')
    sections, start = SECTIONS, args.only or args.start
    if start:
        found = [k for k, (name, _, beats) in enumerate(SECTIONS) if start == name or start in beats]
        if not found:
            print(f'watch_recording: no section or beat {start!r} (--list)', file=sys.stderr)
            return 2
        name, card, beats = SECTIONS[found[0]]
        if start != name:
            beats = beats[beats.index(start):]
            card = beats_tool.BEATS[start][0]
        sections = ((name, card, beats),)
        if not args.only:
            sections += SECTIONS[found[0] + 1:]
    else:
        code = play_route(BOSS_ROUTE if args.boss else ROUTE, 'offscreen' if args.no_window else 'window')
        if code:
            print(f'part 1 stopped (exit {code}): {OUT}/route/run.log', flush=True)
    for section in sections:
        play_section(*section)
    return 0


if __name__ == '__main__':
    sys.exit(main())
