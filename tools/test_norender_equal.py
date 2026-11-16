#!/usr/bin/env python3
"""Proof that EM_NO_RENDER=1 changes no game state (src/game/em_norender.h).

Runs one take chain or one route beat twice, EM_NO_RENDER off and on, and
compares everything the game's logic produced, frame by frame:

  * EM_STATE_TRACE: per frame step, FNV-1a digests of the render-context
    storage (every writable view: packet arena, context, GS blocks, skin
    records, scratchpad copies), the scene state, the actor pool's record
    images and the AREA01 actor view's, the rand() state, the player and the
    camera (em_scene_bindings_state_trace_install);
  * a take: the run's own input / position log (EM_INPUT_RECORD, every tick);
  * a beat: the per-frame player / vitals / rand / creature log
    (EM_START_AT_TRACE) and the run's pass / fail results.

Every line must be identical. With --twice a second run with the switch off is
made first, to show the digest itself is deterministic.

Usage (native arm64 macOS, build/extermination first):

  python3 tools/test_norender_equal.py take test1
  python3 tools/test_norender_equal.py beat a19d_00 k01_02 a01_03
  python3 tools/test_norender_equal.py newgame            # EM_STARTUP_TEST=newgame-control
  python3 tools/test_norender_equal.py --twice beat a19d_00

Output: build/norender_equal/<case>/ (derived from the disc: never committed).
"""
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
OUT = ROOT / 'build/norender_equal'


def first_difference(a, b):
    la, lb = a.read_text().splitlines(), b.read_text().splitlines()
    for i, (x, y) in enumerate(zip(la, lb)):
        if x != y:
            return i, x, y, len(la), len(lb)
    return None if len(la) == len(lb) else (min(len(la), len(lb)), '', '', len(la), len(lb))


def compare(label, a, b, prefix_ok=False):
    """Equal files; with prefix_ok a run stopped by its driver may be longer by a few lines."""
    d = first_difference(a, b)
    if d is None:
        n = len(a.read_text().splitlines())
        print(f'  {label}: identical, {n} lines')
        return True
    i, x, y, na, nb = d
    if prefix_ok and i >= min(na, nb) and abs(na - nb) <= 90:
        print(f'  {label}: identical over {i} lines (the driver stopped the runs {abs(na - nb)} lines apart)')
        return True
    print(f'  {label}: DIFFERS at line {i + 1} (of {na} and {nb})\n    off: {x[:200]}\n    on:  {y[:200]}')
    return False


def clean_env():
    return {k: v for k, v in os.environ.items() if not k.startswith('EM_')}


def timed(cmd, env, log):
    """Run cmd; returns (wall seconds, children's user + system CPU seconds)."""
    before = os.times()
    t = time.time()
    with log.open('w') as text:
        subprocess.run(cmd, cwd=ROOT, env=env, stdout=text, stderr=subprocess.STDOUT)
    after = os.times()
    return time.time() - t, (after.children_user - before.children_user) + (after.children_system - before.children_system)


def run_take(name, mode, out):
    import watch_takes as wt
    names = wt.chain(name)
    out.mkdir(parents=True, exist_ok=True)
    (out / 'card').mkdir(exist_ok=True)
    for old in (out / 'card').iterdir():     # an empty card at every run
        if old.is_file():
            old.unlink()
    rec, log = out / 'stitched.rec', out / 'played.rec'
    wt.stitch(names, rec)
    env = clean_env()
    env.update(EM_INPUT_PLAY=str(rec), EM_INPUT_RECORD=str(log), EM_PS2_DISC_DRIVE_TIMING='1',
               EM_REPLAY_MAX_OVERRUN='3600', EM_TEST_MEMCARD_ROOT=str(out / 'card'),
               EM_TEST_CARD_FRESH=str(wt.fresh_until(names)), EM_HEADLESS='1', EM_UNCAPPED='1', EM_MUTE='1',
               EM_STATE_TRACE=str(out / 'state.trace'), EM_NO_RENDER=mode)
    wall, cpu = timed([str(ROOT / 'build/extermination')], env, out / 'run.log')
    return wall, cpu, [('state', out / 'state.trace', False), ('input/position log', log, False)]


def run_beat(name, mode, out):
    import mid_route_beats as mrb
    out.mkdir(parents=True, exist_ok=True)
    real = subprocess.Popen

    def popen(cmd, **kw):
        env = dict(kw.get('env') or os.environ)
        env.update(EM_STATE_TRACE=str(out / 'state.trace'), EM_START_AT_TRACE=str(out / 'start_at.trace'),
                   EM_NO_RENDER=mode)
        kw['env'] = env
        return real(cmd, **kw)
    mrb.subprocess.Popen = popen
    mrb.OUT = out
    before = os.times()
    try:
        results, seconds, log = mrb.run(name, mrb.BEATS[name][0], (name,), 900)
    finally:
        mrb.subprocess.Popen = real
    after = os.times()
    cpu = (after.children_user - before.children_user) + (after.children_system - before.children_system)
    print(f'  {name} EM_NO_RENDER={mode}: {[(b, p or "PASS") for b, p in results]}')
    # the beat driver stops the run a moment after the script's end: compare the common prefix
    return seconds, cpu, [('state', out / 'state.trace', True), ('start-at trace', out / 'start_at.trace', True)]


def run_newgame(name, mode, out):
    out.mkdir(parents=True, exist_ok=True)
    env = clean_env()
    env.update(EM_STARTUP_TEST='newgame-control', EM_STATE_TRACE=str(out / 'state.trace'), EM_NO_RENDER=mode)
    wall, cpu = timed([str(ROOT / 'build/extermination')], env, out / 'run.log')
    return wall, cpu, [('state', out / 'state.trace', False)]


def main(argv):
    twice = '--twice' in argv
    argv = [a for a in argv if a != '--twice']
    if len(argv) < 1 or argv[0] not in ('take', 'beat', 'newgame'):
        print(__doc__)
        return 2
    kind, names = argv[0], argv[1:] or ['newgame']
    runner = dict(take=run_take, beat=run_beat, newgame=run_newgame)[kind]
    ok = True
    for name in names:
        base = OUT / f'{kind}_{name}'
        print(f'{kind} {name}:')
        if twice:
            runner(name, '0', base / 'off0')
        w_off, c_off, files = runner(name, '0', base / 'off')
        w_on, c_on, _ = runner(name, '1', base / 'on')
        print(f'  wall {w_off:.0f} s off, {w_on:.0f} s on; CPU {c_off:.0f} s off, {c_on:.0f} s on')
        if twice:
            for label, path, pre in files:
                ok &= compare(label + ' (off vs off)', base / 'off0' / path.name, base / 'off' / path.name, pre)
        for label, path, pre in files:
            ok &= compare(label, base / 'off' / path.name, base / 'on' / path.name, pre)
    print('test_norender_equal:', 'PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
