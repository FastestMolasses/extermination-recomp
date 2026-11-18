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
| 0x00200A40 | boot | 1472 | save_memcard | missing | none: em_startup's title state 6 requests EM_STARTUP_OPTIONS and em_frontend's notify leaves it pending ("awaits native translation") | The title's OPTIONS (001AC070 state 6): stops the sound (001FBC50, 001FABB0), loads screen module 0x2B, then a seven-row menu over D_00264EC0 drawn by 00201290 with the sub-screens 00201720 / 00201C50 / 00201F70 / 00202BA0 / 00202D10 (all translated in em_options_original for the in-game options 0022A650). **A player who picks OPTIONS on the title waits on a black screen forever.** Section 3 |
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
| 0x001ACA20 | boot | 1104 | frame_main | stand-in | em_frontend.c EM_STARTUP_ATTRACT: a black screen that serves only the demo's button exit (attract_abort) | The attract demo (001AC070 state 3 after 1200 idle ticks on the title, alternating with the intro movie): AREA06, AREA13 (sub 8) or AREA03 (sub 1) by the counter D_00275BD4, 0xE10 frames of recorded pad (001B5BC0). Its state-0 exit (any of 0x9F0 held returns 2) is executed for every held bit by `make test-title-menu-reference`; the demo itself is not ported (WP-17 residue). **Player visible: the idle title shows black instead of the demo** |
| 0x001ACE70 | boot | 80 | frame_main | stand-in | the same | The demo's per-frame exit test (anim_frame_top_a state 4 sub 1); covered by the same stand-in |
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

1. **The title's OPTIONS (00200A40) hangs.** Translate it into
   em_options_original (its sub-screens are there already) with an oracle case
   beside 0022A650's in test-options-reference, translate its list drawer
   00201290 (census group unknown_04), and bind it on the title the way LOAD
   GAME's card screen is bound (SAVE_GAME.md section 6.4: the screen module's
   upload, here 0x2B).
2. **The attract demo (001ACA20, 001ACE70)** shows black. Porting it means
   running AREA06 / AREA13 / AREA03 from the demo's seeds with the recorded
   pad of 001B5BC0 (census group input_io) for 0xE10 frames.
3. The boot logos (001AB9D0, 001ABC60, 001ABE10) and the movie state
   (001AC3B0) are translated without an original run: an oracle in the style
   of test-title-menu-reference (which runs 001AC480 against em_startup) would
   make them verified. The same oracle could execute 0022A460 against
   em_memcard's answers.
