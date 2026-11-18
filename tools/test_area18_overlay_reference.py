#!/usr/bin/env python3
"""AREA18's overlay holds no owner to translate: proof against the user's
files and Claude's guided run (docs/AREA18_PORT.md).

OVERLAY/AREA18.BIN (header id 0x0F) has 0xC0 bytes of text: the 64-byte
entry pad and one function, the init at runtime 0x823580 that 001E7780's
key 0x1200 calls. em_area01_state translates that init, and
test_second_half_state_reference (make test-second-half-state-reference)
runs the ORIGINAL init over AREA18's own overlay file and compares the
globals, data and BSS byte for byte. This test checks that nothing else
needs a translation:

1. the header equals test_second_half_state_reference's AREA18 row (id,
   text, data address and size, BSS address and size, init);
2. the text's only nonzero words are the init's, which ends in the text's
   one return; every other text word is zero (the pad and the tail);
3. no word of the overlay's data or of its header points into the text;
4. in the two guided-run states with AREA18 loaded (decomp
   build/guided_play/g41_area14_area18 and g43_area14_area18, end.p2s:
   eeMemory extracted once into build/area18_overlay/states/ with the
   decomp's venv), the resident overlay's header and text equal the file,
   the area byte D_00810700 is 0x12, and no live pool node's behaviour
   (+0x10) lies in the overlay text: every owner of AREA18 that runs is a
   boot function.

Reports hold addresses and counts only (no instruction text).
"""
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import test_second_half_state_reference as S  # noqa: E402

DECOMP = ROOT.parent / 'Extermination'
FILE = DECOMP / 'extract/OVERLAY/AREA18.BIN'
GUIDED = DECOMP / 'build/guided_play'
STATES = ROOT / 'build/area18_overlay/states'
TAKES = ('g41_area14_area18', 'g43_area14_area18')
BASE = 0x823500
POOL, NODE, NODES = 0x7A5640, 0x2F0, 400


def words(data, start, end):
    return [(BASE + o, struct.unpack_from('<I', data, o)[0]) for o in range(start - BASE, end - BASE, 4)]


def extract():
    missing = [t for t in TAKES if not (STATES / t / 'eeMemory.bin').exists()]
    if missing:
        code = ('import sys\nfrom pathlib import Path\nfrom tools.parse_pcsx2_state import extract_zstd_entry\n'
                'for n in sys.argv[3:]:\n'
                '    o = Path(sys.argv[1]) / n; o.mkdir(parents=True, exist_ok=True)\n'
                '    p = Path(sys.argv[2]) / n / "end.p2s"\n'
                '    (o / "eeMemory.bin").write_bytes(extract_zstd_entry(p, "eeMemory.bin"))\n')
        subprocess.run([str(DECOMP / '.venv/bin/python'), '-c', code, str(STATES), str(GUIDED), *missing],
                       cwd=DECOMP, check=True)
    return {t: (STATES / t / 'eeMemory.bin').read_bytes() for t in TAKES}


def main():
    data = FILE.read_bytes()
    name, oid, text, daddr, dsize, bss, bss_size, init, _ = S.AREAS[0x12]
    magic, hid, link, tsize, dlen, blen, bss_at, _ = struct.unpack_from('<8I', data, 0)
    assert name == 'AREA18' and (hid, link, tsize, dlen, blen, bss_at) == (oid, BASE, text, dsize, bss_size, bss)
    text_lo, text_hi = BASE + 0x40, BASE + 0x40 + tsize
    assert text_hi == daddr and len(data) == 0x40 + tsize + dsize, 'layout'
    print(f'PASS header: id {hid:#x}, text {tsize:#x}, data {daddr:#x} + {dsize:#x}, BSS {bss:#x} + {bss_size:#x}')

    nonzero = [a for a, v in words(data, text_lo, text_hi) if v]
    returns = [a for a, v in words(data, text_lo, text_hi) if v == 0x03E00008]
    assert nonzero and nonzero[0] == init and nonzero == list(range(init, nonzero[-1] + 4, 4)), 'one function'
    assert returns == [nonzero[-2]], 'the init ends in the text\'s one return (its delay slot follows)'
    print(f'PASS text: one function, the init {init:#x} .. {nonzero[-1] + 4:#x} ({len(nonzero)} words); '
          f'{tsize // 4 - len(nonzero)} zero words')

    into = [a for a, v in words(data, BASE, BASE + 0x40) + words(data, daddr, daddr + dsize)
            if text_lo <= v < text_hi]
    assert not into, [hex(a) for a in into]
    print(f'PASS data: none of {(0x40 + dsize) // 4} header and data words points into the text')

    for take, ram in extract().items():
        assert ram[BASE + 4] == oid and ram[0x810700] == 0x12, (take, 'AREA18 resident')
        assert ram[BASE:BASE + 0x40 + tsize] == data[:0x40 + tsize], (take, 'resident header and text')
        live = [struct.unpack_from('<I', ram, POOL + i * NODE + 0x10)[0] for i in range(NODES)
                if ram[POOL + i * NODE]]
        inside = [hex(b) for b in live if text_lo <= b < text_hi]
        assert not inside, (take, inside)
        boot = sorted({b for b in live if 0x100000 <= b < BASE})
        print(f'PASS {take}: AREA18 resident, {len(live)} live nodes, none in the overlay text '
              f'({len(boot)} boot behaviours)')
    print('PASS AREA18 overlay: nothing to translate beyond the init (test-second-half-state-reference)')


if __name__ == '__main__':
    main()
