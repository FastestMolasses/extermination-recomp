#!/usr/bin/env python3
"""The world clock of a recorded take, for tools/watch_takes.py.

A take played since the clock column holds it; this is for an older take. Its take.rec
holds rand()'s state per tick but not the world clock 0x70003B68, which the bugs' timers
read. A replay of the take on the original, hidden, reads it per tick (the
decomp's record_original_play.py rows do not hold it). A cutscene that ends a tick
early in the port leaves the clock one behind for good, so the playback follows it as
it follows rand().

Usage (native arm64 macOS; the fork app runs x86_64 under Rosetta, hidden, through the
run lock; use the decomp's .venv python; about a third of the take's length):

  ../Extermination/.venv/bin/python tools/take_clock.py run1

Output: build/takes/<take>/clock.txt, "counter clock" per tick, derived from the disc:
never committed.
"""
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
sys.path.insert(0, str(DECOMP / 'tools'))
import record_original_play as play  # noqa: E402

import numpy as np  # noqa: E402

SPANS = [dict(name='counter', src='ee', addr='0x70003B64', len=4), dict(name='clock', src='ee', addr='0x70003B68', len=4)]


def main():
    name = sys.argv[1]
    take = play.OUT / name
    start, end = (json.loads((take / f).read_text()) for f in ('start.json', 'end.json'))
    out = ROOT / 'build/takes' / name
    out.mkdir(parents=True, exist_ok=True)
    rows = play.records(take)
    with play.Session(lock_wait=play.LOCK_WAIT) as s:
        cfg = play.replay_config(Path(start['state']))
        play.use_card(s, take, cfg)
        inst = s.launch('clock', cfg)
        c = inst.client()
        c.wait_vm(120)
        c.call('halt')
        c.call('set_tick_pc', pc=play.session.LOOP_TOP)
        c.call('table_add', name='clock', path='clock.tbl', trigger='tick', spans=SPANS)
        c.call('input_timeline', entries=play.timeline(rows), index='vsync', mode='replace')
        c.call('run', timeout=86400 + 60, until={'vsyncs': end['vsync'] - start['vsync']}, timeout_s=86400)
        c.call('table_remove', name='clock')
        root = Path(inst.info().get('root') or inst.scratch / 'root')
        shutil.copyfile(root / 'clock.tbl', out / 'clock.tbl')
        shutil.copyfile(root / 'clock.tbl.json', out / 'clock.tbl.json')
    table = play.Table(out / 'clock.tbl').numpy()
    counter = np.frombuffer(table['counter'].tobytes(), '<u4')
    clock = np.frombuffer(table['clock'].tobytes(), '<u4')
    (out / 'clock.txt').write_text(''.join(f'{a} {b}\n' for a, b in zip(counter.tolist(), clock.tolist())))
    print(f'{name}: {len(counter)} ticks, build/takes/{name}/clock.txt')


if __name__ == '__main__':
    sys.exit(main())
