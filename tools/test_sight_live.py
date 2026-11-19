#!/usr/bin/env python3
"""The weapon sights live (docs/WHOLE_GAME_LEDGER.md section 4.2): one
headless run per sight from its start point (g67's start, the AREA16 bed
room, with the sight fitted: tools/build_start_saves.py sight_*), R2 held
past the post-load lock so the aim camera's sight switch 001999C0 draws the
sight every frame, with EM_SIGHT_DUMP naming each drawer call and the
slot-1 packet bytes it built.

Checked per sight: no fault, the pad script ends, and the drawers of that
sight's arm ran (the zoom scope: 001DB9D0, 001DC960, 001DBCB0; the night
vision scope: 001DB830, 001DBD50; the DELTA auto sight: 001DB9D0, 001DBF00,
001DC020; the interceptor unit: 001DB9D0, 001DC610, 001DC890; the missile
launcher: 001DB830, 001DBE20, 00199770). sight_missile_a22 (AREA22, nine
class-2 entries in range) must draw lock marks (001DD170). sight_upper
(g60's start, AREA06, item 0x06 on the upper rail: D_00810CA5 = 6) must run
the radar 0022E7F0 every aim frame with blips (0022E630) for the two
class-2 type-7 owners within 200.

The zoom scope against the original (decomp build/sights_ref/zoom, a
hidden fork capture: g66's end, the scope fitted on its own SCOPE MOUNT
page by pad, R2 held; the run's pad here follows its plan: R2, then L1
101..160 aim ticks, R2 alone, L2 231..255): for each captured frame (zoom
0, 1.0 and 0.5) the bytes 001DB9D0, 001DC960 and 001DBCB0 put into slot 1
(22,496) must equal the original's, in the port's frame whose 001DBCB0 got
the same zoom bits. The other four sights have no original capture (no
recording owns their items; their start points are constructed): their
packets are the oracle's (make test-sight-reference) only.

Cards: build/start_saves/sight_* (tools/build_start_saves.py, built on demand). Output:
build/sights/live/<point>/.
"""
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / 'build' / 'extermination'
CARDS = ROOT / 'build' / 'start_saves'
OUT = ROOT / 'build' / 'sights' / 'live'
CAPTURE = ROOT.parent / 'Extermination' / 'build' / 'sights_ref' / 'zoom'
LOCK = 100
R2, L1, L2 = 0x200, 0x400, 0x100
DRAWERS = {
    'sight_zoom': ('001DB9D0', '001DC960', '001DBCB0'),
    'sight_night': ('001DB830', '001DBD50'),
    'sight_delta': ('001DB9D0', '001DBF00', '001DC020'),
    'sight_interceptor': ('001DB9D0', '001DC610', '001DC890'),
    'sight_missile': ('001DB830', '001DBE20', '00199770'),
    # AREA22 past door 18 with nine class-2 entries (the small bugs 0012A5D0, pool records
    # 0x7A70B0..0x7A8830) within 360 and on screen: 00199770 draws their lock marks (001DD170);
    # the entries' fields are their area actor view's (em_sight_live owner_view).
    'sight_missile_a22': ('001DB830', '001DBE20', '00199770'),
    # the UPPER U.R.S. (item 0x06, CA5 = 6): no sight arm (CA4 0xFF, CA7 7), so 001999C0's tail
    # draws the radar 0022E7F0 every aim frame
    'sight_upper': ('0022E7F0',),
}


def pad_script():
    a = LOCK
    rows = [(0, 0), (a, R2), (a + 101, R2 | L1), (a + 161, R2), (a + 231, R2 | L2), (a + 256, R2), (a + 320, 0)]
    last = a + 340
    return '\n'.join([f'EMA1 1 {last} 1'] + [f'{f} {b:x} 127 127' for f, b in rows]) + '\n'


def cards():
    missing = [p for p in DRAWERS if not (CARDS / p).is_dir()]
    if missing:
        subprocess.run([sys.executable, str(ROOT / 'tools' / 'build_start_saves.py'), *missing],
                       cwd=ROOT, check=True, stdout=subprocess.DEVNULL)


def run(point):
    out = OUT / point
    out.mkdir(parents=True, exist_ok=True)
    pad = out / 'aim.pad'
    pad.write_text(pad_script())
    dump = out / 'sight.jsonl'
    if dump.exists():
        dump.unlink()
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(EM_START_AT=point, EM_START_AT_PAD=str(pad),
               EM_SIGHT_DUMP=str(dump), EM_HEADLESS='1', EM_MUTE='1', EM_UNCAPPED='1')
    p = subprocess.Popen([str(BIN)], cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    lines = []
    try:
        for line in p.stdout:
            line = line.rstrip('\n')
            lines.append(line)
            if 'EM_START_AT_PAD: the script ended' in line or 'the game task is stopped' in line:
                break
    finally:
        p.kill()
        p.wait()
    (out / 'run.log').write_text('\n'.join(lines) + '\n')
    calls = [json.loads(x) for x in dump.read_text().splitlines()] if dump.exists() else []
    return lines, calls


def frames_of(calls, names):
    """Consecutive drawer triples in the sight's order."""
    out = []
    for i in range(len(calls) - len(names) + 1):
        if tuple(c['fn'] for c in calls[i:i + len(names)]) == names:
            out.append(calls[i:i + len(names)])
    return out


def original_run(start, size, zoom, fill):
    """The original 001DB9D0(1, 1.0), 001DC960(1, 1.0), 001DBCB0(1, zoom) as
    original code (every leaf too) over the capture's RAM at a tick top
    (frame_aim040), the slot-1 cursor put at `start` and the packet range
    filled with `fill`: the range afterwards."""
    sys.path.insert(0, str(ROOT / 'tools'))
    import test_player_slide_reference as S
    from test_player_fall_reference import FallEE
    frame = CAPTURE / 'frame_aim040'
    ram = bytearray((frame / 'eeMemory.bin').read_bytes())
    ram[start:start + size] = bytes([fill]) * size
    ctx = int.from_bytes(ram[0x00275670:0x00275674], 'little')
    ram[ctx + 0x14:ctx + 0x18] = start.to_bytes(4, 'little')
    e = FallEE(ELF[0] if ELF else S.read_elf(), ram, (frame / 'scratchpad.bin').read_bytes())
    def strlen(ee):              # 001232E0 (its MMI word scan is not in the model): the C result
        a = ee.r[4] & 0xFFFFFFFF
        ee.r[2] = bytes(ee.mem[a:a + 0x1000]).index(0)
    e.hooks = {0x1232E0: strlen}
    for fn, f12 in ((0x1DB9D0, 0x3F800000), (0x1DC960, 0x3F800000), (0x1DBCB0, zoom)):
        e.r[29] = 0x01F00000
        e.r[4] = 1
        e.f[12] = f12
        e.r[31] = S.RETURN
        e.run(fn)
    return bytes(e.mem[start:start + size])


ELF = []


def written_mask(start, size, zoom):
    """The bytes the original writes (equal over two fills) and its result."""
    a, b = original_run(start, size, zoom, 0x00), original_run(start, size, zoom, 0xFF)
    return [a[i] == b[i] for i in range(size)], a


def compare_capture(calls, problems):
    if not CAPTURE.is_dir():
        return 'no capture (decomp build/sights_ref/zoom): byte comparison skipped'
    frames = frames_of(calls, DRAWERS['sight_zoom'])
    done = []
    for folder in sorted(CAPTURE.glob('aim*_drawers')):
        bins = list(folder.glob('slot1_*.bin'))
        if not bins:
            continue
        want = bins[0].read_bytes()
        # the zoom of the drawers' frame: the capture's frame at or just before it (L1 / L2 had
        # stopped by then: aim ticks 31, 161, 301 against the frames 040, 160, 300)
        tick = int(folder.name[3:6])
        near = [f for f in sorted(CAPTURE.glob('frame_aim*')) if int(f.name[9:12]) <= tick] or \
            sorted(CAPTURE.glob('frame_aim*'))[:1]
        facts = json.loads((near[-1] / 'facts.json').read_text())['facts']
        zoom = int.from_bytes(bytes.fromhex(facts['zoom_810248_hex']), 'little')
        cands = [fr for fr in frames if int(fr[2]['f12'], 16) == zoom]
        if not cands:
            problems.append(f'{folder.name}: no port frame with the zoom {zoom:#x}')
            continue
        got = b''.join(bytes.fromhex(c['bytes']) for c in cands[0])
        start = int(bins[0].stem.split('_')[1], 16)
        mask, orig = written_mask(start, len(want), zoom)
        n = sum(mask)
        bad_o = [i for i in range(len(want)) if mask[i] and orig[i] != want[i]]
        if bad_o:
            problems.append(f'{folder.name}: the oracle run differs from the capture at {len(bad_o)} written bytes '
                            f'(first +{bad_o[0]:#x})')
        if len(got) != len(want):
            problems.append(f'{folder.name}: the port built {len(got)} bytes, the original {len(want)}')
        else:
            bad = [i for i in range(len(want)) if mask[i] and got[i] != want[i]]
            if bad:
                problems.append(f'{folder.name}: {len(bad)} of {n} written bytes differ (first +{bad[0]:#x})')
        done.append(f'{folder.name} {n} of {len(want)} bytes written, equal')
    if not done:
        problems.append('the capture has no drawer folders')
    return 'original capture: ' + ', '.join(done)


def check(point):
    lines, calls = run(point)
    problems = []
    faults = [x for x in lines if 'fault' in x.lower() or 'has no binding' in x or 'refused' in x]
    if faults:
        problems.append('faulted: ' + ' | '.join(faults[:3]))
    if not any('EM_START_AT_PAD: the script ended' in x for x in lines):
        problems.append('the pad script did not end')
    seen = {c['fn'] for c in calls}
    lack = [d for d in DRAWERS[point] if d not in seen]
    if lack:
        problems.append('drawers never ran: ' + ' '.join(lack))
    note = '%d drawer calls' % len(calls)
    if point == 'sight_missile_a22':
        marks = [c for c in calls if c['fn'] == '00199770' and c['bytes']]
        if not marks:
            problems.append('00199770 drew no lock mark (001DD170)')
        note += '; %d frames with lock marks' % len(marks)
    if point == 'sight_upper':
        # each blip 0022E630 draws is one 00207E40 sprite carrying tex0 0x20041605113222CD
        blips = [c['bytes'].count('cd22321105160420') for c in calls if c['fn'] == '0022E7F0']
        if not any(blips):
            problems.append('0022E7F0 drew no blip (0022E630)')
        note += '; %d radar frames, %d with blips (at most %d)' % (len(blips), sum(1 for b in blips if b),
                                                                    max(blips, default=0))
    if point == 'sight_zoom' and not problems:
        note += '; ' + compare_capture(calls, problems)
    return point, problems, note


def main():
    if not BIN.is_file():
        print('test_sight_live: build first (make)', file=sys.stderr)
        return 2
    cards()
    points = sys.argv[1:] or list(DRAWERS)
    with ThreadPoolExecutor(max_workers=min(5, len(points))) as ex:
        results = list(ex.map(check, points))
    bad = 0
    for point, problems, note in results:
        print(f"{'FAIL' if problems else 'PASS'} {point}: {note}" + (' -- ' + '; '.join(problems) if problems else ''))
        bad += bool(problems)
    print('sight live: ' + ('FAIL' if bad else 'PASS'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
