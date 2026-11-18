#!/usr/bin/env python3
"""Proof that the fast-forward start (EM_FAST_UNTIL, src/game/em_fast.h) changes
no game state and switches to the normal presentation at the chosen tick.

One recording (Claude's guided chain: the title, New Game, then a point inside
the AREA11 take) is played twice, headless and with no window:

  plain  EM_NO_RENDER=1, uncapped (what `watch_takes.py --check` runs);
  fast   EM_FAST_UNTIL=<segment>:<offset> with EM_UNCAPPED=0 (paced after the
         switch), the picture's work skipped until the switch, the playback
         continuing 120 ticks past it.

Everything the game's logic produced must be identical, frame by frame: the
state digests (EM_STATE_TRACE) and the playback log (every tick's pad, area,
position and heading). The fast run must report the switch, run its ticks
after it at the 59.94 Hz pace (about 2 s), and with EM_REPLAY_CAPTURE its
captures must start after the switch (none is taken while skipping).

Usage (native arm64 macOS, build/extermination first; needs the decomp's
build/guided_play):

  python3 tools/test_fast_from.py

Output: build/fast_from/ (derived from the disc: never committed).
"""
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import watch_takes as wt

GUIDED = ROOT.parent / 'Extermination/build/guided_play'
OUT = ROOT / 'build/fast_from'
NAMES = ['g01_title_newgame', 'g02_area11']
AFTER = 90              # ticks played past the switch


def run(label, extra, rec, out):
    out.mkdir(parents=True, exist_ok=True)
    (out / 'card').mkdir(exist_ok=True)
    for old in (out / 'card').iterdir():
        if old.is_file():
            old.unlink()
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(EM_INPUT_PLAY=str(rec), EM_INPUT_RECORD=str(out / 'played.rec'), EM_PS2_DISC_DRIVE_TIMING='1',
               EM_REPLAY_MAX_OVERRUN='3600', EM_TEST_MEMCARD_ROOT=str(out / 'card'),
               EM_TEST_CARD_FRESH=str(wt.fresh_until(NAMES)), EM_HEADLESS='1', EM_MUTE='1',
               EM_STATE_TRACE=str(out / 'state.trace'))
    env.update(extra)
    log = out / 'run.log'
    began = time.monotonic()
    switched = None
    with log.open('w') as text:
        proc = subprocess.Popen([str(ROOT / 'build/extermination')], cwd=ROOT, env=env, stdout=text,
                                stderr=subprocess.STDOUT)
        while proc.poll() is None:
            time.sleep(0.05)
            if switched is None and 'fast: reached' in log.read_text(errors='replace'):
                switched = time.monotonic()
        end = time.monotonic()
    return (end - began), (end - switched if switched else None), log.read_text(errors='replace')


def same(label, a, b):
    la, lb = a.read_text().splitlines(), b.read_text().splitlines()
    for i, (x, y) in enumerate(zip(la, lb)):
        if x != y:
            print(f'  {label}: DIFFERS at line {i + 1}\n    plain: {x[:160]}\n    fast:  {y[:160]}')
            return False
    if len(la) != len(lb):
        print(f'  {label}: lengths differ ({len(la)} and {len(lb)})')
        return False
    print(f'  {label}: identical, {len(la)} lines')
    return True


def pictures(rec, seg, o):
    """The frames drawn after the switch equal a plain run's at the same ticks (every 30th
    tick of a segment is captured; the fast run takes none until the switch)."""
    shots = {}
    for label, extra in (('plain', {}), ('fast', dict(EM_FAST_UNTIL=f'{seg}:{o}'))):
        out = OUT / f'shots_{label}'
        out.mkdir(parents=True, exist_ok=True)
        for old in out.iterdir():
            if old.is_file():
                old.unlink()
        run(label, dict(EM_REPLAY_CAPTURE=str(out / 'shot_%u.png'), EM_REPLAY_CAPTURE_EVERY='30',
                        **{'EM_UNCAPPED': '1', **extra}), rec, out)
        shots[label] = sorted((p for p in out.glob('shot_*.png')), key=lambda p: int(p.stem.split('_')[1]))
    n = min(len(shots['plain']), len(shots['fast']))
    pairs = list(zip(shots['plain'][-n:], shots['fast'][-n:]))
    bad = [b.name for a, b in pairs if a.read_bytes() != b.read_bytes()]
    print(f'  pictures after the switch: {n} captures, {"identical" if not bad else "DIFFER " + ", ".join(bad)}')
    return n > 0 and not bad


def main():
    if not (GUIDED / 'chain.json').exists():
        print('test_fast_from: SKIP (the decomp\'s build/guided_play is missing)')
        return 0
    wt.use_root(GUIDED)
    OUT.mkdir(parents=True, exist_ok=True)
    rec = OUT / 'stitched.rec'
    marks = {}
    wt.stitch(NAMES, rec, None, marks)
    off, _ = wt.chain_vsyncs(NAMES)
    seg, o, idx = wt.target_of(NAMES, marks, off['g02_area11'] + 240)
    wt.stitch(NAMES, rec, sum(p is not None for p in marks['_pos'][:idx + 1]) + AFTER)
    print(f'fast start at segment {seg} offset {o} (stitched row {idx}), {AFTER} ticks after it:')
    ok = True
    w0, _, _ = run('plain', dict(EM_NO_RENDER='1', EM_UNCAPPED='1'), rec, OUT / 'plain')
    w1, after, log = run('fast', dict(EM_FAST_UNTIL=f'{seg}:{o}', EM_UNCAPPED='0'), rec, OUT / 'fast')
    print(f'  wall {w0:.1f} s plain, {w1:.1f} s fast; {after if after is not None else float("nan"):.2f} s after the switch')
    ok &= f'fast: reached segment {seg} offset {o}' in log
    if not ok:
        print('  the fast run never reported the switch')
    ok &= same('state digests', OUT / 'plain/state.trace', OUT / 'fast/state.trace')
    ok &= same('playback log', OUT / 'plain/played.rec', OUT / 'fast/played.rec')
    paced = after is not None and after >= AFTER / 59.94 * 0.8
    print(f'  paced after the switch: {"yes" if paced else "NO"} ({AFTER} ticks in {after if after else 0:.2f} s, '
          f'{AFTER / 59.94:.2f} s at 59.94 Hz)')
    ok &= paced
    if os.environ.get('EM_TEST_FULL'):
        ok &= pictures(rec, seg, o)
    print('test_fast_from:', 'PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
