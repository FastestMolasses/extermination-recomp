#!/usr/bin/env python3
"""Capture the ORIGINAL's attract demo (001AC070 state 3, anim_frame_top_a at
001ACA20) in a hidden fork session (docs/ATTRACT_DEMO.md).

From the fork state slot01_title (the title fading in, 001AC070 state 2) the
pad stays released until the title's idle countdown hands over to the demo
(001AC480 returns 3, +8 = 3). Every main-loop top from then on is one row of
<out>/rows.jsonl: the frame-task slot 0 record (0x28A750, +8.. bytes), the
fade machine D_0028A9A0, the scratchpad words 0x70003B60..0x70003B93, the
demo counter D_00275BD4 and the busy bytes D_00275BD8 / BE0, the area block
D_00810700, the pad block D_00810E50..E7F (the demo's 001B5BC0 writes E70,
E72, E74, E76, E64, E65, E57), the request block D_008106B0, the player's
position / yaw / action and the rand() state. Rows stop when the demo has
returned to the title (+8 = 2) `--demos` times (each demo ends after 0xE10
pad frames unless the world ends it first). A snapshot (eeMemory.bin etc.)
is saved under <out>/snap_<n>_sub1 at the first frame of each demo's sub 1.

Outputs go under the ignored build/demo_ref/ (disc-derived RAM; never
committed). The session takes the run lock build/.pcsx2.lock of the decomp
and shuts its emulator down on exit; nothing is left running.
"""
from __future__ import annotations

import argparse
import base64
import json
import struct
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = Path('/Users/abe/Documents/Extermination.nosync/Extermination')
sys.path.insert(0, str(DECOMP / 'tools'))
from pcsx2_session import ForkSession, fork_state  # noqa: E402

SPANS = (
    ('task', 0x28A750, 0x20),
    ('fade', 0x28A9A0, 0x10),
    ('spad', 0x70003B60, 0x34),
    ('bd4', 0x275BD4, 0x10),
    ('area', 0x810700, 0x4),
    ('pad', 0x810E50, 0x30),
    ('req', 0x8106B0, 0x48),
    ('player', 0x8102B0 + 0xA0, 0x2C),
    ('action', 0x8102B0 + 0x1F0, 0x4),
    ('rand', 0x2426C8, 0x4),
    ('d810750', 0x810750, 0x4),
    ('stream', 0x28A6AC, 0x4),
)


def read_row(s) -> dict:
    return {name: s.read(a, n).hex() for name, a, n in SPANS}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--out', default=str(DECOMP / 'build/demo_ref/run1'))
    ap.add_argument('--demos', type=int, default=1, help='demos to record (the title idles between them)')
    ap.add_argument('--max-frames', type=int, default=40000)
    ap.add_argument('--snap', action='store_true', help='save a snapshot at each demo sub-1 start')
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    rows = (out / 'rows.jsonl').open('w')
    t0 = time.time()
    with ForkSession(fork_state('slot01_title'), log_dir=out / 'log', boundary_timeout=120.0) as s:
        s.pad(0)
        frame = 0
        demos = 0
        in_demo = False
        snapped = False
        idle = 0
        while frame < args.max_frames:
            s._resume_to_boundary()
            frame += 1
            task = s.read(0x28A750, 0x20)
            state = task[8]
            if not in_demo and state == 3 and struct.unpack_from('<I', task, 4)[0] == 0x1AC070:
                in_demo = True
                snapped = False
                print(f'demo {demos}: begins at session frame {frame} (idle {idle})', flush=True)
            if in_demo:
                row = read_row(s)
                row['frame'] = frame
                row['demo'] = demos
                rows.write(json.dumps(row) + '\n')
                if args.snap and not snapped and task[9] == 4 and task[0xB] == 1:
                    s.snapshot(out / f'snap_{demos}_sub1')
                    snapped = True
                if state != 3:
                    demos += 1
                    in_demo = False
                    print(f'demo {demos - 1}: ends at session frame {frame}, +8 = {state}', flush=True)
                    if demos >= args.demos:
                        break
            else:
                idle += 1
            if frame % 1000 == 0:
                print(f'  frame {frame} ({time.time() - t0:.0f} s) task {task[4:12].hex()}', flush=True)
    rows.close()
    print(f'wrote {out / "rows.jsonl"} ({frame} frames, {time.time() - t0:.0f} s)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
