# Whole-game ledger: reviewed rows for the census's hidden gaps

The reviewed ledger for the functions of `docs/WHOLE_GAME_CENSUS.md` list (B)
that lie outside the per-level route censuses (FIRST/SECOND/THIRD_LEVEL_CENSUS).
The decomp's `tools/game_census.py port` reads this file as it reads those
ledgers: the first cell is the original address, the second the region
(`boot` or `AREAnn`), the fifth the status. `live`, `verified-unbound` and
`unverified` count as ported (P); `boundary` and `stand-in` as handled
another way (C); `missing` changes nothing (the row stays in (B)). Status
words follow SECOND_LEVEL_CENSUS section 2: `live` bound and exercised,
`verified-unbound` an original-instruction-tested translation that the live
path does not call, `unverified` a translation (under another name) with no
original-instruction run, `stand-in` a substitute is selected, `boundary` a
named platform service replaces the original mechanism, `missing` nothing
ported. Addresses and behaviour only: no original code, data or disassembly.

Every row was read from the byte-matched decomp C (`../Extermination/src/`)
and the port source named. Lanes add their own section; keep one row per
function.

## 1. Lane GAP-2 (2026-11-18): save_memcard, frame_main, lowmem

Why none of these ran in a recording: the kept takes start from save states
after the boot (the main 001AAE40's start-up, the boot's card check and logos
are behind them), none idled 20 s on the title (the attract demo) or opened
the title's OPTIONS, and no game formats a float or calls calloc.

| Runtime | Region | Bytes | Group | Status | Port owner | Evidence |
|---|---|---:|---|---|---|---|
| 0x00200A40 | boot | 1472 | save_memcard | live | em_options_original em_options_00200A40, bound by em_frontend options_frame (EM_STARTUP_OPTIONS / EM_STARTUP_OPTIONS_FRAME, em_startup's state 6) through em_options_live on the boot title's own binding (s_card, module 0x2B applied by its loader hook) or, after a death, the AREA11 host's | The title's OPTIONS (001AC070 state 6). `make test-options-reference` group `title` (every state, crafted row tables, every branch both ways under EM_TEST_FULL=1) and `make test-title-options` (lane PORT-OPTIONS, 2026-11-18): the boot title, every page walked by the original capture's closed-loop presses, 56 distinct states equal to the fork capture `../Extermination/build/options_ref` in order, 29 with its exact lengths (the rest end on a press or are module 0x2B's host-speed load). OPTIONS.md section 4.5 |
| 0x00201290 | boot | 1156 | unknown_04 | live | em_options_original em_options_00201290 (00200A40's list) | `make test-options-reference` group `title`: every row, open or not, every value, both pad modes; drawn on every OPTIONS frame of `make test-title-options` |
| 0x00229770 | boot | 492 | save_memcard | boundary | em_memcard (the host card) under em_frontend's native_storage_ready | The boot check's GetInfo on port 0 then port 1 (001FECB0). The host card always answers a formatted card (type 2, format 1, free 8000) or, under EM_TEST_CARD_FRESH, an unformatted one (format 0): section 2 |
| 0x00229960 | boot | 148 | save_memcard | boundary | the same | The boot check's free-space test (00228320, translated as em_card_00228320 and oracle-tested in test-card-screen-reference) on both ports; the host card's 8000 free always passes it: section 2 |
| 0x00229A00 | boot | 112 | save_memcard | boundary | the same | The boot check's no-save step: 00229960, then the full-card screen 0022A020 only when neither port has room: section 2 |
| 0x00229A70 | boot | 396 | save_memcard | boundary | the same | The boot check's GetDir of the save directory BASCUS-97112-DS00-00 on both ports (001FEC20, translated as em_save_001FEC20, test-save-reference); em_memcard answers it from the host folder: section 2 |
| 0x00229C00 | boot | 136 | save_memcard | boundary | the same | The boot check's formatted-card step (00229A70, then 00229A00 when no port holds the save): section 2 |
| 0x00229C90 | boot | 912 | save_memcard | boundary | the same | The no-card screen (help group 7 lines 0x0B, 0x08, 0x20, 0x02, two choices). The host card is never absent, so the port never reaches it: section 2 |
| 0x0022A020 | boot | 1080 | save_memcard | boundary | the same | The full-card screen (help group 7 lines 0x0C, 0x07, 0x20, 0x02, two choices). The host card always has room, so the port never reaches it: section 2 |
| 0x0022A460 | boot | 292 | save_memcard | boundary | em_frontend native_storage_ready -> em_memcard_boot_check (both slot folders made, both ports asked) | The boot's card check, called only by 001AB9D0 state 3 sub 0 with arg 0 (so its result 2 is ignored): on the host card it returns 1 with no screen, which is what em_startup's EM_STARTUP_CARD_CHECK completion does: section 2 |
| 0x001AAE40 | boot | 924 | frame_main | verified-unbound | em_main_loop_and_gap.c em_mlg_001AAE40_init / em_mlg_001AAE40_frame (and em_mlg_001AB140, the vblank handler); the live main loop is the native em_frame_run (main.c, em_frame.c steps A..W) | `make test-main-loop-and-gap-reference`: the original start-up and one loop frame executed over captured route RAM, every worker call, hardware access and written field compared, and the frame's call sites against the original's frame traces |
| 0x001AB1E0 | boot | 400 | frame_main | boundary | main.c em_stream_live_boot (streams.emst) and the host audio thread | The IOP reboot, six IRX loads (the sound driver) and the four IOP audio buffers: the PS2 IOP the port does not have; the buffers' role is em_stream_live's (docs/IOP_STREAM.md) |
| 0x001AB370 | boot | 192 | frame_main | boundary | the native renderer (em_gfx); em_frame.c clears to the colour 001AB370 sets; em_scene_state holds the screen offset 0x70003B94 / 96 and centre 0x70003B70 / 72 it zeroes / sets | VU1 and DMAC reset and the GS double-buffer set-up: hardware the native renderer replaces (FIRST_LEVEL_CENSUS section 4's renderer boundary) |
| 0x001AB650 | boot | 80 | frame_main | unverified | em_task.c em_task_init (called by em_frame_init) | Zeroes the three 0x20-byte frame-task slots at D_0028A750; em_task_init clears the native table and the current-slot pointer. No original-instruction run |
| 0x001AB9D0 | boot | 652 | frame_main | unverified | em_startup.c boot_tick major 2 (first screen, module 0x28, the card check, logo A) | The original's six states mapped one to one (fade full, module 0x28 load, fade in, the card check 0022A460 as EM_STARTUP_CARD_CHECK, fade out to 2, the 300-tick logo countdown); `make test-startup` is a sanitizer fixture, not an original run |
| 0x001ABC60 | boot | 428 | frame_main | unverified | em_startup.c boot_tick major 3 (module 0x29, logo B, bank 0x1B) | As above |
| 0x001ABE10 | boot | 380 | frame_main | unverified | em_startup.c boot_tick major 4 (logo C, bank 0x1C) | As above |
| 0x001AC3B0 | boot | 196 | frame_main | unverified | em_startup.c movie_tick (001AC070 state 1: sound stop, the E900 movie, its skip) | As above; em_frontend's begin_movie plays it (docs/STARTUP.md) |
| 0x001ACA20 | boot | 1104 | frame_main | live | em_sf_001ACA20 (em_scene_task.c), run by em_scene_attract_001ACA20 from em_frontend's attract_tick (lane PORT-DEMO) | The attract demo (001AC070 state 3 after 1200 idle ticks, alternating with the intro movie): AREA06, AREA13 entry 8 or AREA03 sub 1 by D_00275BD4, 0xE10 records of module 0x36's recorded pads (001B5BC0). `make test-scene-task-reference` executes the original over every arm (616 runs); `make test-attract-demo` compares demo 0 with the original's capture (records 1..300 equal, CROSS exit); with `EM_TEST_FULL=1` all three demos are 3600/3600 equal to it (lane PORT-DEMO2: demo 0's stale weapon view in 0016B790's 001607D0, demo 1's 00183AC0 +0x9F read, coat packet and render stamp; docs/ATTRACT_DEMO.md section 4). All three are live |
| 0x001ACE70 | boot | 80 | frame_main | live | em_sf_001ACE70 (em_scene_task.c) | The demo's per-record exit test (001ACA20 state 4 sub 1); executed inside 001ACA20 by `make test-scene-task-reference` and run every record of the live demos |
| 0x001B5BC0 | boot | 200 | input_io | live | em_pad_record_001B5BC0 (em_input.c) through em_frame_pad_record_001B5BC0 and the bindings' w_001B5BC0 (lane PORT-DEMO) | The demo's recorded pad: record a0 of demo D_00275BD4 from D_0028A6AC (module 0x36) into D_00810E70..76, E64, E65, E57. `make test-pad-record-reference` executes the original (closed world) and compares its seven stores with the core over every record of the three captured tables and random tables (lane PORT-DEMO2); the fed pad also equals the capture on all 3600 records of each demo (`make test-attract-demo`, docs/ATTRACT_DEMO.md) |
| 0x001AE900 | boot | 324 | frame_main | boundary | em_frame.c frame_screen_fade_draw and em_fade (the native fade layer) | The boot's build of the 2x2 letterbox / fade display blocks at D_0028A7B0 (GS packets) and their counters: the native renderer draws the same two 32-line bands |
| 0x001AEA50 | boot | 268 | frame_main | boundary | the same | Called only by the boot reset 001AB430 (from 001AAE40's start-up). The build of the two fade GIF packets at D_0028A8E0 and the zeroed fade state D_0028A9A0..A4 (the native fade starts clear), then 001AE900 |
| 0x0011DB10 | boot | 72 | lowmem | boundary | the host C library; em_options_live's sprintf 00122D10 (conversions d and s only, any other faults) | newlib isinf, called only by the SDK's vfprintf 00123750 (class C) for a float conversion; no game format the port serves has one |
| 0x0011DB58 | boot | 56 | lowmem | boundary | the same | newlib isnan, the same caller |
| 0x0011E878 | boot | 188 | lowmem | boundary | the host allocator | newlib _calloc_r, reached only from the SDK's 00121AF8 (class C, the malloc family) |
| 0x0011E938 | boot | 88 | lowmem | boundary | the host C library | newlib's errno-preserving close wrapper (errno cleared, 0010BE50, errno stored on -1), reached only through the stdio vtable entry 00122ED0 (class C) |
| 0x0011E990 | boot | 532 | lowmem | boundary | the host C library | newlib dtoa's quorem, reached only from _dtoa_r 0011EBA8 |
| 0x0011EBA8 | boot | 4556 | lowmem | boundary | the host C library | newlib _dtoa_r, reached only from the SDK's 00124C68 (class C); the port's sprintf faults on a float conversion |

The lowmem rows sit inside game_census.py's libm window (0x11C4C8..0x11FD78,
kept as game arithmetic), but they are libc: their every static caller is an
SDK host service (class C).

## 2. The memory card: what the original shows, what the port does

The original asks the card in three places: the boot's card check (0022A460,
section 1), the card screen of a save or a load (00225AC0 and 00227300's tree,
all translated and bound, docs/SAVE_GAME.md section 6), and the load
enumeration (002283B0). Read from the byte-matched C:

| Situation | The original | The port |
|---|---|---|
| Boot, a card holding the game's save (either port) | GetInfo on both ports, GetDir finds the save directory, the check ends; no screen, the logos follow | The same outcome: em_memcard_boot_check, no screen (the host card answers GetInfo for both ports) |
| Boot, a formatted card with room and no save | GetInfo, GetDir (none), the free-space test passes; no screen | The same outcome (host card: free 8000) |
| Boot, an unformatted card | GetInfo's format word 0 on a PS2 card ends the check at once (the format prompt waits for the save screen); no screen | The same (EM_TEST_CARD_FRESH, test input only) |
| Boot, no card in either slot | 00229C90: help group 7 lines 0x0B, 0x08, 0x20 and 0x02 with a two-way choice (Right / Left move it, cue 0020CDA0). Cross on the first choice continues without a card; Cross on the second, Circle or Triangle (cue 0020CD60) runs the check again; a card that arrives is seen by the polling GetInfo and the check restarts | Not reachable: the host card (data/memcard/slot1, slot2) is always present; a folder that cannot be made fails the boot with a message |
| Boot, formatted cards with no save and too little room on both | 0022A020: lines 0x0C, 0x07, 0x20 and 0x02, the same choices; its polling restarts the check when a card changes | Not reachable: the host card always has room |
| Save or load: no card, an unformatted card (format prompt, Yes / No), a full card, a removed card, a rejected card, a failing read or write, a save over an existing directory, a declined confirmation | The card screen's states | Translated; the oracle `make test-card-screen-reference` runs each of these on a scripted card. The host card can produce three of them: a card without the directory, a card with it (overwrite: only the chosen slot's file is rewritten, SAVE_GAME.md section 2), and the fresh unformatted card (format prompt; the format fails as the fork's did). `make test-memcard-host` and `make test-options-card-live` cover those on the host folder |
| Load: a save copied from a real card into the host folder | The load enumeration and LOAD GAME's read | The host folder holds the card's files byte for byte (SAVE_GAME.md section 5); the translated load reads them; a save the port wrote equals the original's (compare_card_image.py, SAVE_GAME.md section 6.2) |

The game has no delete. No new card situation became reproducible in this lane,
so no new save test was added: the host card's three situations above are
already tested.

## 3. Remaining player-visible gaps of this lane (next steps)

1. **The title's OPTIONS (00200A40)**: done by lane PORT-OPTIONS (section 5
   below; OPTIONS.md section 4.5). Its moving background (0020A7A0) draws
   since lane PORT-TITLE-BG (section 6).
2. **The attract demo (001ACA20, 001ACE70)** is ported (lane PORT-DEMO,
   docs/ATTRACT_DEMO.md) and all three demos are live and 3600/3600 equal to
   the original's capture (lane PORT-DEMO2).
3. The boot logos (001AB9D0, 001ABC60, 001ABE10) and the movie state
   (001AC3B0) are translated without an original run: an oracle in the style
   of test-title-menu-reference (which runs 001AC480 against em_startup) would
   make them verified. The same oracle could execute 0022A460 against
   em_memcard's answers.

## 4. Lane GAP-1 (2026-11-18): list (A) and the small (B) groups

List (A) of the census (executed by a recording, not credited) was, apart
from AREA00's three, translations named without their address. Each row
names the translation, where it is bound, and the oracle. `unreachable`
(class D) marks a function whose only static reach was a data word that is
not a pointer. A `missing` row on an address the census saw only as a
constant in code overrides that mechanical "bound" credit (the camera aim
host's sight drawers: their addresses are callee addresses that
em_camera_live's aim_call does not serve, so they fault).

| Runtime | Region | Bytes | Group | Status | Port owner | Evidence |
|---|---|---:|---|---|---|---|
| 0x00162080 | boot | 268 | actor_anim | live | em_player_climb.c area11_vault_target, inside em_player_climb_vault_tick (00162190) | test-player-climb-reference: 00162190 runs as original code with 00162080, 0017D940, 0017DAF0, 0017DC80 and 0017DE20 unhooked inside it; the vault tick is player state 3 (em_player_closure_live set1 0x03, em_player_climb_live_state) |
| 0x00162190 | boot | 2220 | actor_anim | live | em_player_climb.c em_player_climb_vault_tick (player state 3) | the same |
| 0x0017D940 | boot | 420 | actor_anim | live | em_player_climb.c vault_setup form 0 | the same |
| 0x0017DAF0 | boot | 388 | actor_anim | live | em_player_climb.c vault_setup form 1 | the same |
| 0x0017DC80 | boot | 404 | actor_anim | live | em_player_climb.c vault_setup form 2 | the same |
| 0x0017DE20 | boot | 140 | actor_anim | live | em_player_climb.c vault_step | the same |
| 0x00162A40 | boot | 876 | actor_anim | live | em_player_recovery.c em_player_recovery_hang_entry (em_player_recovery_state4, set1 0x04) | test-player-recovery-reference: 00162A40 (HANG) executed and compared; state4 equals the direct routine on the record |
| 0x001645D0 | boot | 508 | actor_anim | live | em_player_hang.c em_player_hang_up_state (set1 0x0A, over the hang workers) | test-player-hang-up-reference executes 001645D0 (its two callees 001749A0 / 001FBD50 scripted and recorded) over designed states from the captured player record (no kept capture holds +5 = 0x0A): all 0x320 bytes, every call, every branch outcome. The user's run1 reaches it at vsync 120001 |
| 0x00180004 | boot | 60 | actor_anim | live | em_player_closure_0e_18.c em_player_closure_00180000 (00180000's body) | test-player-closure-0e-18-reference executes 00180000/00180004 |
| 0x00187EA0 | boot | 20 | frame_update | live | em_player_floor.c em_player_first_contact case 0x5C; bound in em_player.c (live_contact_001FB9F0) | test-player-floor-reference: the first-contact one-shots 00187DC0 / 00187EA0 executed |
| 0x001CB070 | boot | 184 | anim_runtime | live | em_area01_model_draw.c (method 0x001CB130: +0x80..+0x88 = 1.0) then em_owner_draw_live_method (mode 4, 001C7420, the 001D40E0 list, RET, 001CAAC0) | test-area19-kind9-draw-reference runs 001CB130 -> 001CB070 and every callee unhooked over captured AREA19 RAM (a19c; the freed worm record and live records, designed bytes named in the test): whole EE RAM and scratchpad equal to em_area01_model_draw_call's. It found 001D40E0's store of D_00251BE0..E8 missing (fixed 2026-11-18). The VU1 program: test-object-method-reference. Live in AREA19 (g11) |
| 0x00825040 | AREA04 | 376 | overlay AREA04 | live | em_level16_boot.c a04_825040, called by a04_824EF0 (em_level16_boot_a04_00824EF0, bound for D_00810700 == 4 in em_scene_bindings) | test-level16-boot-reference runs 0x825040 inside its caller 0x824EF0 |
| 0x008251C0 | AREA04 | 180 | overlay AREA04 | live | em_level16_boot.c a04_8251C0 (the same caller) | the same |
| 0x00825280 | AREA04 | 656 | overlay AREA04 | live | em_level16_boot.c a04_825280 and a04_825310 (the slot holds two functions; em_level16_boot_a04_00825310 bound for area 4) | test-level16-boot-reference runs 0x825280 inside 0x825310 |
| 0x00823C20 | AREA07 | 380 | overlay AREA07 | live | em_area08_route.c em_area08_route_a07_00823C20 (half_both, called by the terminal 0x823990, bound in em_area01_runtime_area08.inc and em_scene_bindings for area 7) | test-area08-route-reference FUNCS07 executes 0x823C20 |
| 0x008247C0 | AREA00 | 8 | overlay AREA00 | missing | none | The empty callback 0x8247D0 stores in its record (+0x34). On the AREA00 boss chain (g34), lane a00boss2 |
| 0x008247D0 | AREA00 | 992 | overlay AREA00 | missing | none (not in area01_rebind's sub-2 list) | A debris behaviour of the boot prototype table (0x2593AC): matrix, velocity through it, two packets, stops on 0019A570. Executed in g34; lane a00boss2 |
| 0x008260B0 | AREA00 | 64 | overlay AREA00 | missing | none | Script 0x82B080's op09 record 0x82B400 callback: two 001EFD20 registrations, returns 1. Executed in g34; lane a00boss2 |
| 0x0017D080 | boot | 1920 | actor_anim | live | em_player_recovery.c em_player_recovery_ledge_catch, bound as 00162DB0's test_0017D080 worker (em_player_closure_live w_ledge_catch) | test-player-recovery-reference CATCH executes 0017D080 |
| 0x001DEDF0 | boot | 140 | weapon_equip | live | em_render_context.c em_render_context_001DEDE0 (the jump target written inline), run at the render context's load (em_render_context_live) | test-render-context-reference executes 001DEDE0, which jumps into 001DEDF0 |
| 0x001B5790 | boot | 76 | input_io | boundary | the platform pad layer and em_input (the 001B5940 pad-block translation) | 001AAE40's start-up only (before the frame loop): libpad port open and the pad record's initial bytes; the port's pad block starts from em_input's own model |
| 0x001B5860 | boot | 216 | input_io | boundary | the same | Called only by 001B5790 |
| 0x001F9780 | boot | 148 | unknown_03 | boundary | the host audio thread (em_sfx / em_audio) | 001AAE40's start-up: creates and starts the sound thread (entry 001FB0C0) |
| 0x00225CC0 | boot | 48 | ui_screens | boundary | the port's zeroed card-screen record (em_card_original) and host file reads | 001AAE40's start-up: clears D_00810040 (0xD4 bytes) and starts the CD stream service 001FE910 (class C) |
| 0x002032C0 | boot | 144 | iop_services | boundary | host file reads of the exported assets (em_frontend's file table note) | Called only by the CD stream service 001FEE60 (class C): fills the 9-entry disc file table D_00821010 through the IOP |
| 0x0021B860 | boot | 128 | area_state | boundary | em_render_context_live.c load (RENDER_CONTEXT.md section 8) | The boot builder's fog pair and the +0xA0 copies; the area load rewrites them before a bound routine reads them (the +0x100 copy is read only by 001D2730(0, 0)) |
| 0x00140004 | boot | 1080 | entity_logic | unreachable | none needed | Its only static reach is three overlay words (AREA03 0x827A20, AREA04 0x82A680 / 0x82B5B0) that sit right after a placement record's behaviour pointer: record fields (halfwords 4 and 0x14), not a pointer. No call or code constant reaches it |
| 0x00199770 | boot | 592 | init_io | missing | none: em_camera_aim calls it (FN_99770) and em_camera_live's aim_call faults | The missile-launcher sight's companion (D_00810CA4 == 2) |
| 0x001DB830 | boot | 404 | weapon_equip | missing | the same (FN_DB830) | Sight set-up for the missile launcher and the night-vision sight |
| 0x001DB9D0 | boot | 724 | weapon_equip | missing | the same (FN_DB9D0) | Sight set-up for modes 0 / 1 and the tactical sight |
| 0x001DBCB0 | boot | 148 | weapon_equip | missing | the same (FN_DBCB0) | The tactical sight's zoom read-out |
| 0x001DBD50 | boot | 208 | weapon_equip | missing | the same (FN_NIGHT) | The night-vision sight (D_00810CA7 == 9) |
| 0x001DBE20 | boot | 212 | weapon_equip | missing | the same (FN_MISSILE) | The missile-launcher sight (D_00810CA4 == 2) |
| 0x001DBF00 | boot | 280 | weapon_equip | missing | the same (FN_DELTA) | The auto-sight (D_00810CA4 == 0) |
| 0x001DC020 | boot | 1512 | weapon_equip | missing | the same (FN_DC020) | Mode 0's zoom drawing |
| 0x001DC610 | boot | 636 | weapon_equip | missing | the same (FN_DC610) | Mode 1's sight |
| 0x001DC890 | boot | 204 | weapon_equip | missing | the same (FN_DC890) | Mode 1's zoom drawing |
| 0x001DC960 | boot | 992 | weapon_equip | missing | the same (FN_TACTICAL) | The tactical sight (D_00810CA7 == 8) |
| 0x001DCD40 | boot | 504 | weapon_equip | missing | none | 2D lines of the night-vision and missile sights (001D1F80, then 00207F80 per line) |
| 0x001DCF40 | boot | 168 | weapon_equip | missing | none | 2D marks of the night-vision and tactical sights |
| 0x001DCFF0 | boot | 376 | weapon_equip | missing | none | The scope ring (002082B0) of the auto-sight and mode 1 |
| 0x001E3620 | boot | 8 | weapon_equip | verified-unbound | em_effect_001E3620, accepted by both +0x34 contact dispatchers (em_area11_bindings, em_scene_bindings) | The empty contact callback 001E3630 stores at +0x34; test-effect-001E3630-reference runs the original (it returns at once, storing nothing). Bound in the live path, but no recording reaches it |
| 0x001E3630 | boot | 1764 | weapon_equip | live | em_effect_001E3630 through em_aim_fire_burst (row 0x1E3630 of em_area11_bindings, every area) | The flamethrower's burst: 00188630 mode 4 (sub-weapon 4, the fuel counter D_00810CAE) spawns 0x80000039, whose record names this behaviour. test-effect-001E3630-reference runs the original over every state and branch. The attract demo 1 (equipment 4) fires it from record 2304, and its records stay equal to the original's capture to 3600 (lane PORT-DEMO2; ATTRACT_DEMO.md section 4). No kept take reaches it (EFFECT_MANAGER.md section 8.2) |
| 0x001ECE70 | boot | 124 | hud_objects | verified-unbound | em_effects_live handler_001ECE70 (em_effect_001E3630's translation) | Effect handler of 0x8000001A (subtype 0x10), whose one spawner is 001E3630's retry window: 001CFB50, then 001CFBE0 of D_00257000. test-effect-001E3630-reference runs the original. Bound in the live path, but no recording reaches it |
| 0x001EE830 | boot | 868 | hud_objects | unreachable | none needed | Effect-driver handler D_00255434 for subtype 0x28, whose only record is the global effect 0x80000075. No code on the disc builds that handle and no data word holds it (section 5.2): no effect of subtype 0x28 is ever spawned |
| 0x00823590 | AREA07 | 988 | overlay AREA07 | unreachable | none needed | AREA07's per-area effect 0 (D_00259C74[7], record 0x259550). No factory call that can run in AREA07 passes handle 0: the AREA07 overlay calls none with a per-area handle, the boot's constant per-area handles are 2..9, and 0014CDD0's table handle D_00245CE0[+0xD] is 0 only for +0xD 3, which its one spawner 0014D1E0 never writes (its three callers pass 0, 1, 2). Section 5.2 |
| 0x001F1AA0 | boot | 808 | fx_render | unreachable | none needed | The behaviour of the global effect 0x8000002D (record 0x258500). No code on the disc builds that handle and no data word holds it (section 5.2) |
| 0x001B9FF0 | boot | 144 | math_vector | unreachable | none needed | Script opcode 0x08 (ftab_0024D880[8]). No script record on the disc (boot and all 19 overlays) has opcode 0x08: section 5.1 |
| 0x001B73A0 | boot | 708 | unknown_02 | verified-unbound | em_area_script.c op19 (case 0x19 of the script host, every id) | Script opcode 0x19. Its one use on the disc is AREA15 script 0x8290E0 (record 0x829160, id 5), started by owner 0x824C90 for +0xD 0x5A when D_008107FF is 1 (a second talk); no recording ran it. `make test-area-script-reference`: 14 op19 cases execute the original 001B73A0 (ids 0..5, out of range, every sub-state and both 001B1380 outcomes). Section 5.1 |

### 4.1 What remains (next steps)

1. **The weapon sights (14 functions, ~7.0 KB).** Aiming with the
   auto-sight, the missile launcher, mode 1, the tactical sight or the
   night-vision sight runs em_camera_aim's 001999C0, whose drawers
   em_camera_live's aim_call does not serve: the port faults
   ("aim camera callee ... has no binding"). The census had credited them
   as bound because their addresses are constants in em_camera_aim.c. They
   are 2D draws (00207F80 lines, 002082B0 rings) through the em_gfx 2D
   layer; the next lane translates them with an oracle and serves them in
   aim_call.
2. AREA00's 0x8247C0 / 0x8247D0 / 0x8260B0 are on the boss chain (g34):
   lane a00boss2.
3. Effects, prototypes and script commands never seen in a recording
   (001ECE70, 001EE830, 001F1AA0, AREA07 0x823590, 001E3630 with 001E3620,
   001B9FF0, 001B73A0): resolved by lane GAP-3, section 5.
4. 001645D0 and 001CB070 have original-instruction runs since 2026-11-18 (GAP-3:
   test-player-hang-up-reference, test-area19-kind9-draw-reference).

## 5. Lane GAP-3 (2026-11-18): the effects, the script commands, two oracles

The rows are in section 4 (one row per function). `verified-unbound` here
means translated, oracle-tested and bound where the original dispatches it,
but never exercised by a recording.

### 5.1 Script opcodes 0x08 and 0x19

The script interpreter 001BA1F0 runs 0x40-byte records (opcode = word 0 &
0xFFF, flags in bits 29..31, bit 31 or opcode 0x18 ends, bit 30 jumps to
word 1). Every word-aligned word of the boot image and the 19 overlays whose
low 12 bits are 0x08 or 0x19 and whose other bits are only flags was taken
as a record and followed (+0x40, or the jump, to an end; loops accepted).
The scan finds the known scripts (the door scripts D_0024DE40 / D_0024DEC0,
the item script D_00266620, and every op 0x0B, 0x15 and 0x09 user). Results:

- **0x08 (001B9FF0): no script.** The scan's only chain is at 0x2666E8,
  which lies 8 bytes into record 0x2666E0 of the item script D_00266620 (its
  own records are opcodes 07, 0E, 0A, 00, 0A, 09, 07|end): the word is that
  record's sub-command field. Two flagged words elsewhere (AREA04 0x82AA94,
  AREA14 0x8264CC) are inside placement records and start no valid chain.
- **0x19 (001B73A0): one script**, AREA15 0x8290E0 (07, 00, 19 id 5, 15,
  18), started by 0x824C90 (em_level14_port_area15.c) on the second talk to
  the +0xD 0x5A owner. Translated as op19 in em_area_script.c and run by the
  same script host as AREA15's other scripts.

### 5.2 The effect factory's handles

001EF9D0 (and its nine wrappers 001EFD20 .. 001F0120) resolves a handle:
bit 31 set indexes the global 0x30-byte table at 0x257C90, otherwise the
area's table D_00259C74[D_00810700]; the record's +0xC is the new actor's
behaviour, its +8 the subtype that selects the driver handler D_00255434
(8 bytes per subtype). The factory and its wrappers are reached only by
direct calls (no data word or code constant holds their addresses). Every
direct call on the disc (483) was resolved: 443 pass a constant handle; the
other 40 are the wrappers' own pass-throughs or were read in the decomp C
(variables chosen among constants, the table D_00245CE0, AREA02's
{id, pos} records). Results:

| Function | Effect | Spawned by |
|---|---|---|
| 001E3630 (+ 001E3620) | global 0x80000039 | 00188630, sub-weapon 4 (player +0x275; fuel D_00810CAE, the flamethrower unit) one-shot: 001EFEB0(0x80000039, 0x700036A0). It is the flamethrower's burst, not an enemy beam |
| 001ECE70 | global 0x8000001A (subtype 0x10) | only 001E3630 (both side probes fail in its 1.0..1.7 window). An AREA11 word 0x8000001A at 0x82AAA4 is the +0x14 argument of an op0C record of script 0x82AA90, not a handle |
| 001EE830 | global 0x80000075 (subtype 0x28) | nothing |
| 001F1AA0 | global 0x8000002D | nothing |
| AREA07 0x823590 | AREA07 effect 0 | nothing (row in section 4) |

The flamethrower path: em_player_equipment's 00188630 mode 4 -> w_001EFEB0
-> em_effects_live_001EF9D0 -> the node behaviour 0x1E3630 (row in
em_area11_bindings, every area) -> em_aim_fire_burst -> em_effect_001E3630;
the impact effect's handler 001ECE70 is handler_001ECE70 in
em_effects_live's w_handler. `make test-effect-001E3630-reference` executes
the original 001E3630 / 001E3620 / 001ECE70 over the s87 beat 08 capture
(80 + 12 cases quick, 412 + 64 full: every state, both probe outcomes, the
1.0 and 1.7 boundaries, whole lifetimes) and compares calls, RAM and the
scratchpad. No recording took the flamethrower (AREA13 g[2], FULL_RUN_PLAN),
so the live path has not run in play.

### 5.3 The two unverified rows

- 001645D0 (set1 0x0A): `make test-player-hang-up-reference` runs the
  original over states designed from the captured player record (no kept
  capture holds +5 = 0x0A; every changed byte is listed in the tool).
- 001CB070: `make test-area19-kind9-draw-reference` runs 001CB130 ->
  001CB070 with its callees unhooked over the AREA19 beats a19c_01..05 and
  compares all RAM and the scratchpad with the port's draw path. It found
  that 001D40E0 / 001D4440 store their colour rows (D_00251BE0.., D_00251C20..)
  and the port did not; fixed (the drawn packets were already equal).

## 6. Lane PORT-OPTIONS (2026-11-18): the title's OPTIONS

00200A40 and its list 00201290 are translated (rows in section 1) and bound;
the title's OPTIONS now opens, walks its seven rows, opens each page, changes
the settings and returns to the title. Checked against the original's code
(test-options-reference group `title`) and the fork capture
(test-title-options). The settings it writes are the one storage the in-game
options (0022A650) and the saves use, so the effects (vibration, mono, the
screen offset, the button masks) are the in-game options' (OPTIONS.md
section 1); no launcher option or host setting conflicts.

The moving background (lane PORT-TITLE-BG, 2026-11-18). It drew black on
the boot title's OPTIONS and LOAD GAME screens (and on the title's LOAD
GAME after a death). The cause was not GS memory: em_frontend drew its
black frame as an overlay rect every title tick, and overlay rects draw
over the backdrop layer that 0020A7A0's sprites use, so the rect covered
them. The black is now the overlay pass's bottom fill (the one the status
pages' em_status_background_frame queues), under the backdrop.
The earlier reading (the texture's CLUT missing from module 0x2B, the
title needing its own resident GS state) was wrong, checked against the
captures' GS memory:
- 00200A40 calls 0020A7A0 (TEX0 0x200435259D422180: PSMT4 128 x 64 at
  TBP 0x2180, CLUT CBP 0x21A9) only from its state 3, after its
  001FF080(0, 0x2B) load has finished; 00225AC0 calls it (TBP 0x21E0,
  CBP 0x2260) only in its states 1 and 2, after its module 0x2A load.
- Module 0x2B's upload writes blocks 0x1D00..0x21FF, texture and CLUT
  included; the EMSP's 0x2B step equals the OPTIONS capture's GS memory
  (build/options_ref/menu) in all 1,280 blocks. Module 0x2A's step
  (0x1D00..0x227F) equals the new boot-title LOAD GAME capture's
  (build/title_load_ref, tools/capture_title_load.py) in all 1,408.
- Before those loads the title's GS memory in 0x1D00..0x247F is the
  library module 0x1B the boot uploads (001AB7E0 step 3): build/options_ref
  /title_start equals assets/gs_library.emgm there block for block. No
  screen samples it before the module load replaces it, so the boot
  title's binding still starts from cleared memory, and no export is
  added.
Pictures (the port's GPU overlay, 1920 x 1440 box-filtered to the
capture's 512 x 224, against original.png; the overlay is not the GS
frame, so no GS-exact gate applies): OPTIONS menu, p0..p5 and the boot
title's LOAD GAME show the background; the mean brightness of a
background-only region is the capture's within 1-2 levels. Per page,
pixels within 24 levels: menu 96.0%, p0 98.1%, p1 99.5%, p2 95.9%, p3
94.1%, p4 90.7%, p5 95.8%, LOAD GAME 96.9%. The differences are the
background's scroll phase (the port reaches each picture 2 to 9
0020A7A0 calls after the original's, the documented press-latency and
host-speed load differences: L0 x 217 against 216 at p0 ... 76.5 against
73 at the menu, where the original's layer-1 pulse has already re-armed)
and the GPU overlay's filtering of the glyphs and sprites.
