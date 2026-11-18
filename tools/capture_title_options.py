#!/usr/bin/env python3
"""The ORIGINAL title screen's OPTIONS menu, page by page (PCSX2 fork, hidden).

Loads the fork state slot01_title (the title fading in) in a hidden
ForkSession (run lock build/.pcsx2.lock, waited for, never forced), drives the
title OPTIONS menu with a closed-loop policy, samples one row after every frame
step (main-loop top) and snapshots each page.  This docstring is the reference
the port test replays: policy version "title_options_v1".

Row fields: f, counter (0x70003B64), task (24 bytes 0x0028A758..0x0028A76F:
the title task record 0x0028A750, +8..+0x1F), set (16 bytes at
0x00810118), fade (s16 0x0028A9A0), busy (u8 0x00275BD8), phase (u8
0x00282157), off (s16 0x70003B94, 0x70003B96), masks (16 bytes 0x70003B74),
pad (u16 0x00810E6A mode, 0x00810E70 held, 0x00810E74 pressed, 0x00810E78
repeat), e50 (u8 0x00810E50), e56 (u8 0x00810E56), mono (u8 0x0028215B), press
(button names held during that step).  Row f=0 is the aligned start; row f=k
is sampled after step k.

Policy (task bytes: T8 +8, T9 +9, TF +0xF, TC +0xC, TD +0xD, CUR halfword
+0x1C; ACT = the row-action word table at 0x00264EC0 indexed by CUR, none when
CUR >= 7; MENU = T8==6 and TC==4 and fade==0; TITLE = T8==2 and T9==2 and
fade==0):
  "wait X; P" = step until X holds on the newest row (0 steps if it already
  does), then IDLE=10 more steps, then hold P for exactly 2 steps (released for
  the next step), then step until task+set+off+masks differ from the row
  sampled just before the press; the 2 press steps count, at most 90 steps in
  all (no change by then is logged as "no_change", not an error).  In the
  v1 capture this wait always ended inside the press steps themselves (the
  task bytes change every frame or two), before the press reaches the game.  Any wait
  over 2000 steps aborts.  A page SNAPSHOT is taken after the IDLE steps and
  before the press.
  1. wait TITLE; DOWN.  2. wait TITLE; DOWN (TF becomes 2 three steps after
     the press starts; step 3's wait enforces it).
  3. wait TITLE and TF==2; CROSS.
  4. For target 0..6 (0 skipped when the pad mode at 0x00810E6A is not 7):
     loop { wait MENU; if ACT == target: break; IDLE; DOWN; change }, then
     IDLE; CROSS (that is "wait MENU; CROSS"; for target 6 the snapshot "menu"
     is taken before this CROSS and the exit is the whole page).  Pages:
       0 vibration, 1 sound: wait TC==5 and TD==1; RIGHT;
                             wait TC==5 and TD==1; SNAPSHOT; CROSS.
       2 screen position:    wait TC==7; LEFT; wait TC==7; UP;
                             wait TC==7; SNAPSHOT; CROSS.
       3 brightness:         wait TC==8; SNAPSHOT; CROSS.
       4 button config:      wait TC==9 and TD==1; RIGHT; wait TC==9; SNAPSHOT; CROSS.
       5 default prompt:     wait TC==6 and TD==1; SNAPSHOT; CROSS (keeps the settings).
  5. wait TITLE; 30 idle steps; end.
The record is read at 0x0028A750 always.  The u32 at 0x70003B6C is logged
(`record`) but not followed: at the main-loop top it is 0x0028A7B0, the next
task slot, not the title task (the first run followed it, never saw TITLE and
idled into the attract demo, whose start resets the main-loop counter).
Pad latency on the fork: a pad set before step N first shows in 0x00810E70
after step N+2.  VU1 runs on the EE thread (mtvu off) so GS memory is the same
in every run; the renderer is the session default.

Outputs (disc-derived, gitignored, never committed):
  ../Extermination/build/options_ref/trace.json and one folder per snapshot
  (title_start, p0_vibration, p1_sound, p2_screen_position, p3_brightness,
  p4_button_config, p5_default, menu).

Usage (port root; native arm64 macOS, the fork x86_64 under Rosetta; the
decomp's .venv python, which has zstandard for the snapshots):
  ../Extermination/.venv/bin/python tools/capture_title_options.py
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
sys.path.insert(0, str(DECOMP / 'tools'))
from pcsx2_session import ForkSession, fork_state  # noqa: E402

OUT = DECOMP / 'build/options_ref'
POLICY = 'title_options_v1'
IDLE, PRESS, CHANGE_LIMIT, WAIT_LIMIT = 10, 2, 90, 2000
RECORD = 0x0028A750
PAGES = ['p0_vibration', 'p1_sound', 'p2_screen_position', 'p3_brightness',
         'p4_button_config', 'p5_default', 'menu']


class Capture:
    def __init__(self, s):
        self.s, self.rows, self.presses, self.snaps, self.notes = s, [], [], {}, []
        self.record = struct.unpack('<I', s.read(0x70003B68, 8)[4:])[0]
        self.table = list(struct.unpack('<7I', s.read(0x00264EC0, 28)))
        self.table_ea0 = list(struct.unpack('<8I', s.read(0x00264EA0, 32)))
        self.held = []
        self.sample()

    def sample(self):
        s = self.s
        spad = s.read(0x70003B64, 0x34)
        ptr = struct.unpack_from('<I', spad, 8)[0]
        if ptr != self.record and not any('0x70003B6C' in n for n in self.notes):
            self.notes.append(f'0x70003B6C changed to {ptr:#x} at f{len(self.rows)}')
        task = s.read(RECORD + 8, 24)
        pad = s.read(0x00810E50, 0x2C)
        ph = s.read(0x00282154, 8)
        row = dict(
            f=len(self.rows), counter=struct.unpack_from('<I', spad, 0)[0], task=task.hex(),
            set=s.read(0x00810118, 16).hex(),
            fade=struct.unpack('<h', s.read(0x0028A9A0, 4)[:2])[0], busy=s.read(0x00275BD8, 4)[0],
            phase=ph[3], off=list(struct.unpack_from('<hh', spad, 0x30)), masks=spad[0x10:0x20].hex(),
            pad=[struct.unpack_from('<H', pad, o)[0] for o in (0x1A, 0x20, 0x24, 0x28)],
            e50=pad[0], e56=pad[6], mono=ph[7], press=list(self.held))
        self.rows.append(row)
        return row

    def step(self, buttons=()):
        self.held = list(buttons)
        self.s.step(1, buttons=list(buttons))
        return self.sample()

    # task-byte views of the newest row
    def t(self, off, n=1):
        b = bytes.fromhex(self.rows[-1]['task'])[off - 8:off - 8 + n]
        return b[0] if n == 1 else struct.unpack('<H', b)[0]

    def act(self):
        cur = self.t(0x1C, 2)
        return self.table[cur] if cur < 7 else None

    def menu(self):
        return self.t(0x8) == 6 and self.t(0xC) == 4 and self.rows[-1]['fade'] == 0

    def title(self):
        return self.t(0x8) == 2 and self.t(0x9) == 2 and self.rows[-1]['fade'] == 0

    def wait(self, cond, what):
        start = len(self.rows)
        while not cond():
            if len(self.rows) - start >= WAIT_LIMIT:
                raise RuntimeError(f'wait "{what}" exceeded {WAIT_LIMIT} frames (from f{start - 1})')
            self.step()

    def idle(self, n=IDLE):
        for _ in range(n):
            self.step()

    def press(self, button):
        key = lambda r: (r['task'], r['set'], r['off'], r['masks'])  # noqa: E731
        pre, f0, changed = key(self.rows[-1]), len(self.rows), False
        for k in range(CHANGE_LIMIT):
            changed |= key(self.step([button] if k < PRESS else [])) != pre
            if changed and k >= PRESS - 1:   # the hold always lasts its 2 steps
                break
        else:
            self.notes.append(f'no_change after {button} at f{f0}')
        self.presses.append(dict(f=f0, button=button, changed_at=len(self.rows) - 1))

    def snap(self, name):
        self.s.snapshot(OUT / name)
        self.snaps[name] = self.rows[-1]['f']

    def do(self, cond, what, button, snap=None):
        """wait cond; IDLE; [SNAPSHOT]; press."""
        self.wait(cond, what)
        self.idle()
        if snap:
            self.snap(snap)
        self.press(button)


def run(c):
    tc = lambda v, d=None: (lambda: c.t(0xC) == v and (d is None or c.t(0xD) == d))  # noqa: E731
    c.do(c.title, 'TITLE', 'DOWN')
    c.do(c.title, 'TITLE', 'DOWN')
    c.do(lambda: c.title() and c.t(0xF) == 2, 'TITLE and TF==2', 'CROSS')
    c.pad_mode = c.rows[-1]['pad'][0]
    targets = list(range(7)) if c.pad_mode == 7 else list(range(1, 7))
    for target in targets:
        while True:
            c.wait(c.menu, 'MENU')
            if c.act() == target:
                break
            c.idle()
            c.press('DOWN')
        c.idle()
        if target == 6:
            c.snap('menu')
        c.press('CROSS')
        page = PAGES[target]
        if target in (0, 1):
            c.do(tc(5, 1), 'TC==5 TD==1', 'RIGHT')
            c.do(tc(5, 1), 'TC==5 TD==1', 'CROSS', page)
        elif target == 2:
            c.do(tc(7), 'TC==7', 'LEFT')
            c.do(tc(7), 'TC==7', 'UP')
            c.do(tc(7), 'TC==7', 'CROSS', page)
        elif target == 3:
            c.do(tc(8), 'TC==8', 'CROSS', page)
        elif target == 4:
            c.do(tc(9, 1), 'TC==9 TD==1', 'RIGHT')
            c.do(tc(9), 'TC==9', 'CROSS', page)
        elif target == 5:
            c.do(tc(6, 1), 'TC==6 TD==1', 'CROSS', page)
    c.wait(c.title, 'TITLE (after exit)')
    c.idle(30)


def main():
    if OUT.exists():
        sys.exit(f'{OUT} exists: remove it first (a new capture never overwrites one)')
    OUT.mkdir(parents=True)
    state = fork_state('slot01_title')
    c, error = None, None
    with ForkSession(state, log_dir=OUT / 'logs', mtvu=False, lock_wait=4 * 3600.0) as s:
        try:
            c = Capture(s)
            c.pad_mode = c.rows[-1]['pad'][0]
            c.snap('title_start')
            run(c)
        except Exception as e:  # keep what was recorded
            error = f'{type(e).__name__}: {e}'
        finally:
            if c is not None:
                meta = dict(policy=POLICY, source_state=str(state), record=c.record,
                            record_expected=RECORD, table_264EC0=c.table, table_264EA0=c.table_ea0,
                            pad_mode=c.pad_mode, vibration_skipped=c.pad_mode != 7, snapshots=c.snaps,
                            presses=c.presses, notes=c.notes, error=error, frames=len(c.rows) - 1,
                            pad_interference=s.pad_interference, rows=c.rows)
                (OUT / 'trace.json').write_text(json.dumps(meta) + '\n')
    if error:
        sys.exit(error)
    print(f'{len(c.rows) - 1} frames, snapshots {c.snaps}, notes {c.notes}')


if __name__ == '__main__':
    main()
