#!/usr/bin/env python3
"""The title's OPTIONS (00200A40, 001AC070 state 6) against the original's
capture (docs/OPTIONS.md section 4.5).

`make test-title-options`. The game boots headless (EM_STARTUP_TEST=options,
em_frontend.c options_test_after_frame): the boot screens, the title, Down
twice to OPTIONS and its CROSS, then the closed-loop policy of the
original's capture tools/capture_title_options.py ("title_options_v1"):
every row of the seven-row menu, each page with its presses (vibration
Right, sound Right, screen position Left and Up, brightness, button config
Right, default kept), the exit row, and back to the title. Every tick
prints one "options row" line.

The capture: ../Extermination/build/options_ref/trace.json (ignored,
disc-derived; the fork's hidden session from the title save state, one row
per main-loop top). Compared over the OPTIONS window (001AC070 state 6, the
record's +8 = 6 in the original, the flow's state 6 in the port), sampled
after every frame on both sides:

  * the state: the record's +0xC (00200A40's state), +0xD, +0x12, +0x13,
    the cursor +0x1C and the blink +0x1E, the settings D_00810118 +0..+0xB,
    the screen offset 0x70003B94 / 96, the button masks 0x70003B74..83, the
    fade D_0028A9A0 and the committed output mode D_0028215B;
  * the sequence of distinct states must be the capture's, value for value
    and in order;
  * every state must last the capture's rows, except: a state the policy's
    press ends (the two sides' pad latencies differ: the fork's two frames,
    the port's driven pad one), the screen module 0x2B's load (00200A40
    states 1 and 2: host speed, never longer than the capture's) and the
    window's last state;
  * the capture's row table D_00264EC0 is 0..6 (the port's policy walks the
    cursor by it) and its pad mode 7;
  * both sides are back on the title's interactive menu afterwards.

The page pictures (EM_STARTUP_CAPTURE_DIR, build/title_options/*.bmp and the
capture's <page>/original.png) are written for review; no pixel gate.
"""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = Path(os.environ.get('EM_TITLE_OPTIONS_BIN', str(ROOT / 'build/extermination')))
OUT = ROOT / 'build/title_options'
CAPTURE = ROOT.parent / 'Extermination/build/options_ref/trace.json'
LOAD_STATES = (1, 2)


def state(tc, td, t12, t13, cur, t1e, settings, off, masks, fade, mono):
    return (tc, td, t12, t13, cur, t1e, settings[:24], tuple(off), masks, fade, mono)


def original():
    doc = json.loads(CAPTURE.read_text())
    assert doc['policy'] == 'title_options_v1', doc['policy']
    assert doc['table_264EC0'] == list(range(7)), 'the capture\'s row table is not 0..6'
    assert doc['pad_mode'] == 7, doc['pad_mode']
    rows, out = doc['rows'], []
    for r in rows:
        t = bytes.fromhex(r['task'])
        st = state(t[4], t[5], t[10], t[11], t[20] | t[21] << 8, t[22] | t[23] << 8, r['set'], r['off'],
                   r['masks'].upper(), r['fade'], r['mono'])
        pad = r['pad']
        out.append((t[0], st, bool(pad[2] or pad[3]), t[1]))
    return out


def port():
    OUT.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, EM_UNCAPPED='1', EM_STARTUP_TEST='options', EM_STARTUP_CAPTURE_DIR=str(OUT))
    p = subprocess.run([str(BIN)], env=env, capture_output=True, text=True, timeout=300)
    (OUT / 'run.log').write_text(p.stdout + p.stderr)
    assert p.returncode == 0 and 'startup test: options PASS' in p.stdout, \
        ('the run failed', p.returncode, (p.stdout + p.stderr)[-1500:])
    out = []
    lines = [ln.split() for ln in p.stdout.splitlines() if ln.startswith('options row ')]
    for k, w in enumerate(lines):
        # The fixture samples at the title task's close-out, before the
        # frame's step H (001FB100, the sound byte's commit) and its
        # transition step 001AEE70 (the fade): this frame's committed mode
        # and fade are the ones the next row took at its tick's start (the
        # in-game smoke's "opt_pre").
        nxt = lines[k + 1] if k + 1 < len(lines) else w
        major, sub, t, settings = int(w[3]), int(w[4]), bytes.fromhex(w[5]), w[6]
        off = (int(w[7]), int(w[8]))
        masks = ''.join(int(w[9][i:i + 4], 16).to_bytes(2, 'little').hex() for i in range(0, 32, 4))
        fade, mono = int(nxt[18]), int(nxt[19])
        st = state(t[4], t[5], t[10], t[11], t[20] | t[21] << 8, t[22] | t[23] << 8, settings.lower(), off,
                   masks.upper(), fade, mono)
        pressed = int(w[14], 16) or int(w[15], 16)
        out.append((major, st, bool(pressed), sub))
    return out


def window(rows, name):
    start = next((i for i, r in enumerate(rows) if r[0] == 6), None)
    assert start is not None, (name, 'never in 001AC070 state 6')
    end = next((i for i in range(start, len(rows)) if rows[i][0] != 6), None)
    assert end is not None, (name, 'never back from OPTIONS')
    assert any(r[0] == 2 and r[3] == 2 and r[1][9] == 0 for r in rows[end:]), (name, 'not back on the menu')
    runs = []
    for i in range(start, end):
        st = rows[i][1]
        if runs and runs[-1][0] == st:
            runs[-1][1] += 1
        else:
            runs.append([st, 1, False])
        # a press that reaches the game ends the state it shows in
        if rows[i][2] and len(runs) > 1:
            runs[-2][2] = True
    return runs


def main():
    orig = window(original(), 'original')
    mine = window(port(), 'port')
    for i, (a, b) in enumerate(zip(orig, mine)):
        assert a[0] == b[0], ('state %d differs' % i, 'original', a[0], 'port', b[0])
    assert len(orig) == len(mine), ('state count', len(orig), len(mine), orig[len(mine):][:2], mine[len(orig):][:2])
    checked = 0
    for i, (a, b) in enumerate(zip(orig, mine)):
        tc = a[0][0]
        if i == len(orig) - 1 or a[2] or b[2]:
            continue
        if tc in LOAD_STATES and b[0] == a[0]:
            assert b[1] <= a[1], ('module 0x2B load longer than the original\'s', i, a[1], b[1])
            continue
        assert a[1] == b[1], ('state %d lasts %d rows in the original, %d in the port' % (i, a[1], b[1]), a[0])
        checked += 1
    final = orig[-1][0]
    print('title options: %d distinct states equal to the capture\'s, %d of them with the capture\'s length; '
          'settings %s, offset %s, masks %s' % (len(orig), checked, final[6], final[7], final[8]))
    print('title options: PASS')


if __name__ == '__main__':
    sys.exit(main())
