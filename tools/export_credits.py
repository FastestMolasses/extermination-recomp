#!/usr/bin/env python3
"""export_credits.py - the ending credits' rows for the port (0022D380).

0022D380 (byte-matched; src/game/em_credits_original.c) walks the row table
D_00268B40 ({type, text address} per row, ended by the type 4) and draws
the texts those rows name. This tool copies the two .data windows that
holds from the user's own boot ELF into the port's ignored assets: the
table up to and including its end row, and the span of every text a row
names (each through its NUL). Nothing is interpreted or changed.

Output (disc-derived: git-ignored assets/ only):
  assets/credits/credits.emcr   'EMCR', version 1, the window count, then
                                per window its original address, its size
                                and its bytes (all little-endian words)

Usage (port root; native arm64 macOS, Python stdlib):
  python3 tools/export_credits.py [--elf ../Extermination/config/SCUS_971.12]
"""
from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ELF_SHA256 = 'ee052236783e7d3e865754d3ff9fee71290addeb7d146c86caa7ff2724d1e17a'
TABLE = 0x268B40
OUT = ROOT / 'assets/credits/credits.emcr'


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--elf', type=Path, default=ROOT.parent / 'Extermination/config/SCUS_971.12')
    ap.add_argument('--out', type=Path, default=OUT)
    a = ap.parse_args()
    elf = a.elf.read_bytes()
    if hashlib.sha256(elf).hexdigest() != ELF_SHA256:
        raise SystemExit(f'{a.elf}: not the pinned SCUS-97112 boot ELF')

    def at(address: int, size: int) -> bytes:
        off = address - 0x100000 + 0x300
        if not 0x300 <= off <= len(elf) - size:
            raise SystemExit(f'{address:#x}: outside the boot segment')
        return elf[off:off + size]

    rows, texts = 0, []
    while True:
        kind, text = struct.unpack('<iI', at(TABLE + 8 * rows, 8))
        rows += 1
        if kind == 4:
            break
        if kind in (1, 2, 3):
            texts.append(text)
    lo = min(texts)
    end = max(texts)
    hi = end + at(end, 0x400).index(b'\0') + 1
    for t in texts:
        if b'\0' not in at(t, hi - t):
            raise SystemExit(f'text {t:#x} has no NUL inside the window')
    windows = [(TABLE, at(TABLE, 8 * rows)), (lo, at(lo, hi - lo))]
    blob = bytearray(b'EMCR' + struct.pack('<II', 1, len(windows)))
    for base, data in windows:
        blob += struct.pack('<II', base, len(data)) + data
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_bytes(bytes(blob))
    print(f'{a.out}: {rows} rows, {len(texts)} drawn texts, windows '
          + ', '.join(f'{b:#x}+{len(d):#x}' for b, d in windows))


if __name__ == '__main__':
    main()
