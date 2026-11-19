#!/usr/bin/env python3
"""Step K's Z read-back (001D7410) and the far light's flare (001CEFD0) against the
original's (docs/AREA21_PORT.md "Far light": "Step K's Z read-back").

The original's side: tools/record_far_light_readback.py replays Claude's kept guided
takes on the PCSX2 fork with the software renderer (the exact GS: its local memory
holds the drawn Z) and records every step K transfer (the take's tick, the bank
ctx +0x9C, the point, the 192 bytes) and every 001CEFD0 call (decomp
build/fork_refs/far_light_readback/<take>.json). The port plays the same takes as one
continuous chain (tools/guided_chain.py g20: card g73 -> g74 -> g75 -> ..., the pad
alone) with the GS frame model drawing (EM_HEADLESS_RASTER=1) and EM_READBACK_TRACE.
A transfer of the take's tick k is the chain's script frame start(take) + k.

Compared at every original transfer, and no port transfer at another frame:
  - the point (SSAX, SSAY), the source (SBP = ZBUF_1's ZBP << 5, SBW = FRAME_1's
    FBW, from the original's D_00810F00) and the status (the port read its own
    drawn Z: never the fail-safe);
  - when the port's frame is in the original's phase (the same bank: in the
    original the bank, D_00810E80 and the field D_00810E88 are one bit, and each
    bank's draw environment has its own half-line XYOFFSET, so the bank says which
    field's Z the transfer reads): the read-back word 001D73A0 returns (the flare
    decision, word 0), which pixels are 0 (nothing drawn), and the Z values: exact,
    or off by one LSB, or more (the GS model's open Z items, docs/GS_EXACT.md "Not
    exact": pinned counts below, any change fails);
  - the flare: the 001CEFD0 calls from one transfer to the next, after an in-phase
    transfer.
The chain's loads are shorter in the port (the disc answers at host speed; the
script clock realigns to the recording after a load), and a load that passes an
odd number of frames fewer leaves the port's frames in the other bank from then on
(until the next such load): those transfers read the other field's Z (half a row's
gradient apart), so only their point and status are compared and they are counted
(pinned). That is the chain's comparison alignment, not step K.

Default run: the chain through g76 (g75's 1251 and g76's 1548 transfers; ~19k
frames with the raster, about 4 minutes: an end-to-end capture comparison with its
own timeout in tools/test_sweep.py). EM_TEST_FULL=1: through g77.
"""
import json
import os
import subprocess
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import reference_mode  # noqa: E402
import guided_chain  # noqa: E402

DECOMP = ROOT.parent / 'Extermination'
REFS = DECOMP / 'build/fork_refs/far_light_readback'
NAME = 'far_light_readback'
OUT = ROOT / 'build/guided_chain' / NAME
TAKES = ['g75_area20_area21', 'g76_area21_prep'] + (['g77_area21_phase1'] if reference_mode.FULL else [])
# Pinned per take: in-phase transfers with Z one LSB off / more than one off
# (GS_EXACT.md "Not exact": Z), in-phase transfers whose decision (the word at +8
# zero or not) differs, and the transfers in the other phase. Measured 2026-11-19,
# lane PORT-CARD-CHAINS: the chain starts in the recording's bank and its loads take
# the takes' recorded loader reads (tools/guided_chain.py defaults, STARTUP.md
# "Guided chains"), so every transfer of g75 .. g77 is in the original's phase
# (lane PORT-FLARE-Z had g76 428 and all of g77 in the other). g77 had 139 decisions
# (ticks 1687..1747, 1763..1765, 4474..4489, 6618..6639, 7273..7294, 7597..7609,
# 8558..8561, 12025) and 25 coverages differ where the port's Z was 0: the port did
# not draw 001E0DF0's second CALL, the +0x1E8 list of 001E1AD0 (the sea grid; its
# Z-only strips are the depth there). Lane PORT-FLARE-DRAW (2026-11-19) draws it
# (em_background_live): every decision and coverage equal, 'more' 349 -> 324
# (AREA21_PORT.md "Step K's Z read-back").
PINNED = {
    'g75_area20_area21': dict(lsb1=1, more=0, decision=0, coverage=0, other_phase=0, other_phase_decision=0),
    'g76_area21_prep': dict(lsb1=297, more=8, decision=0, coverage=0, other_phase=0, other_phase_decision=0),
    'g77_area21_phase1': dict(lsb1=213, more=324, decision=0, coverage=0, other_phase=0, other_phase_decision=0),
}


def z24(h):
    b = bytes.fromhex(h)
    return [b[i] | b[i + 1] << 8 | b[i + 2] << 16 for i in range(0, len(b), 3)]


def original():
    takes, env = {}, None
    for take in TAKES:
        path = REFS / f'{take}.json'
        if not path.exists():
            sys.exit(f'missing {path}: run tools/record_far_light_readback.py {take}')
        doc = json.loads(path.read_text())
        if doc.get('renderer') != 13:
            sys.exit(f'{path}: not a software-renderer recording')
        if env is None:
            env = doc['env']
        elif doc['env'] != env:
            sys.exit(f'{take}: D_00810F00 differs from the first take\'s')
        takes[take] = doc
    return takes, bytes.fromhex(env)


def port(again):
    trace = OUT / 'readback.txt'
    if again:
        return read_trace(trace), ''
    env = {k: v for k, v in os.environ.items() if k != 'EM_TEST_FULL'}
    OUT.mkdir(parents=True, exist_ok=True)
    if trace.exists():
        trace.unlink()
    cmd = [sys.executable, str(ROOT / 'tools/guided_chain.py'), 'g20', '--until', TAKES[-1].split('_')[0],
           '--name', NAME, '--set', 'EM_HEADLESS_RASTER=1', '--set', f'EM_READBACK_TRACE={trace}']
    p = subprocess.run(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0 or not trace.exists():
        print(p.stdout[-3000:])
        sys.exit('guided_chain failed')
    return read_trace(trace), p.stdout


def read_trace(trace):
    rows = {}
    for line in trace.read_text().splitlines():
        t, calls, bank, sbp, sbw, x, y, rc, data = line.split()
        rows[int(t)] = dict(flares=int(calls), bank=int(bank), sbp=int(sbp), sbw=int(sbw), x=int(x), y=int(y),
                            rc=int(rc), data=data)
    return rows


def main():
    takes, env = original()
    sbp = ((env[0x10] | env[0x11] << 8) & 0x1FF) << 5
    sbw = env[0x02] & 0x3F
    card, route, chain = guided_chain.CHAINS['g20']
    _, _, starts = guided_chain.load(route, chain[:chain.index(TAKES[-1]) + 1])
    start = {take: starts[chain.index(take)] for take in TAKES}
    rows, _ = port('--compare-only' in sys.argv[1:])
    bad, counts, seen = [], {}, set()
    for take, doc in takes.items():
        c = counts[take] = Counter()
        flare_ticks = sorted(tick for _, tick in doc['flares'])
        events = doc['readbacks']
        for i, (vs, tick, x, y, data, bank) in enumerate(events):
            t = start[take] + tick
            r = rows.get(t)
            where = f'{take} tick {tick} (script frame {t})'
            seen.add(t)
            if r is None:
                bad.append(f'{where}: the port made no transfer')
                continue
            c['transfers'] += 1
            if (r['x'], r['y']) != (x, y):
                bad.append(f'{where}: point {r["x"]},{r["y"]}, original {x},{y}')
            if (r['sbp'], r['sbw']) != (sbp, sbw):
                bad.append(f'{where}: SBP {r["sbp"]:#x} SBW {r["sbw"]}, original {sbp:#x} {sbw}')
            if r['rc'] != 0:
                bad.append(f'{where}: status {r["rc"]}: the fail-safe ran (no drawn frame)')
                continue
            if r['bank'] != bank:
                c['other_phase'] += 1
                if (data[16:24] == '00000000') != (r['data'][16:24] == '00000000'):
                    c['other_phase_decision'] += 1
                continue
            o, p = z24(data), z24(r['data'])
            if (data[16:24] == '00000000') != (r['data'][16:24] == '00000000'):
                # Counted and pinned (PINNED 'decision'); its coverage, Z and the flare
                # calls after it follow from it and are not compared.
                c['decision'] += 1
                continue
            if [v == 0 for v in o] != [v == 0 for v in p]:
                c['coverage'] += 1      # pinned: the same draw difference's edges (decision equal)
            worst = max(abs(a - b) for a, b in zip(o, p))
            c['exact' if worst == 0 else 'lsb1' if worst == 1 else 'more'] += 1
            if data[16:24] == '00000000':
                c['decision_flare'] += 1
            # The flare: 001CEFD0 calls in the frames after this transfer, up to the next one.
            if i + 1 < len(events):
                nt = events[i + 1][1]
                nr = rows.get(start[take] + nt)
                if nr is not None:
                    want = sum(1 for f in flare_ticks if tick < f <= nt)
                    if nr['flares'] - r['flares'] != want:
                        bad.append(f'{where}: 001CEFD0 calls up to tick {nt}: port {nr["flares"] - r["flares"]}, '
                                   f'original {want}')
    last = max(start[t] + takes[t]['readbacks'][-1][1] for t in TAKES if takes[t]['readbacks'])
    # A take's recording starts at its first tick's vsync wait: the transfer of the
    # previous take's last tick (the chain frame start(take)) has no next row there.
    edges = {start[t] for t in TAKES[1:]}
    extra = sorted(t for t in rows if t not in seen and t <= last and t not in edges)
    if extra:
        bad.append(f'{len(extra)} port transfers at frames where the original made none (first: script frame '
                   f'{extra[0]})')
    print(f'{reference_mode.MODE}: SBP {sbp:#x} SBW {sbw}; chain {", ".join(TAKES)}')
    for take in TAKES:
        c = counts[take]
        print(f'  {take}: {len(takes[take]["readbacks"])} original transfers ({len(takes[take]["flares"])} flares); '
              f'port {c["transfers"]}: in phase {c["exact"]} exact, {c["lsb1"]} one LSB, {c["more"]} more '
              f'({c["decision_flare"]} with the word 0), {c["decision"]} with another decision, '
              f'{c["coverage"]} with other pixels at Z 0; '
              f'{c["other_phase"]} in the other phase ({c["other_phase_decision"]} with another decision)')
        pin = PINNED[take]
        for key in ('lsb1', 'more', 'decision', 'coverage', 'other_phase', 'other_phase_decision'):
            if pin[key] is not None and c[key] != pin[key]:
                bad.append(f'{take}: {key} {c[key]}, pinned {pin[key]} (update PINNED with the reason)')
    if bad:
        print('FAIL')
        for b in bad[:20]:
            print('  ' + b)
        if len(bad) > 20:
            print(f'  ... {len(bad) - 20} more')
        return 1
    print('PASS: every transfer\'s point, source and status; in phase its decision, coverage and Z; the flare calls')
    return 0


if __name__ == '__main__':
    sys.exit(main())
