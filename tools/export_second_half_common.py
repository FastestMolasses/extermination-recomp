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
    ('area14', 0): dict(paths=None, previous=None, dynamic=False),
    ('area16', 0): dict(paths=None, previous=None, dynamic=True),
    ('area16', 1): dict(paths=None, previous=None, dynamic=True),
    ('area17', 0): dict(paths=None, previous=None, dynamic=True),
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
FLAG_RULES = {}           # target -> flag_calls(ram) for its 0019C6F0 callers, filled by an area lane


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
    LV.CREATURES[target] = CREATURES.get(target)
    LV.FLAG_OWNERS[target] = None
    LV.flag_calls = FLAG_RULES.get(target, no_flag_calls)
    TB.SCRIPT_ENTRIES[target] = SCRIPT_ENTRIES[target]
    TB.WRITERS[target] = ()
    TB.DOOR_ROW[target] = DOOR_ROW[target]
    TB.DOOR_BEHAVIOURS = DOORS[target]
    TB.EXPLICIT_MODELS = {}
    TB.PLACEMENTS[target], TB.GROUPS[target], TB.SPAWN_SUB0[target] = pins(target, sub)
    LV.install(target)
    return C
