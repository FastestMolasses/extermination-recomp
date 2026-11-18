#!/usr/bin/env python3
"""Check canonical AREA00 SFX and both module-pack subarea deliveries.

The existing AREA00 assets checker independently re-derives scoped sound
registries from original RAM. This check proves that their complete entries
and decoded samples reach the canonical registry without changing older
scopes, and that the native loader accepts it. No audio device is opened.
Module sectors are compared with the disc reads selected by the original
loader's descriptor rules, already oracle-checked by the module-loader test.
"""
import argparse
import copy
import ctypes as CT
import hashlib
import json
from pathlib import Path
import struct
import subprocess

import export_module_loader as M
import export_sfx_registry as X
from test_area11_sfx_reference import parse_emsr

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/level3/delivery'


def key(entry):
    return tuple(entry['scope']), entry['id']


def catalog(path):
    raw = path.read_bytes()
    report = json.loads(path.with_suffix('.json').read_text())
    assert hashlib.sha256(raw).hexdigest() == report['registry_sha256']
    return raw, report, parse_emsr(raw)


def normalized(entry, samples, field):
    entry = copy.deepcopy(entry)
    for op in entry.get(field, []):
        if 'sample' in op and (field != 'ops' or op['kind'] == X.OP_KEY_ON):
            op['sample'] = samples[op['sample']]
    return entry


def sound_check(path):
    raw, report, (entries, samples, ladder) = catalog(path)
    indexed = {key(e): e for e in entries}
    assert len(indexed) == len(entries)
    json_index = {key(e): e for e in report['entries']}
    saved = X.AREA00_SCOPES
    X.AREA00_SCOPES = ()
    try:
        X.export(OUT / 'baseline')
    finally:
        X.AREA00_SCOPES = saved
    _oldraw, old, (old_entries, old_samples, old_ladder) = catalog(OUT / 'baseline/sfx_registry.emsr')
    # The exporter allocates samples in scope order and AREA00 is not the
    # last scope (AREA02, AREA04, the mid areas, AREA19 and the attachments
    # follow it), so the later scopes' sample indices shift by AREA00's
    # samples. Entries compare by the samples they name, not by index, and
    # the older samples must keep their order.
    def subsequence(small, big):
        it = iter(big)
        return all(any(x == y for y in it) for x in small)
    identity = lambda c: [(s['container'], s['offset'], s['adpcm_sha256']) for s in c['samples']]
    assert all(normalized(indexed[key(e)], samples, 'ops') == normalized(e, old_samples, 'ops')
               for e in old_entries)
    assert all(normalized(json_index[key(e)], identity(report), 'events') ==
               normalized(e, identity(old), 'events') for e in old['entries'])
    assert subsequence(old_samples, samples) and ladder == old_ladder
    without_index = lambda c: [{k: v for k, v in s.items() if k != 'index'} for s in c['samples']]
    assert subsequence(without_index(old), without_index(report))
    counts = {}
    for sub in (0, 1):
        _raw, area, (local, local_samples, local_ladder) = catalog(
            ROOT / f'assets/area00/sub{sub}/sfx/sfx_registry.emsr')
        assert ladder == local_ladder
        assert {e['id'] for e in local} == set(X.AREA00_IDS)
        for e in local:
            assert normalized(e, local_samples, 'ops') == normalized(indexed[key(e)], samples, 'ops'), key(e)
        for e in area['entries']:
            assert normalized(e, identity(area), 'events') == normalized(json_index[key(e)], identity(report), 'events'), key(e)
        counts[sub] = {state: sum(e['state'] == state for e in local) for state in (1, 2, 3)}
    libpath, shim = OUT / 'sfx-bank.dylib', OUT / 'sfx-bank-size.c'
    # The registry's size comes from the header (it grew with the pan table).
    shim.write_text('#include <stddef.h>\n#include "game/em_sfx_bank.h"\n'
                    'size_t delivery_registry_size(void) { return sizeof(EmSfxRegistry); }\n')
    subprocess.run(['cc', '-std=c11', '-O1', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-shared', '-fPIC', '-Isrc', 'src/game/em_sfx_bank.c', str(shim), '-o', str(libpath)],
                   cwd=ROOT, check=True)
    lib = CT.CDLL(str(libpath))
    lib.delivery_registry_size.restype = CT.c_size_t
    lib.em_sfx_registry_load.argtypes = [CT.c_void_p, CT.c_char_p]
    lib.em_sfx_registry_find.argtypes = [CT.c_void_p, CT.c_uint, CT.c_int, CT.c_int]
    lib.em_sfx_registry_find.restype = CT.c_void_p
    lib.em_sfx_registry_free.argtypes = [CT.c_void_p]
    storage = CT.create_string_buffer(lib.delivery_registry_size())
    try:
        assert lib.em_sfx_registry_load(storage, str(path).encode()) == 1
        for e in entries:
            assert lib.em_sfx_registry_find(storage, e['id'], *e['scope']), key(e)
        malformed = bytearray(raw)
        struct.pack_into('<I', malformed, 12, 4097)
        bad = OUT / 'too-many-entries.emsr'
        bad.write_bytes(malformed)
        before = lib.em_sfx_registry_find(storage, X.AREA00_IDS[0], 0, 1)
        assert lib.em_sfx_registry_load(storage, str(bad).encode()) == 0
        assert lib.em_sfx_registry_find(storage, X.AREA00_IDS[0], 0, 1) == before
    finally:
        lib.em_sfx_registry_free(storage)
    return dict(entries=len(entries), samples=len(samples), preserved_entries=len(old_entries),
                preserved_samples=len(old_samples), area00_states=counts,
                over_limit_rejected=4097)


def module_check(path):
    raw = path.read_bytes()
    report = json.loads(path.with_suffix('.json').read_text())
    assert hashlib.sha256(raw).hexdigest() == report['sha256']
    magic, version, count, zero = struct.unpack_from('<4sIII', raw)
    assert (magic, version, zero) == (b'EMML', 2, 0)
    ranges = [struct.unpack_from('<4I', raw, M.HEADER + 16 * i) for i in range(count)]
    disc = M.Disc(M.DECOMP / 'Extermination-rebuilt.iso')
    files, _names, _arena, _bases = M.boot_tables(disc, M.DECOMP / 'config/SCUS_971.12')
    result = {}
    for sub in (0, 1):
        assert f'0x0:{sub}' in report['areas']
        _header, reads = M.area_reads(disc, disc.index, disc.data, 0, files, sub)
        total = 0
        for name, lsn, sectors in reads:
            if not sectors:
                continue
            matching = [r for r in ranges if r[0] <= lsn and lsn + sectors <= r[0] + r[1]]
            assert matching, (sub, name, 'missing sector span')
            r = min(matching, key=lambda row: row[1])
            assert r[3] == 0
            at, n = r[2] + (lsn - r[0]) * M.SECTOR, sectors * M.SECTOR
            assert raw[at:at + n] == disc.sectors(lsn, sectors), (sub, name)
            total += n
        result[sub] = dict(reads=len(reads), compared_bytes=total)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--registry', type=Path, default=ROOT / 'assets/sfx/sfx_registry.emsr')
    parser.add_argument('--pack', type=Path, default=ROOT / 'assets/module_loader/modules.emml')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    report = dict(sfx=sound_check(args.registry), modules=module_check(args.pack))
    (OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('AREA00 delivery: PASS ' + json.dumps(report))


if __name__ == '__main__':
    main()
