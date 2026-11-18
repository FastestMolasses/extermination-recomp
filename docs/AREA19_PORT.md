# AREA19 in the shared composition (lane J1)

Lane J1, 2026-10-30, branch `area19`. AREA19 (area byte 0x13, overlay id 0x10,
subs 0 and 1) runs in the shared AREA01 composition, as AREA04, AREA22, AREA06,
AREA13, AREA15 and AREA03 do. The route's ground truth is TENTH_LEVEL_ROUTE.md,
TWELFTH_LEVEL_ROUTE.md, THIRTEENTH_LEVEL_ROUTE.md and FOURTEENTH_LEVEL_ROUTE.md.
This is a start-card binding: row-exact New Game chaining is a chain lane's, and
a beat that passes here ends on its recording's block and position, nothing more.

## Beats

`python3 tools/mid_route_beats.py --single <beat>` (recorded pad, recorded path,
the recording's rand() word for comparison only, end block and end position).
No row comparison has been made for AREA19 yet: nothing here is row-exact.

| Beat | Status alone from its card |
|---|---|
| a19_00, a19_01, a19_02 | ends on the recording's block and position |
| a19b_01 | runs to its end past the dropped quad; 21.08 off (y 170, the recording's 190), infection differs |
| a19b_02 | no end in 240 s: bugs bite at frame ~204 where the recording climbs the ladder (the creatures' rand() stream) |
| a19c_00 | position right; D_00810ACE, a first bite the recording took a beat later (rand() stream) |
| a19c_01 | position right past the dropped quad; health and infection words differ (the fight's stream) |
| a19c_02, a19c_04, a19c_06, a19c_07 | ends on the recording's block and position |
| a19c_03 | position right since slides (action 48) run on the recorded stick; infection differs (a bite) |
| a19c_05 | walks off the y-170 step 1.3 from the recorded path and falls to y 150: steering |
| a19d_00, 02, 05, 06, 08, 09, 10, 16, 17, 18, 19, 20 | ends on the recording's block and position; a19d_20 through door [50] into AREA15 |
| a19d_01 | no end in 240 s alone: earlier bites move the player off the ladder at (892, 929.5). In the chain it takes the ladder and reaches the ceiling |
| a19d_03 | position right; infection and ammunition words differ (the fight's stream) |
| a19d_04 | runs to its end, 35.20 off, after bites the recording did not take; 00130AB0's grab now runs |
| a19d_07 | card artefact: the recording starts inside the status page and uses the health item there; the card does not |
| a19d_11 | card artefact: the area script at 0x82E050 fires again at the arrival (3B8D 2 for the whole beat) |
| a19d_12, 13, 14 | cards start in mid-air on a bar; they need the chain, and the chain does not reach them |
| a19d_15 | the latch runs (kind 0, 0021FB40 then 0021FED0); ends on the recording's block and position, past the dropped quad |

20 of 34 beats pass alone; no beat stops at a fault any more. Chains: a19 passes a19_00
and a19_01, then a19_02's infection byte differs; a19b (2026-11-01) runs past the streak
quad, a19b_01 as alone, a19b_02 stalls at the ladder's bites; a19c (2026-11-01) runs
through a19c_04 with vitals differing and stalls in a19c_05; a19d passes a19d_00, takes the ladder and the ceiling, and the player dies there at
a19d_01..02 (bites the recording did not take), so it never reaches a19d_11. a13_05 ends in AREA19, and
a15_00, a15b_00 .. a15b_04 run through AREA15's doors into it (all pass).

A card loads a saved progress block, so the creatures' rand() stream is not the
recording's. Most of the block differences above are that: health and infection
words, and D_00810ACC's first-bite flags, which follow them. They are not
behaviour defects until a row comparison says so.

## What is bound, and its evidence

- **State** (`em_area01_state`): the overlay row, the init 0x8250C0 (five stores,
  D_00275C20 = the first BSS record 0x82F880). `make test-area19-state-reference`
  runs the original 001E7780 and 0x8250C0 over the AREA19 captures.
- **Owners** (`em_area01_runtime_a19.inc`): `dispatch_area19`, each entry's frame
  from its first instruction; the translations are em_level10..14 and
  em_area19_route, with their oracles. Boot owners bind generally and run through
  `em_area01_runtime_later` (tools/gen_later_dispatch.py); an untranslated one
  faults at its address when it runs.
- **New translations**, each from byte-matched C with an original-instruction
  oracle: the worm 00153F10 / 00154040 / 00154120 and the drum 00156620
  (test_level9_port_reference); the hit family 0013B350, 0013B420, 0013B5B0,
  0013B790, 0013B9A0 (same oracle); 0021BED0 (test_level8_port_reference); the
  player's +4 2 / +5 9, 002236F0 (test_player_major2_reference).
- **The player's ledge, bar and ceiling** (`em_player_closure_ledge.inc`): 0016EF50
  and 0017F9E0 (test_level12_port_reference), 00169250 (test_area06_port_reference),
  0016AC50 (test_level14_port_reference), the stick classifier 001751A0 (now
  test_level12_port_reference). The closure's adapters to them have no oracle.
- **The latch**: 0021BF90, 0021FB40, 0021FED0, 002208C0, 00220B50, 00224FE0, 0017FB90
  and kinds 2, 3, 4 (test_level12_port_reference, every reachable word of each).
  0015B770 runs them by +0xD:

  | +0xD (hit +3) | +5 0xD | +5 0xE | what the code does |
  |---|---|---|---|
  | 2 (7) | 00220D30 | 00221060 | 0xD: clip 0x3C then 0x3D, moved by the root node through the matrix at +0xD0, swept against the wall. 0xE: 0x26, 0x27 at the landing, then the deaths or the recovery |
  | 3 (6) | 00221630 | 002217C0 | 0xD: 0x39 then 0x3A, height from the root's +4, swept against the wall. 0xE: 0x3B, 0x1C5 while +0x314 & 0xE0, then 0x1C6 or the deaths |
  | 4 (5) | 00221C70 | none | 0x33, 0x34; 001754E0(8) or the gate frees with 0x38 and D_008106BD; a hit in +0x224 goes through 0021C350, 0x1CA, and ends in +5 3 / 0x3F or +5 1 / 0x40. 0021C040 ends this kind outright |

  Kind 4 has run live in a04d_01, which still does not reach its end. The recording
  is never grabbed and stays at health 60. The port takes a second bite by pad frame
  200 (health 55), is grabbed at (356.42, 14.42, 224.56), and the script has no
  stick input for 001754E0(8), so bites of 25, 12, 12, then the remaining 6 kill the
  player, with no fault. Why the port is bitten and grabbed is not established; it
  starts before the latch. No beat has exercised kinds 2 and 3, so those rest on the
  oracle alone.
  The grab 00130AB0, 00133CD0
  (test_level10_port_reference); 0019A440 and its twins 001A1B80, 0019DB50
  (test_coll_segment_walkers_reference, the only sight probes over captures).
- **Others**: the ring burst 001F28D0 (NEARMISS C at 98.80%, only a schedule differs),
  the crawl node 00188340 family (test_level9_port_reference); script [18]'s
  callbacks 0x826470, 0x826500, 0x826540 (test_level14_port_reference).
- **Scripts** (`em_area01_script_live`): the window 0x82AD50 .. 0x82F790 and the
  op09 callbacks 0x826B30, 0x827540, 0x827B10, 0x827B20, 0x826470, 0x826500, 0x826540.
- **Boot data**: the AREA19 windows of `export_area00_runtime_tables.py`, checked
  against every AREA19 snapshot; `assets/boot_constants.embc`; the whole model
  library (`export_later_library.py`: AREA19's owners bind library models by
  number from their code).
- **Sound**: the area loops 0x415 .. 0x417, 0x44F, 0x52E. A B0 60 loop restarts
  whatever the accumulator was (they jump with one left over);
  test_area11_sfx_reference runs 0x44F in lockstep past its first loop.
- **Timeline**: every scene id 0022EC30's table knows runs over a blank track
  against the original (test_area01_timeline_reference), scene 27's ramp 3852..3960
  included; no recorded beat reaches that ramp.

## Shared code this changed (each from the original)

- `em_area01_live`: 001C9940, 0011CB90, 0011E080, 0011DF98, 001F6640, 001F66F0,
  001F0460, 001FC580, 001CD370, 001CF470; the clipper stage D_008112C0 as a window
  over em_shadow_live's one storage.
- `em_area01_actor_view`: a freed record's +0 reads the 0 001AFC10 stored, and its
  +0xA0 .. +0x1EF keep their last values (a stale tracker pointer reads them).
- `em_camera_live`: camera actions 12, 13 and 15.
- `em_effects_live`: the boot constants; 001F6BB0's key 0x1301.
- `em_aim_fire`: 001CA6E0 on a burning creature, D_0028A490 read-only.
- `em_chain_page`: capacity (prims 32768, units 512).
- EMET: the window limit is 192.
- `em_level9_port` hooks: 0019AB20, 0019AD00, 0011E620, 001B1D20, 001B2B10, 001B4CF0,
  001F0460, 001FC580, 0021C040; 001B0D80 returns its result.

## Open items

1. **ERCPR and ERLENG of zero: the port drops that one primitive** (chosen by the
   orchestrator under the user's "fix everything", 2026-11-01; the user may veto it;
   `em_vu1_page_programs.h`, emvup_efu_zero). This is the port's one choice
   here, not the original's behaviour. A streak quad whose head has w exactly 0 reaches
   the EFU with a zero operand: the head (1056.25, 160, 920) sits on the camera plane,
   the exact w is 2.2e-5, which the truncating sum takes to 0. It is not rare: a19b_01,
   a19c_01 and a19d_15 all hit it, so a player in AREA19's streak rooms does too.
   What the hardware's EFU returns for a zero operand is not established. The fork
   measured a VU1 program of ours started by a VIF1 MSCAL from the BIOS (recompiler, MTVU
   off): ERCPR(1.0) = 1.0, ERCPR(+0) = 0x7F800000, ERLENG(0,0,0) = 0x7F800000; MULx by
   that P gives 0x7F800000 for 1.0, 0xFF800000 for -2.0 and 0xFFC00000 for 0.0. Those
   are the emulator's IEEE values, not a hardware measurement, and the rest of the quad
   (the lanes after P, VFTOI4 of those words, what the GS does with them) was never
   measured, so the port does not use them. Instead the streak and kind-2 programs
   drop that one quad or line, lower the batch's NLOOP and draw the rest; a batch left
   with nothing kicks nothing. The primitive's other arithmetic runs on P = 0, and
   nothing it writes is read by the next one. The grid, floor and ripple programs'
   ERLENG keep the fail-stop: their lengths are not on a camera plane and no run has
   met one. `tools/test_chain_page_reference.py` E runs the original microcode with
   P = 0 at each zero and compares the kicked packets with that primitive's rows taken
   out (every head on the plane, one head at the start and in the middle of a batch,
   a zero screen length).
2. **The draw methods draw** (2026-11-01): kinds 10, 9, 12 and 8: the tendril pieces
   001546C0 -> 00154F00 (also AREA03, AREA04 sub 1, and AREA08's and AREA07's "boxes",
   001CB1F0), the worms (001CB130), AREA15 sub 1's [3] (001CB2B0) and the cloaking
   creature (001CB060). Their units are walked on the page (CHAIN_PAGE.md section 6.6;
   `make test-object-method-reference`). Evidence: the VU1 programs and the page walk
   equal the original microcode; kind 10's units are captured ones, kinds 9, 12 and 8's
   the executed original builders'. Pixels are not compared with an original frame, and
   no beat of this file was seen to draw a worm or kind 8.
3. **a19b_00**, the arrival by the hatch [63], is the AREA13 side's.

## The guided chain from the title (lane PORT-A19-OWNER, 2026-11-18)

`python3 tools/watch_takes.py --check --root ../Extermination/build/guided_play
g01_title_newgame` now plays g11_area19 through AREA19 sub 0's event and into sub 1:
segments 0 .. 340 exact (161,881 ticks), then 1,195 ticks of segment 341 (g12) on the
recording; 163,194 of 165,990 compared ticks in all. Two gaps were closed:

1. **001BAD40's messages 0x270C and 0x270D through the runtime.** 001BB0E0's state 0
   binds its script command with 001BAD40 (em_sdf_001BAD40, test_script_door_fan_reference
   covers both messages); the runtime adapter `rv_slg_001BAD40` gave it no storage for
   0x270C's published word D_008106C0 (= the node's +0x18, which [6]'s 0x825240 reads back
   at its count 0x424) or for 0x270D's request block D_002821B0 / B4 / B8, so the node
   faulted at 008106C0 (g11, after the cutscene at counter 147,500). Each word now comes
   from its one storage (D_002821B0 is em_message_live's block).
   `make test-area01-revisit-runtime-reference` runs 001BB0E0 with both messages in states
   0 .. 2 against the original instructions (six cases, always in the quick run); without
   the binding they fail at 0x8106C0.
2. **Movie selector 6, E39S2.PSS** (the event script 0x82B6E0 that 0x825240 starts):
   `python3 tools/export_movie.py --iso <iso> --disc-path /MOVIE/E39S2.PSS --out
   assets/movies/e39s2.mov` (240 pictures); feed_end 619 from g11's movie.tbl (one
   main-loop step of 641 vblanks, vsync 3177 -> 3818, less E46S4's 22), derived, not
   probed. Without the export the request failed and 001BA1F0 faulted.

**The ceiling hang's step (lane PORT-A19-HANG, 2026-11-18), closed.** In segment 341
(g12, original counter 168,017) the port stepped 0.0696111 a tick where the original
stepped 0.07018. The cause was neither the clip data nor the transition: the port played
the wrong clip. 0016AE40 +6 0x14 phase 0 picks clip 0xBC (+2F1 = 1) or 0xC3 (+2F1 = 2)
from the sign of wrap(0x700031E4 - +C4). Its live adapter's copy of 0x700031E4 was never
filled, so it read 0 and played 0xC3. Clip 0xC3's root track runs at 0.0696111 a frame and
0xBC's at 0.0701818; the bank bytes equal the original RAM's. Only two routines write
0x700031E4: 00177510 (the ledge frame's heading) and 00175CF0's slope branch.
Each port translation of them now also stores into one storage, `EmSceneState.spad31E4`:
em_player_helper_00177510 (record helpers and the climb's live trampolines) and
em_player_floor_apply. c10_pre copies that storage into the closure's scene.

Evidence:
- A hidden fork replay of g12 from its start state, with a per-tick table of the player
  record, 0x700031E4 and node 0, shows the original's word at 1.568941 (stored at counter
  167,770 by the ledge climb 00161790's 0015DF10 / 00177510) and clip 0xBC with +38 =
  0.07018185.
- The port now has the same word, clip and step.
- Oracles: test_player_record_helpers_reference, test_player_climb_reference and
  test_player_floor_reference compare the original's 0x700031E4 after every case with the
  bound storage. The floor case counts its stores, 1,121 in the quick run. Removing either
  publication fails its oracle.

Chain now: segments 0 .. 360 exact (171,489 ticks, through AREA19 sub 1, the hang, back to
sub 0), then 1,122 ticks of segment 361 (g12, AREA19 sub 0) on the recording; 172,611 of
174,453 compared ticks; the playback overruns at counter 179,634.

**The 0xBA move's +290, closed (lane PORT-A19-MOVE, 2026-11-18).** In segment 361 (g12, AREA19
sub 0, original counter 177,811) the Cross in 001662D0 +6 0x34 sends 001809B0 down its kind-0x34
branch: 00177030(p, 2) projects +B0 / +B8 onto the record 0019A570 just hit (its axis +0x34 / +0x3C
and its centre, 00199DB0), and the distance from there to the centre, in 4.5 steps, gives +290. The
translations were right; the live adapter was not. `lc_dash` ran 00177030 over the ladder scratch
as the last `le_fill_last` had left it, which was an earlier record (node 0x35B, axis (0, 0)). So
+B8 stayed at 902.538 instead of moving to 921.9998, the distance was 38.29 instead of 32.97, the
step count was 9 instead of 8, and +290 x came out 799.7250 instead of 804.2250: 4.5 too far, or
-0.375 a tick over the 12-tick move. `lc_dash` now refills the scratch from that sweep first
(node 0x4AB, axis (1, 0)), as `lc_hit_point` already did. The port's +290 is now (804.2250,
240.9756, 921.9996), the original's to the fourth decimal.

Evidence:
- The original capture: g12's recording, and the chain below.
- The translations it composes keep their oracles: test_player_ladder_climb_reference
  (001809B0) and test_player_ladder_entry_reference (00177030, 00199DB0).
- No original-instruction oracle reaches the closure's live adapters (see "What is bound").
  This one was found by tabulating the record that 00177030 reads.

Chain now: segments 0 .. 453 exact (225,637 ticks). Segment 454 (g15, AREA03 sub 1, a cutscene)
leaves only on its last tick, 0.09 away (original counter 235,955). Segments 455 .. 488 are exact,
then 860 ticks of segment 489 (g16, AREA08 sub 2) are on the recording. In all, 238,784 of 240,324
compared ticks are on the recording, and the playback overruns at counter 245,515.

**Next divergence** (segment 489, g16 `_a08_ceiling`, AREA08 sub 2, original counter 246,510).
The position is equal, but the heading is 0.140 off. The beat is the lab's "along the ceiling" stretch.
Which player state runs at that tick has not been checked, and the divergence is not
diagnosed. Following the AREA19 lessons, the next lane should tabulate every word that state's
heading code reads at that tick: scratchpad, record and probe state.
