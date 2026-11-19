#!/usr/bin/env python3
"""Pixel-for-pixel: the port's exact GS field against the original's at the same row.

The port's side is a dump of tools/guided_chain.py --gs-dump (EM_GS_DUMP_AT):
build/gspics/<run>/port/f<chain frame>.gsfield (512 x 224 x 4, R G B A as
the GS model's memory holds them) and f<chain frame>.json (its FRAME_1).
The original's side is tools/capture_gs_original.py's GS memory at the
main-loop tops after the same row (decomp build/gspics/orig/<take>/r<R>/
s<k>.vram, the software renderer's local memory). The original's field of
row R is whole at loop top R + 2 in the buffer FRAME_1 names when the frame
indices agree (capture_gs_original.py says why); the tool reads that buffer
(PSMCT32, FBW from FRAME_1) and, beside it, the other stops and buffers, to
report the alignment that is exact rather than assume it.

Reported per point: pixels whose R, G, B are equal (of 114,688), the same
with A, the largest channel difference, the bounding box of the differing
pixels, and a diff picture build/gspics/<run>/diff_<take>_r<R>.png (left
the original, middle the port, right the differing pixels in white over a
dimmed original). Disc-derived pictures: never committed.

Usage (port root; native arm64 macOS; numpy):
  python3 tools/compare_gs_frame.py gspics_g20 g77_area21_phase1:5000 [...]
  python3 tools/compare_gs_frame.py gspics_g20 --all
"""
import argparse
import json
import struct
import sys
import zlib
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
ORIG = DECOMP / 'build/gspics/orig'
W, H = 512, 224

BLOCK32 = np.array([[0, 1, 4, 5, 16, 17, 20, 21], [2, 3, 6, 7, 18, 19, 22, 23],
                    [8, 9, 12, 13, 24, 25, 28, 29], [10, 11, 14, 15, 26, 27, 30, 31]])
COLUMN32 = np.array([[0, 1, 4, 5, 8, 9, 12, 13], [2, 3, 6, 7, 10, 11, 14, 15],
                     [16, 17, 20, 21, 24, 25, 28, 29], [18, 19, 22, 23, 26, 27, 30, 31],
                     [32, 33, 36, 37, 40, 41, 44, 45], [34, 35, 38, 39, 42, 43, 46, 47],
                     [48, 49, 52, 53, 56, 57, 60, 61], [50, 51, 54, 55, 58, 59, 62, 63]])


def ct32_words(fbp, fbw, w=W, h=H):
    """Word addresses of a PSMCT32 buffer's pixels (GS local memory layout)."""
    y, x = np.mgrid[0:h, 0:w]
    page = (y // 32) * fbw + x // 64
    block = BLOCK32[(y % 32) // 8, (x % 64) // 8]
    return (fbp + page) * 2048 + block * 64 + COLUMN32[y % 8, x % 8]


def vram_field(vram, frame_1):
    fbp, fbw, psm = frame_1 & 0x1FF, (frame_1 >> 16) & 0x3F, (frame_1 >> 24) & 0x3F
    if psm != 0:
        raise ValueError(f'FRAME_1 {frame_1:#x}: PSM {psm} is not PSMCT32')
    words = np.frombuffer(vram, dtype='<u4')
    return words[ct32_words(fbp, fbw)].view(np.uint8).reshape(H, W, 4)


def write_png(path, rgb):
    h, w, _ = rgb.shape
    raw = b''.join(b'\x00' + rgb[y].tobytes() for y in range(h))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xFFFFFFFF)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                     chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def metrics(port, orig):
    d = np.abs(port.astype(np.int16) - orig.astype(np.int16))
    rgb_eq = (d[..., :3].max(axis=2) == 0)
    rgba_eq = (d.max(axis=2) == 0)
    ys, xs = np.nonzero(~rgb_eq)
    box = [int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())] if len(xs) else None
    return dict(rgb_equal=int(rgb_eq.sum()), rgba_equal=int(rgba_eq.sum()), pixels=W * H,
                max_diff=int(d[..., :3].max()), box=box), rgb_eq


def diff_picture(path, orig, port, eq):
    dim = (orig[..., :3] // 3).copy()
    dim[~eq] = 255
    write_png(path, np.concatenate([orig[..., :3], port[..., :3], dim], axis=1))


def compare_point(run, take, row, write=True):
    pdir = ROOT / 'build/gspics' / run / 'port'
    points = json.loads((pdir / 'points.json').read_text())
    key = f'{take}:{row}'
    if key not in points:
        raise KeyError(f'{key} was not dumped in {run} (points.json: {", ".join(points)})')
    f = points[key]
    name = f if isinstance(f, str) else f'f{f}'
    pj = json.loads((pdir / f'{name}.json').read_text())
    port = np.frombuffer((pdir / f'{name}.gsfield').read_bytes(), np.uint8).reshape(H, W, 4)
    frame_1 = int(pj['frame_1'], 16)
    odir = ORIG / take / f'r{row}'
    res = dict(point=key, chain_frame=name, port_frame_1=hex(frame_1), port_counter=pj['counter'])
    tries = []
    best = None
    for k in (2, 1, 3):
        vp = odir / f's{k}.vram'
        if not vp.exists():
            continue
        vram = vp.read_bytes()
        for fbp in (frame_1 & 0x1FF, 0x38 if (frame_1 & 0x1FF) == 0 else 0):
            fr = (frame_1 & ~0x1FF) | fbp
            orig = vram_field(vram, fr)
            m, eq = metrics(port, orig)
            tries.append(dict(stop=k, fbp=hex(fbp), rgb_equal=m['rgb_equal']))
            if k == 2 and fbp == frame_1 & 0x1FF:
                res.update(m)
                res['orig'] = json.loads((odir / 's2.json').read_text()).get('on_take')
                if write:
                    diff_picture(ROOT / 'build/gspics' / run / f'diff_{take}_r{row}.png', orig, port, eq)
            if best is None or m['rgb_equal'] > best['rgb_equal']:
                best = dict(stop=k, fbp=hex(fbp), rgb_equal=m['rgb_equal'])
    res['alignments'] = tries
    res['best'] = best
    return res


def line(r):
    if 'rgb_equal' not in r:
        return f'{r["point"]}: no original at loop top R + 2'
    pct = 100.0 * r['rgb_equal'] / r['pixels']
    s = (f'{r["point"]} (chain frame {r["chain_frame"]}, FRAME_1 {r["port_frame_1"]}): RGB exact '
         f'{r["rgb_equal"]}/{r["pixels"]} ({pct:.2f} %), RGBA {r["rgba_equal"]}, max diff {r["max_diff"]}, '
         f'box {r["box"]}')
    b = r['best']
    if b and (b['stop'] != 2 or b['rgb_equal'] > r['rgb_equal']):
        s += f'; best alignment stop {b["stop"]} FBP {b["fbp"]}: {b["rgb_equal"]}'
    return s


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('run')
    ap.add_argument('points', nargs='*', metavar='TAKE:ROW')
    ap.add_argument('--all', action='store_true', help='every dumped point that has an original')
    a = ap.parse_args(argv)
    pdir = ROOT / 'build/gspics' / a.run / 'port'
    points = a.points
    if a.all:
        dumped = json.loads((pdir / 'points.json').read_text())
        points = [k for k, v in dumped.items()
                  if (ORIG / k.split(':')[0] / f'r{k.split(":")[1]}' / 's2.vram').exists() and
                  (pdir / f'{v if isinstance(v, str) else f"f{v}"}.json').exists()]
    results = []
    for p in points:
        take, _, row = p.rpartition(':')
        r = compare_point(a.run, take, int(row))
        results.append(r)
        print(line(r), flush=True)
    (ROOT / 'build/gspics' / a.run / 'compare.json').write_text(json.dumps(results, indent=1) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
