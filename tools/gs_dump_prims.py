#!/usr/bin/env python3
"""The primitives of a GS stream dump (the fork's gs_dump), field by field.

A diagnostic for the GS frame comparisons (docs/DRAW_LIST_AUDIT.md section
4): tools/capture_gs_original.py --gs-dump keeps the ORIGINAL's GIF stream
from loop top R + 1 (r<R>/s1.gs.zst). This tool decodes it with the public
upstream dump layout (a header, the GS state, the privileged registers,
then packets: 0 a GIF transfer (path, size, bytes), 1 a vsync (field), 2 a
FIFO read (size), 3 the privileged registers (8 KiB)) and walks the GIF
tags (PACKED, REGLIST, IMAGE) keeping the context registers, so each drawing
kick is listed with the state it drew under: PRIM, the vertices, FRAME
(FBP, FBMSK), TEST, ALPHA, TEX0, ZBUF, SCISSOR. Filters pick the primitives
that can write a given pixel's alpha (--pixel X,Y in window coordinates
of the FRAME it names) or that cover most of a field (--big).

The dump holds disc-derived data (texels, the game's packets): it stays in
ignored build folders and is never committed; this tool prints register
values and coordinates only.

Usage (port root; native arm64 macOS; needs `zstd` on PATH):
  python3 tools/gs_dump_prims.py ../Extermination/build/gspics/gsdump/<take>/r<R>/s1.gs.zst --field 0 --big
"""
import argparse
import struct
import subprocess
import sys

REG_NAMES = {0x00: 'PRIM', 0x01: 'RGBAQ', 0x02: 'ST', 0x03: 'UV', 0x04: 'XYZF2', 0x05: 'XYZ2', 0x06: 'TEX0_1',
             0x07: 'TEX0_2', 0x08: 'CLAMP_1', 0x09: 'CLAMP_2', 0x0A: 'FOG', 0x0C: 'XYZF3', 0x0D: 'XYZ3',
             0x14: 'TEX1_1', 0x15: 'TEX1_2', 0x16: 'TEX2_1', 0x17: 'TEX2_2', 0x18: 'XYOFFSET_1',
             0x19: 'XYOFFSET_2', 0x1A: 'PRMODECONT', 0x1B: 'PRMODE', 0x1C: 'TEXCLUT', 0x22: 'SCANMSK',
             0x34: 'MIPTBP1_1', 0x35: 'MIPTBP1_2', 0x36: 'MIPTBP2_1', 0x37: 'MIPTBP2_2', 0x3B: 'TEXA',
             0x3D: 'FOGCOL', 0x3F: 'TEXFLUSH', 0x40: 'SCISSOR_1', 0x41: 'SCISSOR_2', 0x42: 'ALPHA_1',
             0x43: 'ALPHA_2', 0x44: 'DIMX', 0x45: 'DTHE', 0x46: 'COLCLAMP', 0x47: 'TEST_1', 0x48: 'TEST_2',
             0x49: 'PABE', 0x4A: 'FBA_1', 0x4B: 'FBA_2', 0x4C: 'FRAME_1', 0x4D: 'FRAME_2', 0x4E: 'ZBUF_1',
             0x4F: 'ZBUF_2', 0x50: 'BITBLTBUF', 0x51: 'TRXPOS', 0x52: 'TRXREG', 0x53: 'TRXDIR', 0x54: 'HWREG',
             0x60: 'SIGNAL', 0x61: 'FINISH', 0x62: 'LABEL'}
VERTS = {0: 1, 1: 2, 2: 2, 3: 3, 4: 3, 5: 3, 6: 2, 7: 0}


def read_dump(path):
    raw = subprocess.run(['zstd', '-dc', path], check=True, capture_output=True).stdout
    off = 0
    crc, = struct.unpack_from('<I', raw, off)
    off += 4
    if crc == 0xFFFFFFFF:            # the newer header: its size, then state_version, state_size, ...
        hsize, _, ssize = struct.unpack_from('<III', raw, off)
        off += 4 + hsize
    else:
        ssize, = struct.unpack_from('<I', raw, off)
        off += 4
    off += ssize                      # the GS state
    off += 8192                       # the privileged registers
    packets = []
    while off < len(raw):
        kind = raw[off]
        off += 1
        if kind == 0:
            pathi = raw[off]
            size, = struct.unpack_from('<I', raw, off + 1)
            off += 5
            packets.append(('gif', pathi, raw[off:off + size]))
            off += size
        elif kind == 1:
            packets.append(('vsync', raw[off]))
            off += 1
        elif kind == 2:
            off += 4
        elif kind == 3:
            off += 8192
        else:
            raise ValueError(f'unknown packet {kind} at {off - 1:#x}')
    return packets


class Gs:
    def __init__(self):
        self.r = {}
        self.queue = []
        self.prims = []
        self.field = 0

    def write(self, reg, val):
        name = REG_NAMES.get(reg, f'R{reg:02X}')
        if reg in (0x04, 0x05, 0x0C, 0x0D):
            x, y = (val & 0xFFFF) / 16.0, ((val >> 16) & 0xFFFF) / 16.0
            z = (val >> 32) & (0xFFFFFF if reg in (0x04, 0x0C) else 0xFFFFFFFF)
            self.queue.append((x, y, z, self.r.get('RGBAQ', 0), self.r.get('UV', 0), self.r.get('ST', 0)))
            prim = self.r.get('PRIM', 0)
            n = VERTS[prim & 7]
            if n and len(self.queue) >= n and reg in (0x04, 0x05):
                self.kick(prim, self.queue[-n:])
                if prim & 7 in (0, 1, 3, 6):
                    self.queue = []
                elif prim & 7 == 5:
                    self.queue = [self.queue[0]] + self.queue[-1:]
                else:
                    self.queue = self.queue[-2:]
            return
        if reg == 0x00:
            self.queue = []
        self.r[name] = val

    def kick(self, prim, verts):
        ctx = 2 if prim & 0x200 else 1
        g = lambda n: self.r.get(f'{n}_{ctx}', 0)
        xo = g('XYOFFSET')
        xs = [v[0] - (xo & 0xFFFF) / 16.0 for v in verts]
        ys = [v[1] - ((xo >> 32) & 0xFFFF) / 16.0 for v in verts]
        self.prims.append(dict(field=self.field, prim=prim, ctx=ctx, n=len(self.prims), frame=g('FRAME'),
                               test=g('TEST'), alpha=g('ALPHA'), tex0=g('TEX0'), zbuf=g('ZBUF'),
                               scissor=g('SCISSOR'), fba=g('FBA'), box=(min(xs), min(ys), max(xs), max(ys)),
                               rgba=verts[-1][3] & 0xFFFFFFFF, z=verts[-1][2], verts=len(verts)))


def walk(gs, data):
    off = 0
    while off + 16 <= len(data):
        lo, hi = struct.unpack_from('<QQ', data, off)
        off += 16
        nloop = lo & 0x7FFF
        pre = (lo >> 46) & 1
        flg = (lo >> 58) & 3
        nreg = (lo >> 60) & 0xF or 16
        if pre:
            gs.write(0x00, (lo >> 47) & 0x7FF)
        regs = [(hi >> (4 * i)) & 0xF for i in range(nreg)]
        if flg == 0:
            for _ in range(nloop):
                for d in regs:
                    a, b = struct.unpack_from('<QQ', data, off)
                    off += 16
                    if d == 0x0:
                        gs.write(0x00, a & 0x7FF)
                    elif d == 0x1:
                        gs.write(0x01, (a & 0xFF) | ((a >> 32) & 0xFF) << 8 | (b & 0xFF) << 16 |
                                 ((b >> 32) & 0xFF) << 24)
                    elif d == 0x2:
                        gs.write(0x02, a)
                    elif d == 0x3:
                        gs.write(0x03, (a & 0x3FFF) | ((a >> 32) & 0x3FFF) << 16)
                    elif d in (0x4, 0x5):
                        adc = (b >> 47) & 1
                        xyz = (a & 0xFFFF) | ((a >> 32) & 0xFFFF) << 16
                        if d == 0x4:
                            xyz |= ((b >> 4) & 0xFFFFFF) << 32
                            gs.write(0x0C if adc else 0x04, xyz)
                        else:
                            xyz |= (b & 0xFFFFFFFF) << 32
                            gs.write(0x0D if adc else 0x05, xyz)
                    elif d == 0xE:
                        gs.write(b & 0xFF, a)
                    elif d == 0xF:
                        pass
                    else:
                        gs.write(d, a)
        elif flg == 1:
            words = nloop * nreg
            for i in range(words):
                v, = struct.unpack_from('<Q', data, off + 8 * i)
                d = regs[i % nreg]
                if d != 0xF:
                    gs.write(d, v)
            off += ((words + 1) // 2) * 16
        else:
            off += nloop * 16


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('dump')
    ap.add_argument('--field', type=int, help='only this field (0 = the first after the dump starts)')
    ap.add_argument('--big', action='store_true', help='only primitives covering at least half a 512x224 field')
    ap.add_argument('--pixel', help='X,Y: only primitives whose box holds this window pixel')
    a = ap.parse_args(argv)
    gs = Gs()
    for p in read_dump(a.dump):
        if p[0] == 'vsync':
            gs.field += 1
        else:
            walk(gs, p[2])
    px = tuple(map(float, a.pixel.split(','))) if a.pixel else None
    for q in gs.prims:
        if a.field is not None and q['field'] != a.field:
            continue
        x0, y0, x1, y1 = q['box']
        if a.big and (x1 - x0) * (y1 - y0) < 0.5 * 512 * 224:
            continue
        if px and not (x0 <= px[0] <= x1 and y0 <= px[1] <= y1):
            continue
        f = q['frame']
        print(f"f{q['field']} #{q['n']} PRIM {q['prim']:#05x} ctx{q['ctx']} box ({x0:.1f},{y0:.1f})-({x1:.1f},{y1:.1f}) "
              f"FBP {f & 0x1FF:#x} FBMSK {f >> 32:#010x} TEST {q['test']:#x} ALPHA {q['alpha']:#x} "
              f"TEX0 {q['tex0']:#x} ZBUF {q['zbuf']:#x} FBA {q['fba']} RGBA {q['rgba']:#010x} Z {q['z']:#x}")
    print(f'{len(gs.prims)} primitives in {gs.field + 1} field(s)', file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
