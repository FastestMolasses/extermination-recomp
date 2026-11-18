#!/usr/bin/env python3
"""Opt-in retained AREA00 library delivery and model-binding instruction oracle.

Native arm64 macOS; private disc/capture inputs only. The unchanged production
resource provider is tested under ASan/UBSan, then the existing shared model
adapter is compared with the original ELF, including complete RAM/scratchpad.
"""
from __future__ import annotations

import contextlib
import json
import os
from pathlib import Path
import subprocess

import export_area00_library as X
import test_area01_model_live_reference as M

OUT = X.A.ROOT / 'build/levels/area00-library-reference'


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    library_path = X.B.DECOMP / 'extract/chunk27/f01_id37.bin'
    library = library_path.read_bytes()
    installed = (X.A.ROOT / 'assets/scene_snow/roger/resources.emrs').read_bytes()
    # Reconstruct the predecessor by retaining each original first model only.
    header, regions = X.parse(installed)
    base = X.B.u32(header, 32 + 4 * X.B.GLOBAL_TABLE_INDEX)
    import struct
    before, pieces = bytearray(header), 0
    for address, writable, retained, data in regions:
        starts = [base + (X.B.u32(library, 4 + 4 * first) & ~3) for first, _ in X.SPANS]
        inside = [a for a in starts if address <= a < address + len(data)]
        if not inside:
            before += struct.pack('<4I', address, len(data), writable, retained) + data
            pieces += 1
            continue
        # The region holding a span's first model (AREA02's export may have
        # extended it downwards; a later library export may hold both spans
        # in one region): keep up to the first span's first model and give
        # each further span its first model as a region of its own.
        for k, at in enumerate(inside):
            lo = address if k == 0 else at
            piece = data[lo - address:at - address + X.B.block_model_size(library, at - base)]
            before += struct.pack('<4I', lo, len(piece), writable, retained) + piece
            pieces += 1
    struct.pack_into('<I', before, 16, pieces)          # the header's region count
    after = X.extend(bytes(before), library)
    assert X.extend(after, library) == after, 'export is not idempotent'
    for malformed in (before[:-1], before[:31], before + b'x'):
        try:
            X.parse(malformed)
        except ValueError:
            pass
        else:
            raise AssertionError('Malformed retained resource pack accepted')
    altered = bytearray(before)
    cursor = len(header)
    target = base + (X.B.u32(library, 4 + 4 * X.SPANS[0][0]) & ~3)
    for address, _, _, raw in X.parse(before)[1]:
        if address <= target < address + len(raw):      # the span's first model
            altered[cursor + 16 + target - address] ^= 1
        cursor += 16 + len(raw)
    try:
        X.extend(bytes(altered), library)
    except ValueError:
        pass
    else:
        raise AssertionError('Changed predecessor model accepted')
    report = X.verify(after, library)
    for label, data in (('before', before), ('after', after)):
        folder = OUT / label / 'assets/scene_snow/roger'
        folder.mkdir(parents=True, exist_ok=True)
        (folder / 'resources.emrs').write_bytes(data)
    binary = OUT / 'readview'
    subprocess.run(['cc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-ffp-contract=off', '-Wl,-dead_strip',
                    '-Isrc', 'tests/area00_library_readview_test.c', '-o', str(binary)],
                   cwd=X.A.ROOT, check=True)
    for label, present in (('before', 0), ('after', 1)):
        regions_in = len(X.parse(before if label == 'before' else after)[1])
        subprocess.run([str(binary), str(library_path), str(present), str(regions_in)], cwd=OUT / label,
                       env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'), check=True)
    M.OUT = OUT / 'model'
    lib = M.build()
    elf = (X.B.DECOMP / 'config/SCUS_971.12').read_bytes()
    captures = X.A.all_captures()
    full = os.environ.get('EM_TEST_FULL') == '1'
    if not full:
        captures = [next(pair for pair in captures if pair[1] == sub) for sub in X.A.SUBS]
    cases = 0
    for capture, _ in captures:
        ram, scratch = capture.ram, capture.spad
        owners = [0x7A5640 + slot * 0x2F0 for slot in range(256)
                  if ram[0x7A5640 + slot * 0x2F0] and
                  X.B.u32(ram, 0x7A5640 + slot * 0x2F0 + 0x10) == 0x15AFA0]
        assert owners, 'no original pickup owner'
        for model in sorted(X.required_models()):
            with (OUT / 'oracle-private.log').open('a') as log, contextlib.redirect_stdout(log), contextlib.redirect_stderr(log):
                M.check(lib, elf, ram, scratch, owners[0], 0x1B1020, model, 0xFFFFFFFF)
            cases += 1
    report.update(original_cases=cases, full=full, result='passed',
                  preserved_regions=len(regions), added_bytes=len(after) - len(before))
    (OUT / 'receipt.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS AREA00 library: {cases} model binding cases; production read provider under ASan/UBSan')


if __name__ == '__main__':
    main()
