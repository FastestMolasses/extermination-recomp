# Draw-list audit: every list the original links into a frame (lane PORT-DRAW-AUDIT, 2026-11-19)

Why: lane PORT-FLARE-DRAW (port 905e0e95) found AREA21's sea grid (context
+0x1E8, 001E1AD0's list) built every frame but never drawn: the port's
`em_background_live` walked only +0x1D8. The game state does not depend on a
picture-only list, so the tick-exact chains cannot catch this class. This
audit enumerates, from the decomp C, every place the original links a list
into a frame's DMA chain, and checks that the port's Original profile (the
GS frame model, `gpu_renderer` 0) draws each where the original's code path
is live.

Method: the decomp's C for the list builders and linkers (every function
named below is byte-matched or NEARMISS C in `src/`); the area flag words
D_008106C8 from the room records D_0024D650[area][sub][entry] + 0x1C
(001B0250; read from the user's own ELF, scratch only); the render context
of the original at the end of every kept guided take (decomp
`build/guided_play/g*/end.p2s`, the fork's exact states: context
D_00275670 = 0x00811CC0, words +0x0C / +0x174 (render flags), +0x1D8,
+0x1E8, +0x2520), and the port's modules. Scratch: decomp
`build/drawaudit/` (never committed).

## 1. The frame's links (001D2300, main-loop step V)

001D2300 is the only builder of the main list and, with 001D1EA0's page
splice (001CB800), the only kicker of a world frame (a scan of the decomp
for 001D2110 / 001D2130 / 001D2160 / 001D2180 / 001D21B0 / 001D21E0 /
001CB800 callers). Its links, in DMA order (RENDER_CONTEXT.md section 9.1):

| # | List | Builder (original) | Linker | Areas live | Port builder | Port draws it? GS frame / GPU | Evidence |
|---|---|---|---|---|---|---|---|
| 1 | channel-1 head: REF of GS block D_00275674 (0x10 qword) | 001D2300, 001D1F80(1, 0, 7) | 001D2300 NEXT (channel 1, below) | all | em_frame_kick (001D2300) | yes / yes | GS_EXACT.md section 9 |
| 2 | draw environment (GS block + 0x20 + 0x190 slot, 0x19 qw) and the clear (+0x420 with flag 3, else +0x3A0) | boot GS blocks, 001D2300's 001015A8 / 00101810 offsets | 001D2180 REFs | all | em_frame_kick | yes (em_rcl_kick_head, the model runs the head) / n.a. | GS_EXACT.md section 9 "The kick" |
| 3 | channel 3 at +0x1D8 (the level background: grid program 0x23C990; AREA21's far light sprite 001D75E0 inside it) | 001C1D00 -> 001E0CF0 -> 001E1E60 | 001E0DF0 CALL 1 (flag 0x20, flag 4 clear, world frame) | flag 0x20: AREA11, 13, 14, 15 sub 0, 17, 18, 21 (+0x174 bit 0 in every end state of those areas) | em_static_world (001E1E60) | yes / yes (the asset grid) | BACKGROUND.md; level smoke check_background |
| 4 | channel 3 at +0x1E8 (AREA21's sea: ripple texture, screen grid, Z-only strips) | 001E1AD0 (render flag 0x22) | 001E0DF0 CALL 2 | flag 0x22: AREA21 only (+0x174 = 0xF / 0x1F in g75..g80) | em_area21 grid (001E1AD0) | yes since 905e0e95 / no | AREA21_PORT.md "The sea's depth"; far-light read-back g77 |
| 5 | **channel 3 node at +0x2520 (the weather's list)** | 001E55F0 -> 001E67C0 (snow tiles, 001CFFE0) or 001E5AC0 (strips, 001CFFE0 + 001E6F60), closed by 001D2DE0(0, start) | 001E0DF0 CALL 3: when +0x2520 != 0 and render flag 5 clear; non-zero at step V only when the frame close's 001E0D70 left it (D_008106C8 & 0x0E000000) | AREA15 sub 0 (0x14290009), AREA18 (0x040A1000 / 01), AREA21 entries 1, 3, 5 (0x02070001, 0x22070001): end states g12, g41, g43, g75, g76 hold +0x2520 != 0 and flag 5 clear (+0x0C = 0x43) | em_snow_runtime (001E55F0 / 001E67C0 / 001E5AC0) | **was: built, never drawn; now yes (this lane) / no** | section 2 |
| 6 | channel 0 (D_0028F700 + slot 0x60800 + 0x8000 .. context +0x10): the world | section 3 | 001D2300 NEXT, end tagged back (001D2160) | all world frames | per producer (section 3) | per producer | section 3 |
| 7 | the chain page D_007635C0 (context +0x00 / +0x04) with every CALL in it | 001CB800 splice; CALL inserters below | 001D2300 NEXT | all | em_chain_page_live (the port's page, walked whole) | yes (all CALLs, the slot-0xFFF passes drawn as passes) / yes except the passes (walked over) | CHAIN_PAGE.md sections 6, 7, 12 |
| 8 | channel 1 (D_0028F700 + slot 0x95760 + 0xC9000 .. context +0x14): 2D (sight, gauges, fades, glyphs, letterbox, radar, credits lines) | GS_EXACT.md section 9's slot-1 census | 001D2300 NEXT | all | per producer | yes (census) / the overlay pass | GS_EXACT.md section 9 "The producers of a world frame's slot-1 packets" |

Status frames (D_008106C4 != 0) link channel 1 first, then channel 0 and the
page, and skip 001E0DF0 (rows 3 to 5 never apply): STATUS_FIELD.md. The
load veil's list frames (0021B1B0) and the tear-down kick (001D1EF0's
001D1EA0(0)) are list-mode frames of their own (CHAIN_PAGE.md section 11,
GS_EXACT.md section 9).

Page CALL inserters (001CB760 / 001CB8A0 callers in the decomp, overlays
included): 001CAAC0, 001CACC0, 001CAFA0, 001CFBE0, 001D1AE0, 001D9530,
001DDB70, 001DDE10, 001DE920, 001DF110, 001DF5A0, 001DF9D0, 001E0D70,
001E7D20, 001E9280, 001E9E60, 001F0720, 001F3E30, 001F4A10. Each has a port
translation that writes the port's page; the page walk draws whatever they
built (a list the port does not build is a "not built" gap of the census,
FIRST_LEVEL_CENSUS.md / WHOLE_GAME_LEDGER.md, not of the renderer).

## 2. The +0x2520 node: built, never drawn (fixed)

**The original.** 001E55F0 (the weather actor) builds its list in channel 3
and stores the start with 001D2DE0(0, start) (context +0x2520). Two readers
send it:
- the frame close 001D1EA0(1) -> 001E0D70: with flag 0x20 set and
  001B0070() & 0x0E000000 == 0 it CALLs the list from the chain page
  (001CB760(&D_007635C0, 0xFFC000, node)) and zeroes the word (AREA11's and
  AREA15 sub 1's and AREA17's snow: CHAIN_PAGE.md section 6.1);
- step V's 001E0DF0, after +0x1D8 and +0x1E8: 001D2E00(0) != 0 and
  001D2910(5) == 0 -> 001D21B0(node), a CALL from the main list, ahead of
  channel 0.
So where D_008106C8 has a bit of 0x0E000000 the weather is drawn from the
main list, before the world: AREA15 sub 0 and AREA18 (bit 0x04000000: it
also selects 001E5AC0's strips, flags & 0x0C000060) and AREA21's entries 1,
3 and 5 (bit 0x02000000: 001E67C0's snow tiles, count 6 for key 0x1500).
Render flag 5 is clear in every captured end state (+0x0C = 0x43; its only
writer in the C is 001D19E0's clear). The original's end states of g12
(AREA15 sub 0), g41 / g43 (AREA18) and g75 / g76 (AREA21) hold the node:
a list of 001CFFE0 tiles (REF of the preset 0x008149C0, CALL of
D_00233800, three CNTs) ending in 001E55F0's RET; g75's and g76's are 64+
tiles long.

**The port before.** em_snow_runtime builds the list and calls 001D2DE0;
em_render_context_001E0D70 leaves it (the same test); em_frame_kick's
001E0DF0 tags it into the port's main list. The renderer drew only +0x1D8
and +0x1E8: the weather in those areas was never in the picture.
CHAIN_PAGE.md section 6.1 had recorded it as unreached ("in the first level
001E0D70 always runs first"), true of AREA11 only.

**The fix.** em_background_live_draw walks the node after +0x1E8 when
+0x2520 != 0 and render flag 5 is clear (read at the frame close, after
001D1EA0's 001E0D70 ran, as step V reads it): chain-page call mode, every
primitive with the environment the list wrote, drawn by the GS frame model
(the GPU renderer does not draw it). A walk fault, a refused primitive or a
node / +0x1E8 with no +0x1D8 list (not met on any route) fail-stops.
`em_background_live_node_pending` is the condition; the log (EmBackgroundLiveLog
node_start / node_prims / node_draws) records each draw (the counts below came from a temporary per-draw print, not committed).

**Evidence.**
- The condition, from the original: the end states above (+0x2520 != 0,
  +0x0C bit 5 clear, D_008106C8 with a 0x0E000000 bit); every other area's
  end state holds +0x2520 = 0.
- AREA21: chain g20 through g76 with the raster (`make
  test-far-light-readback-reference`, EM_HEADLESS_RASTER=1): the node is
  drawn in 4,416 frames (counters 12087..20603), each 108 snow-program
  MSCALs and 108 DIRECT packets, 67..75 primitives (about 70 sprites);
  the GS frame model accepts every one (residency, order; no fault), and
  the read-back comparison passes with its pinned counts unchanged (g75
  1,250 exact / 1 one LSB; g76 1,243 / 297 / 8): the snow writes no Z the
  step-K points read.
- AREA18 and AREA15 sub 0: not run with the raster (the guided run from the
  title to g41 takes about an hour of fast-forward; the g41 card chain g18
  is not exact from row 14). Their lists are the same kind (six 001CFFE0
  tiles of the snow program, in the original's g12 / g41 end states); a
  residency fault there would fail-stop. Next: run `watch_takes.py --root
  ../Extermination/build/guided_play g01_title_newgame --from
  g41_area14_area18 --then 2600 --hidden --env EM_HEADLESS_RASTER=1` (and
  --from g12_area19_area15) and check no fault.
- The snow program is the one AREA11's page draws (em_vu1_snow_program_mscal,
  verified against the original microcode: CHAIN_PAGE.md section 8); the
  list's bytes are the weather translation's (em_snow_runtime, tick-exact
  chains).
- Not verified: the picture (section 4); the VU1 registers the tiles
  inherit (call mode starts the walk from reset registers; the page walk,
  which draws the same tiles in AREA11, starts from reset too).

## 3. Channel 0's producers

Channel 0 is not walked whole in the port: each producer's module draws its
own part. The producers (calls of the channel-argument builders with
channel 0, and every direct writer of the +0x10 cursor; decomp C):

| Producer | What | Port |
|---|---|---|
| 001C1D00 -> 001D5370 (001D4DA0, 001D49D0, 001D4CD0, 001D4960) | the static world's run | em_static_world_live (STATIC_WORLD.md section 7) |
| 001CAA00 walk (001CA990, 001CB3C0, 001CB360 / 001C7420) | the owner units | em_owner_draw_live (OWNER_DRAW.md section 10) |
| 001DA6A0 (001DA310, 001D5C80, 001D9EE0) | the drop shadow's passes | em_shadow_live (SHADOW_ORIGINAL.md) |
| 0020E1E0 / 0020E6F0 (0020E460, 0020EC80), 001CB580 / 001CB4F0 | the status models | status frames only (STATUS_FIELD.md) |
| 0021B1B0 (001DFA40) | the load veil's particles | em_load_veil_live (list frames) |

Open: this census is by literal channel argument and direct cursor writes;
a builder called with a channel in a variable is found only through its
callers. A dynamic check (walk the port's channel 0 at each kick in list
mode and match its primitives against what the modules drew) would close
it; not built.

## 4. The pictures compared (lane PORT-GS-PICS, 2026-11-19)

**How.** The port: `tools/guided_chain.py <chain> --gs-dump TAKE:ROW,...`
(EM_GS_DUMP_AT, comparison only, em_start_at.c) writes the Original
profile's exact GS field of each row (the field the frame whose head hook
traced the row drew) and quits after the last one; `watch_takes.py
--gs-dump` does the same from the title (EM_INPUT_PLAY's segment:offset).
The original: `tools/capture_gs_original.py TAKE ROW...` replays the kept
take's input log from its start state on the fork, hidden, SOFTWARE
renderer 13 (its own scratch ini: the user's settings are untouched), MTVU
off, the take's block mode, and keeps the GS local memory at the loop tops
of rows R + 1..R + 3, checking every stop against take.rec (counter,
position): every stop of this lane was on the take. `tools/compare_gs_frame.py`
pairs them: the frame the port drew for row R is the buffer the port's
FRAME_1 names in the original's memory at loop top R + 2 (the list built in
iteration R -> R + 1 is drawn in R + 1 -> R + 2), compared R, G, B and A
pixel for pixel, with a diff picture under build/gspics/<run>/. Disc-derived
captures stay in decomp build/gspics/orig/ and port build/gspics/ (ignored).

**The half line.** A chain started from a card can have the other relation
between the frame index D_00810E80 and the field D_00810E88 than the
recording (the original's is equal at every loop top of the guided run;
only a movie changes it). The field's OFY follows D_00810E88, so every line
is then half a line off and only ~20 % of the pixels are exact (chain g18 at
g42: 23,841 of 114,688). `--gs-dump` therefore passes EM_START_AT_FIELD (the
card's source state's relation; comparison only): at the script's start the
port's field bit flips once if needed (em_frame_flip_field). With it g42
row 300 went from 23,841 to 114,278 exact.

**Results** (RGB exact of 114,688; max = the largest channel difference):

| Point | Area | Shows | Before this lane | After |
|---|---|---|---|---|
| g77 10 / 300 | AREA21 entry 5, the west room | indoor, title glyphs | 114,545 / 114,598 (max 37 / 6: one-LSB texel residue, a few window pixels) | not re-measured |
| g77 3300 | AREA21 entry 6, the west tower | ladder, snow | 113,717 (max 44) | **114,677 (max 1)** |
| g77 5000 | AREA21, the Gatling | sea grid (+0x1E8), snow, sight, gauge | 113,414 (max 43) | **114,641 (max 1)** |
| g77 8000 | AREA21, the Gatling | the creature over the sea, splash, gauge | 111,577 (max 41) | **114,142 (max 3)** |
| g75 2600 / 3500 | AREA21 entry 3 | the arrival scene: letterbox, glyphs | 114,622 / 114,626 (max 3) | same |
| g76 800 | AREA21 entry 3 | the arrival room | 114,652 (max 2) | same |
| g76 1300 | AREA21 entry 4, the deck | snow from the +0x2520 node | 113,803 (max 68) | **114,664 (max 1)** |
| g73 2700 | AREA20 | the dock, the player's shadow | 114,622 (max 53) | same (open: 18 pixels on the shadow's edge) |
| g73 3500 | AREA20 | a letterboxed scene with its message | 114,671 RGB, **35,140 RGBA** | same (open: alpha, below) |
| g42 300 / 1500 / 2400 | AREA18 | cargo hold, weather node | 114,278 / 114,507 / 114,547 | same (open: the player's hair, below) |
| g42 2600 / 3000 / 5600 | AREA18 | deck, crates | 114,577 / 114,642 / 114,671 (max 2) | same |

- **(a) The sea grid's colour (+0x1E8): verified.** In g77 5000 and 8000 every
  pixel of the sea (the screen grid sampling the ripple texture, colour only)
  equals the original's; the differences before the fix were all snow flakes.
- **(b, c) The weather node (+0x2520): verified in AREA21; drawn and in place
  in AREA18.** AREA21's flakes were drawn at other places: **fixed.** 001E55F0
  passes 001E67C0 flags bit 1 when the area key (D_00810700 << 8) +
  D_00810701 is 0x1500 (decomp func_001E55F0.c), and 001E67C0 then takes the
  reversed rows: the row tilt -row[0] minus the wobble, and each row's phase
  moving down by 1.5 x row[2] x strength, wrapping below 1.0 (func_001E67C0.c).
  The port's runtime tested the area flags D_008106C8's bit 1 instead (never
  set) and always drew the normal rows; at g76 1300 its phases were 1.616 ..
  1.884 where the original's record +0x1F0 held 1.150 .. 1.334 (drift,
  intensity, target, seed equal). em_snow_tiles_mode takes the bit (em_snow.c,
  em_snow_runtime.c), and tools/test_snow_tiles_reference.py runs the
  original's own 001E67C0 instructions with flags 2 and area 0x1500 (3 quick,
  12 full cases: post-state, matrices, parameters and descriptors equal; the
  normal rows on the reversed cases fail it).
- **Open: the player's hair in AREA18 (g42 300, 1500, 2400).** Drawn with other
  texels (teal where the original is brown; up to 188 off), exact from g42
  2600 on, after the bomb pickup's scene (phase C, rows ~2405..2505). The
  chain starts from card g41 (a LOAD GAME into AREA14), so a state the card
  start leaves (the player's face record, a CLUT the scene reloads) is the
  first candidate; the GS memory outside the per-frame buffers was not found
  to differ at row 300. Not traced.
- **Open: g73 3500's alpha.** RGB 99.99 % exact, but the original's buffer
  holds A = 0x80 over the whole picture inside the letterbox where the
  port's holds 0: one primitive writes alpha there in the original (an
  alpha-only pass) that the port does not. Nothing shows it (the display
  ignores A); a later read of destination alpha would. Not traced.
- **Open: g73 2700's shadow edge** (18 pixels, two lines at the player's
  shadow decal's edge) and the one-LSB texel residue elsewhere (GS_EXACT.md
  section 8).
- **(e) The 2D layer:** the gauge bar (AREA21's 0x8260A0), the Gatling's
  sight, the area-title and message glyphs and the letterbox are exact in
  the points above (g77 5000 / 8000, g75 2600 / 3500, g73 3500, g77 10).
  The radar (item 0x06 on the upper rail) is not in any kept take's AREA20 /
  21 frame: not compared.

Reference test: `tools/test_gs_second_half_reference.py` (quick: g76 1300;
EM_TEST_FULL=1: every pinned point through g77), skipped where the
original's captures are absent; timeout 900 s in tools/test_sweep.py.

## 5. Summary

- Top-level links of a world frame: 8 (section 1). Drawn in the GS frame:
  all 8 after this lane; gaps found 1 (row 5, the +0x2520 node: built,
  never drawn), fixed 1. Row 4 (+0x1E8) was the gap lane PORT-FLARE-DRAW
  fixed.
- GPU renderer (Enhanced): rows 4 and 5 are not drawn there (open).
- Open: the channel-0 dynamic census (section 3). Pictures (section 4):
  AREA21 verified after one fix (the reversed snow rows); open the hair in
  AREA18 after a card start, g73's alpha-only pass, one shadow edge.
