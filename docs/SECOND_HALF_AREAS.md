# The second half: AREA14, AREA16, AREA17, AREA18, AREA20, AREA21

Lane ASSETS2, 2026-11-02, branch `assets2`. The user's instruction of 2026-11-02
("keep porting more levels. Try to finish all of them") leaves six areas never
ported: AREA14, AREA16, AREA17, AREA18, AREA20 and AREA21 (overlay ids 0x0B,
0x0D, 0x0E, 0x0F, 0x11, 0x12; the area byte equals the area number). This lane
did everything that does not need a route capture, so that the area lanes can
bind each area from CAP2's recordings. Nothing here has run live in an area:
no start card, beat or capture of these areas exists yet.

The route through them (decomp docs/WORLD_GRAPH.md section 7, item 7, inferred
from which gate each step opens): AREA00 sub 2 -> AREA14 (item 0x26) -> AREA18
(item 0x28) -> AREA17 (0x824700 takes 0x28 and gives 0x2B) -> AREA11 entry 3 ->
... -> AREA15's room -> AREA06 sub 1 door [3] -> AREA16 (flag 0x31) -> AREA06
sub 1 -> AREA04 sub 1 [48] -> AREA20 -> AREA21 -> the ending.

Each overlay's decompilation (functions, split slots, what each owner does as
far as its C was read) is in the decomp's docs/AREA14_OVERLAY.md,
AREA16_OVERLAY.md, AREA17_OVERLAY.md, AREA18_OVERLAY.md, AREA20_OVERLAY.md and
AREA21_OVERLAY.md; read an owner's C there before translating it.

## 1. What is done, and its evidence

- **Overlay state** (`em_area01_state.c`, `k_mid` and 001E7780's keys). Each
  row as its MWo3 header states it:

  | Area | Byte | Id | Text | Data | BSS | BSS size | Init (key) |
  |---|---|---|---|---|---|---|---|
  | AREA14 | 0x0E | 0x0B | 0x2C40 | 0x826180 + 0x2300 | 0x828480 | 0x140C00 | 0x823740 (0xE00) |
  | AREA16 | 0x10 | 0x0D | 0x4F40 | 0x828480 + 0x4080 | 0x82C500 | 0x140C00 | 0x824420 (0x1000, 0x1001) |
  | AREA17 | 0x11 | 0x0E | 0x2D40 | 0x826280 + 0x2C00 | 0x828E80 | 0x140C80 | 0x823B80 (0x1100) |
  | AREA18 | 0x12 | 0x0F | 0xC0 | 0x823600 + 0x780 | 0x823D80 | 0x140C00 | 0x823580 (0x1200) |
  | AREA20 | 0x14 | 0x11 | 0x2BC0 | 0x826100 + 0x1E00 | 0x827F00 | 0x140C00 | 0x823990 (0x1400) |
  | AREA21 | 0x15 | 0x12 | 0x6E40 | 0x82A380 + 0x3900 | 0x82DC80 | 0x140C00 | 0x826180 (0x1500) |

  Every init is AREA15's four stores (C28 = 0x20, C1C = the overlay's BSS, C2C
  = 0, C24 = 0), each byte-identical C in its overlay. AREA17's BSS is 0x80
  longer than the others'. `make test-second-half-state-reference` runs the
  ORIGINAL 001E7780 and the inits for subs 0..7 and 0xFF over each area's own
  overlay file, on a host capture's boot RAM (a08_01; it supplies only the
  boot's memory) with synthetic BSS and globals (zeros, a poison pattern, the
  host's leftover bytes), and compares the six globals, the overlay data and
  the whole BSS byte for byte; 21 of the runs go through an init, the rest
  through no key. Plus 48 header refusals and 6 wrong-area refusals. The
  quick run is 57 of 162 runs; the full sweep (EM_TEST_FULL=1) passed once.
- **Module pack** (`tools/export_module_loader.py` DEFAULT_AREAS and
  `export_level_module_pack.py`): AREA14, AREA16 rooms 0 and 1, AREA17,
  AREA18, AREA20 and AREA21. Disc sectors only, no capture: the pack is
  211,229,952 bytes, 190 ranges.
- **Static exports** (`tools/export_second_half_assets.py --kind static`, over
  `tools/export_second_half_common.py`): per area `assets/areaNN/` with
  `sub<s>/roster.emro`, `spawn_table.emsp`, `door_destinations.emsp`,
  `scripts.emsc` (not AREA18: no chain), `overlay_data.emsc` and the receipt
  `static.json`. The roster, spawn and door walks are the AREA13 lane's
  exporters, run without captures over the pinned ELF and overlays. Each file
  loads through the port's own loaders (em_actor_roster_load,
  em_spawn_table_load, em_script_image_load; checked once in a scratch
  program). AREA20's door row 0x27560C is the highest row D_0024E140 names, so
  no next row ends it: the export ends it after the highest door id a door
  placement of AREA20 uses (001BC150 reads row + 4 * (id & 0x7F) only).
- **Scripts windows** (`em_area01_script_live.c`): the lowest chain entry up
  to the end of the last record any chain reaches (export_area13_tables'
  rule; it reproduces AREA03's, AREA13's and AREA19's committed windows), the
  overlay data section after it, and each area's door destinations. The chain
  entries are the data addresses the original's script-starting overlay
  functions pass to 001BA1A0 (decomp `tools/area_overview.py`); the committed
  overlay C's func_001BA1A0 calls name the same set. No capture has run them.

  | Area | Window | Chains |
  |---|---|---:|
  | AREA14 | 0x826800 .. 0x8283A0 | 7 |
  | AREA16 | 0x829340 .. 0x82C240 | 10 |
  | AREA17 | 0x826A20 .. 0x828730 | 7 |
  | AREA18 | none | 0 |
  | AREA20 | 0x826830 .. 0x8273B0 | 2 |
  | AREA21 | 0x82B240 .. 0x82D110 | 7 |

- **Boot behaviours** (lane BOOT2, 2026-11-02, branch `boot2`; section 6):
  all six that appeared nowhere are translated from the decomp C, each
  proven by an original-instruction oracle and bound through the later
  dispatch: 001C2420 and 001C4A00 (`em_level17_boot`), AREA17's creature
  00133E30 (`em_area17_creature`), AREA21's creatures 00137830
  (`em_area21_boot`, with the helpers 001283D0 and 0014C220 .. 0014D1E0 the
  other two share), 0014EC00 (`em_area21_creature_b`) and 00151590
  (`em_area21_creature_c`). Nothing of them has run live in an area.
- **Owner translations: none.** No overlay owner of these areas was
  translated: an original-instruction oracle for an owner needs either a
  capture of the area or a designed machine state that this lane could not
  justify for any owner here. Every overlay owner is a fail-stop (the
  runtime's missing worker fault) until an area lane translates it.

## 2. What waits for the route captures

The level (load map, cells, collision), world models, the messages' area bank,
world textures, shadow receivers and the sound bindings are all built from a
capture: the load map's cursors D_0028A73C / D_0028A740 differ from run to run
(0x1335F40 .. 0x133C1C0 across the recorded areas), and every exporter checks
its bytes against the captures. `export_second_half_common.SUBS` holds no
capture folder yet; its capture kinds refuse with a clear message. An area lane:

1. fills its `SUBS[(target, sub)]` row (`paths`: the capture folders in route
   order; `previous`: the capture before the load);
2. models each 001A2370 caller as `CREATURES[target]` and each 0019C6F0
   caller as `FLAG_RULES[target]` by reading its function (the install
   refuses otherwise): AREA14 0x823FC0; AREA16 0x8273F0, 0x825800; AREA17
   0x825560, 0x823BA0, 0x824700; AREA20 0x823D30, 0x824440; AREA21 0x8297C0,
   0x82A1D0, 0x826840, 0x827CD0;
3. runs `python3 tools/export_second_half_assets.py --kind all --only
   areaNN:S` (then `--kind place`), and adds the area to
   `tools/level_asset_profiles.py` for the textures and receivers, as the
   AREA08 lane did.

Layouts, from each INDEX.IDX sector: AREA14, AREA17, AREA18, AREA20 and AREA21
are flat (like AREA13 and AREA22); AREA16 has AREA08's layout (a top block
with no sections, one nested block per sub). AREA16's two lists, and AREA17's
and AREA21's flat lists, relocate id 0x45: those take
export_area15_level.export_dynamic. A flat list with id 0x45 is new (AREA13's
and AREA22's have none); export_dynamic over the flat builder is untried.

## 3. Running the exports (after the merge, in main)

```sh
# port root, macOS arm64, pure Python; the user's disc image, ELF and extract/
python3 tools/export_module_loader.py              # the pack with the six areas
python3 tools/export_second_half_assets.py         # --kind static: assets/area14 .. area21
make test-second-half-state-reference
# the inventory below (needs the decomp's census of each area first):
#   (decomp root) .venv/bin/python tools/area_overview.py --area N   for N in 14 16 17 18 20 21
#   and for the ported areas 0 1 2 3 4 6 7 8 11 13 15 19 22 (the twin search)
python3 tools/second_half_inventory.py --doc docs/SECOND_HALF_AREAS.md
```

In a worktree whose `assets/module_loader/modules.emml` is a symlink to
main's, unlink the symlink first (the exporter writes through it otherwise).

## 4. The checklist, per area

Generated by `tools/second_half_inventory.py` from the decomp's static census
(`tools/area_overview.py`, over the user's ELF and overlays) and this repo's
sources. Read it as a work list, not as evidence:

- **Port** for a boot behaviour: its rows in the first-level census ledgers
  (FIRST/SECOND/THIRD_LEVEL_CENSUS.md: `live`, `verified-unbound`, ...) and the
  files that define a function named for the address (tools/level_readiness.py's
  reader). Owners bound through macro tables and not in a ledger show as
  missing; an area lane confirms each one by reading the definition.
- **Overlay** behaviours: none is translated. "twin of" names a function in a
  ported area's overlay whose words are equal with the overlay-relocated
  fields masked (lui halves, the low halves added to them, arena jumps): a
  lead, not a proof; near-twins (another constant, one more call) are not
  found (AREA07's turret 0x823F80 does not match AREA03's).
- **Op/sub pairs the census snapshot flags**: the chain's op/sub pairs that
  decomp tools/area_overview.py's admission snapshot of em_area_script.c does
  not list. The snapshot's sub rules are dated: today ops 0x12, 0x13, 0x17
  and 0x1A go to the composition's external handlers (001B76D0, 001B7670,
  001B6D70, 001B6AE0, all defined in the em_area01_script_* and runtime
  files), and op 0x0A / 0x0B have their own sub switches. Each flagged pair is
  to be checked against the handler's definition before the chain runs; a
  sub the handler does not take faults there.
- **Boot functions reachable** and not in the first-level census: their port
  status by the same reader; the rest of the list needs each function's
  definition read.

<!-- second_half:summary begin -->
| Area | Subs | Placements | Behaviours (boot / overlay) | Boot behaviours in a ledger or defined | Chains | Doors used |
|---|---|---|---|---|---:|---:|
| AREA14 | 0 | 37 | 8 / 8 | 8 of 8 | 7 | 0 |
| AREA16 | 0, 1 | 18 + 73 | 24 / 14 | 24 of 24 | 10 | 7 |
| AREA17 | 0 | 43 | 9 / 15 | 9 of 9 | 7 | 0 |
| AREA18 | 0 | 32 | 11 / 0 | 11 of 11 | 0 | 1 |
| AREA20 | 0 | 66 | 17 / 4 | 17 of 17 | 2 | 1 |
| AREA21 | 0 | 63 | 16 / 25 | 16 of 16 | 7 | 3 |
<!-- second_half:summary end -->

Notes the tables do not carry (decomp WORLD_GRAPH.md section 6):

- **AREA14**: the way on to AREA17 is [6] 0x825FD0's script 0x828120, whose op09
  callback 0x825C40 requests area 0x11 entry 0 (001B0C60); AREA18 is entered by
  the camera action 00190F20 (the player inside the quad D_0024A4B0; live in the
  first-level census). Flags 0x2C, 0x3D..0x3F; no door placement.
- **AREA16**: [1] (sub 0) reloads AREA16 sub 1; [7] goes to AREA06 entry 3; [3]
  is a room move gated by D_00810851 bit 1, which the panel [2] 00158EC0 sets;
  sub 1's panel [6] 00159210 sets bit 0. Flags and counters 0x31, 0x33.
- **AREA17**: [38] 0x825470 requests AREA11 entry 3 (counter 0x5D, a scan);
  [34] 0x824700 takes item 0x28 and gives 0x2B. Flags 0x2D..0x30.
- **AREA18**: one room move [18]; the way back to AREA14 is 00190F20 (the
  player's x at most 285). The panel [21] 00159210 sets D_00810853 bit 0. No
  area message table (D_00264DD0[0x13] is 0, as AREA22's), no script chain.
- **AREA20**: [2] goes to AREA04 entry 5; [32] 0x823B00 / 0x823BE0 request
  AREA21 entry 3 (counter 0x34 and a quad).
- **AREA21**: three overlay doors ([27], [33] 0x826E60; [30] 0x826BB0, which
  also writes the lock bytes D_00810841 indexed) make room moves; [53]
  0x827040 stores D_008106B8 = 1 (the ending) after flag 0x36's event; lock
  and counter writers 0x826310, 0x8261A0, 0x826840, 0x8268F0, 0x8269A0.

### AREA14

<!-- second_half:area14 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 00141D20 | boot | sub0 g 0x826180 [21], sub0 g 0x826180 [22] | `em_level11_port_creature.c` |
| 001551B0 | boot | s0[8], s0[9], s0[10], s0[11], s0[12], s0[13] (+2) | census: live (level 1) |
| 00156620 | boot | s0[16], s0[17], s0[18], s0[19], s0[20], s0[21] (+13) | census: live (level 1); `em_level9_port_exit.c` |
| 0015AFA0 | boot | sub0 g 0x826180 [10], sub0 g 0x826180 [11], sub0 g 0x826180 [12] | census: live (level 1) |
| 001C4CB0 | boot | s0[7] | `em_area02_math.c` |
| 001C5C90 | boot | s0[1], s0[3] | census: live (level 1); `em_area11_roger.c`, `em_roger_actor_original.c` |
| 001E3D90 | boot | group 0x8265a0 [0], group 0x8265a0 [1], group 0x8265a0 [2], group 0x8265a0 [3], group 0x8265a0 [4], group 0x8265a0 [5] | census: live (level 2); `em_area01_sys.c` |
| 00219550 | boot | sub0 g 0x826180 [0], sub0 g 0x826180 [1], sub0 g 0x826180 [2], sub0 g 0x826180 [3], sub0 g 0x826180 [4], sub0 g 0x826180 [5] (+4) | census: live (level 1) |
| 0x823580 | overlay | s0[35], s0[36] | `em_area14_overlay.c` |
| 0x823760 | overlay | s0[5] | `em_area14_overlay.c` |
| 0x8238E0 | overlay | s0[0] | `em_area14_overlay.c` |
| 0x823B30 | overlay | s0[2] | `em_area14_overlay.c` |
| 0x823E30 | overlay | s0[4] | `em_area14_overlay.c` |
| 0x823FC0 | overlay | sub0 g 0x826180 [13], sub0 g 0x826180 [15], sub0 g 0x826180 [17], sub0 g 0x826180 [19] | missing |
| 0x825AA0 | overlay | sub0 g 0x826180 [14], sub0 g 0x826180 [16], sub0 g 0x826180 [18], sub0 g 0x826180 [20] | missing; twin of AREA03 0x825290, AREA11 0x827490, AREA19 0x8298D0 |
| 0x825FD0 | overlay | s0[6] | `em_area14_overlay.c` |

| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |
|---|---:|---|---|---|
| 0x826800 | 13 | 0x823760 | 0x8238C0 | 12/0 |
| 0x826C10 | 12 | 0x8238E0 | — | — |
| 0x826FC0 | 14 | 0x823B30 | — | 0B/5 |
| 0x827340 | 5 | 0x823B30 | — | — |
| 0x827490 | 4 | 0x823E30 | — | — |
| 0x827590 | 22 | 0x823E30 | — | 12/0, 13/1 |
| 0x828120 | 10 | 0x825FD0 | 0x825C40 | 17/0 |

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| — | — | — | — | no door placement |

Boot functions reachable from this area's owners and not in the first-level census: 144 (66,280 bytes); the port defines or ledgers 134 of them; 7 more are named in the sources only (a comment, a table or another name: read each): 00183160, 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130, 001F6B00; 3 appear nowhere: 001CACC0, 001D39A0, 001D3DA0.

Overlay functions: 14 (AI 1, C 12, NM 1; decomp status at the census commit); 8 have a translation in the area's own module (em_area14_overlay.c).
<!-- second_half:area14 end -->

### AREA16

<!-- second_half:area16 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 0012A5D0 | boot | sub0 g 0x828480 [4], sub0 g 0x828480 [5], sub0 g 0x828480 [6], sub0 g 0x828480 [7], sub0 g 0x828480 [8], sub0 g 0x828480 [9] (+5) | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exita.c` |
| 0012E3A0 | boot | nest group link 0 0x828b30 [0], nest group link 0 0x828b30 [1] | `em_level8_port_creature.c` |
| 0013D2D0 | boot | nest group link 0 0x828b30 [2], nest group link 0 0x828b30 [3], nest group link 0 0x828b30 [4], nest group link 0 0x828b30 [5], nest group link 0 0x828b30 [6] | `em_level15_creatures.c` |
| 00158BD0 | boot | s0[4], s0[5] | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exitb.c` |
| 00158D30 | boot | s0[8] | census: live (level 2); `em_area01_sys.c` |
| 00158EC0 | boot | s0[2] | `em_area02_math.c` |
| 00159210 | boot | s1[6] | census: live (level 1); `em_area02_math.c` |
| 00159620 | boot | s1[7] | `em_area02_math.c` |
| 00159970 | boot | s1[4] | `em_area02_math.c` |
| 00159B90 | boot | s1[5] | census: live (level 2); `em_area01_sys.c` |
| 0015A2C0 | boot | s1[62], s1[63], s1[64], s1[65], s1[66], s1[67] (+9) | census: live (level 2); `em_area01_sys.c` |
| 0015AFA0 | boot | sub0 g 0x828480 [2], sub0 g 0x828480 [3], sub1 g 0x8287d0 [10], sub1 g 0x8287d0 [11], sub1 g 0x8287d0 [12], sub1 g 0x8287d0 [13] | census: live (level 1) |
| 001BB860 | boot | s0[1], s0[3], s0[7], s1[0], s1[1], s1[2] | census: live (level 2); `em_area01_math_owner.c` |
| 001BC350 | boot | s1[3] | census: live (level 1) |
| 001BECC0 | boot | s0[17], sub1 g 0x8287d0 [17] | `em_level15_fountain.c` |
| 001BF6B0 | boot | s0[14], sub1 g 0x8287d0 [14] | `em_level8_port_gap.c` |
| 001C06E0 | boot | sub1 g 0x8287d0 [15], sub1 g 0x8287d0 [16] | `em_level9_port_anim.c` |
| 001C1030 | boot | sub0 g 0x828480 [15], sub0 g 0x828480 [16], sub0 g 0x828480 [17] | `em_level9_port_anim.c` |
| 001C1A80 | boot | s0[15], s0[16], s1[71], s1[72] | `em_area04_port.c` |
| 001C2420 | boot | s0[0], s0[12], s1[12] | `em_level17_boot.c` |
| 001C4820 | boot | s0[6], s1[35], s1[36], s1[37], s1[38], s1[40] (+15) | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001C4AF0 | boot | s1[8] | `em_area02_math.c` |
| 00219550 | boot | sub0 g 0x828480 [0], sub0 g 0x828480 [1], sub1 g 0x8287d0 [0], sub1 g 0x8287d0 [1], sub1 g 0x8287d0 [2], sub1 g 0x8287d0 [3] (+6) | census: live (level 1) |
| 0022DCD0 | boot | s1[9] | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exita.c` |
| 0x823580 | overlay | group 0x829750 [4], group 0x829750 [5], group 0x829750 [6], group 0x829750 [7], group 0x829750 [8] | `em_area16_overlay.c` |
| 0x824440 | overlay | s1[39] | `em_area16_overlay_b.c` |
| 0x824690 | overlay | s1[11] | `em_area16_overlay_b.c` |
| 0x824E10 | overlay | group 0x829660 [1], group 0x829750 [3] | `em_area16_overlay_b.c` |
| 0x825310 | overlay | s1[10] | `em_area16_overlay_e.c` |
| 0x825800 | overlay | s0[10] | `em_area16_overlay_c.c` |
| 0x8261A0 | overlay | s0[11] | `em_area16_overlay.c` |
| 0x8263D0 | overlay | s0[13] | `em_area16_overlay_e.c` |
| 0x826C50 | overlay | s1[44], s1[45], s1[46] | `em_area16_overlay.c` |
| 0x827030 | overlay | s1[42], s1[43] | `em_area16_overlay.c` |
| 0x8273F0 | overlay | s0[9], s1[13], s1[14], s1[15], s1[16], s1[17] (+10) | `em_area16_overlay_d.c` |
| 0x827B70 | overlay | s1[41] | `em_area16_overlay_d.c` |
| 0x827CE0 | overlay | s1[28], s1[29] | `em_area16_overlay_d.c` |
| 0x827FA0 | overlay | s1[30], s1[31], s1[32], s1[33], s1[34] | `em_area16_overlay_d.c` |

| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |
|---|---:|---|---|---|
| 0x829340 | 11 | 0x824540 | — | — |
| 0x829970 | 30 | 0x824690 | 0x825630 | 13/0, 13/1, 1A/1 |
| 0x82A1C0 | 8 | 0x825800 | 0x826C20 | — |
| 0x82A3C0 | 12 | 0x825800 | 0x826C20 | — |
| 0x82A6C0 | 6 | 0x8263D0 | — | — |
| 0x82A840 | 9 | 0x8263D0 | — | — |
| 0x82AA80 | 10 | 0x8263D0 | — | — |
| 0x82BC40 | 8 | 0x826C50 | — | — |
| 0x82BE40 | 8 | 0x826C50 | — | — |
| 0x82C040 | 8 | 0x826C50 | — | — |

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| 0 | [1] | 0x1bb860 | 0 | area change: area 0x10 entry 0 sub 1 |
| 0 | [3] | 0x1bb860 | 1 | room move: entries 2 / 3 |
| 0 | [7] | 0x1bb860 | 2 | area change: area 0x06 entry 3 sub kept |
| 1 | [0] | 0x1bb860 | 3 | area change: area 0x10 entry 1 sub 0 |
| 1 | [1] | 0x1bb860 | 4 | room move: entries 1 / 2 |
| 1 | [2] | 0x1bb860 | 5 | room move: entries 4 / 3 |
| 1 | [3] | 0x1bc350 | 6 | room move: entries 6 / 5 |

Boot functions reachable from this area's owners and not in the first-level census: 273 (145,244 bytes); the port defines or ledgers 256 of them; 11 more are named in the sources only (a comment, a table or another name: read each): 001B1CE0, 001C1500, 001C1570, 001CAE30, 001CAF70, 001CB070, 001CB130, 001DAFA0, 001DB240, 001DB250, 001DB480; 6 appear nowhere: 001CACC0, 001D39A0, 001D3DA0, 001DAC90, 001DAE10, 001DB040.

Overlay functions: 30 (AI 1, C 25, NM 4; decomp status at the census commit); 23 have a translation in the area's own module (em_area16_overlay.c, em_area16_overlay_b.c, em_area16_overlay_c.c, em_area16_overlay_d.c, em_area16_overlay_e.c).
<!-- second_half:area16 end -->

AREA16's overlay behaviours above (all 14, with the children 0x825260 /
0x828200, the op09 callbacks 0x825630 / 0x826C20 and the helpers) are
translated by lane PORT-A16 (`em_area16_overlay*.c`,
`make test-area16-overlay-reference`: the original AREA16.BIN over a
capture's arena with designed states, every reachable word) and bound in
`dispatch_second_half` for area byte 0x10. The effect functions 0x8237A0 ..
0x824110 have no static reference, but they are reached at run time: an
effect node's behaviour comes from its effect id (001EFD20 -> 001EF9D0;
AREA16's ids 0..4 are the five in order); lane PORT-A16FX translated and
bound them (em_area16_overlay_f, AREA16_PORT.md "Effects"). The water grid [10] calls
(001DAFA0 .. 001DB480) is translated in `em_area16_water` (section 6).
AREA16_PORT.md.

### AREA17

<!-- second_half:area17 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 00133E30 | boot | group 0x8263c0 [0] | `em_area17_creature.c` |
| 001551B0 | boot | s0[4] | census: live (level 1) |
| 00156620 | boot | s0[9], s0[10], s0[11], s0[12], s0[13], s0[14] (+15) | census: live (level 1); `em_level9_port_exit.c` |
| 0015AFA0 | boot | sub0 g 0x826280 [1], sub0 g 0x826280 [2], sub0 g 0x826280 [3], sub0 g 0x826280 [5], group 0x826500 [0], group 0x826560 [0] | census: live (level 1) |
| 001C2420 | boot | s0[0], s0[1], s0[2], s0[3] | `em_level17_boot.c` |
| 001C4820 | boot | s0[32] | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001C5C90 | boot | s0[35] | census: live (level 1); `em_area11_roger.c`, `em_roger_actor_original.c` |
| 001E3D90 | boot | group 0x826420 [0], group 0x826420 [1], group 0x826420 [2], group 0x826420 [3] | census: live (level 2); `em_area01_sys.c` |
| 00219550 | boot | sub0 g 0x826280 [0] | census: live (level 1) |
| 0x8239D0 | overlay | group 0x826890 [4], group 0x826890 [5], group 0x826890 [6], group 0x826890 [7] | `em_area17_overlay.c` |
| 0x823BA0 | overlay | s0[30] | `em_area17_overlay.c` |
| 0x8242D0 | overlay | s0[39] | `em_area17_overlay.c` |
| 0x8243E0 | overlay | s0[31] | `em_area17_overlay.c` |
| 0x8245A0 | overlay | s0[33] | `em_area17_overlay_npc.c` |
| 0x824700 | overlay | s0[34] | `em_area17_overlay_npc.c` |
| 0x824CF0 | overlay | s0[42] | `em_area17_overlay.c` |
| 0x824E40 | overlay | s0[40] | `em_area17_overlay.c` |
| 0x8250E0 | overlay | s0[41] | `em_area17_overlay.c` |
| 0x8251B0 | overlay | group 0x827f40 [4] | `em_area17_overlay_npc.c` |
| 0x825470 | overlay | s0[38] | `em_area17_overlay.c` |
| 0x825560 | overlay | s0[5], s0[6], s0[7], s0[8] | `em_area17_overlay_npc.c` |
| 0x825EA0 | overlay | s0[36] | `em_area17_overlay_npc.c` |
| 0x8260C0 | overlay | sub0 g 0x826280 [4] | `em_area17_overlay.c` |
| 0x826170 | overlay | s0[37] | `em_area17_overlay.c` |

| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |
|---|---:|---|---|---|
| 0x826A20 | 26 | 0x823BA0 | — | — |
| 0x8270A0 | 18 | 0x823BA0 | 0x823F70, 0x824280 | — |
| 0x8275E0 | 16 | 0x824700 | — | 0B/5 |
| 0x827A40 | 15 | 0x824700 | — | 0B/5 |
| 0x827E00 | 5 | 0x824700 | — | — |
| 0x828050 | 13 | 0x824700 | — | — |
| 0x8284F0 | 9 | 0x825470 | — | — |

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| — | — | — | — | no door placement |

Boot functions reachable from this area's owners and not in the first-level census: 111 (44,932 bytes); the port defines or ledgers 102 of them; 6 more are named in the sources only (a comment, a table or another name: read each): 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130, 001F6B60; 3 appear nowhere: 001CACC0, 001D39A0, 001D3DA0.

Overlay functions: 24 (AI 1, C 22, NM 1; decomp status at the census commit); 22 have a translation in the area's own module (em_area17_overlay.c, em_area17_overlay_npc.c).
<!-- second_half:area17 end -->

### AREA18

<!-- second_half:area18 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 0012A5D0 | boot | sub0 g 0x823600 [7], sub0 g 0x823600 [8], sub0 g 0x823600 [9], sub0 g 0x823600 [10], sub0 g 0x823600 [11] | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exita.c` |
| 00147390 | boot | sub0 g 0x823600 [5], sub0 g 0x823600 [6] | `em_level16_beast.c` |
| 001551B0 | boot | s0[12], s0[13], s0[14], s0[15] | census: live (level 1) |
| 00159210 | boot | s0[21] | census: live (level 1); `em_area02_math.c` |
| 00159970 | boot | s0[19] | `em_area02_math.c` |
| 00159B90 | boot | s0[20] | census: live (level 2); `em_area01_sys.c` |
| 0015AFA0 | boot | sub0 g 0x823600 [2], sub0 g 0x823600 [3], sub0 g 0x823600 [4] | census: live (level 1) |
| 001BB860 | boot | s0[18] | census: live (level 2); `em_area01_math_owner.c` |
| 001C4820 | boot | s0[0], s0[1], s0[2], s0[3], s0[4], s0[5] (+12) | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001E3D90 | boot | s0[26], s0[27], s0[28], s0[29], s0[30], s0[31] | census: live (level 2); `em_area01_sys.c` |
| 00219550 | boot | sub0 g 0x823600 [0], sub0 g 0x823600 [1] | census: live (level 1) |

No script chain.

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| 0 | [18] | 0x1bb860 | 0 | room move: entries 2 / 1 |

Boot functions reachable from this area's owners and not in the first-level census: 196 (97,392 bytes); the port defines or ledgers 188 of them; 5 more are named in the sources only (a comment, a table or another name: read each): 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130; 3 appear nowhere: 001CACC0, 001D39A0, 001D3DA0.

Overlay functions: 1 (AI 1; decomp status at the census commit); none has a port translation.
<!-- second_half:area18 end -->

### AREA20

<!-- second_half:area20 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 0012A5D0 | boot | sub0 g 0x826100 [18], sub0 g 0x826100 [19], sub0 g 0x826100 [20], sub0 g 0x826100 [21], sub0 g 0x826100 [22], sub0 g 0x826100 [23] (+2) | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exita.c` |
| 001383C0 | boot | sub0 g 0x826100 [16] | `em_level9_port_exit.c` |
| 00147390 | boot | sub0 g 0x826100 [17] | `em_level16_beast.c` |
| 001551B0 | boot | s0[3], s0[4] | census: live (level 1) |
| 00156620 | boot | s0[5], s0[6], s0[7], s0[8], s0[9], s0[10] (+8) | census: live (level 1); `em_level9_port_exit.c` |
| 0015A2C0 | boot | s0[51], s0[52], s0[53], s0[54], s0[55], s0[56] (+1) | census: live (level 2); `em_area01_sys.c` |
| 0015AFA0 | boot | sub0 g 0x826100 [5], sub0 g 0x826100 [6], sub0 g 0x826100 [7], sub0 g 0x826100 [8], sub0 g 0x826100 [9], sub0 g 0x826100 [10] | census: live (level 1) |
| 001BC350 | boot | s0[2] | census: live (level 1) |
| 001BECC0 | boot | sub0 g 0x826100 [26], sub0 g 0x826100 [27], sub0 g 0x826100 [28], sub0 g 0x826100 [29], sub0 g 0x826100 [30] | `em_level15_fountain.c` |
| 001C1A80 | boot | s0[58], s0[59], s0[60], s0[61], s0[62], s0[63] (+2) | `em_area04_port.c` |
| 001C2420 | boot | s0[0], s0[1] | `em_level17_boot.c` |
| 001C48C0 | boot | s0[40], s0[41], s0[42], s0[43], s0[44] | `em_area02_math.c` |
| 001C4A00 | boot | s0[45], s0[46], s0[47], s0[48], s0[49], s0[50] | `em_level17_boot.c` |
| 001C5C90 | boot | s0[33] | census: live (level 1); `em_area11_roger.c`, `em_roger_actor_original.c` |
| 001E3D90 | boot | s0[34], s0[35], s0[36], s0[37], s0[38], s0[39] | census: live (level 2); `em_area01_sys.c` |
| 00219550 | boot | sub0 g 0x826100 [0], sub0 g 0x826100 [1], sub0 g 0x826100 [2], sub0 g 0x826100 [3], sub0 g 0x826100 [4] | census: live (level 1) |
| 00219870 | boot | sub0 g 0x826100 [11] | `em_area06_port_node.c` |
| 0x8239B0 | overlay | s0[32] | `em_area20_overlay.c` |
| 0x823D30 | overlay | s0[19], s0[20], s0[21], s0[22], s0[23], s0[24] (+7) | `em_area20_overlay.c` |
| 0x824440 | overlay | sub0 g 0x826100 [12], sub0 g 0x826100 [14] | missing |
| 0x825F20 | overlay | sub0 g 0x826100 [13], sub0 g 0x826100 [15] | missing |

| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |
|---|---:|---|---|---|
| 0x826830 | 25 | 0x823B00 | — | 12/0 |
| 0x8270B0 | 12 | 0x823BE0 | — | 0B/5 |

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| 0 | [2] | 0x1bc350 | 0 | area change: area 0x04 entry 5 sub kept |

Boot functions reachable from this area's owners and not in the first-level census: 262 (139,032 bytes); the port defines or ledgers 250 of them; 9 more are named in the sources only (a comment, a table or another name: read each): 001399F0, 0013B350, 001B1CE0, 001C1500, 001C1570, 001CAE30, 001CAF70, 001CB070, 001CB130; 3 appear nowhere: 001CACC0, 001D39A0, 001D3DA0.

Overlay functions: 15 (AI 1, C 14; decomp status at the census commit); 9 have a translation in the area's own module (em_area20_overlay.c).
<!-- second_half:area20 end -->

AREA20's overlay owners are all translated (lane PORT-A20,
docs/AREA20_PORT.md; the generated rows above still say "missing", as the
generator marks every overlay row so): 0x8239B0 (with 0x823B00 / 0x823BE0)
and 0x823D30 in `em_area20_overlay` (test_area20_overlay_reference, every
reachable word), plus the effect objects 0x823580, 0x823600, 0x823660,
0x823780, 0x823840; 0x824440 = AREA14's 0x823FC0 with AREA20's callees and
miss effect (`em_area19_route_00824440`), its probe 0x8259B0 and spawner
0x825E90 (`em_level10_port_008259B0`, `em_level13_port_00825E90`), and
0x825F20 = AREA03's 0x825290 word for word (`em_area03_route_00825290`).
All bound in `dispatch_second_half` for area 0x14; the area does not load
yet.

### AREA21

<!-- second_half:area21 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 00137830 | boot | group 0x82a600 [0] | `em_area21_boot.c` |
| 0014EC00 | boot | group 0x82a540 [0] | `em_area21_creature_b.c` |
| 00151590 | boot | group 0x82a5a0 [0] | `em_area21_creature_c.c` |
| 001551B0 | boot | s0[10], s0[11], s0[12], s0[13], s0[14] | census: live (level 1) |
| 00156620 | boot | s0[15], s0[16], s0[17], s0[18], s0[19], s0[20] (+6) | census: live (level 1); `em_level9_port_exit.c` |
| 00158BD0 | boot | s0[28], s0[29], s0[31], s0[32], s0[34], s0[35] | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exitb.c` |
| 00159210 | boot | s0[38] | census: live (level 1); `em_area02_math.c` |
| 00159620 | boot | s0[40] | `em_area02_math.c` |
| 00159970 | boot | s0[36], s0[39] | `em_area02_math.c` |
| 00159B90 | boot | s0[37] | census: live (level 2); `em_area01_sys.c` |
| 0015AFA0 | boot | sub0 g 0x82a380 [4], sub0 g 0x82a380 [5], sub0 g 0x82a380 [6], sub0 g 0x82a380 [8] | census: live (level 1) |
| 001C2420 | boot | s0[0], s0[1], s0[2], s0[3], s0[4], s0[5] (+4) | `em_level17_boot.c` |
| 001C4820 | boot | s0[46] | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001C48C0 | boot | s0[48] | `em_area02_math.c` |
| 001C4AF0 | boot | s0[41] | `em_area02_math.c` |
| 00219550 | boot | sub0 g 0x82a380 [0], sub0 g 0x82a380 [1], sub0 g 0x82a380 [2], sub0 g 0x82a380 [3] | census: live (level 1) |
| 0x823580 | overlay | group 0x82b500 [1], group 0x82b500 [3] | `em_area21_groups.c` |
| 0x8237B0 | overlay | group 0x82ba70 [4], group 0x82ba70 [5], group 0x82ba70 [6], group 0x82ba70 [7], group 0x82ba70 [8], group 0x82ba70 [9] (+6) | `em_area21_groups.c` |
| 0x823860 | overlay | group 0x82ba70 [16], group 0x82ba70 [17], group 0x82ba70 [18] | `em_area21_groups.c` |
| 0x8238E0 | overlay | group 0x82ba70 [19], group 0x82ba70 [20], group 0x82ba70 [21] | `em_area21_groups.c` |
| 0x823A10 | overlay | group 0x82b500 [2] | missing; twin of AREA01 0x8239C0, AREA19 0x825030 |
| 0x823AA0 | overlay | group 0x82ba70 [22] | `em_area21_groups.c` |
| 0x823AB0 | overlay | group 0x82ba70 [23] | missing; twin of AREA01 0x8239C0, AREA19 0x825030 |
| 0x8243B0 | overlay | s0[52] | `em_area21_groups.c` |
| 0x8261A0 | overlay | s0[42] | `em_area21_owners.c` |
| 0x8262D0 | overlay | s0[43], s0[44] | `em_area21_owners.c` |
| 0x8266F0 | overlay | s0[45] | `em_area21_owners.c` |
| 0x826BB0 | overlay | s0[30] | `em_area21_doors.c` |
| 0x826E60 | overlay | s0[27], s0[33] | `em_area21_doors.c` |
| 0x827040 | overlay | s0[53] | `em_area21_owners.c` |
| 0x8272C0 | overlay | s0[47] | `em_area21_doors.c` |
| 0x8273C0 | overlay | s0[49], s0[50] | `em_area21_doors.c` |
| 0x827640 | overlay | s0[51] | `em_area21_doors.c` |
| 0x827880 | overlay | s0[56] | `em_area21_doors.c` |
| 0x827A30 | overlay | s0[57] | `em_area21_doors.c` |
| 0x827CD0 | overlay | s0[58] | `em_area21_owners.c` |
| 0x827E70 | overlay | s0[59], s0[60] | `em_area21_doors.c` |
| 0x828350 | overlay | sub0 g 0x82a380 [7] | `em_area21_doors.c` |
| 0x828700 | overlay | s0[61] | `em_area21_overlay_cannon.c` |
| 0x8297C0 | overlay | s0[62] | `em_area21_overlay.c` |
| 0x82A1D0 | overlay | s0[54], s0[55] | `em_area21_doors.c` |

| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |
|---|---:|---|---|---|
| 0x82B240 | 11 | 0x8261A0 | — | — |
| 0x82B5E0 | 11 | 0x826310 | — | — |
| 0x82B8A0 | 7 | 0x8264D0 | — | — |
| 0x82BEC0 | 28 | 0x826840 | 0x8269A0, 0x8269F0 | 0A/8, 12/0, 17/0 |
| 0x82C5C0 | 17 | 0x8268F0 | — | 0A/8, 13/1 |
| 0x82CB00 | 1 | 0x826BB0, 0x826E60 | — | — |
| 0x82CBD0 | 21 | 0x827040 | 0x827250 | 0A/8, 12/0, 13/1, 1A/0 |

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| 0 | [27] | 0x826e60 | 0 | room move: entries 1 / 2 |
| 0 | [30] | 0x826bb0 | 1 | room move: entries 3 / 4 |
| 0 | [33] | 0x826e60 | 2 | room move: entries 5 / 6 |

Boot functions reachable from this area's owners and not in the first-level census: 276 (122,056 bytes); the port defines or ledgers 203 of them; 6 more are named in the sources only (a comment, a table or another name: read each): 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130, 001CEEE0; 67 appear nowhere: 0010BBF0, 0010BDB0, 0010BE50, 0010BE60, 0010C858, 0010C888, 0010C8B0, 0010C8E8, 0010C9C8, 0010C9F0, 0010CA30, 0010CA70, 0010CC08, 0010CD58, 0010D008, 0010D160, 0010D2C8, 0010D890, 00119AA0, 00119D38, 0011DB10, 0011DB58, 0011E878, 0011E938, 0011E990, 0011EBA8, 00120038, 001200E8, 00120408, 001209B0, 00120B38, 00121AF8, 00121BA0, 00121BD0, 00121E30, 00121EB8, 00121F78, 00121FB0, 001221E0, 001222E0, 00122458, 001224C0, 00122868, 00122BE8, 00122D80, 00122E68, 00122ED0, 00123620, 00124C68, 00124E18, 00125068, 00125758, 00125848, 001258E0, 00125FA8, 00126578, 00126C88, 00126EC8, 00126F20, 00126F88, 00127230, 00127500, 001275B8, 00127650, 001CACC0, 001D39A0, 001D3DA0.

Overlay functions: 49 (AI 1, AU 3, C 43, NM 2; decomp status at the census commit); 41 have a translation in the area's own module (em_area21_doors.c, em_area21_groups.c, em_area21_overlay.c, em_area21_overlay_cannon.c, em_area21_owners.c).
<!-- second_half:area21 end -->

## 5. Open

- Every overlay owner of the six areas (66 behaviours, their op09 callbacks
  and helpers) but AREA14's (section 6's twins and em_area14_overlay,
  AREA14_PORT.md), AREA16's (em_area16_overlay, AREA16_PORT.md: proven over
  designed states, no capture exists), AREA17's and AREA18's (below),
  AREA20's (all: em_area20_overlay and four twins, AREA20_PORT.md) and
  AREA21's (all: em_area21_*, [52] 0x8243B0 included; AREA21_PORT.md)
  is untranslated; the area lanes translate them against captures. The
  twins and the translated owners are bound only in the shared
  composition's dispatch; of these areas only AREA14 runs there yet.
- AREA17 and AREA18 (lane PORT-A17A18, AREA17_PORT.md, AREA18_PORT.md):
  every AREA17 overlay function but the init and the pad (22, the
  NEARMISS 0x825560 included) is translated in em_area17_overlay /
  em_area17_overlay_npc and proven by `make test-area17-overlay-reference`
  (the original over the guided run's AREA17 states g46 / g47: 1304 cases,
  1766 with EM_TEST_FULL=1, 2129 of 2129 reachable words); they bind in
  dispatch_second_half for area 0x11, with script 0x8270A0's callbacks and
  001B6660's data window. AREA18's overlay holds only the init
  (em_area01_state) and the pad: `make test-area18-overlay-reference`
  proves there is nothing else. Neither area has a SUBS row, asset export,
  start card or live run yet; the guided run's takes g41 .. g48 are their
  recordings. section 4's generator names an own-area overlay module
  (tools/second_half_inventory.py own_overlay); all blocks are regenerated.
- AREA17 live (lane PORT-LIVE2, AREA17_PORT.md "Live"): its SUBS row holds
  the guided run's g46 / g47 end states (tools/capture_from_take.py turns
  kept guided takes into capture folders under the decomp's
  build/fork_refs/guided-20261117/), with its CREATURES / FLAG_RULES /
  EXPLICIT_MODELS rules, the named orphan-unproven drums and the attachment
  bank; all kinds, profile 17, sfx scope (0x11, 0) and its stream cues are
  exported; cards g46 .. g48 load it live. Lane PORT-LIVE-A17 (branch
  live-a17): the row-310 pose is 001B07C0's spawn entry 0 on the original's
  area change, not a script op (a card's LOAD GAME places from the load
  pose); g46 drops the recording's 312 load rows, gets the rand() table
  and world clock from its own rows (comparison only), runs [30]'s skip at
  the recording's frame; 001DA6A0 returns for kind 0 as the original does,
  music cue 0xB is exported, the muzzle node binds in AREA17. g46 and g47
  now stop at the untranslated particle burst 001F1F60 (effects
  0x80000037 / 0x80000038 of the creature's spit), g48 at 001BAD40 (from
  0x8251B0) after ~12200 frames. Lane PORT-FX (branch fx): 001F1F60 and
  001F1DD0 translated (em_area00_fx_burst.c, test_area00_fx_reference
  burst / 001F1DD0 groups) and bound (six slots in em_aim_fire_trail);
  001BAD40 runs for an overlay owner's direct call (em_area01_runtime's
  rv_001BAD40, the workers 001BB0E0's 001BAD40 already uses). Now g46
  runs to pad frame ~1180, where the player has taken four spit hits and
  dies (the game returns to the title; the original's take ends at health
  35 after one item 0x22, so the hit count is not confirmed); g47 runs to
  pad frame ~2600 and stops at 001D6B10 (00133E30's haze texture, no live
  worker); g48 runs to pad frame ~18800 and stops at the player closure
  0016BC40 (code 5 at 0x810835). Lane PORT-WORKERS (branch workers,
  AREA17_PORT.md "Lane PORT-WORKERS"): 001D6B10 and the haze's other
  channel-3 calls are live (em_area01_rcl_workers over the em_rcl
  translations; the haze list is filed by depth, its drawing not modelled:
  the chain page refuses it with rendering on); D_00810835 is mapped in
  the player closure. g47 now runs its whole pad with no fault (its card's
  LOAD GAME runs [30]'s intro, which the take never did, so the end state
  differs); g48 runs to pad ~19590 (001BB0E0 touches the freed record 39);
  g46 is unchanged: the original took three hits there too (threats.json,
  now trace.json `vitals` via capture_from_take), the port's first comes
  12 frames early and a second follows, after the rand() draws diverge
  from script frame 128 (the card's shorter op07 sub 0xC wait). No beat
  passes yet.
- Continuous chains (lane PORT-CHAIN, `tools/guided_chain.py`; AREA17_PORT.md
  and AREA16_PORT.md "Lane PORT-CHAIN"): the guided takes play as one run
  from one card with no reload, every frame compared with the takes' rows.
  g17 (card g45, AREA14, then a real area change into AREA17): g46 is
  row-exact (1967 of 1967 frames, its three hits at the original's frames,
  the heal) once the recorded drive timing is on: the 17-frame op07 sub
  0xC wait is disc timing, not a card difference. [6]'s ride 0x825C40 no
  longer fails the player view (00183010's w-lane adds). g45 leaves the
  rows at 248 from the card's effect timers (the beasts' draws); g47 and g48 (to
  row 19587) were later made row-exact. Lane
  PORT-A17-NEXT (AREA17_PORT.md): g46, g47 and g48 are on the row to g48's
  area change at row 19587 (g47's hit at the original's row 663, Roger's
  heal; g48's cable ride): the chain's clock now follows the engine's frames
  across a status page's hookless frames and an end-of-frame step's pad is
  read a frame later (EM_START_AT_FRAMES; it was the page-close offset),
  capture_from_take places hits one row later (threats.json's c is the
  snapshot before the loss) and keeps each input's row (g48's first stick),
  a script actor 001BB0E0 reads only its freed owner's +0x2E (g48's record
  39), and EM_START_AT_STATUS_BG restores the pages' background D_002655A0.
  The run stops in the AREA11 entry-3 load (001FFCD0 code 2). g16
  (card g61): since lane PORT-A16-DRAW (001CAE30, below) it plays to chain
  frame 11186 of 37219: g61 on the recorded position for all 2788 rows
  (its three melees break the curtains, so g62's card question is gone in
  the chain), but rand() differs from row 4 and two hits the original did
  not take (rows 2345, 2457); g62 stays in sub 0 at row 4336; the player
  dies in g63. A start before AREA16 (card g60, AREA06 sub 1) needs
  AREA06 sub 1 exported (assets and the module pack's room (0x06, 1)). AREA16's owners now bind in the live composition too (area 0x10:
  dispatch_second_half, scene bindings, 001B6660's window, op09 callbacks).
- AREA16 live (lane PORT-LIVE-A16, AREA16_PORT.md "Live"): SUBS rows for
  subs 0 and 1 from the guided run's g60 .. g66 (route_g16), CREATURES
  0x8273F0 with its node + 0x118 matrix rule (MATRIX_RULES), FLAG_RULES
  from 0x825800's C, two explicit models; all kinds, profile 16, sfx
  scopes (0x10, 0) / (0x10, 1) and its stream cues are exported; cards and
  beats g61 .. g67. g63 passes (end position and progress block); g66
  returns to the title. g64 passed 001BAD40 (lane PORT-FX) and, since
  lane PORT-WORKERS, 001D8060 (em_rcl_001D8060) and the 9-bone bind of
  0x824E10's attachment child (indicator slot capacity); it stops on the
  same frame (pad ~2900) at the attachment follow 00102958 into the
  child's +0x12C slot, which the indicator bind does not expose.
- AREA16's water grid (lane PORT-WATER, AREA16_PORT.md "Water grid"):
  001DAFA0, 001DB240, 001DB250, 001DB040, 001DB480 (with 001DAE10 /
  001DAC90) in `em_area16_water`, bound through the fallback dispatch, the
  grid block 0x818000 .. 0x81C03F mapped live. g62 plays in sub 0 to pad
  frame 3042 with no fault, then returns to the title (engine frame 8105)
  before sub 1: a death from its card, not a translation (lane
  PORT-A16FX, AREA16_PORT.md "Live": the card respawns the three curtains
  0x1C1030 the guided run broke by melee in g61, the recorded pad walks
  into one and the bugs bite).
- AREA16's effects (lane PORT-A16FX, AREA16_PORT.md "Effects"): the five
  effect behaviours of ids 0..4 (0x8237A0 .. 0x824110) in
  em_area16_overlay_f, proven by test_area16_overlay_reference (2018 cases
  quick, 2193 full) and bound; 001DB480's render callees run live
  (em_rcl_001D6B10 / em_rcl_001D6C90, 001CAAC0's chain insertion). g61 now
  passes [13]'s effect 4 and the grid's display list. Lane PORT-A16-DRAW
  (AREA16_PORT.md "Lane PORT-A16-DRAW"): [10]'s +0x4C draw method 001CAE30
  (model kind 3: 001CACC0, 001D39A0 / 001D3DA0, a class-3 unit: set 2
  class 3, the subtractive blend) translated, proven by
  test_owner_draw_reference F and test_object_unit_reference K3 (original
  code and microcode over g60 / g61) and bound; beat g61 runs its whole
  pad (end position good; D_0081085A differs).
- Every boot behaviour of the six areas is now in a census ledger or
  defined (section 6 for the six BOOT2 added); those marked only "defined"
  by an earlier lane still need their oracle named before they count. The
  boot functions still listed as appearing nowhere (section 4's per-area
  lines) are reached by overlay owners or shared draw/effect paths, not by
  the boot behaviours.
- The flagged op/sub pairs (0x0A sub 8, 0x0B sub 5, 0x12 sub 0, 0x13 subs 0
  and 1, 0x17 sub 0, 0x1A subs 0 and 1) are not checked against the
  handlers here.
- AREA14 (lane P14, docs/AREA14_PORT.md): its SUBS row holds the k01
  recordings (k01_02 .. k01_05), its capture kinds, profile 14 and its sfx
  scope are exported, and k01_02 loads it live from AREA00 sub 2; its group
  owner 0x823FC0 and the probe / spawner 0x825530 / 0x825A10 run (lane
  P14b: AREA19's translations with AREA14's differences as parameters,
  section 6); lane P14c translated the rest of AREA14's owners and the
  camera arm 00230230 (em_area14_overlay, test_area14_overlay_reference) and
  the beats k01_02 .. k01_05 pass from their cards (AREA14_PORT.md). The capture
  kinds of AREA18 and AREA17 wait for their SUBS rows: the guided run
  recorded both (g41 .. g48); AREA16's takes g59 .. g67 are its recording
  (its bullet above); AREA21 has none yet. AREA20's owners (lane PORT-A20) are proven by their oracles only:
  the guided run's g73 .. g75 AREA20 tries are not kept recordings (the
  chain was forked at g69), so its SUBS row still waits.
- AREA21 (lane PORT-A21, docs/AREA21_PORT.md): every overlay owner the
  placements, groups, scripts and spawns name, [52] 0x8243B0 included
  (lane PORT-A21-52, NEARMISS C), is translated from the decomp C into em_area21_overlay (the Gatling
  [62] 0x8297C0, the cannon [61] 0x828700 and its projectile 0x828F30),
  em_area21_owners (counters, scripts, the ending request [53]),
  em_area21_doors (the doors, [54] / [55], the rounds 0x825C70, the
  effects) and em_area21_groups (the group members); `make
  test-area21-overlay-reference` runs the original instructions of each
  (AREA21.BIN over capture a03b_03_item2a, designed states: no AREA21
  recording exists) with every reachable word. They are bound in
  dispatch_second_half and em_scene_bindings for area 0x15, but AREA21
  has not loaded live: its SUBS row, capture kinds and 001A2370 /
  0019C6F0 rules (section 2) wait for a recording. Section 4's blocks list them by module.
- No other of these areas has loaded live: their module pack rows, state and
  script windows are checked only by the oracle and the static export.

## 6. Boot translations (lane BOOT2)

Each module translates byte-matched decomp C (NEARMISS C where noted; the
oracle runs the original instructions, so it decides) and has its own
oracle on the shared harness (tools/test_level14_port_reference.py):
the original code runs from the user's ELF over capture a03b_03_item2a
(its AREA03 text and boot text checked against the user's files) with
designed actor records in zero RAM; every callee outside the module is a
hook with a scripted result (the vector and float helpers run as original
code, their writes replayed). Compared per case: calls and arguments,
memory at every call, the memory accesses between calls one for one in
order, memory after the last store, a poisoned re-run, every reachable
original word executed, the callee inventory and the fail-stop contract.
All are bound through `em_area01_runtime_later.inc` (tools/gen_later_dispatch.py)
with hook adapters in `em_mid_route_bridge.inc` (tools/gen_mid_route_bridge.py).

| Module | Functions | Oracle (quick / EM_TEST_FULL=1) |
|---|---|---|
| `em_level17_boot` | 001C2420 (returns at once), 001C4A00 (001C48C0's states; state 1 gated by the scratchpad flag 0x70003B92 and D_008107D8[+3] against +0x56); AREA21's twins below; 001C6190 (001C6160(node) less 001281C0 of the float at +0x3C; AREA21's 0x823580 and 0x8237B0, and AREA20) | `make test-level17-boot-reference`: 384 cases, 104/104 words |
| `em_area17_creature` | 00133E30 and 25 functions it reaches: 00133FD0 .. 001373B0, 001E00F0, 001E0240, 001E04E0 (NEARMISS C: 00134300, 001348E0, 00135D00, 001370C0, 001373B0, 001E00F0) | `make test-area17-creature-reference`: 889 / 1740 cases, every word but one dead delay slot (0x136F60: 00136F20's out-of-range selector, which its one caller never passes; the native faults there) |
| `em_area21_boot` | 001283D0, 0014C220 .. 0014D1E0 (12 shared helpers), 00137830 with 00137920 .. 001381D0 (00137EB0 NEARMISS) | `make test-area21-boot-reference`: 648 / 1351 cases, every word |
| `em_area21_creature_b` | 0014EC00 and 17 functions it reaches, 0014ED10 .. 001513B0 (00150F40 NEARMISS) | `make test-area21-creature-b-reference`: 1030 / 1696 cases, every word |
| `em_area21_creature_c` | 00151590 and 17 functions it reaches, 001516B0 .. 00153770 | `make test-area21-creature-c-reference`: 621 / 1287 cases, every word |
| `em_area16_water` (lane PORT-WATER) | AREA16 sub 0's water grid: 001DAFA0, 001DB240, 001DB250, 001DB040 (NEARMISS C), 001DB480 with 001DAE10 and 001DAC90 (NEARMISS C) nested; over Claude's guided-run captures g60 / g61 (route_g16), not a03b_03_item2a | `make test-area16-water-reference`: 44 / 81 cases, every word (001DB480's fail-stop contract under EM_TEST_FULL=1) |

**Twins** (section 4's "twin of" rows), each compared word for word with the
relocated fields unmasked, then bound in the shared composition
(`em_area01_runtime_second_half.inc`, `dispatch_second_half`, areas 0x0E
and 0x15 only):

| Owner | Twin of | Words | Translation and oracle |
|---|---|---|---|
| AREA14 0x825AA0 (sub 0 group 0x826180 [14], [16], [18], [20]) | AREA03 0x825290, AREA11 0x827490, AREA19 0x8298D0 | all 104 equal, no overlay-relocated word | `em_area03_route_00825290`; `make test-area03-route-reference` runs every 0x825290 case again on AREA14's header and text loaded over the same images (72/72 words) |
| AREA21 0x823A10 (group 0x82B500 [2]), 0x823AB0 (group 0x82BA70 [23]) | AREA01 0x8239C0, AREA19 0x825030 | 35 of 36 equal; the one that differs is the rate's float immediate: 40.0 and 8.0 (AREA01's and AREA19's 5.0), so AREA01's translation (which fixes 5.0) is not reused | `em_level17_boot_a21_00823A10` / `_00823AB0`; `make test-level17-boot-reference` runs them on AREA21's header and text over the capture (27/27 words each; a 5.0 mutation fails) |
| AREA14 0x823FC0 (sub 0 group 0x826180 [13], [15], [17], [19]) with 0x825530 and 0x825A10 (lane P14b) | AREA19 0x827DD0, 0x829370, 0x829840 | aligned word for word but the owner's three overlay calls and its missed-probe effect (kind 5: 0x8000002C, else 0x80000003), the probe's tint record (0x828100) and register-only set-up of its 001CD520 call, the spawner's colour (0x828110) | `em_area19_route_00823FC0`, `em_level10_port_00825530`, `em_level13_port_00825A10`; test_area19_route_reference / test_level10_port_reference / test_level13_port_reference rerun the AREA19 cases on AREA14's overlay, plus the 16 captured nodes of k01_02 .. k01_05 (docs/AREA14_PORT.md) |

Section 4's tables list an overlay owner by the port module of its own area
(tools/second_half_inventory.py own_overlay).

**The rest of the reachable boot functions** (section 4's lines for AREA17
and AREA21), by the owners that reach them:
- 001F6B60 (AREA17's [30] 0x823BA0) is 001F6B30 word for word (the test
  checks the ELF): the host binds it to em_effects_live_001F6B30.
- 001CACC0, 001D39A0, 001D3DA0 are reached from every area's owners,
  the first level's included, only through pointers (no direct call path
  in the census): stopped here, as speculative.
- AREA21's 0010BBF0 .. 00127650 (lowmem, SDK): 001FBC50 (0x827040, the
  ending request) reaches 00119AA0, the sound library, and 0x828700 /
  0x8297C0 call 0010BBF0 (sceSifSetDma); the others 0x827250 reaches, only through
  pointers. The port's sound layer is native, not a translation of the SDK;
  whether any of them must be translated is the AREA21 lane's question with
  a capture of the ending.
- 001CEEE0 (gs upload) is reached only from 0x825C70, which nothing names
  statically: not translated.

Notes for the area lanes:
- The creature modules call 001283D0 and 0014D1E0 (em_area21_boot) and
  each other's boot callees through the runtime, by address.
- Hooks whose port signatures differ between headers were taken as the
  oracle checked them at the original's call sites: 0011E620 (two floats
  in, a float out), 001DA6A0 (a plain word: 00134090 passes the word at
  0x275B44), 0011E0A8 (a0 the integral part's address, f12 the value, a
  float result).
- The names in the modules' comments ("spit", "haze", "lunge") describe
  what the code does; they are not verified game semantics.
- No capture of these areas exists yet: the oracles prove the translations
  against the original instructions, not the creatures' behaviour in a run.
