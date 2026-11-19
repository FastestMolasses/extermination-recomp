#!/usr/bin/env python3
"""The ORIGINAL's GS frames at a guided take's rows (the PCSX2 fork, software renderer).

The picture reference for tools/compare_gs_frame.py (docs/DRAW_LIST_AUDIT.md
section 4). A guided take (decomp build/guided_play/<take>/) starts from an
exact fork state (start.json's state_abs, the previous take's end.p2s) and
replays exactly from its pad log (input.log as a vsync-indexed input
timeline: decomp record_guided_run.replay's rule). This tool replays it the
same way, hidden, under the run lock (decomp pcsx2_session.ForkSession: its
own scratch ini, so the user's settings are never touched), with the
SOFTWARE renderer (13: only it keeps the drawn pixels in GS memory; the
takes were recorded under Metal) and VU1 on the EE thread (MTVU off, as the
takes), in the take's own block mode (start.json's `blocks`).

At each requested row R (the take's trace row: take.rec's rows without the
movie rows, as tools/capture_from_take.py and tools/guided_chain.py number
them) it stops at the main-loop tops of rows R + 1, R + 2 and R + 3 and keeps
the GS local memory there (gs_vram) with the display registers (gs_field's
regs) and the game's counter, frame index D_00810E80, field D_00810E88, the
player's position and the area bytes. Every stop is checked against
take.rec (counter, position): a replay that leaves the take is reported, and
its frames are not a reference.

Which buffer is the frame of port script frame R (tools/guided_chain.py
--gs-dump, EM_GS_DUMP_AT): the port's dump of row R is the field its frame R
drew (the list built from row R's state onwards). In the original that list
is built in iteration R -> R + 1 and drawn in iteration R + 1 -> R + 2
(tools/test_fb2_pixels.py: "the list built in iteration s0 -> s1 is drawn in
s1 -> s2"), so the field is whole at loop top R + 2, in the buffer the
port's FRAME_1 names when both frame indices agree (guided_chain keeps row
0's bank). compare_gs_frame.py reads it from the R + 2 memory.

Output (ignored, disc-derived; a kept capture is a recording: never deleted):
  ../Extermination/build/gspics/orig/<take>/r<R>/s<k>.vram   (4 MiB GS memory at loop top R + k)
  ../Extermination/build/gspics/orig/<take>/r<R>/s<k>.json   (regs, counters, the take.rec check)
  ../Extermination/build/gspics/orig/<take>/run.json

Usage (port root; native arm64 macOS, the fork x86_64 under Rosetta; the
decomp's .venv is used automatically):
  python3 tools/capture_gs_original.py g77_area21_phase1 40 400
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
OUT = DECOMP / 'build/gspics/orig'
STOPS = (1, 2, 3)

if __name__ == '__main__' and os.environ.get('GSPICS_VENV') != '1':
    venv = DECOMP / '.venv/bin/python'
    if venv.exists() and Path(sys.prefix).resolve() != (DECOMP / '.venv').resolve():
        os.environ['GSPICS_VENV'] = '1'
        sys.exit(subprocess.call([str(venv), __file__, *sys.argv[1:]]))

sys.path.insert(0, str(DECOMP / 'tools'))


def take_rows(take):
    """take.rec as dicts (its movie rows marked), in step order."""
    cols, rows = None, []
    for line in (GP / take / 'take.rec').read_text().splitlines():
        if line.startswith('#cols'):
            cols = line[1:].split()[1:]
            continue
        if not line or line.startswith('#'):
            continue
        v = line.split()
        rows.append(dict(zip(cols, v)))
    return rows


def trace_rows(rows):
    """The trace's numbering: take.rec's rows without the movie rows (mv 1)."""
    return [r for r in rows if r.get('mv', '0') == '0']


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('take')
    ap.add_argument('rows', nargs='+', type=int, help='trace rows R (each read at loop tops R+1..R+3)')
    ap.add_argument('--out', type=Path, default=None)
    ap.add_argument('--renderer', type=int, default=13, help='13 = software (the only reference)')
    ap.add_argument('--ram', action='store_true', help='also keep the EE RAM at loop top R + 1 (s1.ee, 32 MiB)')
    ap.add_argument('--gs-dump', action='store_true', help='also record the GS stream from loop top R + 1 on for '
                    'three fields (the fork\'s gs_dump, upstream GS dump format: every GIF transfer of the field '
                    'drawn in R + 1 -> R + 2; r<R>/s1.gs.zst, tools/gs_dump_prims.py reads it)')
    a = ap.parse_args(argv)
    import pcsx2_session as session
    import record_guided_run as rgr

    take = a.take
    start = json.loads((GP / take / 'start.json').read_text())
    recs = take_rows(take)
    trace = trace_rows(recs)
    step_of = {int(r['counter']): int(r['step']) for r in recs if r.get('mv', '0') == '0'}
    out = a.out or OUT / take
    out.mkdir(parents=True, exist_ok=True)
    targets = sorted(set(a.rows))
    for R in targets:
        if R + max(STOPS) >= len(trace):
            sys.exit(f'{take}: row {R} + {max(STOPS)} is past the take\'s {len(trace)} trace rows')
    blocks = start.get('blocks', 'upstream')
    pads = rgr.timeline(session_records(take), start['vsync'])
    run = dict(take=take, blocks=blocks, renderer=a.renderer, state=start['state_abs'], rows=targets, stops=[])
    t0 = time.time()
    with session.ForkSession(Path(start['state_abs']), log_dir=out / 'logs', renderer=a.renderer, mtvu=False,
                             upstream_blocks=blocks == 'upstream', align=False,
                             label='capture_gs_original') as s:
        c = s._client
        got0 = c.call('hash_regions', regions=['ee_ram', 'ee_scratch', 'vu0_data', 'vu1_data', 'iop_ram'])['regions']
        st0 = c.call('state')
        same0 = got0 == start['hashes'] and st0.get('vsync') == start['vsync']
        run['start_machine_equal'] = same0
        if not same0:
            print(f'{take}: the session does not start from the take\'s machine (vsync {st0.get("vsync")} '
                  f'against {start["vsync"]})', flush=True)
        c.call('input_timeline', entries=pads, index='vsync', mode='replace')
        ticks = extra = 0
        for R in targets:
            for k in STOPS:
                want = trace[R + k]
                # the state stands before a loop top (end.json's pc): the first tick reaches row 0's;
                # a movie row (mv 1) is no loop top of its own in the fork
                step = step_of[int(want['counter'])] + 1 - movies_before(recs, step_of[int(want['counter'])]) + extra
                if step > ticks:
                    c.call('run', timeout=3600, until={'ticks': step - ticks}, timeout_s=3600)
                    ticks = step
                counter = s.u32(session.FRAME_COUNTER)
                # a movie's iteration can stop at the loop top more than once (take.rec gives it one
                # row): catch up on the counter, a few ticks at most, and keep the offset
                while counter < int(want['counter']) and extra < 8:
                    c.call('run', timeout=600, until={'ticks': 1}, timeout_s=600)
                    ticks += 1
                    extra += 1
                    counter = s.u32(session.FRAME_COUNTER)
                pos = struct.unpack('<3f', s.read(0x810350, 12))
                area = s.read(0x810700, 4)[:3]
                e80 = s.u32(session.FRAME_INDEX)
                e88 = s.u32(session.FIELD)
                d = out / f'r{R}'
                d.mkdir(exist_ok=True)
                rel = f'gsp_{R}_{k}.vram'
                c.call('gs_vram', path=rel, timeout=120)
                field = c.call('gs_field', path=f'gsp_{R}_{k}.png', timeout=120)
                os.replace(s._inst.root / rel, d / f's{k}.vram')
                os.replace(s._inst.root / f'gsp_{R}_{k}.png', d / f's{k}.png')
                if a.gs_dump and k == 1:
                    c.call('gs_dump', path=f'gsp_{R}', frames=3, timeout=120)
                if a.ram and k == 1:
                    c.call('mem_read', space='ee', addr=0, len=0x2000000, timeout=120, **{'as': 'file', 'path': f'gsp_{R}.ee'})
                    os.replace(s._inst.root / f'gsp_{R}.ee', d / 's1.ee')
                rec_pos = tuple(float(want[x]) for x in ('x', 'y', 'z'))
                ok = counter == int(want['counter']) and all(abs(p - q) <= 1e-4 for p, q in zip(pos, rec_pos))
                info = dict(row=R + k, stop=k, step=step, counter=counter, rec_counter=int(want['counter']),
                            pos=pos, rec_pos=rec_pos, on_take=ok, area=list(area), e80=e80, e88=e88,
                            regs=field.get('regs'), renderer=field.get('renderer'), psm=field.get('psm'))
                (d / f's{k}.json').write_text(json.dumps(info, indent=1) + '\n')
                if a.gs_dump and k == max(STOPS) and (s._inst.root / f'gsp_{R}.gs.zst').exists():
                    os.replace(s._inst.root / f'gsp_{R}.gs.zst', d / 's1.gs.zst')
                run['stops'].append(dict(row=R + k, on_take=ok, counter=counter))
                print(f'{take} row {R} + {k}: counter {counter} ({"on the take" if ok else "OFF the take"}), '
                      f'pos {pos[0]:.3f} {pos[1]:.3f} {pos[2]:.3f}, area {area.hex()}, e80 {e80}', flush=True)
    run['seconds'] = round(time.time() - t0, 1)
    (out / 'run.json').write_text(json.dumps(run, indent=1) + '\n')
    return 0 if all(x['on_take'] for x in run['stops']) else 1


def movies_before(recs, step):
    return sum(1 for r in recs if r.get('mv', '0') != '0' and int(r['step']) < step)


def session_records(take):
    import record_original_play as rop
    return rop.records(GP / take)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
