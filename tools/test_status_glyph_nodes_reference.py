#!/usr/bin/env python3
"""The status screen's equipment letters with several nodes against the
original instructions.

0020E250 spawns one letter per selected part through 0020E1E0(glyph):
001AFF10, +0x44 = 001C6120(D_0028A56C, glyph), +0x0C = 001C6150 (the byte
at model + 8). The walk 001B0000 runs 001CB590(record, 0x2F0, +0x09) and
the letter's 0020E460: state 0 takes +0x0C bone slots (001AF7C0), copies
each node's parent and bind (001C62C0) and places the record; state 1 runs
001C6380 (build_trs_matrix, then 001C9610 over the nodes) and the 001CB580
draw. Glyph 0x35 (code 3) has three nodes whose binds move its parts;
0x33, 0x34 and 0x3D have two.

The oracle executes the original 0020E1E0, 001CB590 and 0020E460 (every
instruction and callee, the draw packet 001D2040 and the draw method
001CB580 recorded and skipped) over the status-hub capture
(../Extermination/build/startup-reference/status-hub: its RAM holds
D_0028A56C, the bone-slot stack and D_00810610). The port spawns the same
letters through em_status_models (tests/status_glyph_nodes_bridge.c, the
exported assets/status_models) and walks twice. Compared, bit for bit:
the record's state, slot count, scale, colour, position, rotation and draw
method, and every node's world matrix +0x90; and that the original drew it
once with 001D2040(0, 1) before and (0, 0) after.

Also: a single-node letter (0x30) as the control, and the exported models'
node counts and vertex slots against the library (each vertex on a node
the model has; every node of 0x35 carries geometry).
"""
import ctypes as C
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

from test_coll_move_reference import FloatEE
from test_module_loader_reference import read_elf_bytes

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'Extermination'
REF = DECOMP / 'build/startup-reference/status-hub'
OUT = ROOT / 'build/status_glyph_nodes'
POOL, STRIDE, RECORDS = 0x28B020, 0x2F0, 24
D_00275B44 = 0x275B44
D_00810610 = 0x810610
SOURCES = ('src/game/em_status_models.c', 'src/game/em_status_scene_original.c',
           'src/game/em_owner_services_original.c', 'src/game/em_actor_light_001D89D0.c',
           'src/game/em_frame_render_heads.c', 'src/game/em_player_pose.c', 'src/game/em_pose_bank.c',
           'src/game/em_pose_transition.c', 'src/em_model.c', 'src/game/em_random.c',
           'src/game/em_area01_math_core.c', 'src/game/em_area01_math_actor.c',
           'src/game/em_pose_host_workers.c', 'src/game/em_player_stage_workers.c',
           'src/game/em_player_floor.c', 'src/game/em_player_reaction.c', 'src/game/em_player_fall.c',
           'src/game/em_stream_lanes_original.c')
# (glyph, D_00810CA4..A7 that makes 0020E250 spawn it, its node count)
CASES = ((0x35, (5, 1, 2, 3), 3), (0x33, (5, 1, 2, 3), 2), (0x34, (5, 1, 2, 3), 2),
         (0x3D, (2, 0, 0, 0), 2), (0x30, (5, 1, 2, 3), 1))


def word(b, a):
    return struct.unpack_from('<I', b, a)[0]


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / 'bridge.dylib'
    subprocess.run(['cc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-shared',
                    '-fPIC', '-Isrc', 'tests/status_glyph_nodes_bridge.c', *SOURCES, '-lm', '-o', str(target)],
                   cwd=ROOT, check=True)
    lib = C.CDLL(str(target))
    lib.sgn_run.argtypes = [C.c_char_p, C.c_char_p, C.c_char_p, C.c_uint8, C.c_void_p, C.c_void_p, C.c_int]
    lib.sgn_run.restype = C.c_int
    return lib


def original(elf, ram, spr, glyph):
    """0020E1E0(glyph), then two 001B0000 passes over the new record. The
    record bytes, the node +0x90 matrices and the skipped calls."""
    o = FloatEE(elf, ram, spr)
    calls = []

    def skip(name):
        def hook(ee):
            calls.append((name, ee.r[4] & 0xFFFFFFFF, ee.r[5] & 0xFFFFFFFF))
            ee.r[2] = 0
        return hook
    o.hooks[0x1D2040] = skip('001D2040')
    o.hooks[0x1CB580] = skip('001CB580')
    free = [i for i in range(RECORDS) if ram[POOL + STRIDE * i] == 0]
    o.call(0x20E1E0, (glyph,))  # void
    record = POOL + STRIDE * free[0]
    assert o.mem[record] != 0 and o.mem[record + 0xD] == glyph, 'the first free record'
    for _ in range(2):  # 001B0000's two steps for this record
        o.call(0x1CB590, (record, STRIDE, o.mem[record + 9]))
        o.call(0x20E460, (word(o.mem, D_00275B44),))
    rec = bytes(o.mem[record:record + STRIDE])
    worlds = [bytes(o.mem[word(rec, 0x110 + 4 * i) + 0x90:word(rec, 0x110 + 4 * i) + 0xD0])
              for i in range(rec[0xC])]
    return rec, worlds, calls


# The record fields 0020E1E0 / 0020E460 / 001AFF10 write that the port
# models (by original offset, size).
FIELDS = ((0x00, 1), (0x04, 1), (0x09, 1), (0x0C, 1), (0x0D, 1), (0x10, 4), (0x4C, 4),
          (0x60, 16), (0x80, 16), (0x94, 2), (0xB0, 12), (0xC0, 12))


def export_checks():
    """The exported letters' node counts and vertex slots against the
    library (read only)."""
    sys.path.insert(0, str(ROOT / 'tools'))
    sys.path.insert(0, str(DECOMP / 'tools'))
    import export_props as ep
    import export_status_models as S
    library = (DECOMP / S.LIBRARY_FILE).read_bytes()
    emsk = (ROOT / 'assets/status_models/models.emsk').read_bytes()
    counts, at = {}, 12
    for _ in range(word(emsk, 8)):
        token, n = struct.unpack_from('<II', emsk, at)
        counts[token] = n
        at += 8 + n * 68
    directory = ep.read_directory(library)
    out = {}
    for glyph in S.GLYPHS:
        offset = directory[glyph]
        n = library[offset + 8]
        assert counts[glyph] == n, ('models.emsk node count', hex(glyph), counts[glyph], n)
        slots = {s for _q, corners, _p in ep.model_tris_slots(library, offset) for *_x, s in corners}
        assert max(slots) < n, ('vertex slot outside the nodes', hex(glyph), sorted(slots))
        if n > 1:
            out[f'{glyph:#x}'] = {'nodes': n, 'slots': sorted(slots)}
    assert out['0x35']['slots'] == [0, 1, 2]
    return out


def main():
    start = time.monotonic()
    if not (REF / 'eeMemory.bin').exists() or not (ROOT / 'assets/status_models/letter_35.emdl').exists():
        print('status glyph nodes: SKIP (status-hub capture or tools/export_status_models.py output missing)')
        return 77
    lib = build()
    elf = read_elf_bytes()
    ram = (REF / 'eeMemory.bin').read_bytes()
    spr = (REF / 'scratchpad.bin').read_bytes()
    view = ram[D_00810610:D_00810610 + 64]
    report = {'cases': []}
    for glyph, ca, nodes in CASES:  # five cases, about a second each: no quick subset
        rec, worlds, calls = original(elf, ram, spr, glyph)
        assert rec[0xC] == nodes, ('original node count', hex(glyph), rec[0xC])
        assert rec[4] == 1, ('original state after two steps', hex(glyph), rec[4])
        assert [c[0] for c in calls] == ['001D2040', '001CB580', '001D2040'] and \
            calls[0][1:] == (0, 1) and calls[2][1:] == (0, 0), ('draw calls', calls)
        native = C.create_string_buffer(STRIDE)
        out = (C.c_float * (16 * 64))()
        n = lib.sgn_run(str(ROOT / 'assets/status_models').encode(), bytes(ca), view, glyph, native, out, 64)
        assert n == nodes, ('port node count', hex(glyph), n)
        port = native.raw[:STRIDE]
        for at, size in FIELDS:
            assert port[at:at + size] == rec[at:at + size], \
                ('record field', hex(glyph), hex(at), port[at:at + size].hex(), rec[at:at + size].hex())
        exact = 0
        for i in range(nodes):
            got = bytes(out)[64 * i:64 * i + 64]
            assert got == worlds[i], ('node +0x90', hex(glyph), i, struct.unpack('<16f', got),
                                      struct.unpack('<16f', worlds[i]))
            exact += 1
        report['cases'].append({'glyph': f'{glyph:#x}', 'nodes': nodes, 'worlds_bit_exact': exact,
                                'node1_translation': [round(v, 4) for v in struct.unpack('<3f', worlds[1][48:60])]
                                if nodes > 1 else None})
    report['export'] = export_checks()
    report['seconds'] = round(time.monotonic() - start, 2)
    print('status glyph nodes: PASS ' + json.dumps(report))
    return 0


if __name__ == '__main__':
    sys.exit(main())
