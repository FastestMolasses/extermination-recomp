#!/usr/bin/env python3
"""The ORIGINAL boot title's LOAD GAME screen (PCSX2 fork, hidden): one
snapshot of the memory-card slot choice, for the port's picture of the same
screen (docs/OPTIONS.md section 4.5; em_frontend's EM_STARTUP_TEST=loadgame
with EM_STARTUP_CAPTURE_DIR writes load_screen.bmp).

Policy "title_load_v1", built on tools/capture_title_options.py's Capture
(its rows, waits, IDLE = 10 and two-step presses): from the fork state
slot01_title (the title fading in)
  1. wait TITLE; DOWN.
  2. wait TITLE and TF==1; CROSS (001AC070 state 5, 00225AC0(0) every tick).
  3. wait SLOT: the card record D_00810040's +0 == 1 and +0x15 == 1 and the
     fade D_0028A9A0 == 0 (the slot choice idle, the condition em_frontend's
     load_screen picture waits for); IDLE; SNAPSHOT "load_screen"; TRIANGLE.
  4. wait TITLE; 30 idle steps; end.

Outputs (disc-derived, gitignored, never committed):
  ../Extermination/build/title_load_ref/trace.json and load_screen/.

Usage (port root; native arm64 macOS, the fork x86_64 under Rosetta; the
decomp's .venv python):
  ../Extermination/.venv/bin/python tools/capture_title_load.py
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import capture_title_options as C  # noqa: E402

OUT = C.DECOMP / 'build/title_load_ref'
POLICY = 'title_load_v1'


def slot_choice(c):
    card = c.s.read(0x00810040, 0x18)
    return card[0] == 1 and card[0x15] == 1 and c.rows[-1]['fade'] == 0


def main():
    if OUT.exists():
        sys.exit(f'{OUT} exists: remove it first (a new capture never overwrites one)')
    OUT.mkdir(parents=True)
    C.OUT = OUT   # Capture.snap writes under C.OUT
    state = C.fork_state('slot01_title')
    c, error = None, None
    with C.ForkSession(state, log_dir=OUT / 'logs', mtvu=False, lock_wait=4 * 3600.0) as s:
        try:
            c = C.Capture(s)
            c.do(c.title, 'TITLE', 'DOWN')
            c.do(lambda: c.title() and c.t(0xF) == 1, 'TITLE and TF==1', 'CROSS')
            c.do(lambda: slot_choice(c), 'SLOT', 'TRIANGLE', 'load_screen')
            c.wait(c.title, 'TITLE (after exit)')
            c.idle(30)
        except Exception as e:  # keep what was recorded
            error = f'{type(e).__name__}: {e}'
        finally:
            if c is not None:
                meta = dict(policy=POLICY, source_state=str(state), snapshots=c.snaps,
                            presses=c.presses, notes=c.notes, error=error, frames=len(c.rows) - 1,
                            pad_interference=s.pad_interference, rows=c.rows)
                (OUT / 'trace.json').write_text(json.dumps(meta) + '\n')
    if error:
        sys.exit(error)
    print(f'{len(c.rows) - 1} frames, snapshots {c.snaps}, notes {c.notes}')


if __name__ == '__main__':
    main()
