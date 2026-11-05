# AREA00 sub 2 and AREA14 in the shared composition (lane P14)

Lane P14, 2026-11-02 .. 04, branch `p14`. The constructed start k01
(EIGHTEENTH_LEVEL_ROUTE.md sections 7c / 7d) enters AREA00 sub 2 and leaves it
for AREA14 entry 0 through [28]. Both run in the shared AREA01 composition, as
AREA08 and AREA19 do (AREA08_PORT.md, AREA19_PORT.md). This is a start-card
binding: nothing here is row-exact, and a beat that passes ends on its
recording's block and position, nothing more.

## Beats

`python3 tools/mid_route_beats.py --single <beat>` (recorded pad, recorded
path; the rand() tables of k01_01 and k01_03 are not usable, CAP3's note).

| Beat | Status alone from its card |
|---|---|
| k01_01 | runs to its end with no fail-stop, at the recording's position (26.11, -60, -1630.21); vitals differ: health 25 against 55 (hits of 15 at frame ~200 and 20 by frame 300 where the recording took one of 5 at frame 198), infection 100 and D_00810706/07 1 / 1 against 95 and 0 / 0. Its rand() table is not usable, so the hits are not yet attributed |
| k01_02 | the ledge climb onto [23], item 0x26 used on [28] from the EVENT page, script 0x82D070 with its op14 nodes (0x823EB0, 0x824130), the op09 0x826CB0, the count to 824 (001F6AD0, the group 0x827FB0), the timeline's grey track, the area change to AREA14 and its load, the four 0x823FC0 group owners with their probe 0x825530 (lane P14b); stops at the fail-stop of [0] 0x8238E0 (untranslated) on the first frame there |
| k01_03 .. k01_05 | not run: they start in AREA14, whose owner [0] 0x8238E0 faults |

Chain `k01` (`python3 tools/mid_route_beats.py k01`) stops where k01_02 does.

## AREA00 sub 2

AREA00's subs 0 and 1 are export_area00_*'s (chunk04.n0 / n1, two loads that
differ only in their static bank). Sub 2's nested block chunk04.n2 is another
level: sixteen files, none equal to n0's. It is bound as a later area's sub.

- **Assets** (`tools/export_area00s2_assets.py`): the AREA13 lane's
  per-(target, sub) exporters, run unchanged over a Target for AREA00 sub 2
  (captures `k01_00_load`, `k01_01_slab`; the load's previous state
  `a02c_01_door22`). Output `assets/area00/sub2/` (level, collision, cells,
  roster, world models, messages, sound). AREA00's spawn table, overlay data
  and the root door file stay export_area00_tables' (the spawn table and the
  overlay data are byte-equal to this run's). Two adaptations, both in the
  driver: AREA00's C casts the chain argument of func_001BA1A0, so the chain
  pattern allows the cast; the room check takes export_area08_common's
  retained colour windows. `--profile 00s2` of export_level_world_textures /
  export_level_shadow_receivers / export_player_texture_variants writes
  `assets/levels/00/sub2/`; the module packs and the sfx registry's mid scopes
  hold (0, 2).
- **Runtime**: `mid_area(0, 2)` (assets/area00/sub2), the textures, receivers
  and sound by (area, sub). `em_area01_runtime`: sub 2's boot owners take the
  later areas' dispatch (`area00_sub2`: D_00810701 == 2), its overlay owners
  stay dispatch_overlay00's. The AREA00 scripts window reaches sub 2's chain
  0x82D070 (0x8284E0 .. 0x82D370, the exporter's rule over all 15 chains).
  op14 (001BAC00) reads AREA00's overlay data window for its lists.
- **Overlay owners** (`em_area00_overlay.c`, test_area00_overlay_reference:
  the original over the k01 images and designed states; every reachable
  word, the hook contract): [30] 0x825D70 with 0x825E80 (the quad local it
  copies is compared as 001B1EA0's argument) and 0x825FC0, [31] 0x8260F0,
  [28] 0x826790 (item 0x26, script 0x82D070, 001B0C60(0xE, 0, 0): AREA14),
  [29] 0x826BE0, [32] 0x826CC0, and the effect node 0x8241B0 the creature
  spawns.
- **The creature** 0014D260 (group 0x827EF0, spawned by [30]) with 0014D3B0,
  0014D470, 0014D5F0, 0014D7C0, 0014DC30, 0014E050, 0014E2C0, 0014E370,
  0014E420, 0014E4F0, 0014E640, 0014E670, 0014E7A0, 0014E890, 0014E9C0
  (`em_area00s2_creature*`, test_area00s2_creature_reference: 666 cases, every
  reachable word but one dead word named in the test), bound through the
  later levels' fallback dispatch (tools/gen_later_dispatch.py,
  gen_mid_route_bridge.py).
- **Script 0x82D070's nodes and callback**: group 0x82CDD0 (op14 places
  more than 16 nodes: the placement binder holds 64) runs 0x823EB0 (the lamps:
  frame 0xF0 with a matrix copy, a packet and a colour copy it writes itself;
  the matrix's last row is compared as 001026A0's argument) and 0x824130 (the
  emitters); the op09 callback 0x826CB0 sets [28]'s +0x2EC = 2. Same oracle.
- **Boot pieces on the path**: 001C47E0 (item count less n; the decomp C) in
  `em_area01_live`, its only evidence the end block of k01_02 (D_00810C8A
  1 -> 0); 001F6AD0 runs as the port's 001F6B30 (equal word for word in the
  ELF, asserted by test_area00_overlay_reference); the timeline's +0x7C
  track 0021BA80 / 0021BA70 through `em_rcl` over the packet chain's context
  (em_packet_chain_0021BA70 added, test_packet_chain_reference `fogword`).

## AREA14

- **Assets** (`export_second_half_common.SUBS[('area14', 0)]`: k01_02 ..
  k01_05, previous k01_01_slab; `python3 tools/export_second_half_assets.py
  --only area14:0 --kind all`): level (753 objects, background armed),
  collision, cells (the directory runs past its own load-map row:
  export_area13_level.directory_span reads the contiguous rows after it, and
  the cells checks compare every byte with the four captures), roster, world
  models, messages (148 area records), sound. 0x823FC0's 001A2370 rule is
  export_area13_level's creature rule (REC_B = *(node + 0x11C)). `--profile
  14` writes `assets/levels/14/sub0/` (textures: (0, 0), (1, 1), (1, 2) from
  the captures, the other pairs by export_player_texture_variants; (1, 1) and
  (1, 2) are cross-checked by k01_02 .. k01_05). The sfx registry holds
  (0x0E, 0).
- **Runtime**: a mid area (`assets/area14/sub0`), a later area for the boot
  dispatch, bound by em_area01_runtime_bind_area; its placements' overlay
  behaviours (0x823580, 0x823760, 0x8238E0, 0x823B30, 0x823E30, 0x823FC0,
  0x825AA0, 0x825FD0) bind, 0x825AA0 runs BOOT2's twin (dispatch_second_half)
  and every other one faults at its address when it runs; op14 reads
  AREA14's overlay data window.
- **The group owner 0x823FC0 and its two callees** (lane P14b,
  `dispatch_second_half`). The three AREA14 functions align word for word
  with AREA19's 0x827DD0 / 0x829370 / 0x829840 (an instruction-stream
  alignment with the immediates and branch offsets masked) except: the
  owner's three calls to its own overlay callees and its missed-probe
  effect (0x8000002C when the earlier hit record's +0x1A was 5, else
  0x80000003, where AREA19 tests 6 and 8); the probe's tint record
  (0x828100 for 0x82F670) and the order of its 001CD520 argument set-up
  (registers only: the loads, stores and calls are the same); the
  spawner's colour (0x828110 for 0x82F680). So the existing translations
  run them with those differences as parameters:
  - 0x823FC0: `em_area19_route_00823FC0` (the turret with `o->area14`; two
    hooks w_00825530 / w_00825A10 added). `test_area19_route_reference`
    runs every designed 0x827DD0 case again on AREA14's overlay header and
    text loaded over the same images, five AREA14 miss kinds (5, 6, 4, 0,
    255) and the 16 captured 0x823FC0 nodes of k01_02 .. k01_05: 1285
    cases, 1170/1170 reachable words of 0x823FC0. A mutation of the kind-5
    effect id fails it.
  - 0x825530: `em_level10_port_00825530` (0x829370's probe with the tint
    record as a parameter). `test_level10_port_reference` runs every
    0x829370 case again (labels `a14 ...`) on images holding AREA14's whole
    overlay file, and 0x825530 on the 16 captured 0x823FC0 nodes: 64
    cases, 265/265 reachable words. On AREA14's entry the oracle installs
    no hook at an overlay address: the harness's AREA19 hook 0x825930 lies
    inside AREA14's probe body, and the first run stopped at the step limit
    there (an artifact of the harness, not of the code).
  - 0x825A10: `em_level13_port_00825A10` (0x829840 with the colour as a
    parameter); `test_level13_port_reference` reruns the 0x829840 cases on
    AREA14's overlay (`a14 ov 829840 ...`): 2 cases, 27/27 reachable words.
- **Missing (fail-stops)**: every other AREA14 overlay owner. The first to
  run is [0] 0x8238E0 (script 0x826C10 on entering its polygon); then [5]
  0x823760 (the arrival script 0x826800 that sets flag 0x2C) with its op09
  0x8238C0, [2] 0x823B30 (0x826FC0 at the climb's top), [4] 0x823E30, [6]
  0x825FD0 with its op09 0x825C40, [35] / [36] 0x823580. None has a twin in
  another overlay (the same alignment over every overlay file: best match
  under 33 % of the words, except 0x8238C0, a two-call wrapper); each needs
  its own translation and oracle. The boot behaviours AREA14 places are all
  in a ledger or defined (the checklist in SECOND_HALF_AREAS.md section 4);
  which of them fault live is not known until the owners run.

## After the merge (in main)

```sh
# port root, macOS arm64; the user's disc image, ELF, extract/ and the k01 recordings
python3 tools/export_module_loader.py                 # the pack gains AREA00 sub 2
python3 tools/export_level_module_pack.py
python3 tools/export_area00s2_assets.py               # assets/area00/sub2
python3 tools/export_second_half_assets.py --only area14:0 --kind all
python3 -c "import sys; sys.path.insert(0,'tools'); import export_level_world_textures as W, export_level_shadow_receivers as S, export_player_texture_variants as V, level_asset_profiles as P, json
for prof, sub in (('00s2', 2), ('14', 0)):
    W.export(prof, sub, None); d, r = S.build(prof, sub); o = P.output(prof, sub) / 'shadow_receivers.emsr'; o.write_bytes(d); o.with_suffix('.json').write_text(json.dumps(r, indent=2) + '\\n'); V.export(prof, sub, None)"
python3 tools/export_sfx_registry.py                  # scopes (0, 2) and (0x0E, 0)
```

(`--profile 00s2` / `14` of the three texture/receiver tools work as well
where `assets/` holds no symlinks; a worktree with per-file symlinks needs
the direct calls above, since level_asset_audit.private_assets refuses.)

## Checks of this lane

- `test_area00_overlay_reference`: 1734 cases, 2360 of 2360 reachable words,
  the hook contract on every hook.
- `test_area00s2_creature_reference`: 666 cases, every reachable word but
  0x14D9DC (named dead in the test).
- `test_packet_chain_reference` (with 0021BA70's own cases).
- Lane P14b: `test_area19_route_reference` (1285 cases), the `a14 `
  subsets of `test_level10_port_reference` and
  `test_level13_port_reference` (the full default level10 run, before the
  hook fix, passed every non-AREA14 check, its contract and reuse checks
  included).
- `test_area01_sfx_registry` fails here: its pre-extension baseline differs
  from the registry in 8,581 entries spread over every scope after AREA00's,
  AREA04 sub 0 included, which the two scopes this lane appends cannot shift.
  Not run against main (so not proven to fail there too); not changed here.
