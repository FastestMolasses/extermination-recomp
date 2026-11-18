#!/usr/bin/env python3
"""Compare the port's attract demo (EM_STARTUP_TEST=attract rows) with the
original's capture (tools/capture_attract_demo.py rows.jsonl), record by
record: the port row and the capture row whose 001ACA20 record counter +0x16
(sub 1, after its 001B5BC0) is the same. Compared per record: the pad block
the demo fed (D_00810E70, E64, E65, E57), the area block D_00810700..703,
the player's position and yaw (player +0xA0 / +0xA4 / +0xA8 / +0xC4), the
fade substate D_0028A9A0 and the rand() state. The loads before sub 1 run at
host speed in the port (the disc answers at once), so rows are aligned on
the record counter, not on the engine frame.

  compare_attract_demo.py <port log> <rows.jsonl> [--demo N] [--tol T] [--first K]

Prints the first differing record per field and the count of equal records;
exit 1 on any difference inside the compared span."""
from __future__ import annotations

import argparse
import json
import math
import struct
import sys


def port_rows(path, demo):
    out = {}
    for line in open(path, errors='replace'):
        if not line.startswith('attract row '):
            continue
        f = line.split()
        if int(f[3]) != demo:
            continue
        task = bytes.fromhex(f[5])
        if task[1] != 4 or task[3] != 1:
            continue
        n = task[0x16 - 8] | task[0x17 - 8] << 8
        out.setdefault(n, dict(area=f[7], held=int(f[9], 16), lx=int(f[10], 16), ly=int(f[11], 16), gait=int(f[12], 16),
                      pos=tuple(float(x) for x in f[14:17]), yaw=float(f[18]), fade=int(f[20]),
                      rand=int(f[22], 16), rc=int(f[24])))
    return out


def capture_rows(path, demo):
    out = {}
    for line in open(path):
        r = json.loads(line)
        if r['demo'] != demo:
            continue
        task = bytes.fromhex(r['task'])
        if task[9] != 4 or task[0xB] != 1:
            continue
        n = struct.unpack_from('<H', task, 0x16)[0]
        pad = bytes.fromhex(r['pad'])
        pl = bytes.fromhex(r['player'])
        out.setdefault(n, dict(area=r['area'], held=struct.unpack_from('<H', pad, 0x20)[0], lx=pad[0x14],
                               ly=pad[0x15], gait=pad[0x07], pos=struct.unpack_from('<3f', pl, 0),
                               yaw=struct.unpack_from('<f', pl, 0x24)[0],
                               fade=struct.unpack_from('<h', bytes.fromhex(r['fade']), 0)[0],
                               rand=struct.unpack_from('<I', bytes.fromhex(r['rand']), 0)[0]))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('port')
    ap.add_argument('capture')
    ap.add_argument('--demo', type=int, default=0)
    ap.add_argument('--tol', type=float, default=1e-3, help='position tolerance (world units)')
    ap.add_argument('--first', type=int, default=None, help='compare records 1..K only')
    ap.add_argument('--quiet', action='store_true')
    args = ap.parse_args()
    port, cap = port_rows(args.port, args.demo), capture_rows(args.capture, args.demo)
    keys = sorted(k for k in set(port) & set(cap) if k >= 1)   # record 0: step C's own pad, no record yet
    if args.first is not None:
        keys = [k for k in keys if k <= args.first]
    if not keys:
        print('compare_attract_demo: no common records')
        return 1
    fields = ('area', 'held', 'lx', 'ly', 'gait', 'pos', 'yaw', 'fade', 'rand')
    first = {}
    equal = 0
    for n in keys:
        p, c = port[n], cap[n]
        bad = []
        for k in fields:
            if k == 'pos':
                same = all(math.isclose(a, b, abs_tol=args.tol) for a, b in zip(p[k], c[k]))
            elif k == 'yaw':
                same = math.isclose(p[k], c[k], abs_tol=1e-3)
            else:
                same = p[k] == c[k]
            if not same:
                bad.append(k)
                first.setdefault(k, (n, p[k], c[k]))
        equal += not bad
    print(f'attract demo {args.demo}: records {keys[0]}..{keys[-1]} compared: {equal}/{len(keys)} equal '
          f'(port has {len(port)}, capture {len(cap)})')
    for k, (n, p, c) in sorted(first.items(), key=lambda kv: kv[1][0]):
        print(f'  first {k} difference at record {n}: port {p} original {c}')
    return 0 if equal == len(keys) else 1


if __name__ == '__main__':
    sys.exit(main())
