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
| AREA16 | 0, 1 | 18 + 73 | 24 / 14 | 23 of 24 | 10 | 7 |
| AREA17 | 0 | 43 | 9 / 15 | 7 of 9 | 7 | 0 |
| AREA18 | 0 | 32 | 11 / 0 | 11 of 11 | 0 | 1 |
| AREA20 | 0 | 66 | 17 / 4 | 15 of 17 | 2 | 1 |
| AREA21 | 0 | 63 | 16 / 25 | 12 of 16 | 7 | 3 |
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
| 0x823580 | overlay | s0[35], s0[36] | missing |
| 0x823760 | overlay | s0[5] | missing |
| 0x8238E0 | overlay | s0[0] | missing |
| 0x823B30 | overlay | s0[2] | missing |
| 0x823E30 | overlay | s0[4] | missing |
| 0x823FC0 | overlay | sub0 g 0x826180 [13], sub0 g 0x826180 [15], sub0 g 0x826180 [17], sub0 g 0x826180 [19] | missing |
| 0x825AA0 | overlay | sub0 g 0x826180 [14], sub0 g 0x826180 [16], sub0 g 0x826180 [18], sub0 g 0x826180 [20] | missing; twin of AREA03 0x825290, AREA11 0x827490, AREA19 0x8298D0 |
| 0x825FD0 | overlay | s0[6] | missing |

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

Boot functions reachable from this area's owners and not in the first-level census: 144 (66,280 bytes); the port defines or ledgers 134 of them; 6 more are named in the sources only (a comment, a table or another name: read each): 00183160, 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130; 4 appear nowhere: 001CACC0, 001D39A0, 001D3DA0, 001F6B00.

Overlay functions: 14 (AI 1, C 12, NM 1; decomp status at the census commit); none has a port translation.
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
| 001C2420 | boot | s0[0], s0[12], s1[12] | **missing** |
| 001C4820 | boot | s0[6], s1[35], s1[36], s1[37], s1[38], s1[40] (+15) | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001C4AF0 | boot | s1[8] | `em_area02_math.c` |
| 00219550 | boot | sub0 g 0x828480 [0], sub0 g 0x828480 [1], sub1 g 0x8287d0 [0], sub1 g 0x8287d0 [1], sub1 g 0x8287d0 [2], sub1 g 0x8287d0 [3] (+6) | census: live (level 1) |
| 0022DCD0 | boot | s1[9] | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exita.c` |
| 0x823580 | overlay | group 0x829750 [4], group 0x829750 [5], group 0x829750 [6], group 0x829750 [7], group 0x829750 [8] | missing |
| 0x824440 | overlay | s1[39] | missing |
| 0x824690 | overlay | s1[11] | missing |
| 0x824E10 | overlay | group 0x829660 [1], group 0x829750 [3] | missing |
| 0x825310 | overlay | s1[10] | missing |
| 0x825800 | overlay | s0[10] | missing |
| 0x8261A0 | overlay | s0[11] | missing |
| 0x8263D0 | overlay | s0[13] | missing |
| 0x826C50 | overlay | s1[44], s1[45], s1[46] | missing |
| 0x827030 | overlay | s1[42], s1[43] | missing |
| 0x8273F0 | overlay | s0[9], s1[13], s1[14], s1[15], s1[16], s1[17] (+10) | missing |
| 0x827B70 | overlay | s1[41] | missing |
| 0x827CE0 | overlay | s1[28], s1[29] | missing |
| 0x827FA0 | overlay | s1[30], s1[31], s1[32], s1[33], s1[34] | missing |

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

Boot functions reachable from this area's owners and not in the first-level census: 273 (145,244 bytes); the port defines or ledgers 255 of them; 7 more are named in the sources only (a comment, a table or another name: read each): 001B1CE0, 001C1500, 001C1570, 001CAE30, 001CAF70, 001CB070, 001CB130; 11 appear nowhere: 001C2420, 001CACC0, 001D39A0, 001D3DA0, 001DAC90, 001DAE10, 001DAFA0, 001DB040, 001DB240, 001DB250, 001DB480.

Overlay functions: 30 (AI 1, C 25, NM 4; decomp status at the census commit); none has a port translation.
<!-- second_half:area16 end -->

### AREA17

<!-- second_half:area17 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 00133E30 | boot | group 0x8263c0 [0] | **missing** |
| 001551B0 | boot | s0[4] | census: live (level 1) |
| 00156620 | boot | s0[9], s0[10], s0[11], s0[12], s0[13], s0[14] (+15) | census: live (level 1); `em_level9_port_exit.c` |
| 0015AFA0 | boot | sub0 g 0x826280 [1], sub0 g 0x826280 [2], sub0 g 0x826280 [3], sub0 g 0x826280 [5], group 0x826500 [0], group 0x826560 [0] | census: live (level 1) |
| 001C2420 | boot | s0[0], s0[1], s0[2], s0[3] | **missing** |
| 001C4820 | boot | s0[32] | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001C5C90 | boot | s0[35] | census: live (level 1); `em_area11_roger.c`, `em_roger_actor_original.c` |
| 001E3D90 | boot | group 0x826420 [0], group 0x826420 [1], group 0x826420 [2], group 0x826420 [3] | census: live (level 2); `em_area01_sys.c` |
| 00219550 | boot | sub0 g 0x826280 [0] | census: live (level 1) |
| 0x8239D0 | overlay | group 0x826890 [4], group 0x826890 [5], group 0x826890 [6], group 0x826890 [7] | missing |
| 0x823BA0 | overlay | s0[30] | missing |
| 0x8242D0 | overlay | s0[39] | missing |
| 0x8243E0 | overlay | s0[31] | missing |
| 0x8245A0 | overlay | s0[33] | missing |
| 0x824700 | overlay | s0[34] | missing |
| 0x824CF0 | overlay | s0[42] | missing |
| 0x824E40 | overlay | s0[40] | missing |
| 0x8250E0 | overlay | s0[41] | missing |
| 0x8251B0 | overlay | group 0x827f40 [4] | missing |
| 0x825470 | overlay | s0[38] | missing |
| 0x825560 | overlay | s0[5], s0[6], s0[7], s0[8] | missing |
| 0x825EA0 | overlay | s0[36] | missing |
| 0x8260C0 | overlay | sub0 g 0x826280 [4] | missing |
| 0x826170 | overlay | s0[37] | missing |

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

Boot functions reachable from this area's owners and not in the first-level census: 111 (44,932 bytes); the port defines or ledgers 75 of them; 6 more are named in the sources only (a comment, a table or another name: read each): 00136630, 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130; 30 appear nowhere: 00133E30, 00133FD0, 00134090, 00134300, 00134500, 001348E0, 00134DC0, 00134FF0, 001352E0, 00135870, 00135D00, 00136140, 00136240, 00136490, 00136600, 001366E0, 001368D0, 00136C40, 00136CB0, 00136F20, 001370C0, 001373B0, 001C2420, 001CACC0, 001D39A0, 001D3DA0, 001E00F0, 001E0240, 001E04E0, 001F6B60.

Overlay functions: 24 (AI 1, C 22, NM 1; decomp status at the census commit); none has a port translation.
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
| 001C2420 | boot | s0[0], s0[1] | **missing** |
| 001C48C0 | boot | s0[40], s0[41], s0[42], s0[43], s0[44] | `em_area02_math.c` |
| 001C4A00 | boot | s0[45], s0[46], s0[47], s0[48], s0[49], s0[50] | **missing** |
| 001C5C90 | boot | s0[33] | census: live (level 1); `em_area11_roger.c`, `em_roger_actor_original.c` |
| 001E3D90 | boot | s0[34], s0[35], s0[36], s0[37], s0[38], s0[39] | census: live (level 2); `em_area01_sys.c` |
| 00219550 | boot | sub0 g 0x826100 [0], sub0 g 0x826100 [1], sub0 g 0x826100 [2], sub0 g 0x826100 [3], sub0 g 0x826100 [4] | census: live (level 1) |
| 00219870 | boot | sub0 g 0x826100 [11] | `em_area06_port_node.c` |
| 0x8239B0 | overlay | s0[32] | missing |
| 0x823D30 | overlay | s0[19], s0[20], s0[21], s0[22], s0[23], s0[24] (+7) | missing |
| 0x824440 | overlay | sub0 g 0x826100 [12], sub0 g 0x826100 [14] | missing |
| 0x825F20 | overlay | sub0 g 0x826100 [13], sub0 g 0x826100 [15] | missing |

| Chain | Records | Started by | op09 callbacks | Op/sub pairs the census snapshot flags |
|---|---:|---|---|---|
| 0x826830 | 25 | 0x823B00 | — | 12/0 |
| 0x8270B0 | 12 | 0x823BE0 | — | 0B/5 |

| Sub | Placement | Behaviour | Door id | Meaning |
|---|---|---|---:|---|
| 0 | [2] | 0x1bc350 | 0 | area change: area 0x04 entry 5 sub kept |

Boot functions reachable from this area's owners and not in the first-level census: 262 (139,032 bytes); the port defines or ledgers 247 of them; 9 more are named in the sources only (a comment, a table or another name: read each): 001399F0, 0013B350, 001B1CE0, 001C1500, 001C1570, 001CAE30, 001CAF70, 001CB070, 001CB130; 6 appear nowhere: 001C2420, 001C4A00, 001C6190, 001CACC0, 001D39A0, 001D3DA0.

Overlay functions: 15 (AI 1, C 14; decomp status at the census commit); none has a port translation.
<!-- second_half:area20 end -->

### AREA21

<!-- second_half:area21 begin -->
| Behaviour | Kind | Used by | Port |
|---|---|---|---|
| 00137830 | boot | group 0x82a600 [0] | **missing** |
| 0014EC00 | boot | group 0x82a540 [0] | **missing** |
| 00151590 | boot | group 0x82a5a0 [0] | **missing** |
| 001551B0 | boot | s0[10], s0[11], s0[12], s0[13], s0[14] | census: live (level 1) |
| 00156620 | boot | s0[15], s0[16], s0[17], s0[18], s0[19], s0[20] (+6) | census: live (level 1); `em_level9_port_exit.c` |
| 00158BD0 | boot | s0[28], s0[29], s0[31], s0[32], s0[34], s0[35] | census: verified-unbound (level 2), verified-unbound (level 3); `em_area01_exitb.c` |
| 00159210 | boot | s0[38] | census: live (level 1); `em_area02_math.c` |
| 00159620 | boot | s0[40] | `em_area02_math.c` |
| 00159970 | boot | s0[36], s0[39] | `em_area02_math.c` |
| 00159B90 | boot | s0[37] | census: live (level 2); `em_area01_sys.c` |
| 0015AFA0 | boot | sub0 g 0x82a380 [4], sub0 g 0x82a380 [5], sub0 g 0x82a380 [6], sub0 g 0x82a380 [8] | census: live (level 1) |
| 001C2420 | boot | s0[0], s0[1], s0[2], s0[3], s0[4], s0[5] (+4) | **missing** |
| 001C4820 | boot | s0[46] | census: live (level 1); `em_area11_bindings.c`, `em_status_ui_leftovers.c` |
| 001C48C0 | boot | s0[48] | `em_area02_math.c` |
| 001C4AF0 | boot | s0[41] | `em_area02_math.c` |
| 00219550 | boot | sub0 g 0x82a380 [0], sub0 g 0x82a380 [1], sub0 g 0x82a380 [2], sub0 g 0x82a380 [3] | census: live (level 1) |
| 0x823580 | overlay | group 0x82b500 [1], group 0x82b500 [3] | missing |
| 0x8237B0 | overlay | group 0x82ba70 [4], group 0x82ba70 [5], group 0x82ba70 [6], group 0x82ba70 [7], group 0x82ba70 [8], group 0x82ba70 [9] (+6) | missing |
| 0x823860 | overlay | group 0x82ba70 [16], group 0x82ba70 [17], group 0x82ba70 [18] | missing |
| 0x8238E0 | overlay | group 0x82ba70 [19], group 0x82ba70 [20], group 0x82ba70 [21] | missing |
| 0x823A10 | overlay | group 0x82b500 [2] | missing; twin of AREA01 0x8239C0, AREA19 0x825030 |
| 0x823AA0 | overlay | group 0x82ba70 [22] | missing |
| 0x823AB0 | overlay | group 0x82ba70 [23] | missing; twin of AREA01 0x8239C0, AREA19 0x825030 |
| 0x8243B0 | overlay | s0[52] | missing |
| 0x8261A0 | overlay | s0[42] | missing |
| 0x8262D0 | overlay | s0[43], s0[44] | missing |
| 0x8266F0 | overlay | s0[45] | missing |
| 0x826BB0 | overlay | s0[30] | missing |
| 0x826E60 | overlay | s0[27], s0[33] | missing |
| 0x827040 | overlay | s0[53] | missing |
| 0x8272C0 | overlay | s0[47] | missing |
| 0x8273C0 | overlay | s0[49], s0[50] | missing |
| 0x827640 | overlay | s0[51] | missing |
| 0x827880 | overlay | s0[56] | missing |
| 0x827A30 | overlay | s0[57] | missing |
| 0x827CD0 | overlay | s0[58] | missing |
| 0x827E70 | overlay | s0[59], s0[60] | missing |
| 0x828350 | overlay | sub0 g 0x82a380 [7] | missing |
| 0x828700 | overlay | s0[61] | missing |
| 0x8297C0 | overlay | s0[62] | missing |
| 0x82A1D0 | overlay | s0[54], s0[55] | missing |

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

Boot functions reachable from this area's owners and not in the first-level census: 276 (122,056 bytes); the port defines or ledgers 146 of them; 5 more are named in the sources only (a comment, a table or another name: read each): 001B1CE0, 001CAE30, 001CAF70, 001CB070, 001CB130; 125 appear nowhere: 0010BBF0, 0010BDB0, 0010BE50, 0010BE60, 0010C858, 0010C888, 0010C8B0, 0010C8E8, 0010C9C8, 0010C9F0, 0010CA30, 0010CA70, 0010CC08, 0010CD58, 0010D008, 0010D160, 0010D2C8, 0010D890, 00119AA0, 00119D38, 0011DB10, 0011DB58, 0011E878, 0011E938, 0011E990, 0011EBA8, 00120038, 001200E8, 00120408, 001209B0, 00120B38, 00121AF8, 00121BA0, 00121BD0, 00121E30, 00121EB8, 00121F78, 00121FB0, 001221E0, 001222E0, 00122458, 001224C0, 00122868, 00122BE8, 00122D80, 00122E68, 00122ED0, 00123620, 00124C68, 00124E18, 00125068, 00125758, 00125848, 001258E0, 00125FA8, 00126578, 00126C88, 00126EC8, 00126F20, 00126F88, 00127230, 00127500, 001275B8, 00127650, 001283D0, 00137830, 00137920, 00137A50, 00137C80, 00137E80, 00137EB0, 001381D0, 0014C220, 0014C2E0, 0014C3A0, 0014C490, 0014C680, 0014C9D0, 0014CCC0, 0014CD00, 0014CD30, 0014CDD0, 0014D1E0, 0014EC00, 0014ED10, 0014EDD0, 0014EEF0, 0014F390, 0014FBC0, 00150190, 00150260, 001504D0, 00150600, 00150820, 00150850, 00150900, 001509A0, 00150DA0, 00150F40, 00151200, 001513B0, 00151590, 001516B0, 00151770, 00151940, 00151E50, 00152040, 00152330, 00152660, 00152930, 00152B60, 00152CC0, 00152E80, 00152EB0, 00152F20, 00153180, 00153290, 00153540, 00153770, 001C2420, 001C6190, 001CACC0, 001CEEE0, 001D39A0, 001D3DA0.

Overlay functions: 49 (AI 1, AU 3, C 43, NM 2; decomp status at the census commit); none has a port translation.
<!-- second_half:area21 end -->

## 5. Open

- Every overlay owner of the six areas (66 behaviours, their op09 callbacks
  and helpers) is untranslated; the area lanes translate them against
  captures, the twins first.
- Six boot behaviours appear nowhere in the port's sources (not even in a
  comment): 001C2420 (AREA16, AREA17, AREA20, AREA21; the most used: 3 + 4 + 2
  + 10 placements), 001C4A00 (AREA20, six placements), 00133E30 (AREA17's code
  group 0x8263C0), 00137830, 0014EC00 and 00151590 (AREA21's code groups).
  The other boot behaviours are in a census ledger or defined; those marked
  only "defined" still need their oracle named before they count.
- The flagged op/sub pairs (0x0A sub 8, 0x0B sub 5, 0x12 sub 0, 0x13 subs 0
  and 1, 0x17 sub 0, 0x1A subs 0 and 1) are not checked against the
  handlers here.
- The capture kinds of the exporter, the texture and receiver profiles, and
  the start cards wait for CAP2 (AREA14, AREA18, AREA17 first); AREA16, AREA20
  and AREA21 have no recording planned yet.
- No live run has loaded any of these areas: the module pack, the state and
  the script windows are checked only by the oracle and the static export.
