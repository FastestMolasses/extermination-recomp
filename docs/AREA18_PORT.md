# AREA18's overlay (lane PORT-A17A18)

Lane PORT-A17A18, 2026-11-17, branch `a17a18`. AREA18 (area byte 0x12,
overlay id 0x0F, `OVERLAY/AREA18.BIN`) is the transport plane entered from
AREA14 by 00190F20 and left back to AREA14 entry 1 the same way: item 0x28
(g[1], which AREA17's [34] takes), the panel [21] and one room move [18]
(SECOND_HALF_AREAS.md section 4).
Every behaviour its placements and groups name is a boot function; the
overlay holds only the entry pad and the area init.

## The overlay has nothing more to translate

- **The init** at 0x823580 (decomp `overlay_AREA18_func_00823540.c`) is
  em_area01_state's: 001E7780's key 0x1200 runs it, and
  `make test-second-half-state-reference` runs the ORIGINAL init over
  AREA18's own overlay file and compares the six globals, the data and the
  whole BSS byte for byte (its AREA18 row).
- **Nothing else**: `make test-area18-overlay-reference`
  (`tools/test_area18_overlay_reference.py`, under a second) checks against
  the user's file that the header equals the state test's AREA18 row, that
  the text's only nonzero words are the init's 8 words (one return; the pad
  and the tail are zero), and that no header or data word points into the
  text; and, in the two guided-run states with AREA18 loaded (decomp
  `build/guided_play/g41_area14_area18` and `g43_area14_area18`, end.p2s,
  extracted once into ignored `build/area18_overlay/states/`), that the
  resident header and text equal the file, D_00810700 is 0x12, and no live
  pool node's behaviour lies in the overlay text (68 / 69 live nodes, 21 boot
  behaviours).

## Live (lane PORT-A18A14, 2026-11-17, branch `a18a14`)

AREA18 loads and plays live in the shared composition, entered by a real
area change from AREA14 (the cargo ramp's 00190F20) in the guided chain
`g18`, and left the same way by its room move [18] (001BB860) to AREA14
entry 1. Its boot behaviours (section 4 of SECOND_HALF_AREAS.md) run
through the later areas' dispatch; nothing new was translated for AREA18
itself.

**Capture source.** `tools/capture_from_take.py --set
guided-20261117/s87/route_g18 g39_area14_face g40_area14_booster
g41_area14_area18 g42_area18_area14 g43_area14_area18 g44_area18_area14`
(decomp `build/fork_refs/`, ignored). g41's and g43's ends are AREA18's
arrivals (area bytes 12 00 00, overlay id 0x0F resident); no kept take
ends inside AREA18 later.

**Exports** (`export_second_half_common`): `SUBS[('area18', 0)]` = g41
(previous g40). g43 is another load (its load map differs), so one SUBS
row holds g41 only. Three cases new to the second-half exporters, each
from the captures:
- **No frame drawn.** Both arrival captures were taken before the first
  frame after the load was drawn: one display-list buffer walks to an empty
  list, the other is AREA14's stale chain. `export_area13_level.UNDRAWN`
  (and `export_level_world_textures.UNDRAWN`) report the level-kick and
  GS-state checks as not shown, and refuse a capture that draws anything.
  Every other check runs: the load map against RAM, the 155 level texture
  decodes against the GS freeze, the cells, the render-context rebuild.
- **No area message bank or table.** The load relocates no id 0x41
  (D_0028A594 keeps g40's AREA14 word) and D_00264DD0[0x13] is 0:
  `export_area13_tables.GLOBAL_ONLY_MESSAGES` writes AREA22's EMMD layout
  (global records and bank, compared with the capture's RAM; 54 global
  records, no area records).
- **No script chain.** No `scripts.emsc` (export_area13_tables skips the
  window when the C starts no chain).
`--kind all` passes: level (766 objects in one zone EMDL), collision (305
polys), cells (35 uids, moved 26 / 27), roster (32 placements, group
0x823600 x 12), spawn (3 records), door row 0x275608, world models (18),
sound (1000 ids; group 3 bank 0 is the fitted attachment's, the
attachment scope's). Profile 18 (`level_asset_profiles`; 478 world
textures, the receivers, the player variants: `RESIDENT_OVERLAY` 0x0F);
001D5370 has no 0x1200 arm, so no dynamic list. sfx scope (0x12, 0);
`export_streams` adds AREA18's spawn records' music (no D_0026EC60 row
names area 0x12). The module pack already held (0x12, 0).

**Runtime.** A mid area (`assets/area18/sub0`), a later area for the boot
dispatch (`em_area01_runtime_bind_area`), its placements' and group's
boot behaviours bind in `em_scene_bindings` (an overlay address is
refused), the muzzle node 001F5040 binds for the gunners 00147390, world
textures and shadow receivers of profile 18; the boot script workers
(001B7670, 001B7700, 001B8AB0, 001B6AE0) serve AREA14 and AREA18 too.

**Start cards** g40 .. g44 (`build_start_saves.py`, route_g18;
`em_start_at.c`): g40 / g41 / g43 AREA14, g42 / g44 AREA18 at its arrival.

**Where it stands** (`python3 tools/guided_chain.py g18 --steer g41`):
g42 (all of AREA18 and the walk-out on AREA14's ramp) is on the row for
8821 of 8821 compared rows, its three hits at the original's rows;
g44 (AREA18 again, the rifle refill) 5295 of 5295. See AREA14_PORT.md
"Lane PORT-A18A14" for the chain and what it needed in AREA14.

**Picture** (lane PORT-GS-PICS, 2026-11-19; DRAW_LIST_AUDIT.md section 4):
the exact GS field of g42 rows 300 .. 5600 against the original's
(`guided_chain.py g18 --steer g41 --until g42 --gs-dump ...`, which sets
EM_START_AT_FIELD: from card g41 the port's field bit is on the other half
line until it flips). Rows 2600, 3000 and 5600 are exact within 2 (99.90 to
99.99 %). Rows 300, 1500 and 2400 drew the player's hair with the
costume-2 texels (teal) until the bomb pickup's rebind: the area load's
001B07C0 (frame state 0) sets the costume D_00810C60 to 0 for AREA18 and its
0015C1F0 sends the player's texture packet again, which the port skipped
(not a card-start artefact). Fixed by lane PORT-GS-PICS2: 114,643 / 114,653
/ 114,550 RGB exact (DRAW_LIST_AUDIT.md section 4 (f)).

## Sound against the original

g42 rows 0..600: 599 of 601 exchanges equal; the repeating 0x7E6 is reset a tick late (the SFX-side IOP tick phase), and at 420285 two voices of a key-on find no free voice in the port (audible; open). Tools, results and open items: SOUND_SECOND_HALF.md (lane PORT-SND2, 2026-11-19).
