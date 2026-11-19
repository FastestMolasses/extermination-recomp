#!/usr/bin/env python3
"""export_sight_tables.py - the boot ELF data the weapon sights read.

The sight drawers 001999C0 calls (src/game/em_sight.c, served live by
src/game/em_sight_live.c; docs/CAMERA_LIVE.md section 7) read .data words
no other port module owns. This tool copies them from the user's own pinned
boot ELF (config/SCUS_971.12, file offset = address - 0x100000 + 0x300) into
an ignored asset; nothing is embedded here.

Blocks (original address, bytes, what reads them):
  0x00253240   0x190   D_00253240 (001DB830's colour quadword), D_00253250..
                       D_0025327F (001DB9D0's centre, radius term and vertex
                       templates), D_00253280 (001DBCB0's colour quadword),
                       D_00253290 / D_002532D0 / D_00253310 / D_00253350
                       (001DC610's line templates) and D_00253390 (001DC960's)
  0x0026E6F0   0x120   the strings the sight frames' text runs (001CBA50)
                       name, D_0026E6F0..D_0026E80F
  0x0026A990   0x150   the radar 0022E7F0's heading fan corners D_0026A990 /
                       A0 / B0 and its three 002082B0 ring records
                       D_0026A9C0 / D_0026AA20 / D_0026AA80 (0x60 each)

--verify-ram (default: the opening capture and every AREA11 route capture
00..14) checks that each block equals captured RAM (nothing writes them).

Output (disc-derived: git-ignored assets/ only):
  assets/sight_tables.emst, little-endian (the layout of
  tools/export_render_context.py's .emrc):
    0x00 'EMST', u32 version 1, u32 block count, u32 0
    0x10 count x (u32 address, u32 size, u32 file offset, u32 0)
    then the block bytes at their offsets (16-aligned)

Runs natively on arm64 macOS (pure Python).

Usage (port root):
  python3 tools/export_sight_tables.py
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
ELF_SHA256 = 'ee052236783e7d3e865754d3ff9fee71290addeb7d146c86caa7ff2724d1e17a'
BLOCKS = ((0x00253240, 0x190), (0x0026E6F0, 0x120), (0x0026A990, 0x150))


def elf_block(elf: bytes, address: int, size: int) -> bytes:
    offset = address - 0x100000 + 0x300
    if offset < 0x300 or offset + size > 0x175E00:
        raise SystemExit(f'{address:#x}: outside the loadable section')
    return elf[offset:offset + size]


def serialize(blocks) -> bytes:
    head = struct.pack('<4s3I', b'EMST', 1, len(blocks), 0)
    table, body = b'', b''
    offset = 0x10 + 0x10 * len(blocks)
    for address, data in blocks:
        at = offset + len(body)
        table += struct.pack('<4I', address, len(data), at, 0)
        body += data + bytes(-len(data) % 16)
    return head + table + body


def verify(blocks, ram: bytes) -> int:
    for address, data in blocks:
        got = ram[address:address + len(data)]
        if got != data:
            at = next(i for i in range(len(data)) if got[i] != data[i])
            raise SystemExit(f'{address + at:#x}: ELF {data[at]:#04x}, captured {got[at]:#04x}')
    return sum(len(d) for _, d in blocks)


def default_captures():
    ref = DECOMP / 'build/startup-reference/opening_ee.bin'
    route = DECOMP / 'build/s87/route'
    return ([ref] if ref.exists() else []) + sorted(
        p for p in route.glob('*/eeMemory.bin') if not p.parent.name.startswith('15'))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--elf', type=Path, default=DECOMP / 'config/SCUS_971.12')
    ap.add_argument('--out', type=Path, default=ROOT / 'assets/sight_tables.emst')
    ap.add_argument('--verify-ram', type=Path, action='append', default=None)
    ap.add_argument('--no-verify', action='store_true')
    args = ap.parse_args(argv)
    elf = args.elf.read_bytes()
    if hashlib.sha256(elf).hexdigest() != ELF_SHA256:
        raise SystemExit(f'{args.elf}: not the pinned SCUS-97112 boot ELF')
    blocks = [(a, elf_block(elf, a, n)) for a, n in BLOCKS]
    captures = [] if args.no_verify else (args.verify_ram or default_captures())
    compared = sum(verify(blocks, p.read_bytes()) for p in captures)
    if args.out.is_symlink():
        raise SystemExit(f'{args.out}: a link (it would write through to another checkout)')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(serialize(blocks))
    print(f'wrote {args.out}: {len(blocks)} blocks, {sum(len(d) for _, d in blocks)} bytes; '
          f'{compared} bytes equal in {len(captures)} captures')
    return 0


if __name__ == '__main__':
    sys.exit(main())
