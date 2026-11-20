#!/usr/bin/env python3
"""The ORIGINAL's EE sound state and RPC 0x64 exchanges over a guided take's rows (the PCSX2 fork).

The sound reference for tools/compare_sound_ticks.py (docs/SOUND_SECOND_HALF.md).
A guided take (decomp build/guided_play/<take>/) starts from an exact fork state
(start.json's state_abs) and replays exactly from its pad log (input.log as a
vsync-indexed input timeline, decomp record_guided_run.replay's rule), as
tools/capture_gs_original.py replays it: hidden, under the run lock (decomp
pcsx2_session.ForkSession, its own scratch ini), VU1 on the EE thread, the
take's block mode, under the takes' own renderer (Metal, 17: decomp
record_guided_run.take_renderer; a software-renderer replay leaves g75 on).

Over the trace rows R0..R1 (take.rec's rows without the movie rows) two watch
tables of the fork sample the EE, inline (no stops, no observer effect):

- "snd", at every main-loop top (the tick PC): the engine counter 0x70003B64,
  the player position D_00810350 (the take.rec check), D_00810E90, the block
  D_002817C0..D_002821AF (the IOP reply D_002817C0, D_00281B70 / C30, the voice
  ring D_00281CF0, D_00281D50, D_00281F30, the lanes and cues D_00282150..A7),
  the voice records D_0027CCC0 (48 x 0x6A), the tracks D_0027E0C0 (48 x 0x78),
  the library block D_0027F740 and D_00275B18..57 (the music clip and the ring
  head and tail) -- the layout of the decomp's first-level audio captures plus
  the tracks;
- "xchg", at 001152D8's call of 001191F0 (the RPC 0x64 exchange, 0x001157B4):
  the counter, D_00810E90, D_0027F740 (+0x3C the queue bank, +0x40 the count)
  and both queue banks D_0027F7C0 (2 x 0x1000 bytes of {command, voice, a, b}).

Every loop-top row is checked against take.rec (counter, position): a replay
that leaves the take is reported and is not a reference.

Output (ignored, disc-derived; a kept capture is a recording: never deleted):
  ../Extermination/build/sndpics/orig/<take>/r<R0>_<R1>/snd.tbl(.json), xchg.tbl(.json), run.json

Usage (port root; native arm64 macOS, the fork x86_64 under Rosetta; the
decomp's .venv is used automatically):
  python3 tools/capture_sound_original.py g42_area18_area14 0 600
"""
import argparse
import json
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
GP = DECOMP / 'build/guided_play'
OUT = DECOMP / 'build/sndpics/orig'
EXCHANGE_PC = 0x001157B4      # 001152D8's call of 001191F0(0x64, ...)

SND_SPANS = (('counter', 0x70003B64, 4), ('pos', 0x810350, 12), ('vblank', 0x810E90, 4),
             ('reply', 0x2817C0, 0x9F0), ('voices', 0x27CCC0, 48 * 0x6A), ('tracks', 0x27E0C0, 48 * 0x78),
             ('lib', 0x27F740, 0x50), ('iopbuf', 0x275B18, 0x40))
XCHG_SPANS = (('counter', 0x70003B64, 4), ('vblank', 0x810E90, 4), ('lib', 0x27F740, 0x50),
              ('queue', 0x27F7C0, 0x2000))
# --iop-ticks: the IOP driver SNDN2DRV.IRX (loaded at 0x56330 in every capture,
# docs/IOP_STREAM.md "What was read"): its driver thread's per-wake routine
# (+0x328: the ring's commands, then the status snapshot) and its RPC function
# 0x64 (+0x12C: the exchange's commands join the ring; the reply is the status
# block). Each row's EE cycle places the driver's ticks against the exchanges.
IRX_BASE = 0x56330
IOP_TICK_PC = IRX_BASE + 0x328
IOP_RPC_PC = IRX_BASE + 0x12C
IOP_SPANS = (('ring_write', IRX_BASE + 0x34A0, 4),)

if __name__ == '__main__' and os.environ.get('SNDPICS_VENV') != '1':
    venv = DECOMP / '.venv/bin/python'
    if venv.exists() and Path(sys.prefix).resolve() != (DECOMP / '.venv').resolve():
        os.environ['SNDPICS_VENV'] = '1'
        sys.exit(subprocess.call([str(venv), __file__, *sys.argv[1:]]))

sys.path.insert(0, str(DECOMP / 'tools'))
sys.path.insert(0, str(ROOT / 'tools'))
from capture_gs_original import take_rows, trace_rows, movies_before, session_records  # noqa: E402


def spans(table, src='ee'):
    return [dict(name=n, src=src, addr=f'0x{a:08X}', len=ln) for n, a, ln in table]


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('take')
    ap.add_argument('r0', type=int)
    ap.add_argument('r1', type=int)
    ap.add_argument('--out', type=Path, default=None)
    ap.add_argument('--renderer', type=int, default=17, help='17 = Metal, the takes\' own')
    ap.add_argument('--iop-ticks', action='store_true',
                    help='also the IOP driver\'s ticks and RPC 0x64 calls and each vsync (iop_tick.tbl, iop_rpc.tbl, vsync.tbl)')
    a = ap.parse_args(argv)
    import pcsx2_session as session
    import record_guided_run as rgr
    from pcsx2dbg.tables import Table

    take = a.take
    start = json.loads((GP / take / 'start.json').read_text())
    recs = take_rows(take)
    trace = trace_rows(recs)
    if not 0 <= a.r0 <= a.r1 < len(trace) - 1:
        sys.exit(f'{take}: rows {a.r0}..{a.r1} are not inside the take\'s {len(trace)} trace rows')
    step_of = {int(r['counter']): int(r['step']) for r in recs if r.get('mv', '0') == '0'}

    def step(R):
        st = step_of[int(trace[R]['counter'])]
        return st + 1 - movies_before(recs, st)

    out = a.out or OUT / take / f'r{a.r0}_{a.r1}'
    out.mkdir(parents=True, exist_ok=True)
    blocks = start.get('blocks', 'upstream')
    pads = rgr.timeline(session_records(take), start['vsync'])
    run = dict(take=take, rows=[a.r0, a.r1], blocks=blocks, state=start['state_abs'])
    t0 = time.time()
    with session.ForkSession(Path(start['state_abs']), log_dir=out / 'logs', renderer=a.renderer, mtvu=False,
                             upstream_blocks=blocks == 'upstream', align=False,
                             label='capture_sound_original') as s:
        c = s._client
        got0 = c.call('hash_regions', regions=['ee_ram', 'ee_scratch', 'vu0_data', 'vu1_data', 'iop_ram'])['regions']
        st0 = c.call('state')
        run['start_machine_equal'] = got0 == start['hashes'] and st0.get('vsync') == start['vsync']
        if not run['start_machine_equal']:
            print(f'{take}: the session does not start from the take\'s machine', flush=True)
        c.call('input_timeline', entries=pads, index='vsync', mode='replace')
        ticks = 0
        first = step(a.r0) - 1        # the stop before row R0's loop top
        if first > 0:
            c.call('run', timeout=3600, until={'ticks': first}, timeout_s=3600)
            ticks = first
        c.call('table_add', name='snd', path='snd.tbl', trigger='tick', spans=spans(SND_SPANS))
        c.call('table_add', name='xchg', path='xchg.tbl', trigger={'pc': EXCHANGE_PC, 'cpu': 'ee'},
               spans=spans(XCHG_SPANS))
        names = ['snd', 'xchg']
        if a.iop_ticks:
            for name, pc in (('iop_tick', IOP_TICK_PC), ('iop_rpc', IOP_RPC_PC)):
                c.call('table_add', name=name, path=f'{name}.tbl', trigger={'pc': pc, 'cpu': 'iop'},
                       spans=spans(IOP_SPANS, 'iop'))
                names.append(name)
            c.call('table_add', name='vsync', path='vsync.tbl', trigger='vsync',
                   spans=spans((('vblank', 0x810E90, 4),)))
            names.append('vsync')
        last = step(a.r1)
        c.call('run', timeout=3600, until={'ticks': last - ticks}, timeout_s=3600)
        for name in names:
            c.call('table_remove', name=name)
            for suffix in ('.tbl', '.tbl.json'):
                os.replace(s._inst.root / f'{name}{suffix}', out / f'{name}{suffix}')
    run['seconds'] = round(time.time() - t0, 1)
    # the take.rec check: every loop-top row's counter and position
    by_counter = {int(r['counter']): r for r in trace}
    rows = Table(out / 'snd.tbl').rows()
    seen, off = set(), []
    for row in rows:
        counter = struct.unpack('<I', row['counter'])[0]
        pos = struct.unpack('<3f', row['pos'])
        want = by_counter.get(counter)
        if want is None:
            continue
        seen.add(counter)
        if any(abs(p - float(want[x])) > 1e-4 for p, x in zip(pos, 'xyz')):
            off.append(counter)
    wanted = [int(trace[R]['counter']) for R in range(a.r0, a.r1 + 1)]
    missing = [c for c in wanted if c not in seen]
    run.update(loop_tops=len(rows), exchanges=Table(out / 'xchg.tbl').count(), rows_checked=len(seen),
               rows_missing=len(missing), rows_off_take=len(off), first_off=off[:5], first_missing=missing[:5])
    run['on_take'] = not off and not missing
    (out / 'run.json').write_text(json.dumps(run, indent=1) + '\n')
    print(f'{take} rows {a.r0}..{a.r1}: {len(rows)} loop tops, {run["exchanges"]} exchanges, '
          f'{len(seen)} rows checked, {len(off)} off the take, {len(missing)} missing; {run["seconds"]} s', flush=True)
    return 0 if run['on_take'] else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
