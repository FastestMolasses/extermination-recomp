#!/usr/bin/env python3
"""A save the port wrote against the original's, byte for byte.

Reads a raw 8 MB PS2 card image (528-byte pages, 512 of data; a fork session's
Mcd001.ps2, as tools/record_original_play.py keeps it) and compares its files
with a port host card folder (em_memcard's slot directory). docs/SAVE_GAME.md
section 2 says what a save holds; for a progress block the bytes that differ
are listed by address.

Usage (native arm64 macOS): python3 tools/compare_card_image.py <card.ps2> <slot dir>

Both inputs hold disc-derived bytes (the icon, the game's state): they stay
under ignored build/ folders.
"""
import struct
import sys
from pathlib import Path

PAGE, RAW, BLOCK = 512, 528, 0x640
BASE = 0x00810700


class Card:
    def __init__(self, path):
        raw = Path(path).read_bytes()
        self.data = b''.join(raw[i:i + PAGE] for i in range(0, len(raw), RAW))
        head = self.data
        if not head.startswith(b'Sony PS2 Memory Card Format'):
            raise SystemExit(f'{path}: not a formatted PS2 card image')
        self.cluster_size = PAGE * struct.unpack_from('<H', head, 42)[0]
        self.alloc, = struct.unpack_from('<I', head, 52)
        self.root, = struct.unpack_from('<I', head, 60)
        self.ifc, = struct.unpack_from('<I', head, 0x50)

    def cluster(self, n):
        return self.data[n * self.cluster_size:(n + 1) * self.cluster_size]

    def fat(self, n):
        per = self.cluster_size // 4
        fat_cluster = struct.unpack_from('<I', self.cluster(self.ifc), 4 * (n // per))[0]
        return struct.unpack_from('<I', self.cluster(fat_cluster), 4 * (n % per))[0]

    def chain(self, first):
        n = first
        while True:
            yield self.cluster(self.alloc + n)
            entry = self.fat(n)
            if entry == 0xFFFFFFFF or not entry & 0x80000000:
                return
            n = entry & 0x7FFFFFFF

    def read(self, first, size):
        return b''.join(self.chain(first))[:size]

    def entries(self, first, count):
        table = b''.join(self.chain(first))
        for i in range(count):
            e = table[i * 512:(i + 1) * 512]
            mode, length = struct.unpack_from('<HxxI', e, 0)
            cluster, = struct.unpack_from('<I', e, 0x10)
            name = e[0x40:0x60].split(b'\0')[0].decode('ascii', 'replace')
            if mode & 0x8000 and name not in ('.', '..'):
                yield name, mode, length, cluster

    def files(self):
        out = {}
        root_size = struct.unpack_from('<I', self.read(self.root, 512), 4)[0]
        for name, mode, length, cluster in self.entries(self.root, root_size):
            if mode & 0x20:
                for sub, m, n, c in self.entries(cluster, length):
                    out[f'{name}/{sub}'] = None if m & 0x20 else self.read(c, n)
            else:
                out[name] = self.read(cluster, length)
        return out


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    want = Card(sys.argv[1]).files()
    root = Path(sys.argv[2])
    got = {str(p.relative_to(root)): p.read_bytes() for p in root.rglob('*') if p.is_file()}
    print(f'{len(want)} files on the card, {len(got)} in the folder')
    same = True
    for name in sorted(set(want) | set(got)):
        a, b = want.get(name), got.get(name)
        if a == b:
            print(f'  {name}: {len(a)} bytes, equal')
            continue
        same = False
        if a is None or b is None:
            print(f'  {name}: only on the {"card" if b is None else "host"}')
            continue
        diff = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
        print(f'  {name}: {len(a)} / {len(b)} bytes, {len(diff)} differ')
        at = BASE if len(a) == BLOCK == len(b) else 0
        for i in diff[:64]:
            print(f'    {at + i:#010x}: original {a[i]:02x} port {b[i]:02x}')
    return 0 if same else 1


if __name__ == '__main__':
    sys.exit(main())
