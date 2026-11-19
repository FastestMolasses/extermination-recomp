#!/usr/bin/env python3
"""Claude's guided takes played as one continuous run from one start card.

tools/mid_route_beats.py runs a guided take from its own card: a LOAD GAME of
the take's source state. A LOAD GAME is not the continuous play the take
recorded, and the differences are the original's own (docs/SECOND_HALF_AREAS.md
section 5): g47's card reruns [30]'s arrival script, AREA16's curtains
(001C1030) respawn because their kill bits are never saved, g46's arrival
script waits ~17 frames less from a card. So here a chain of kept takes is
played the way the user's takes are (tools/watch_takes.py): one card at a
quiet point, before an area change, then every take's recorded pad in order
with no reload, so the area change into the checked area is a real one.

What runs (EM_START_AT, docs/STARTUP.md; comparison aids as the g4x beats'):
  - the card (tools/build_start_saves.py: the first take's source state);
  - EM_START_AT_PAD: the takes' recorded pads, stitched on the takes' counters
    (a pad change read at tick k is an input at f = k - 2, route_capture's
    rule), with no path steering: the pad alone;
  - EM_START_AT_RANDS: each row's rand() word (take.rec's 'rand'; script
    frame r takes row r's word) and EM_START_AT_CLOCKS each row's world clock
    0x70003B68 where the take has the column (route_g17's; route_g16's traces
    predate it): a difference then comes from the logic, not from the stream;
  - EM_START_AT_PHASES: when a load segment ends (phase L: em_replay.c's and
    take.rec's rule) the script's frame becomes the recording's first row after
    its load (the port's disc answers at host speed, so its load is shorter);
  - EM_START_AT_FRAMES: the script's clock follows the engine's frames across a
    status page's open and close (frames no hook steps), and a step at a frame's
    end sets the pad the next frame reads (docs/STARTUP.md);
  - EM_START_AT_STATUS_BG: the pages' background D_002655A0 from the card's
    source state (never reset by the original, so a card from boot differs);
  - EM_START_AT_TRACE: the port's position, heading, area bytes, vitals,
    rand() word, frame-machine state and D_00275BD8 per script frame.

Every frame is then compared with the takes' rows (route set trace.json, read
only): position (within 0.01), heading, the area bytes, the rand() word (the
port's word at the frame's start, before its reseed: equal when the frame before
drew as many times as the original's; the trace samples each row at the frame's
start, as the recording does: em_start_at_frame_end), and the vitals where the take has them (its hits, from
threats.json: the frame and the health after each). Load rows (phase L) are not
compared. Per take: the first frame of each kind of difference and where the
run stopped (a fault line, the script's end).

Usage (native arm64 macOS; build/extermination and the cards g45 / g49 / g59 first):

  python3 tools/guided_chain.py --list
  python3 tools/guided_chain.py g17            # g45 -> g46 -> g47 -> g48 from card g45 (AREA14)
  python3 tools/guided_chain.py g18 --steer g41   # card g41 (AREA14) -> AREA18 -> AREA14 -> ... -> g45 -> AREA17 ...
  python3 tools/guided_chain.py g16            # g59 -> g60 -> ... from card g59 (AREA01 sub 1)
  python3 tools/guided_chain.py g16 --until g62   # stop the chain after g62
  python3 tools/guided_chain.py g17 --steer    # steer where the player falls behind the path

Output: build/guided_chain/<chain>/ (run.log, trace.jsonl, the pad and anchor
files, report.txt; disc-derived: never committed).
"""
import argparse
import json
import math
import os
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import reference_root as refs  # noqa: E402
import mid_route_beats as mrb  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/guided_chain'
SAME = 0.01
PAD_LAG = 2     # capture_from_take / route_capture: a pad read at row k is an input at f = k - 2
TURNED = 0.001

# Takes whose route set is not the chain's own (tools/capture_from_take.py --set ... g48 .. g58).
G49_ROUTE = 'guided-20261117/s87/route_g49'
G49_TAKES = ('g49_area11_area01', 'g50_area01_area02', 'g51_area02_area04', 'g52_area04_area03',
             'g53_area03_tag7', 'g54_area03_tag10', 'g55_area03_area19', 'g56_area15_cindy',
             'g57_area19_area04', 'g58_area04_area01')


# Takes before the AREA17 ones (lane PORT-A18A14: tools/capture_from_take.py --set ... g39 .. g44):
# AREA14 -> AREA18 -> AREA14 -> AREA18 -> AREA14.
G18_ROUTE = 'guided-20261117/s87/route_g18'
G18_TAKES = ('g40_area14_booster', 'g41_area14_area18', 'g42_area18_area14', 'g43_area14_area18',
             'g44_area18_area14')
G17_TAKES = ('g45_area14_area17', 'g46_area17_ramp', 'g47_area17_roger', 'g48_area17_cable')


def take_route(route, take):
    if take in G49_TAKES:
        return G49_ROUTE
    if take in G18_TAKES:
        return G18_ROUTE
    if take in G17_TAKES:
        return mrb.G17_ROUTE
    return route


# chain: (start card, route set, takes in order). The card is the first take's source state.
CHAINS = {
    # AREA14 entry 1 (g44's end, in play) -> [6]'s Use -> AREA17 entry 0 (a real area change) -> ...
    # ... -> [38]'s cable: AREA11 entry 3 -> AREA01 sub 1 -> AREA02 sub 1 -> AREA04 sub 1 -> AREA03 ...
    # (the takes after g48 are in route_g49: G49_TAKES)
    'g17': ('g45', mrb.G17_ROUTE, ('g45_area14_area17', 'g46_area17_ramp', 'g47_area17_roger',
                                   'g48_area17_cable', 'g49_area11_area01', 'g50_area01_area02',
                                   'g51_area02_area04', 'g52_area04_area03', 'g53_area03_tag7',
                                   'g54_area03_tag10', 'g55_area03_area19', 'g56_area15_cindy',
                                   'g57_area19_area04', 'g58_area04_area01')),
    # AREA14 (g40's end, in play) -> the cargo ramp (00190F20): AREA18 entry 0 (a real area change)
    # -> item 0x28 -> AREA14 entry 1 -> the fork-back into AREA18 again -> AREA14 entry 1 -> [6]'s
    # Use -> AREA17 entry 0 (g45 from a real area change, not the card g45) -> ... as g17
    'g18': ('g41', G18_ROUTE, G18_TAKES[1:] + G17_TAKES + G49_TAKES),
    # AREA11 entry 3 in play after the cable ride's scene (g48's end; the scene itself, script
    # 0x828C70, is not ported yet: the g17 chain stops there) -> AREA01 sub 1 -> AREA02 sub 1 -> ...
    'g11': ('g49', G49_ROUTE, G49_TAKES),
    # AREA01 sub 1 (g58's end, in play) -> door [15]: AREA06 sub 1 (g59, a real area change) -> the
    # area change into AREA16 sub 0 (g60, a real one) -> the barrier -> the curtains -> sub 1 -> ...
    # The card g59 loads AREA01 sub 1 (tools/export_area01s1_assets.py, lane PORT-A01S1; before it
    # the chain started from card g60, AREA06 sub 1: lane PORT-A06S1).
    'g16': ('g59', mrb.G16_ROUTE, ('g59_area01_area06', 'g60_area06_area16', 'g61_area16_water', 'g62_area16_sub1',
                                   'g63_area16_recharge', 'g64_area16_booth', 'g65_area16_vaccine', 'g66_area16_bed',
                                   'g67_area16_out', 'g68_area06_area01')),
    # The endgame (lane PORT-LIVE-A2021, route_g20): AREA04 sub 1 entry 7 (g72's end, in play after
    # the lift [10]'s arrival scene, which the card's LOAD GAME runs again: SCENE_WAIT_AT_START) ->
    # the radio, doors [46] / [48]: AREA20 entry 0 (a real area change) -> dog tag 8 -> [32]'s quad:
    # script 0x8270B0, the hovercraft movie, AREA21 entry 3 (a real one) -> the prep -> the Origin
    # fight's three phases -> the ending. (From card g72, AREA13 entry 7, the chain leaves the rows
    # at g72 row 4773 by AREA13's beasts: rand() differs from row 1.)
    'g20': ('g73', mrb.G20_ROUTE, ('g73_area04_area20', 'g74_area20_tag8', 'g75_area20_area21', 'g76_area21_prep',
                                   'g77_area21_phase1', 'g78_area21_phase2', 'g79_area21_phase3',
                                   'g80_area21_ending')),
    # The ending alone (lane PORT-A21-ENDING), while g20 does not reach it: card g80 (g79's end,
    # AREA21 after the Origin's phase 3) -> the ending script -> 001AD740: the credits 0022D380,
    # screen module 0x37's two pictures, the clear save 00225A00 / 00225AC0(1) -> +9 = 4.
    'g21': ('g80', mrb.G20_ROUTE, ('g80_area21_ending',)),
}


# The behaviours the trace lists per frame (EM_START_AT_TRACE_KINDS): the creatures that roam.
KINDS = {'g17': '141d20,133e30', 'g16': '12a5d0,1c1030', 'g18': '141d20,147390,133e30'}
# Chains whose card rebuilds records the recording had mid-cycle: at frame 0 they take the source
# state's bytes (EM_START_AT_RESTORE, comparison only). g11: AREA11's security gun (its patrol
# timers ran through the cable ride's scene) and the fan pair's spin (em_area11_bindings_restore_owners).
RESTORE_AT_START = {'g11', 'g18'}
# Chains whose card's LOAD GAME runs a scene the card's source state had already played: the
# script's row 0 waits for 0x70003B8D to return to 0 (EM_START_AT_SCENE_WAIT, comparison only).
# g20's card g73 is g72's end, AREA04 sub 1 entry 7 at control after the lift [10]'s arrival
# scene; a LOAD GAME there runs that scene again.
SCENE_WAIT_AT_START = {'g20'}
# Takes at whose row 0 the chain takes the vitals (health, infection) and the restore kinds of
# em_scene_restore_bugs again from the previous take's end image (EM_START_AT_RESTORE's later
# entries, comparison only). g18: the card stretch g41 is steered (the card restarts AREA14's beasts
# and the camera; its stick heading leaves the rows at row 6) and takes a hit the original did not
# (row 1488, 20 health); from g42's row 0 (AREA18 after a real area change) the chain plays the pad
# alone, and the original's vitals there keep the walk (health changes the gait) on the row.
RESTORE_AT_TAKES = {'g18': ('g42_area18_area14',)}


def load(route, takes):
    """The takes' traces stitched on their counters: (rows, inputs, starts).
    rows[f] is the chain's frame f (each row tagged with its take and local
    frame); starts[k] the chain frame of take k's row 0."""
    rows, inputs, starts = [], [], []
    base = None
    for k, take in enumerate(takes):
        t = json.loads((refs.capture(take_route(route, take)) / take / 'trace.json').read_text())
        assert not t.get('teleports') and len(t['rows']) == t['frames'] + 1, take
        if base is None:
            base = t['first_counter']
        off = t['first_counter'] - base
        assert off == len(rows), (take, 'the takes are not consecutive', off, len(rows))
        starts.append(off)
        for r in t['rows']:
            rows.append(dict(r, take=k, local=r['f'], f=r['f'] + off))
        for i in t['inputs']:
            # A take's first pad changes are read at its rows 0 and 1, so their inputs lie in the
            # previous take's last rows (f = k - 2: capture_from_take clamps them to 0 for a lone
            # beat; 'k' keeps the row): g48's first stick at row 1 is the chain's frame before g48.
            # The first take's too: f = -1 / -2 are set before the script's first frame (the card's
            # row 1 reads g59's first stick in the original; EM_START_AT_PAD takes f >= -2).
            f = i['k'] - PAD_LAG if 'k' in i else i['f']
            inputs.append(dict(i, f=f + off))
        inputs.sort(key=lambda i: i['f'])
        hits = (t.get('vitals') or {}).get('hits', [])
        for h in hits:
            rows[off + h['f']]['hit'] = h['hp']
    return rows, inputs, starts


def write_inputs(out, rows, inputs, starts):
    pad = out / 'chain.pad'
    lines, sent = [], None
    for r in inputs:
        value = (r['buttons'], r['lx'], r['ly'])
        if value != sent:
            lines.append(f'{r["f"]} {value[0]:x} {value[1]} {value[2]}')
            sent = value
    last = len(rows) - 1
    assert len(lines) <= 65536, 'more pad rows than EM_START_AT_PAD holds (PAD_MAX, em_start_at.c)'
    pad.write_text('\n'.join([f'EMA1 1 {last} 1'] + lines) + '\n')
    rands = out / 'chain.rands'
    rands.write_text(''.join(f'{r["f"]} {r["rand"]}\n' for r in rows))
    clocks = None
    if all('clock' in r for r in rows):
        clocks = out / 'chain.clocks'
        clocks.write_text(''.join(f'{r["f"]} {r["clock"]:x}\n' for r in rows))
    # Load anchors: the first row after each load segment (phase L), em_replay.c's segment rule
    # (the port's loads are shorter: its disc answers at host speed).
    # The original places the player inside the first tick after its load (001B07C0); the port's
    # unstepped load frames place it before its first step: that step is the placement row (the
    # first row within three whose position leaves the one before).
    anchors = []
    for i in range(1, len(rows)):
        if rows[i - 1]['ph'] == 'L' and rows[i]['ph'] != 'L':
            moved = [k for k in range(i, min(i + 4, len(rows))) if rows[k]['pos'] != rows[k - 1]['pos']]
            anchors.append(moved[0] if moved else i)
    loads = None
    if anchors:
        loads = out / 'chain.phases'
        loads.write_text(''.join(f'{f}\n' for f in anchors))
    marks = ','.join(str(s - 1) for s in starts[1:])
    return pad, rands, clocks, loads, marks, anchors


def row0_fade(row):
    fade = bytes.fromhex(row['fade'])
    return fade[4] if fade[0] else 0


def run(name, card, rows, files, timeout, kinds, extra=(), steer=False, source_ram=None, restore=False,
        scene_wait=False, restore_more=()):
    pad, rands, clocks, loads, marks, _ = files
    out = OUT / name
    trace = out / 'trace.jsonl'
    log_path = out / 'run.log'
    env = {k: v for k, v in os.environ.items() if not k.startswith('EM_')}
    env.update(mrb.HOST)
    # The recorded disc-drive timing (as tools/watch_takes.py): the takes were played on the PS2's
    # drive, and AREA17's arrival script waits 17 frames on it (its op07 sub 0xC fade).
    env['EM_PS2_DISC_DRIVE_TIMING'] = '1'
    env.update(EM_START_AT=card, EM_START_AT_PAD=str(pad), EM_START_AT_RAND=rows[0]['rand'],
               EM_START_AT_RANDS=str(rands), EM_START_AT_FADE=str(row0_fade(rows[0])),
               EM_START_AT_TRACE=str(trace), EM_START_AT_FRAMES='1')
    if kinds:
        env['EM_START_AT_TRACE_KINDS'] = kinds
    if scene_wait:
        env['EM_START_AT_SCENE_WAIT'] = '1'
    if restore and source_ram and source_ram.exists():
        env['EM_START_AT_RESTORE'] = ','.join([f"{source_ram}:0:{rows[0].get('clock', 0):x}"] +
                                              [f"{ram}:{f}:{rows[f].get('clock', 0):x}" for ram, f in restore_more])
    if source_ram and source_ram.exists():
        # The status pages' background D_002655A0 as the card's source state holds it (its pulse
        # timers draw rand(); a card started from boot holds the ELF's .data image instead).
        env['EM_START_AT_STATUS_BG'] = str(source_ram)
    env.update(v.split('=', 1) for v in extra)
    if steer:
        path = out / 'chain.path'
        upto = len(rows) if steer is True else steer
        path.write_text(''.join(f'{r["f"]} {r["pos"][0]!r} {r["pos"][2]!r} {r["yaw"]!r}\n' for r in rows[:upto]))
        env['EM_START_AT_PATH'] = str(path)
    if clocks:
        env['EM_START_AT_CLOCKS'] = str(clocks)
    elif 'clock' in rows[0]:
        env['EM_START_AT_CLOCK'] = f"0:{rows[0]['clock']:x}"
    if loads:
        env['EM_START_AT_PHASES'] = str(loads)
    if marks:
        env['EM_START_AT_MARKS'] = marks
    start = time.time()
    with open(log_path, 'w') as log:
        proc = subprocess.Popen([str(mrb.BIN)], cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            while proc.poll() is None:
                time.sleep(0.5)
                text = log_path.read_text(errors='replace')
                if 'pad end at' in text and 'block at the pad end' in text:
                    break
                if mrb.FAULT.search(text) or time.time() - start > timeout:
                    break
                if '001AC070 again' in text:     # back at the title (the player died): nothing more to compare
                    break
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
    text = log_path.read_text(errors='replace')
    faults = [ln for ln in text.splitlines() if (mrb.FAULT.search(ln) and 'block ' not in ln) or
              '001AC070 again' in ln]
    return time.time() - start, faults, trace, log_path


def compare(rows, takes, trace, anchors=()):
    """Per take: frames compared, the first frame of each kind of difference.
    The rand() word of a load anchor's row is not compared: the port's word
    there follows its own (shorter) load, not the recording's last load row."""
    port = {}
    if trace.exists():
        for line in trace.read_text(errors='replace').splitlines():
            try:
                d = json.loads(line)
            except ValueError:
                continue
            port[d['t']] = d        # the last line of a frame (an anchor may revisit one)
    report = []
    reached = max(port) if port else -1
    for k, take in enumerate(takes):
        mine = [r for r in rows if r['take'] == k]
        first = dict(pos=None, yaw=None, area=None, rand=None, held=None)
        compared = exact = 0
        hits_o = [(r['local'], r['hit']) for r in mine if 'hit' in r]
        hits_p = []
        prev_hp = None
        for r in mine:
            p = port.get(r['f'])
            if p is None:
                continue
            if prev_hp is not None and p['hp'] < prev_hp - 1e-3:
                hits_p.append((r['local'], p['hp']))
            prev_hp = p['hp']
            if r['ph'] == 'L':
                continue
            compared += 1
            away = math.dist(p['pos'], r['pos'])
            same = away <= SAME
            if not same and first['pos'] is None:
                first['pos'] = (r['local'], away, p['pos'], r['pos'])
            if 'yaw' in p:
                turned = abs(math.remainder(p['yaw'] - r['yaw'], math.tau))
                if turned > TURNED and first['yaw'] is None:
                    first['yaw'] = (r['local'], turned)
                same = same and turned <= TURNED
            if p.get('area') and p['area'] != r['area4'][:6] and first['area'] is None:
                first['area'] = (r['local'], p['area'], r['area4'][:6])
            # The script selector 0x70003B8D: the row's phase 'C' (em_replay.c's rule) against the
            # port's s8d (traces before it have none).
            if 's8d' in p and (p['s8d'] != 0) != (r['ph'] == 'C') and first['held'] is None:
                first['held'] = (r['local'], p['s8d'], r['ph'])
            if r['f'] not in anchors and p['rand'] != int(r['rand'], 16) and first['rand'] is None:
                first['rand'] = (r['local'], f'{p["rand"]:08x}', r['rand'])
            exact += same
        last = max((r['local'] for r in mine if r['f'] in port), default=None)
        lines = [f'{take}: {len(mine)} rows, {compared} compared, {exact} on the row'
                 + (f', the port reached row {last}' if last is not None else ', not reached')]
        if first['pos']:
            f, d, pp, rp = first['pos']
            lines.append(f'  position leaves at row {f}: {d:.3f} away (port {[round(v, 3) for v in pp]}, '
                         f'row {[round(v, 3) for v in rp]})')
        if first['yaw']:
            lines.append(f'  heading leaves at row {first["yaw"][0]}: {first["yaw"][1]:.4f} off')
        if first['area']:
            lines.append(f'  area bytes differ at row {first["area"][0]}: port {first["area"][1]}, '
                         f'row {first["area"][2]}')
        if first['held']:
            lines.append(f'  script selector differs at row {first["held"][0]}: port 3B8D {first["held"][1]}, '
                         f'row phase {first["held"][2]}')
        if first['rand']:
            lines.append(f'  rand() word differs at row {first["rand"][0]}: port {first["rand"][1]}, '
                         f'row {first["rand"][2]} (the frame before drew differently)')
        if hits_o or hits_p:
            lines.append(f'  hits (row, health after): original {hits_o}, port {hits_p}')
        report += lines
    return report, reached


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('chain', nargs='?')
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--until', help='the chain\'s last take (a prefix of its name)')
    parser.add_argument('--kinds', help='behaviours whose records the trace lists (hex, comma separated; '
                        'default the chain\'s KINDS)')
    parser.add_argument('--set', action='append', default=[], metavar='EM_X=V',
                        help='an extra variable for the run (instrumentation such as EM_RAND_TRACE)')
    parser.add_argument('--steer', nargs='?', const='*', metavar='TAKE',
                        help='steer along the recorded path (EM_START_AT_PATH, mid_route_beats\' rule) where the '
                        'player falls behind, through TAKE (a prefix; default the whole chain); without it the '
                        'pad alone plays')
    parser.add_argument('--name', help='the output folder under build/guided_chain (default: the chain\'s name; '
                        'a test\'s own run, e.g. tools/test_far_light_readback_reference.py)')
    parser.add_argument('--compare-only', action='store_true', help='compare the last run\'s trace again')
    parser.add_argument('--drive-replay', action='store_true', help='serve the original\'s recorded per-read drive '
                        'timing (tools/record_drive_replay_guided.py tables, EM_PS2_DRIVE_REPLAY with '
                        'EM_START_AT_COUNTER0; docs/IOP_STREAM.md "Guided takes"): a comparison aid, off by default')
    args = parser.parse_args(argv)
    if args.list or not args.chain:
        for n, (card, route, takes) in CHAINS.items():
            print(f'{n}: EM_START_AT={card} {route}: {" -> ".join(takes)}')
        return 0
    card, route, takes = CHAINS[args.chain]
    if args.until:
        cut = [i for i, t in enumerate(takes) if t.startswith(args.until)]
        if not cut:
            sys.exit(f'guided_chain: no take {args.until!r} in {args.chain}')
        takes = takes[:cut[0] + 1]
    if not mrb.BIN.exists():
        sys.exit(f'{mrb.BIN} is missing (make all)')
    name = args.name or args.chain
    out = OUT / name
    out.mkdir(parents=True, exist_ok=True)
    rows, inputs, starts = load(route, takes)
    steer = None
    if args.steer == '*':
        steer = True
    elif args.steer:
        k = [i for i, t in enumerate(takes) if t.startswith(args.steer)]
        if not k:
            sys.exit(f'guided_chain: no take {args.steer!r} in the chain')
        steer = starts[k[0] + 1] if k[0] + 1 < len(starts) else True
    files = write_inputs(out, rows, inputs, starts)
    print(f'{args.chain}: card {card}, {" -> ".join(takes)}: {len(rows)} frames; load anchors '
          f'{files[5]}', flush=True)
    if not args.compare_only:
        timeout = float(os.environ.get('EM_GUIDED_CHAIN_TIMEOUT', '1800'))
        source = json.loads((refs.capture(route) / takes[0] / 'trace.json').read_text()).get('source_state')
        source_ram = Path(source).parent / 'eeMemory.bin' if source else None
        more = []
        for take in RESTORE_AT_TAKES.get(args.chain, ()):
            if take in takes:
                k = takes.index(take)
                state = json.loads((refs.capture(take_route(route, take)) / take / 'trace.json').read_text())
                more.append((Path(state['source_state']).parent / 'eeMemory.bin', starts[k]))
        extra = list(args.set)
        if args.drive_replay:
            import drive_replay
            info = drive_replay.guided_table(takes, out / 'drive_replay.txt')
            gaps = f'; {len(info["missing"])} with no table take the drive model ({", ".join(info["missing"][:4])}' \
        f'{", ..." if len(info["missing"]) > 4 else ""})' if info['missing'] else ''
            print(f'drive replay: {info["entries"]} recorded reads from {len(info["takes"])} of {len(takes)} '
                  f'takes{gaps}', flush=True)
            if info['entries']:
                extra += [f'EM_PS2_DRIVE_REPLAY={info["table"]}', f'EM_START_AT_COUNTER0={rows[0]["counter"]}']
        seconds, faults, trace, log_path = run(name, card, rows, files, timeout,
                                               args.kinds or KINDS.get(args.chain), extra, steer, source_ram,
                                               args.chain in RESTORE_AT_START, args.chain in SCENE_WAIT_AT_START, more)
        print(f'run: {seconds:.0f} s, {log_path.relative_to(ROOT)}')
        for f in faults[:3]:
            print(f'  stopped: {f[:240]}')
    trace = out / 'trace.jsonl'
    report, reached = compare(rows, takes, trace, set(files[5]))
    if reached >= 0:
        r = rows[min(reached, len(rows) - 1)]
        report.insert(0, f'the port\'s script reached chain frame {reached} of {len(rows) - 1} '
                         f'({takes[r["take"]]} row {r["local"]})')
    (out / 'report.txt').write_text('\n'.join(report) + '\n')
    print('\n'.join(report))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
