#!/usr/bin/env python3
"""Where the ORIGINAL's IOP driver ticked against each RPC 0x64 exchange (the PCSX2 fork's capture).

docs/SOUND_SECOND_HALF.md section 5.1. The IOP sound driver SNDN2DRV.IRX has one
timer: its driver thread wakes every 64 H-lines (128 half-lines), runs every
command the exchanges left in its ring (the SFX and the stream commands alike)
and writes every voice's status, which the next exchange returns. Which field a
tick falls in therefore sets when a key-on sounds and when a sample's end
reaches the EE's reaper. tools/capture_sound_original.py --iop-ticks records,
over a window of a guided take, the EE cycle of every driver wake (+0x328), of
every RPC function 0x64 call (+0x12C) and of every vsync. Measured over all
eleven windows: the fork's driver tick is 127.998 half-lines (not 128), its
wake-ups spread over about 14 half-lines (a median on the fitted line, late
outliers), and the RPC runs 7.7 half-lines after the vsync; so the fork's
phase against the field drifts through a whole period every ~14,500 fields and
no single phase holds for a whole run.

This gives, for counters inside a captured window, the half-lines from the
exchange to the fitted next driver tick: EM_IOP_TICK_ALIGN's entries
(comparison only; em_scene_bindings.c), which move the port's one tick grid
there so that a comparison measures the game code, not the capture's timer.

Usage (port root; the decomp's .venv for the fork's table reader):
  python3 tools/iop_tick_align.py ../Extermination/build/sndpics/orig/g42_area18_area14/iop_r0_600 419733 419883
  -> 419733:100,419883:68
"""
import bisect
import os
import statistics
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
ORIG = DECOMP / 'build/sndpics/orig'
FORK_PY = ROOT.parent / 'pcsx2-fork/extermination/python'
# IOP_TICK_ALIGN_SHIFT=<half-lines>: every entry that much later (a measurement knob: how far the grid can
# move before a window's events change)
SHIFT = int(os.environ.get('IOP_TICK_ALIGN_SHIFT', '0'))
STEP = 150            # counters between entries (the fork drifts ~1.3 half-lines in 150 fields)


def _rows(path):
    if str(FORK_PY) not in sys.path:
        sys.path.insert(0, str(FORK_PY))
    from pcsx2dbg.tables import Table
    return Table(path).rows()


def iop_dir(take, r0, r1):
    """The --iop-ticks capture (decomp build/sndpics/orig/<take>/iop_r<A>_<B>) whose rows hold r0..r1."""
    for d in sorted((ORIG / take).glob('iop_r*_*')):
        a, _, b = d.name[5:].partition('_')
        if a.isdigit() and b.isdigit() and int(a) <= r0 and r1 <= int(b) and (d / 'iop_tick.tbl').exists():
            return d
    return None


def entries(orig_dir, counters=None, step=STEP):
    """[(counter, half-lines 1..128)] at the given loop-top counters (default: every `step` from the window's
    second loop top), from the fitted driver tick (least squares over the window, plus the median wake)."""
    d = Path(orig_dir)
    ticks = [r['ee_cycle'] for r in _rows(d / 'iop_tick.tbl')]
    rpcs = [r['ee_cycle'] for r in _rows(d / 'iop_rpc.tbl')]
    vs = [r['ee_cycle'] for r in _rows(d / 'vsync.tbl')]
    loops = [(r['ee_cycle'], struct.unpack('<I', r['counter'])[0]) for r in _rows(d / 'snd.tbl')]
    xc = [r['ee_cycle'] for r in _rows(d / 'xchg.tbl')]
    hl = (vs[-1] - vs[0]) / (len(vs) - 1) / 525
    n = len(ticks)
    xm, ym = (n - 1) / 2, sum(ticks) / n
    b = sum((i - xm) * (y - ym) for i, y in enumerate(ticks)) / sum((i - xm) ** 2 for i in range(n))
    a = ym - b * xm
    med = statistics.median(t - (a + b * i) for i, t in enumerate(ticks))
    if counters is None:
        counters = [c for _, c in loops[1::step]]
    want, out = set(counters), []
    for k in range(1, len(loops)):
        if loops[k][1] not in want:
            continue
        # the exchange paired with loop top k (compare_sound_ticks: after the previous loop top)
        xx = xc[bisect.bisect_right(xc, loops[k - 1][0]):bisect.bisect_left(xc, loops[k][0])]
        if not xx or bisect.bisect_left(rpcs, xx[0]) >= len(rpcs):
            continue
        rpc = rpcs[bisect.bisect_left(rpcs, xx[0])]
        i = int((rpc - a - med) / b) + 1
        while a + b * i + med <= rpc:
            i += 1
        while a + b * (i - 1) + med > rpc:
            i -= 1
        d = round((a + b * i + med - rpc) / hl) + SHIFT
        out.append((loops[k][1], (d - 1) % 128 + 1))
    return out


def env_value(items):
    return ','.join(f'{c}:{d}' for c, d in items)


def for_windows(windows):
    """EM_IOP_TICK_ALIGN for [(take, r0, r1)] whose --iop-ticks captures exist; (value, missing windows)."""
    got, missing = [], []
    for take, r0, r1 in windows:
        d = iop_dir(take, r0, r1)
        if d:
            got += entries(d)
        else:
            missing.append(f'{take}:{r0}-{r1}')
    return env_value(sorted(got)), missing


def main(argv):
    if len(argv) < 1:
        sys.exit(__doc__)
    counters = [int(c) for c in argv[1:]] or None
    print(env_value(entries(argv[0], counters)))
    return 0


if __name__ == '__main__':
    if os.environ.get('IOP_ALIGN_VENV') != '1' and (DECOMP / '.venv/bin/python').exists() and \
            Path(sys.prefix).resolve() != (DECOMP / '.venv').resolve():
        os.environ['IOP_ALIGN_VENV'] = '1'
        sys.exit(subprocess.call([str(DECOMP / '.venv/bin/python'), __file__, *sys.argv[1:]]))
    sys.exit(main(sys.argv[1:]))
