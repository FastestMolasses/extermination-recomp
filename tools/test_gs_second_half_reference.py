#!/usr/bin/env python3
"""The second half's picture: the port's exact GS field against the original's at the same row.

docs/DRAW_LIST_AUDIT.md section 4. The original's side: tools/capture_gs_original.py
replays Claude's kept guided takes on the PCSX2 fork with the software renderer and
keeps the GS local memory at the loop tops after each row (decomp
build/gspics/orig/<take>/r<R>/, disc-derived, local only). The port's side: the
same takes as one continuous chain (tools/guided_chain.py g20: card g73 -> g74 ->
..., the pad alone) with the GS frame model drawing, EM_GS_DUMP_AT writing the
field of each chosen row (guided_chain.py --gs-dump, which also sets
EM_START_AT_FIELD so the fields are drawn on the recording's half line).
tools/compare_gs_frame.py pairs them (the original's buffer FRAME_1 names at loop
top R + 2).

Pinned per point: the count of pixels whose R, G, B are exact (any loss fails) and
the largest channel difference (at most 3: the GS model's open interpolation
items, GS_EXACT.md section 8; a draw that is missing, extra or wrong differs by far
more). What the points cover:
  - g76 row 1300 (AREA21 entry 4, the deck): the weather node (+0x2520) with
    001E67C0's reversed rows (flags bit 1 for the area key 0x1500), the sky;
  - EM_TEST_FULL=1, through g79: g75 rows 2600 / 3500 (the arrival scene, letterbox
    and glyphs), g76 800, g77 3300 / 5000 / 8000 (the sea grid +0x1E8 with the snow,
    the Gatling's sight and gauge bar), g73 2700 / 3500 (AREA20, the area title),
    g79 1718 / 3131 (the grenade launcher's sight while aiming); and, from chain g18
    (card g41, LOAD GAME into AREA14, then AREA18), g42 rows 300 / 2400 (the player's
    hair: the area load's 001B07C0 sends the costume-0 texture packet).
  g73 row 3500 also pins the alpha (R, G, B and A exact): an NPC's drop shadow is
  drawn at its place among the actor walk's units, so its alpha strip no longer
  clears the units drawn after it.

Default run: the chain through g76 row 1300 (about 14k frames with the raster, a few
minutes: its own timeout in tools/test_sweep.py). Skips (exit 0) when the original's
captures are not on this machine.
"""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
import compare_gs_frame as cmp  # noqa: E402

NAME = 'gs_second_half_test'
# point -> (RGB-exact pixels of 114,688, largest channel difference[, RGBA-exact pixels]),
# measured 2026-11-19 (g73, g79, g42: lane PORT-GS-PICS2; g73 2700 keeps its open
# shadow edge, g73 3500 17 open pixels, DRAW_LIST_AUDIT.md section 4)
QUICK = {'g76_area21_prep:1300': (114664, 1)}
FULL = {
    'g73_area04_area20:2700': (114622, 53), 'g73_area04_area20:3500': (114671, 171, 114671),
    'g75_area20_area21:2600': (114622, 3), 'g75_area20_area21:3500': (114626, 3),
    'g76_area21_prep:800': (114652, 2), 'g76_area21_prep:1300': (114664, 1),
    'g77_area21_phase1:3300': (114677, 1), 'g77_area21_phase1:5000': (114641, 1),
    'g77_area21_phase1:8000': (114142, 3),
    'g79_area21_phase3:1718': (114566, 2), 'g79_area21_phase3:3131': (114581, 4),
}
# chain g18 (card g41; guided_chain --steer g41): FULL only
FULL_G18 = {'g42_area18_area14:300': (114643, 4), 'g42_area18_area14:2400': (114550, 2)}


def run_chain(chain, name, points, extra=()):
    until = max(points, key=lambda k: k.split('_')[0]).split('_')[0]
    dump = ','.join(f'{k.split("_")[0]}:{k.split(":")[1]}' for k in points)
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    cmd = [sys.executable, str(ROOT / 'tools/guided_chain.py'), chain, *extra, '--until', until, '--name', name,
           '--gs-dump', dump]
    p = subprocess.run(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode:
        print(p.stdout[-3000:])
        sys.exit(f'guided_chain {chain} failed')
    failures = []
    for key, pin in points.items():
        floor, worst = pin[:2]
        take, _, row = key.rpartition(':')
        r = cmp.compare_point(name, take, int(row), write=False)
        print(cmp.line(r))
        if r.get('rgb_equal', 0) < floor or r.get('max_diff', 999) > worst:
            failures.append(f'{key}: {r.get("rgb_equal")} exact (pinned {floor}), max diff {r.get("max_diff")} '
                            f'(pinned {worst})')
        if len(pin) > 2 and r.get('rgba_equal', 0) < pin[2]:
            failures.append(f'{key}: {r.get("rgba_equal")} RGBA exact (pinned {pin[2]})')
    return failures


def main():
    points = FULL if reference_mode.FULL else QUICK
    g18 = FULL_G18 if reference_mode.FULL else {}
    missing = [k for k in [*points, *g18]
               if not (cmp.ORIG / k.split(':')[0] / f'r{k.split(":")[1]}' / 's2.vram').exists()]
    if missing:
        print(f'SKIP: no original capture for {", ".join(missing)} (tools/capture_gs_original.py)')
        return 0
    failures = run_chain('g20', NAME, points)
    if g18:
        failures += run_chain('g18', NAME + '_g18', g18, ('--steer', 'g41'))
    reference_mode.banner(reference_mode.part(len(points) + len(g18), len(FULL) + len(FULL_G18),
                                              'points compared pixel for pixel'))
    if failures:
        print('\n'.join(failures))
        return 1
    print(json.dumps({'status': 'PASS', 'mode': reference_mode.MODE, 'points': len(points)}))
    return 0


if __name__ == '__main__':
    sys.exit(main())
