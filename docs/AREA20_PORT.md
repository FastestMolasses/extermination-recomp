# AREA20's overlay owners (lane PORT-A20)

Lane PORT-A20, 2026-11-17, branch `a20`. Every overlay owner AREA20 places,
groups or spawns is now translated and checked against the original
instructions. AREA20 itself does not load in the port yet: it has no
`export_second_half_common.SUBS` row (no capture kinds, level, collision or
sound) and its boot owners are not bound. So nothing here has run live in
the area; the oracles prove the translations against the original code,
not against a recording of the area.

The decomp's docs/AREA20_OVERLAY.md lists the overlay's 15 functions (14
byte-identical C, the entry pad). Owners from decomp `tools/area_overview.py
--area 20` (SECOND_HALF_AREAS.md section 4) and the overlay data:

| Owner | Who runs it | Port | Evidence |
|---|---|---|---|
| [32] 0x8239B0 with 0x823B00 and 0x823BE0 | sub 0 placement [32] | `em_area20_overlay_008239B0` (and the two callees as entries) | test_area20_overlay_reference |
| 0x823D30, the breakables | placements [19] .. [31] | `em_area20_overlay_00823D30` | test_area20_overlay_reference |
| 0x823580, 0x823600, 0x823780 | the node lists of the arrival script 0x826830 (words at 0x826FCC .. 0x82707C) | `em_area20_overlay_00823580` / `_00823600` / `_00823780` | test_area20_overlay_reference |
| 0x823660, 0x823840 | no word of the overlay names them; boot data does (below) | `em_area20_overlay_00823660` / `_00823840` | test_area20_overlay_reference |
| 0x824440, the group owner | deferred group 0x826100 [12], [14] | `em_area19_route_00824440` (the AREA14 / AREA19 turret, `o->area20`) | test_area19_route_reference |
| 0x8259B0, its probe | called by 0x824440 | `em_level10_port_008259B0` (tint record 0x827E90) | test_level10_port_reference |
| 0x825E90, its spawner | called by 0x824440 | `em_level13_port_00825E90` (colour 0x827EA0) | test_level13_port_reference |
| 0x825F20 | deferred group 0x826100 [13], [15] | `em_area03_route_00825290` | test_area03_route_reference |

The init 0x823990 was already covered (`em_area01_state`,
test_second_half_state_reference).

## What the owners do (from the C; the oracle confirms the instructions)

- **[32] 0x8239B0** (the arrival and the AREA21 request; counter 0x34 is
  the byte D_0081080C). State 0: state 3 when flag 0x34 is set, else the
  model set-up (001B10B0 0x5B, 001C63E0 3, 001CA6F0 2, 001BA8E0), state 1.
  State 1: while the counter is 0, 0x823B00 starts the arrival script
  0x826830 (after 001FABB0) and, at its end, 001FB0B0(0) and counter = 1;
  once the counter is set, 0x823BE0 copies the 64-byte quad record 0x8273B0
  into its frame and asks 001B1EA0(0, the player's position, the copy, 4);
  inside it, counter = 2 and script 0x8270B0, and at that script's end
  001B0C60(0x15, 0, 3) (AREA21 entry 3), +0x2E = 0xFFFF, state 3. A
  001F9100 marker sits at +0xB0 every state-1 frame. The guided run's
  measurement of the quad (decomp FULL_RUN_PLAN.md 54.1: x 148..180, z
  226..305, no height) matches the record's words the oracle loaded.
- **0x823D30, the breakables.** A shot (+0x36) breaks one (state 2) and, when
  0019A570 finds a hit along y +-4, puts effect 4 there. The burst plays
  effects 0x80000013 and 0x8000001C with sound 0x1A1 (kinds 0x18 / 0x2A,
  which end after two frames) or 0x8000002E with sound 0x19F; kinds 10 and
  12 then fall with a random heading, a speed and lift from the tables
  0x827E70 / 0x827E80, a turn, a wall stop (0019AD00) and a landing burst
  (0019AB20, sound 0x1A0). While D_70003B92 is set with counter 0x34 = 2
  (the boarding script) an unbroken one skips its 001B1B70 and method.
- **The effect objects**: 0x823580 / 0x823600 effects each state-1 frame;
  0x823780 emits effect 1 every 5th frame (every 25th while 001C6190 gives
  151..449) and copies its +0x94; 0x823660 / 0x823840 run 001D04B0 packets
  (tables 0x826680; 0x826710 / 0x8267A0 by +0x94) for 2.0 / 1.2 in steps of
  0.02 / 0.03.
- **The twins.** 0x824440, 0x8259B0 and 0x825E90 differ from AREA14's
  0x823FC0 / 0x825530 / 0x825A10 in six, two and two words, each one of the
  differences the decomp C names: the three overlay call targets, the miss
  kind (8 for 5, twice) and the effect's low half (0x67 for 0x2C); the tint
  and colour addresses. 0x825F20 is AREA03's 0x825290 (and AREA14's
  0x825AA0) word for word, with no overlay-relocated word.

## The oracle

`make test-area20-overlay-reference` (`tools/test_area20_overlay_reference.py`,
the test_level14_port_reference harness configured as test_area14_overlay_reference):
the ORIGINAL code runs from the pinned ELF and the user's
`extract/OVERLAY/AREA20.BIN`, written over the overlay arena of a capture's
boot RAM (k01_02, AREA14's recording; boot text checked against the ELF), with
designed nodes in zero RAM. Every callee is a hook with a scripted result; the
vector, matrix and float helpers (00102948, 001029C0, 00102B08, 00102C58,
00102918, 0011DE90, 0011E2A8, 001B1470) run as original code with their
writes replayed. Compared per case: calls and arguments, memory at every
call, the memory accesses between calls one for one in order, memory after
the last store, a poisoned re-run, every reachable word, the callee
inventory, the hook contract and the entry guards.

- Default: 497 cases, about 2 s; `EM_TEST_FULL=1`: 686 cases, about 9 s.
  Every reachable word of the nine functions (719 words) runs.
- **Live nodes (optional).** `python3 tools/test_area20_overlay_reference.py
  --extract-live`, run with the decomp's `.venv/bin/python` (it needs
  zstandard), writes the RAM of the guided run's g73 / g74 end states
  (`build/guided_play/_tries/<take>/abandoned/end.p2s`, AREA20 resident) to
  ignored `build/area20_overlay/live/`. When those files are there, the test
  also runs every live node of this module's owners as recorded: 28 nodes
  ([32] and the 13 breakables in each state; no effect object is alive
  there; the group owners 0x824440 / 0x825F20 belong to the twin tests),
  their overlay text checked against the file. They passed this lane.
- Mutations caught: the lift step 0.06 -> 0.07, the model id 0x5B -> 0x5C,
  the quad copy's frame offset, the emitter's 450 bound; in the turret twin,
  the kind-8 test.

The twins' cases (lane PORT-A20 additions to the existing tests):
- `test_area19_route_reference`: every designed 0x827DD0 case again on
  AREA20's overlay as 0x824440 (callees renamed) and five miss kinds (8, 5,
  6, 0, 255): 1840 cases in all, 1170/1170 reachable words of 0x824440.
- `test_level10_port_reference`: every 0x829370 case again as 0x8259B0 on
  AREA20's overlay (`a20 ...`, 48 cases), 265/265 words.
- `test_level13_port_reference`: the 0x829840 cases as 0x825E90, 27/27 words.
- `test_area03_route_reference`: every 0x825290 case as 0x825F20 on
  AREA20's overlay, 72/72 words.

## Binding

- `em_area01_runtime_second_half.inc` (`dispatch_second_half`) runs area
  0x14's owners (`a20_owner`) and the four twins; `em_area01_runtime.c`
  sends area 0x14 there. Frames from each function's first instruction
  (0x8239B0: 0x20 plus 0x823BE0's 0x70).
- `em_scene_bindings.c` binds AREA20's nine overlay behaviours; any other
  overlay address of the area returns -1 (fail-stop).
- `tools/gen_mid_route_bridge.py` gained `em_area20_overlay.h` (`a20o`);
  the regenerated `em_mid_route_bridge.inc` also holds AREA14's camera-arm
  adapters (`a14o_w_0011DE90` .. `a14o_w_0018D7B0`), which the committed copy
  lacked (it was generated before em_area14_overlay.h gained them).
- Hooks: `tools/gen_area20_overlay.py` writes the header's hook block and
  the wrappers.

## What is left

- **Loading AREA20**: its `SUBS[('area20', 0)]` row, capture kinds, level,
  collision, cells, sound and textures need captures of the area
  (SECOND_HALF_AREAS.md section 2); the guided run's g73 .. g75 states
  were recorded, but the kept chain was forked at g69 (decomp FULL_RUN_PLAN
  62), so their tries sit in `_tries/` and are not recordings to build on.
  The CREATURES / FLAG_RULES rules for 0x823D30 (001A2370) and 0x824440
  (0019C6F0 through the turret) wait for that lane.
- **Boot owners** of AREA20 (section 4: 17, all defined or in a ledger) are
  not bound for area 0x14 in em_scene_bindings.
- **The scripts** 0x826830 and 0x8270B0 (25 and 12 records; the flagged
  pairs 12/0 and 0B/5) are not run; their op14 node lists place 0x823580,
  0x823600 and 0x823780.
- Who spawns 0x823660 and 0x823840 is not traced: no overlay word names
  them, but boot data does (0x823660 at 0x259A0C; 0x823840 at 0x259A3C and
  0x275A3C; 0x823580 / 0x823600 / 0x823780 appear in the same tables), so
  they are probably effect-table rows an effect spawner reads while AREA20
  is resident. Their translations are ready either way.
