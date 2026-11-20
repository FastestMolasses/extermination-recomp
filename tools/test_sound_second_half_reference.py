#!/usr/bin/env python3
"""The second half's sound: the port's EE sound state and RPC 0x64 exchanges against the original's.

docs/SOUND_SECOND_HALF.md. The original's side: tools/capture_sound_original.py
replays Claude's kept guided takes on the PCSX2 fork and samples, inline, the
sound tables at every main-loop top and the queue at every exchange (decomp
build/sndpics/orig/<take>/r<R0>_<R1>/, disc-derived, local only). The port's
side: EM_SOUND_DUMP, keyed by the recording's engine counter. tools/
compare_sound_ticks.py pairs them tick by tick (its docstring lists the keys and
the normalisations: voice numbers, serials and sample addresses are history).

Pinned per window: the number of paired ticks, and the count of ticks at which
each key differs (0 for every key where the port equals the original; an open
item keeps its measured count, SOUND_SECOND_HALF.md "Open", so a change in
either direction fails).

Default run (quick, about a minute): chain g20 (card g73, LOAD GAME into AREA04,
the pad alone) to g73 row 2680 with --snd-dump g73:2440-2680: AREA20's arrival
scene after the area load, against the original's g73 rows 2080..2680.
EM_TEST_FULL=1: the whole guided run from the title in one process
(tools/watch_takes.py --check --root .../guided_play g01_title_newgame
--snd-dump ..., well over an hour) and every window of the results table.
Skips (exit 0) when the original's captures are not on this machine.
"""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
import compare_sound_ticks as cmp  # noqa: E402

ORIG = ROOT.parent / 'Extermination/build/sndpics/orig'
NAME = 'sound_second_half_test'
# window -> (paired ticks, {key: ticks that differ}); keys not named must be equal at every tick.
# Measured 2026-11-19 (lane PORT-SND2; the differences are SOUND_SECOND_HALF.md section 5's open items:
# the SFX-side IOP tick phase, the music refill read phase, stale delayed-cue records).
# The quick window starts after the arrival's load (rows 2080..2426 are the load, which a card
# chain does not play on the recording's frames) and keeps the card start's one open key: the
# delayed cues D_00281F30 hold what the continuous run's earlier areas left (stale, cue -1).
QUICK = {'g73_area04_area20:2080-2680': (241, {'f30': 241})}
QUICK_DUMP = 'g73:2440-2680'
# EM_TEST_FULL: the continuous run with --iop-align (lane PORT-SFX-PHASE, SOUND_SECOND_HALF.md 5.1);
# the windows from g60 on are not reached: the run faults in AREA16 with the dump on (5.7)
FULL = {
    'g02_area11:100-160': (61, {}),
    'g12_area19_area15:16485-17085': (601, {'f30': 601, 'vo': 2, 'envx': 2}),
    'g40_area14_booster:4000-4600': (601, {'lane': 4, 'f30': 601, 'cur': 93}),
    'g42_area18_area14:0-600': (601, {'lane': 8, 'f30': 601, 'cur': 234, 'ser': 47, 'vo': 59, 'envx': 57,
                                      'tlr': 38, 'xsfx': 1}),
    'g46_area17_ramp:0-600': (601, {'b70': 180, 'c30': 180, 'f30': 601, 'cur': 184, 'vo': 186, 'envx': 186,
                                    'tlr': 185}),
    'g60_area06_area16:1334-1761': None,
    'g73_area04_area20:2080-2680': None,
    'g75_area20_area21:1884-2484': None,
    'g77_area21_phase1:1000-1600': None,
    'g80_area21_ending:100-700': None,
    'g80_area21_ending:21000-21600': None,
}


def orig_dir(key):
    take, _, span = key.partition(':')
    return ORIG / take / ('r' + span.replace('-', '_'))


def check(port, points):
    failures = []
    for key, pin in points.items():
        r = cmp.compare(port, orig_dir(key), quiet=True)
        if pin is None:
            got = {k: n for k, n in r['differ'].items() if n}
            print(f'{key}: {r["paired"]} paired (not pinned); differing {got}')
            continue
        paired, differ = pin
        got = {k: n for k, n in r['differ'].items() if n}
        print(f'{key}: {r["paired"]} paired; ' + (', '.join(f'{k} {n} (first {r["first"][k]})'
                                                          for k, n in got.items()) or 'every key equal'))
        if r['paired'] != paired:
            failures.append(f'{key}: {r["paired"]} ticks paired (pinned {paired})')
        if got != differ:
            failures.append(f'{key}: differing keys {got} (pinned {differ})')
        if r.get('map_conflicts'):
            failures.append(f'{key}: {r["map_conflicts"]} voice-map conflicts')
    return failures


def main():
    points = FULL if reference_mode.FULL else QUICK
    missing = [k for k in points if not (orig_dir(k) / 'snd.tbl').exists()]
    if missing:
        print(f'SKIP: no original capture for {", ".join(missing)} (tools/capture_sound_original.py)')
        return 0
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    if reference_mode.FULL:
        dump = ','.join(f'{k.split("_")[0]}:{k.split(":")[1]}' for k in points)
        cmd = [sys.executable, str(ROOT / 'tools/watch_takes.py'), '--check', '--root',
               str(ROOT.parent / 'Extermination/build/guided_play'), 'g01_title_newgame', '--snd-dump', dump,
               '--iop-align']
        port = ROOT / 'build/sndpics/watch_g01_title_newgame/port/sound.jsonl'
    else:
        cmd = [sys.executable, str(ROOT / 'tools/guided_chain.py'), 'g20', '--until', 'g73', '--name', NAME,
               '--snd-dump', QUICK_DUMP, '--iop-align']
        port = ROOT / 'build/sndpics' / NAME / 'port/sound.jsonl'
    if port.exists():
        port.unlink()
    p = subprocess.run(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if not port.exists():
        print(p.stdout[-3000:])
        sys.exit('the port wrote no sound dump')
    failures = check(port, points)
    reference_mode.banner(reference_mode.part(len(points), len(FULL), 'windows compared tick for tick'))
    if failures:
        print('\n'.join(failures))
        return 1
    print(json.dumps({'status': 'PASS', 'mode': reference_mode.MODE, 'windows': len(points)}))
    return 0


if __name__ == '__main__':
    sys.exit(main())
