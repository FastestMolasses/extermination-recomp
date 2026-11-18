#!/usr/bin/env python3
"""A world frame's slot-1 packets in the Original profile's GS field
(docs/GS_EXACT.md section 9 "Slot 1"): the weapon sight against the original.

001D2300 kicks a world frame's list as the head, slot 0, the chain page and
then slot 1, the 2D layer the sights, the Gatling's gauge bars and the credits'
underlines are built into. The port's GS frame now runs that segment after
the recorded body (em_status_gs_page em_sgp_world_slot1 / em_sgp_world_tail).

Check (default, ~15 s): the zoom scope start point (tools/build_start_saves.py
sight_zoom; the pad of tools/test_sight_live.py: R2, L1, R2, L2) is played
twice headless, with the GS field captured at the three frames the decomp's
fork capture holds (build/sights_ref/zoom: frame_aim040 zoom 0, frame_aim160
zoom 1.0, frame_aim300 zoom 0.5; port ticks 279, 401 and 541, in the
original's field phase), once as the game runs and once with EM_SLOT1_FIELD=0
(slot 1 left undrawn, the field as before). The pixels the two runs differ in
are the ones slot 1 drew: there the field must equal the original's
displayed field (at most SIGHT_OFF pixels off, from the world the scope's
glass blends over), and outside them both runs are the same (the slot-1
segment changes nothing else). The whole field's exact count is reported and
held to a floor (the world under the sight is the port's own, not part of
this check).

The Gatling's gauge bars (AREA21, chain g20 into g77's mount) were measured
once against a hidden fork capture of g77 (GS_EXACT.md section 9 "Slot 1");
a chain run takes minutes, so they are not part of this default check.

Output: build/slot1_field/ (disc-derived pictures: never committed).
"""
import json
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / 'build' / 'extermination'
OUT = ROOT / 'build' / 'slot1_field'
CAPTURE = ROOT.parent / 'Extermination' / 'build' / 'sights_ref' / 'zoom'
LOCK = 100
R2, L1, L2 = 0x200, 0x400, 0x100
# port tick -> the original's frame (same zoom, same field phase: OFY 1936.5)
POINTS = {279: 'frame_aim040', 401: 'frame_aim160', 541: 'frame_aim300'}
# slot-1 pixels allowed off the original (where the scope's translucent glass
# blends over the port's world, which differs there: measured 1, 2 and 152,
# GS_EXACT.md section 9 "Slot 1"), and the whole field's exact floor
SIGHT_OFF = {279: 4, 401: 8, 541: 400}
FLOOR = {279: 114000, 401: 109000, 541: 112800}
# The start point arrives in AREA16 by LOAD GAME, so the area-title card
# 001C5930 (its glyph runs, in slot 1 since GS_EXACT.md 9 "Slot 1") is still
# up at ticks 279 and 401; the capture was taken from g66's end state, long
# after the arrival, and shows none. The card's box (field columns 102..369,
# rows 18..26) is reported apart: the sight's pixels are judged outside it,
# the whole field's floor over the rest.
TITLE = {279: (18, 27, 102, 370), 401: (18, 27, 102, 370)}


def pad_script():
    a = LOCK
    rows = [(0, 0), (a, R2), (a + 101, R2 | L1), (a + 161, R2), (a + 231, R2 | L2), (a + 256, R2), (a + 320, 0)]
    return '\n'.join([f'EMA1 1 {a + 340} 1'] + [f'{f} {b:x} 127 127' for f, b in rows]) + '\n'


def run(tag, extra):
    out = OUT / tag
    out.mkdir(parents=True, exist_ok=True)
    pad = OUT / 'aim.pad'
    spec = ';'.join(f'{t}:{out / f"t{t}.bmp"}' for t in POINTS)
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(EM_START_AT='sight_zoom', EM_START_AT_PAD=str(pad), EM_HEADLESS='1', EM_MUTE='1',
               EM_UNCAPPED='1', EM_AREA_CHANGE_LOG=os.devnull, EM_FB_CAPTURE_TICKS=spec, **extra)
    for t in POINTS:
        for ext in ('', '.gsfield', '.present'):
            Path(str(out / f't{t}.bmp') + ext).unlink(missing_ok=True)
    p = subprocess.Popen([str(BIN)], cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    lines = []
    try:
        for line in p.stdout:
            lines.append(line.rstrip('\n'))
            if 'EM_START_AT_PAD: the script ended' in line or 'the game task is stopped' in line:
                break
    finally:
        p.kill()
        p.wait()
    (out / 'run.log').write_text('\n'.join(lines) + '\n')
    return lines


def field(path):
    return np.fromfile(path, np.uint8).reshape(224, 512, 4)[..., :3].astype(np.int16)


def original(name):
    from PIL import Image
    return np.array(Image.open(CAPTURE / name / 'original.png').convert('RGB')).astype(np.int16)


def main():
    if not BIN.is_file():
        print('test_slot1_field: build first (make)', file=sys.stderr)
        return 2
    if not CAPTURE.is_dir():
        print(f'SKIP slot-1 field: no capture {CAPTURE}')
        return 0
    if not (ROOT / 'build' / 'start_saves' / 'sight_zoom').is_dir():
        subprocess.run([sys.executable, str(ROOT / 'tools' / 'build_start_saves.py'), 'sight_zoom'], cwd=ROOT,
                       check=True, stdout=subprocess.DEVNULL)
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 'aim.pad').write_text(pad_script())
    t0 = time.time()
    with ThreadPoolExecutor(max_workers=2) as ex:
        on, off = ex.map(lambda a: run(*a), (('on', {}), ('off', {'EM_SLOT1_FIELD': '0'})))
    bad = []
    for tag, lines in (('on', on), ('off', off)):
        faults = [x for x in lines if 'fault' in x.lower() or 'refused' in x]
        if faults:
            bad.append(f'{tag}: faulted: ' + ' | '.join(faults[:2]))
    for t, name in POINTS.items():
        a, b = OUT / 'on' / f't{t}.bmp.gsfield', OUT / 'off' / f't{t}.bmp.gsfield'
        if not a.exists() or not b.exists():
            bad.append(f'tick {t}: no field captured')
            continue
        new, old, orig = field(a), field(b), original(name)
        drawn = (new != old).any(axis=2)
        exact = (new == orig).all(axis=2)
        box = np.zeros(drawn.shape, bool)
        title = ''
        if t in TITLE:
            y0, y1, x0, x1 = TITLE[t]
            box[y0:y1, x0:x1] = True
            card = int((drawn & box).sum())
            title = f'; the area-title card drew {card} pixels in its box (not in the capture)'
            if card < 300:
                bad.append(f'tick {t}: the area-title card is not on the field ({card} pixels)')
        drawn &= ~box
        n, off_px = int(drawn.sum()), int((drawn & ~exact).sum())
        worst = int(np.abs(new - orig).max(axis=2)[drawn].max()) if n else 0
        whole = int((exact & ~box).sum())
        print(f'  tick {t} ({name}): slot 1 drew {n} pixels, {n - off_px} equal to the original '
              f'({off_px} off, worst {worst}); whole field {whole} of {exact.size - int(box.sum())} exact '
              f'(before {int(((old == orig).all(axis=2) & ~box).sum())}){title}')
        if n < 60000:
            bad.append(f'tick {t}: slot 1 drew only {n} pixels (the sight is not on the field)')
        if off_px > SIGHT_OFF[t]:
            bad.append(f'tick {t}: {off_px} slot-1 pixels off the original (at most {SIGHT_OFF[t]})')
        if whole < FLOOR[t] - int(box.sum()):
            bad.append(f'tick {t}: {whole} exact pixels, under the floor {FLOOR[t]}')
    for x in bad:
        print('  FAIL ' + x)
    print(f'slot-1 field: {"FAIL" if bad else "PASS"} ({time.time() - t0:.0f} s)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
