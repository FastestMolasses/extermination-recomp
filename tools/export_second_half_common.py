#!/usr/bin/env python3
"""Shared inputs of the second half's asset exports (docs/SECOND_HALF_AREAS.md):
AREA14, AREA16, AREA17, AREA18, AREA20 and AREA21, every sub their spawn
descriptors name.

  AREA14 (area 0x0E, OVERLAY/AREA14.BIN = MWo3 id 0x0B, INDEX sector 18)  sub 0
  AREA16 (area 0x10, AREA16.BIN = id 0x0D, sector 20)                      subs 0, 1
  AREA17 (area 0x11, AREA17.BIN = id 0x0E, sector 21)                      sub 0
  AREA18 (area 0x12, AREA18.BIN = id 0x0F, sector 22)                      sub 0
  AREA20 (area 0x14, AREA20.BIN = id 0x11, sector 24)                      sub 0
  AREA21 (area 0x15, AREA21.BIN = id 0x12, sector 25)                      sub 0

Like export_area08_common: the AREA13 lane's exporters model an area through
a Target and per-target tables, imported and run unchanged; `install` points
them at one (target, sub). The load layouts, read from each INDEX.IDX sector
(`layout`, checked by export_second_half_assets.py --kind static):

  * AREA14, AREA17, AREA18, AREA20, AREA21 are FLAT, like AREA13 and AREA22:
    no nested block, the sound container and one GS upload section in the top
    descriptor, a resident offset (Target nested False,
    export_area22_common.build_load_map_flat). AREA17's and AREA21's lists
    relocate id 0x45, which no earlier flat area's did (AREA13's and AREA22's
    do not): `dynamic` True, so they take export_area15_level.export_dynamic
    over the flat builder, a pairing no earlier area tried;
  * AREA16 is AREA08's layout: a top block with no sections and no resident
    region, two nested blocks (one per sub), each with the sound container,
    one upload section and a resident offset; both nested lists relocate id
    0x45, so both subs take export_area15_level.export_dynamic (AREA08's
    sub-0 rule: `dynamic` below is that reading of each nested list, the
    rule that gives AREA08 subs 0 and 3 and AREA07 subs 0 and 3 True and
    their subs 2 False).

What needs the route captures (CAP2's recordings; none exists on
2026-11-02): the level (the load map is built from a capture's cursors
D_0028A73C / D_0028A740, which differ run to run), the cells, the world
models, the messages' area bank, the textures and the sound bindings. SUBS
holds no capture folder yet: those kinds refuse until an area lane fills in
its (target, sub) row, the capture folders in route order and the capture
before the load. What does not need them (the static kind): the overlay pins,
the roster and spawn walks over the pinned ELF and overlay, the door rows,
the script chains and the overlay data window.

001A2370 / 0019C6F0 (the level exporter's CREATURES / FLAG_OWNERS rules) are
called from these overlay functions (decomp tools/area_overview.py, direct
calls in the original text): AREA14 0x823FC0; AREA16 0x8273F0 (001A2370),
0x825800 (0019C6F0); AREA17 0x825560 (001A2370), 0x823BA0 and 0x824700
(0019C6F0); AREA20 0x823D30, 0x824440; AREA21 0x8297C0, 0x82A1D0
(001A2370), 0x826840, 0x827CD0 (0019C6F0); AREA18 neither. No rule is
modelled for any of them yet: `install` refuses the level kind for an area
whose callers are not in CREATURES / FLAG_RULES (fail-stop), and an area lane
models each one by reading its function, as AREA07's 0x823F80 was.

Nothing here is original data: every byte is read from the user's own files.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_area13_common as A13  # noqa: E402  (first: it keeps AREA01's nested load-map builder)
import export_area13_level as LV  # noqa: E402
import export_area13_tables as TB  # noqa: E402
import export_area13_sfx as SX  # noqa: E402

C = A13.C
ROOT, DECOMP = A13.ROOT, A13.DECOMP
SCRATCH = ROOT / 'build/second_half/assets'
K01 = DECOMP / 'build/fork_refs/levels-recordings-20261102-1/s87/route_k01'
# Claude's guided run as capture folders (tools/capture_from_take.py: each folder is a kept
# take's end.p2s entries; the take's end is the next take's start)
G17 = DECOMP / 'build/fork_refs/guided-20261117/s87/route_g17'
G16 = DECOMP / 'build/fork_refs/guided-20261117/s87/route_g16'

# SHA-256 of the user's extract/OVERLAY files (hashes, not disc data)
AREA14_SHA256 = '43f7e9bbb9fb18bf398f197689231ec1b91ff5ece7e6eb05abbbba98d86690f7'
AREA16_SHA256 = '2761ebfc195bae2b3dbb531fccdb525644936366862d38fdcf8c3a7baccd4a6f'
AREA17_SHA256 = '2e068fc86d8edc21dada721fb14061e75a174913282b8fe04651174a0ee98d27'
AREA18_SHA256 = 'feee323a6044d4ede3dcd2a77a32ce473c899f3fb7e5b4244ed0bd9708e5368f'
AREA20_SHA256 = 'd13317fb5f21eae507ed12832e3b3897735ee8d8adfd25a1e3ab70870bfaa352'
AREA21_SHA256 = 'c0f8677f17b2be81fa8237d78ee6b51cd842dac3be7d9e6a0a16bd8b6c4965e8'
# (name, area byte, overlay file size, header id, overlay SHA-256, INDEX
# chunk, nested layout)
_ROWS = (
    ('area14', 0x0E, 0x4F80, 0x0B, AREA14_SHA256, 'chunk18', False),
    ('area16', 0x10, 0x9000, 0x0D, AREA16_SHA256, 'chunk20', True),
    ('area17', 0x11, 0x5980, 0x0E, AREA17_SHA256, 'chunk21', False),
    ('area18', 0x12, 0x0880, 0x0F, AREA18_SHA256, 'chunk22', False),
    ('area20', 0x14, 0x4A00, 0x11, AREA20_SHA256, 'chunk24', False),
    ('area21', 0x15, 0xA780, 0x12, AREA21_SHA256, 'chunk25', False),
)
TARGETS = {}
for _n, _area, _size, _id, _sha, _chunk, _nested in _ROWS:
    TARGETS[_n] = A13.Target(_n, _area, 0, f'AREA{_area:02d}.BIN', _size, _id, _sha, _chunk, _nested,
                             ROOT / f'assets/{_n}/sub0', f'AREA{_area:02d}')
A13.TARGETS.update(TARGETS)

# (target, sub): the capture folders in route order and the capture before
# the load (None until an area lane binds the area from CAP2's recordings);
# `dynamic`: the load's relocation list holds id 0x45 (module docstring)
SUBS = {
    # AREA14 sub 0 (lane P14): the k01 run's beats that end in AREA14, in
    # route order, after the load from AREA00 sub 2 (k01_01_slab's end)
    ('area14', 0): dict(paths=tuple(K01 / b for b in ('k01_02_item26', 'k01_03_item19', 'k01_04_face',
                                                     'k01_05_plateau')),
                        previous=K01 / 'k01_01_slab', dynamic=False),
    # AREA16 (lane PORT-LIVE-A16): the guided takes' end states. Sub 0: g60 (the arrival from
    # AREA06 sub 1's slider [1], at control) and g61 (the water barrier passed), one load; the
    # capture before it is g59's end (AREA06 sub 1). Sub 1: g62 (the arrival by sub 0's slider [1])
    # .. g66 (the bed room), one load (the room moves between them keep it); before it, g61's end.
    ('area16', 0): dict(paths=tuple(G16 / b for b in ('g60_area06_area16', 'g61_area16_water')),
                        previous=G16 / 'g59_area01_area06', dynamic=True),
    ('area16', 1): dict(paths=tuple(G16 / b for b in ('g62_area16_sub1', 'g63_area16_recharge', 'g64_area16_booth',
                                                     'g65_area16_vaccine', 'g66_area16_bed')),
                        previous=G16 / 'g61_area16_water', dynamic=True),
    # AREA17 sub 0 (lane PORT-LIVE2): the guided takes' end states in AREA17 after its load, in
    # route order (g46: the ramp after the skipped intro; g47: after Roger); the capture before
    # the load is g45's end (the area bytes already 0x11 entry 0, AREA14's overlay still resident)
    ('area17', 0): dict(paths=tuple(G17 / b for b in ('g46_area17_ramp', 'g47_area17_roger')),
                        previous=G17 / 'g45_area14_area17', dynamic=True),
    ('area18', 0): dict(paths=None, previous=None, dynamic=False),
    ('area20', 0): dict(paths=None, previous=None, dynamic=False),
    ('area21', 0): dict(paths=None, previous=None, dynamic=True),
}
# every chain entry the original starts: the data addresses the script-starting
# overlay functions pass to 001BA1A0 (decomp tools/area_overview.py over the
# original text, checked against the committed C's func_001BA1A0 calls by the
# static kind); AREA18 has no chain
SCRIPT_ENTRIES = {
    'area14': (0x826800, 0x826C10, 0x826FC0, 0x827340, 0x827490, 0x827590, 0x828120),
    'area16': (0x829340, 0x829970, 0x82A1C0, 0x82A3C0, 0x82A6C0, 0x82A840, 0x82AA80, 0x82BC40, 0x82BE40,
               0x82C040),
    'area17': (0x826A20, 0x8270A0, 0x8275E0, 0x827A40, 0x827E00, 0x828050, 0x8284F0),
    'area18': (),
    'area20': (0x826830, 0x8270B0),
    'area21': (0x82B240, 0x82B5E0, 0x82B8A0, 0x82BEC0, 0x82C5C0, 0x82CB00, 0x82CBD0),
}
# the scripts window (lowest entry .. end of the last record any chain
# reaches: export_area13_tables.scripts_window's rule, which gives AREA03's,
# AREA13's and AREA19's committed windows); the static kind recomputes it
SCRIPT_WINDOWS = {
    'area14': (0x826800, 0x8283A0), 'area16': (0x829340, 0x82C240), 'area17': (0x826A20, 0x828730),
    'area18': None, 'area20': (0x826830, 0x8273B0), 'area21': (0x82B240, 0x82D110),
}
# placements whose behaviour reaches 001BC150 (the door commit): the boot
# doors, and AREA21's overlay doors 0x826BB0 / 0x826E60 (direct callers of
# 001BC150 / 001BC240 in the original text)
BOOT_DOORS = (0x1BC350, 0x1BB860, 0x1BD9F0, 0x1BD560)
DOORS = {n: BOOT_DOORS for n in TARGETS}
DOORS['area21'] = BOOT_DOORS + (0x826BB0, 0x826E60)
DOOR_ROW = {'area14': 0x275600, 'area16': 0x24E0D0, 'area17': 0x275604, 'area18': 0x275608,
            'area20': 0x27560C, 'area21': 0x24E118}                 # D_0024E140[area]
# the overlay callers of 001A2370 / 0019C6F0 (module docstring); a level
# export needs a modelled rule for each
CALLERS_001A2370 = {'area14': (0x823FC0,), 'area16': (0x8273F0,), 'area17': (0x825560,), 'area18': (),
                    'area20': (0x823D30, 0x824440), 'area21': (0x8297C0, 0x82A1D0)}
CALLERS_0019C6F0 = {'area14': (), 'area16': (0x825800,), 'area17': (0x823BA0, 0x824700), 'area18': (),
                    'area20': (), 'area21': (0x826840, 0x827CD0)}
CREATURES = {}            # target -> the modelled 001A2370 behaviour (LV.CREATURES), filled by an area lane
# AREA14's 0x823FC0 (func_overlay_AREA14_00823F80.c, byte-identical C) passes
# REC_B + 0x90, REC_B = *(D_00275B40 + 0xC) with D_00275B40 = node + 0x110 while
# its behaviour runs: *(node + 0x11C) + 0x90, export_area13_level's rule.
CREATURES['area14'] = 0x823FC0
# AREA17's 0x825560 (func_overlay_AREA17_00825520.c, NEARMISS: the call is the C's
# func_001A2370(self, self + 0xD0) in its state-1 arm, which the original's words make too
# (test_area17_overlay_reference runs them)) passes the node's own +0xD0 matrix, the pickup's
# and the drum's rule (MATRIX_RULES), not export_area13_level's *(node + 0x11C) + 0x90.
CREATURES['area17'] = 0x825560
# AREA16's 0x8273F0 (func_overlay_AREA16_008273B0.c, byte-identical) passes D_00275B40[2] + 0x90 in
# both its state-0 calls, with D_00275B40 a char ** = node + 0x110 while its behaviour runs: the
# word at node + 0x118 (AREA14's REC_A, not export_area13_level's *(node + 0x11C)) plus 0x90.
CREATURES['area16'] = 0x8273F0
# the creature's matrix where it is not export_area13_level's rule: (ram, node) -> address
MATRIX_RULES = {'area17': lambda ram, node: node + 0xD0,
                'area16': lambda ram, node: C.u32(ram, node + 0x118) + 0x90}
_LV_OWNER_MATRIX = {}
FLAG_RULES = {}           # target -> flag_calls(ram) for its 0019C6F0 callers, filled by an area lane


def area17_flag_calls(ram):
    """AREA17's 0019C6F0 calls in this load, from the byte-identical C:
      * [30] 0x823BA0 (func_overlay_AREA17_00823B60.c) calls (0xE, 1), (0xF, 1) in state 2 when
        D_00810805 is 2 (then stores 3) and in the skip state 0x19 always (then stores 4); its
        state 0 goes to state 3 with no call when counter 0x2D is set. Nothing else stores 3 or 4
        in D_00810805. So with D_00810805 0 in the capture before the load (the first arrival)
        and at least 3 in the capture, this load made both calls; 0 .. 2 in the capture: none yet;
      * [34] 0x824700 (func_overlay_AREA17_008246C0.c) calls (0xF, 0) when script 0x8275E0 ends,
        storing D_00810806 = 0xFF: so 0xFF in the capture and not in the one before the load
        means this load made it, after [30]'s.
    A previous capture with D_00810805 other than 0, or D_00810806 already 0xFF, is not modelled
    (ValueError): whether this load's [30] / [34] called again is not in the RAM."""
    t = A13.current()
    if ram[0x810700] != t.area or ram[0x810701] != t.sub:
        return ()
    before = LV.previous_word(t.name, 0x810804)
    if before is None:
        raise ValueError('area17: the capture before the load is missing (D_00810805 / D_00810806 there)')
    b805, b806 = (before >> 8) & 0xFF, (before >> 16) & 0xFF
    if b805 != 0 or b806 == 0xFF:
        raise ValueError(f'area17: D_00810805 {b805:#x} / D_00810806 {b806:#x} before the load: not modelled')
    calls = ()
    if ram[0x810805] >= 3:
        calls += ((0xE, 1), (0xF, 1))
    if ram[0x810806] == 0xFF:
        if not calls:
            raise ValueError('area17: D_00810806 0xFF with no [30] calls: not modelled')
        calls += ((0xF, 0),)
    return calls


FLAG_RULES['area17'] = area17_flag_calls


def area16_flag_calls(ram):
    """AREA16's 0019C6F0 calls in this load, from the byte-identical C of sub 0's [10] 0x825800
    (func_overlay_AREA16_008257C0.c), the only caller:
      * state 0 (the node's first frame) calls (0x28, 1) and goes to state 1 when D_0081078B is not
        0xFF; with 0xFF it goes to state 2 with no call;
      * state 1 calls (0x28, 0) on its way to state 0xC8 (when D_0081080B & 7 >= 3);
      * 0xC8 ends in state 2, storing D_0081078B = 0xFF (its only writer in the code).
    So with D_0081078B not 0xFF in the capture before the load: a live node in state 1 made
    (0x28, 1); one in state 0xC8 or 2 made (0x28, 1) then (0x28, 0). With 0xFF before the load:
    state 2, no call. Any other state, node count or byte: ValueError (not modelled)."""
    t = A13.current()
    if ram[0x810700] != t.area or ram[0x810701] != t.sub or t.sub != 0:
        return ()
    before = LV.previous_word(t.name, 0x810788)
    if before is None:
        raise ValueError('area16: the capture before the load is missing (D_0081078B there)')
    b78b = (before >> 24) & 0xFF
    nodes = [a for _s, a in LV.E02.pool(ram) if C.u32(ram, a + 0x10) == 0x825800]
    state = ram[nodes[0] + 4] if len(nodes) == 1 else None
    if b78b == 0xFF and state == 2:
        return ()
    if b78b != 0xFF and state == 1:
        return ((0x28, 1),)
    if b78b != 0xFF and state in (0xC8, 2):
        return ((0x28, 1), (0x28, 0))
    raise ValueError(f'area16: [10] 0x825800 nodes {[(hex(a), ram[a + 4]) for a in nodes]} with D_0081078B '
                     f'{b78b:#x} before the load: not modelled')


FLAG_RULES['area16'] = area16_flag_calls

# Orphaned hulls no capture can prove (a known gap, recorded in level.json's cells report as
# 'orphan-unproven'): AREA17's drums [20], [21] and [23] (placements of 0x156620 with uids 19,
# 20, 22) are freed before the first capture after the load (g46: no shot was fired, so the
# arrival freed them) and no capture holds them live, so export_area13_level's rule (equal to a
# derivation in another capture) has nothing to compare with. The export writes the disc bytes
# either way (the runtime's own owners re-transform hulls); what is relaxed is only the check
# that the captures' moved bytes are explained. Accepted only when every differing word is one
# the ORIGINAL 001A2370 writes for that hull (export_area02_level.written_words, the EE
# interpreter) and the bytes are equal in every capture.
# Model owners whose +0x44 is not 001C6120(table, +0x0D) (export_area13_tables.EXPLICIT_MODELS,
# {behaviour: (state, model id)}): AREA17's [31] 0x8243E0 (func_overlay_AREA17_008243A0.c,
# byte-identical) sets func_001CA6E0(self, func_001C6120(D_0028A59C, 0xD)) on its way to state
# 2 and leaves +0x0D as it was.
# AREA16's [11] 0x8261A0 (func_overlay_AREA16_00826160.c, byte-identical) in state 2: from state 1 it
# sets func_001CA6E0(self, func_001C6120(D_0028A59C, 0x10)) and leaves +0x0D (0xF) as it was; from
# state 0 (D_0081080B bit 1 already set) it stores +0x0D = 0x10 before its model loads, so model
# 0x10 either way.
# AREA16's 0x828200 (func_overlay_AREA16_008281C0.c, byte-identical; 0x8273F0's and 0x827B70's
# child) in state 1 draws its parts with func_001CA5E0(self, func_001C6120(D_0028A59C, id), 1) for
# each id of the table its +0x0D selects (6: 4 ids at 0x82C2A0; 0x25: 4 at 0x82C2F0; 0x14: 16 at
# 0x82C340), so +0x44 is the last id's model at the end of every frame.
A16_PART_IDS = {6: (0x82C2A0, 4), 0x25: (0x82C2F0, 4), 0x14: (0x82C340, 16)}


def area16_part_model(ram, node):
    if ram[node + 0x0D] not in A16_PART_IDS:
        raise ValueError(f'area16: 0x828200 node {node:#x} with +0x0D {ram[node + 0x0D]:#x} (the C has no table)')
    table, count = A16_PART_IDS[ram[node + 0x0D]]
    return C.u32(ram, table + 4 * (count - 1))


EXPLICIT_MODELS = {'area17': {0x8243E0: (2, 0xD)},
                   'area16': {0x8261A0: (2, 0x10), 0x828200: (1, area16_part_model)}}
ORPHANS_UNPROVEN = {('area17', 0): {19: 0x156620, 20: 0x156620, 22: 0x156620}}
UNPROVEN_REPORT = {}


def _verify_directory_with_gap(original, key):
    def verify(elf, disc, caps):
        rows, problems, calls = original(elf, disc, caps)
        import export_area02_level as E02
        gap = ORPHANS_UNPROVEN[key]
        _count, hulls, _size = LV.L.cell_directory(disc, 0)
        keep, seen = [], {}
        for line in problems:
            hit = next((u for u in gap if f': hull {u} differs, no live node owns it' in line), None)
            if hit is None:
                keep.append(line)
                continue
            cap = next(c for c in caps if line.startswith(c.name + ':'))
            table = C.u32(cap.spad, LV.L.SPAD_CELLS)
            st, en, _f = hulls[hit]
            data = cap.ram[table + st:table + en]
            moved = {k for k in range(0, en - st, 4) if data[k:k + 4] != disc[st + k:st + k + 4]}
            written = E02.written_words(elf, disc, cap, hit, hulls)
            if not moved <= written:
                keep.append(f'{cap.name}: hull {hit} orphan words outside 001A2370\'s writes')
            elif seen.setdefault(hit, data) != data:
                keep.append(f'{cap.name}: hull {hit} orphan bytes differ between the captures')
            else:
                UNPROVEN_REPORT.setdefault(key, {})[hit] = dict(
                    owner=hex(gap[hit]), words=len(moved), captures=sorted({*UNPROVEN_REPORT.get(key, {}).get(
                        hit, {}).get('captures', ()), cap.name}))
                row = next(r for r in rows if r['capture'] == cap.name)
                row['proofs'][hit] = 'orphan-unproven (export_second_half_common.ORPHANS_UNPROVEN)'
        return rows, keep, calls
    return verify


# Group 3 bank 0 is the fitted attachment's bank (export_sfx_registry ATTACHMENT_SCOPES: 001FEF70's
# modules 0x32..0x35 from extract/chunk50..53/f00_id86.bin, the scope em_sfx_set_attachment
# selects). A capture with an attachment fitted (the guided run's AREA17 takes: its header, at
# D_0028A740 + 0x30, is chunk50's container byte for byte) binds it there; the area's own binding
# leaves the slot to that scope, as the areas whose captures show no attachment do (refused).
ATTACHMENTS = tuple(f'extract/chunk{50 + i}/f00_id86.bin' for i in range(4))
_ATTACHED = {}


def _attachment_aware(bindings_from_captures, refusal_reasons):
    def bind(caps, containers):
        extra = {}
        for name in ATTACHMENTS:
            data = (DECOMP / name).read_bytes()
            parsed = SX.X.A.parse_container(data)
            if parsed is not None:
                extra[name] = (data, parsed)
        bound, refused, report = bindings_from_captures(caps, {**containers, **extra})
        for key, value in list(bound.items()):
            if value[0][0] in extra:
                if key != (3, 0):
                    raise SystemExit(f'slot {key}: an attachment bank outside group 3 bank 0')
                _ATTACHED[key] = value[0][0]
                refused[key] = value[1]
                del bound[key]
        return bound, refused, report

    def reasons(caps, refused):
        plain = {k: v for k, v in refused.items() if k not in _ATTACHED}
        out = refusal_reasons(caps, plain)
        for key in refused:
            if key in _ATTACHED:
                out[key] = (f'group 3 bank 0 = handle {refused[key]}, the fitted attachment\'s bank '
                            f'({_ATTACHED[key]}; the attachment scope holds it, export_sfx_registry ATTACHMENT_SCOPES)')
        return out
    return bind, reasons


def subs_of(target):
    return sorted(s for t, s in SUBS if t == target)


def layout(target):
    """(top descriptor, [nested descriptors]) of the target's INDEX.IDX sector
    (area + 4) read from the user's disc image; refused when the layout is
    not the one the Target states."""
    t = TARGETS[target]
    d = C.iso_descriptor(t.area + 4)
    if d is None:
        raise SystemExit('the user\'s disc image is required (export_area01_common.ISO_PATH)')
    if C.u32(d, 0) != t.area + 4:
        raise SystemExit(f'{target}: sector word {C.u32(d, 0)}, not {t.area + 4}')
    count = C.u32(d, 0x18)
    nested = [d[C.NESTED_BASE + C.NESTED_SIZE * s:C.NESTED_BASE + C.NESTED_SIZE * (s + 1)] for s in range(count)]
    if bool(count) != t.nested:
        raise SystemExit(f'{target}: {count} nested blocks, the Target says nested={t.nested}')
    if count and count != len(subs_of(target)):
        raise SystemExit(f'{target}: {count} nested blocks, SUBS names {subs_of(target)}')
    return d[:C.DESCRIPTOR_SIZE], nested


def dynamic_rule(target, sub):
    """True when the load's relocation list for (target, sub) holds id 0x45."""
    top, nested = layout(target)
    ids = [i for i, _o in A13._files(nested[sub] if nested else top)]
    return 0x45 in ids


def pins(target, sub):
    """The roster and spawn pins of (target, sub), by the original's walks
    over the pinned ELF and overlay."""
    t = TARGETS[target]
    read = C.static_reader(C.read_elf(), A13.read_overlay(t))
    groups, paddr, _placements = TB.R.walk_roster(read, t.area, sub)
    _table, entries, count = TB.spawn_rows(read)
    return paddr, tuple((a, len(r)) for a, r in groups), (entries, count)


def no_flag_calls(ram):
    return ()


def install(target, sub, tree=None, need_captures=True):
    """Point the AREA13 lane's modules at (target, sub); `tree` is the scratch
    tree the parts go under (default SCRATCH/tree). With need_captures, a
    (target, sub) without capture folders, or with a 001A2370 / 0019C6F0
    caller whose rule is not modelled, is refused. Returns the module C."""
    v = SUBS[(target, sub)]
    t = TARGETS[target]
    if need_captures:
        if not v['paths'] or v['previous'] is None:
            raise SystemExit(f'{target} sub {sub}: no route capture yet (SUBS); only --kind static runs')
        if CALLERS_001A2370[target] and target not in CREATURES:
            raise SystemExit(f'{target}: 001A2370 callers {[hex(a) for a in CALLERS_001A2370[target]]} '
                             'without a modelled rule (CREATURES)')
        if CALLERS_0019C6F0[target] and target not in FLAG_RULES:
            raise SystemExit(f'{target}: 0019C6F0 callers {[hex(a) for a in CALLERS_0019C6F0[target]]} '
                             'without a modelled rule (FLAG_RULES)')
    t.sub = sub
    A13.SCRATCH = SCRATCH / 'scratch'
    A13.OUT = (tree or SCRATCH / 'tree') / target / f'part{sub}'
    t.out = A13.OUT / f'sub{sub}'
    A13.capture_paths = lambda: list(v['paths'] or ())
    A13.configure(t)
    LV.PREVIOUS[target] = v['previous']
    LV.CREATURES[target] = None if target in MATRIX_RULES else CREATURES.get(target)
    LV.FLAG_OWNERS[target] = None
    LV.flag_calls = FLAG_RULES.get(target, no_flag_calls)
    # the colour windows D_00251BE0 / D_00251C20 hold the last class-6 draw's
    # tint in a capture, no load state (export_area08_common.retained_colours,
    # the original routines run over the capture)
    import export_area08_common as A08
    LV.E22.check_ctx_block = A08.check_ctx_block
    TB.SCRIPT_ENTRIES[target] = SCRIPT_ENTRIES[target]
    TB.WRITERS[target] = ()
    TB.DOOR_ROW[target] = DOOR_ROW[target]
    TB.DOOR_BEHAVIOURS = DOORS[target]
    TB.EXPLICIT_MODELS = dict(EXPLICIT_MODELS.get(target, {}))
    TB.PLACEMENTS[target], TB.GROUPS[target], TB.SPAWN_SUB0[target] = pins(target, sub)
    LV.install(target)
    if not getattr(SX.S.bindings_from_captures, 'attachments', False):
        SX.S.bindings_from_captures, SX.E04.refusal_reasons = _attachment_aware(SX.S.bindings_from_captures,
                                                                                SX.E04.refusal_reasons)
        SX.S.bindings_from_captures.attachments = True
    if (target, sub) in ORPHANS_UNPROVEN and not getattr(LV.verify_directory, 'gap', None):
        LV.verify_directory = _verify_directory_with_gap(LV.verify_directory, (target, sub))
        LV.verify_directory.gap = (target, sub)
    # the creature's matrix by its own rule (MATRIX_RULES), around export_area13_level's
    # owner_matrix: wrapped in LV itself, since LV.install (which the level kind calls again)
    # points export_area01_level at LV's owner_matrix by name
    rule = _LV_OWNER_MATRIX.setdefault('rule', LV.owner_matrix)
    if target in MATRIX_RULES:
        owner, at = CREATURES[target], MATRIX_RULES[target]
        LV.owner_matrix = lambda ram, node: at(ram, node) if C.u32(ram, node + 0x10) == owner else rule(ram, node)
    else:
        LV.owner_matrix = rule
    LV.L.owner_matrix = LV.owner_matrix
    return C
