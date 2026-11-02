# Toward the second half, from AREA04 sub 1: eighteenth-level route, original ground truth

Lane CAP2 (capture), 2026-11-02. Original executable SHA-256
`ee052236783e7d3e865754d3ff9fee71290addeb7d146c86caa7ff2724d1e17a`.

`SEVENTEENTH_LEVEL_ROUTE.md` stopped at a07b_07: AREA04 sub 1 entry 10 (the AREA07 lift's arrival),
health 55, infection 85. The task was the second half's first segment as WORLD_GRAPH.md section 7
step 7 derives it: AREA00 sub 2, item 0x26, [28] into AREA14, AREA18 (item 0x28), AREA17, and
AREA17's [38] to AREA11 entry 3. This document records what the code requires before that segment
can start, the four beats played toward it, and where the recording stops. Beat groups `a04e` and
`a02c` of `../Extermination/tools/route_capture.py`. Hidden PCSX2 fork, exact one-frame steps, pad
input only, no teleports and no memory writes (the recorder rejects a write), one trace and one
resumable snapshot per beat. Nothing here is port behaviour.

In-game text is described, never quoted. Positions are world units; yaw is the player's +0xC4
(X = sin, Z = cos). Frames `f` are trace frames. Actor indices `[n]` are the registry's placement
indices of the area and sub named (`tools/route_census.py graph`).

**Status.**

- **The segment as derived cannot start from a07b_07 without a detour.** Item 0x26 (AREA00 sub 2
  g[3]) lies in the room behind AREA00's slider [20], which opens only with D_00810841 bit 4. That
  bit comes only from the reader [19] (00158810, model 0x13), and 001576E0 (BM) accepts model 0x13
  only with item 0x24 held (D_00810C88). Item 0x24 comes only from AREA19 sub 1 [53] (0x827550, C):
  with flag 0x25 = 0xFF and flag 0x26 clear, the player at y 445..460 inside the polygon 0x82F820
  (the stair tower's y-450 landing) starts script 0x82D590, and at its end 001C47A0(0x24, 1). The
  a07b_07 player holds no item 0x24 (C64 0x24 = 0; D_00810841 = 0x04).
- **From AREA04 sub 1's hall, AREA19 is not reached the way the earlier levels went** (section 2):
  the conveyor to the east balcony (the lift [57] to AREA13, door [43] to AREA22) is closed in sub 1,
  and AREA03's door [44] stands on the west balcony. The hall's ways out are the AREA07 lift, the
  slider [41] to AREA02 sub 2, and the west balcony's platform [68] (console [69]), found and run
  once in a side beat but not ridden.
- **Recorded:** the hall to the slider [41] (a04e_00, a04e_01), AREA02 sub 2 from entry 4 across
  the channel, the seal [21] broken from the AREA02 side and door [22] to AREA01 entry 3 (a02c_00,
  a02c_01). Side beat a04e_s0_console: the console [69] moved the platform [68].
- **Not entered**: AREA00 sub 2, AREA14, AREA17, AREA18, AREA19 sub 1's tower.
- Output: `../Extermination/build/fork_refs/levels-recordings-20261102-1/` (`s87/route_a04e/`,
  `route_a02c/`; the run log per beat, `progress.json`, `owner-bindings.json` at the top). Earlier
  sets were not touched; the chain starts from `levels-recordings-20261031-1`'s a07b_07.

## 1. Tool and conventions

```sh
# macOS arm64 host (the fork runs x86_64 under Rosetta); from the decomp repo root, decomp .venv.
.venv/bin/python ../port-cap2/tools/record_level_routes.py \
    --out-root build/fork_refs/levels-recordings-20261102-1 --append \
    --source-root build/fork_refs/levels-recordings-20261031-1 --beats a02c_00_seal
```

- Beats: `EIGHTEENTH_BEATS` in route_capture.py (`A04E_BEATS`, `A02C_BEATS`), groups in
  `EIGHTEENTH_GROUPS` (added to `SIXTEENTH_GROUPS`, so each writes `s87/route_<group>/<beat>/`) and
  in route_lanes.py's `LEVEL_ORDER`. The side beat `a04e_s0_console` runs only by name.
- Rows: `use_sixteenth_sampler`; a02c_00 also samples AREA02's creatures and the pod
  (`A02C_OWNERS`: `drainA..C`, `pod`, `e1546a..d`; the `k` span holds +0x34).
- Helpers: `l18_use`, `l18_door`, `l18_lock`, `l18_melee_until`, `l18_shoot`, `l18_owner_hp`.
- Port side: chains `a04e`, `a02c` in `tools/mid_route_beats.py`; cards `a04e_00`, `a04e_01`,
  `a02c_00`, `a02c_01` in `tools/build_start_saves.py`. All four cards were built into the port's
  `build/start_saves/` and passed the tool's save and read-back checks. Per-row rand() tables:
  `tools/record_beat_rands.py a04e_00 a04e_01 a02c_00 a02c_01` wrote `build/beat_rands/<card>.txt`
  (the replays matched the traces: largest position difference at the 50-row samples 0.102 in
  a04e_00, 0.000 in the others). `tools/record_rand_replay.py` has no entry for these area
  changes (its BEATS table names the earlier entries only); not run.
- `mid_route_beats.py --single` was not run: this worktree has no port build, and no port lane has
  shown AREA02 sub 2 or AREA01 sub 1 loading yet.
- Scratch readers (not committed): the grid read of a snapshot's `eeMemory.bin` through
  D_0028A598[0] (export_collision.decode_grid), height and wall maps, and a live-actor list from
  0x275BC0. They are leads; every position below that a beat depends on was walked.

## 2. What decides each step

**AREA04 sub 1's hall (read from the grid of a04e_00's snapshot, then walked).**

- The conveyor (x 406 -> 500, z 245..275, y 19 -> 55) is the sub-0 way onto the east balcony
  (a04_04). In sub 1 a mound stands on it (attribute-8 nodes x 418..455, z 240..293, up to y 75)
  inside two player-only walls (attribute 0x51: node 741 at x 424, z 245..275; node 744 at z 245.1,
  x 424..454; y 12.9..97.7). The first a04e_01 attempt stopped at (419.5, 31.6, 259.9) and took two
  hits and a bite (55 -> 35, infection 100). Section 6.
- The north balcony is cut at x 408..468 (z 140..170, the north room's corridor at floor level);
  the west balcony (x 305..347, z 140..380, y 54.9..55.9) has no stair or ramp from the hall floor.
  The box stack at x 343..362, z 334..380 (tops 28.9 and 42.9, attribute 0x46) meets the balcony
  under its fence (attribute 0x50 at x 346..350, y 54.9..65.4; 0x51 above to 82.8). The beam at
  z 376 (node 257, attribute 0x3D, the hang ledge at y 90.3..91.6, x 319.7..500) runs from the west
  balcony's step (node 255, y 58) east to x 500, where a player-only wall (x 499.5) ends it: a
  one-way way down from the balcony into the hall (inferred from the grid, not played).
- [68] / [69] (0x825510, C): [68] (subtype 0x11) is a platform at (317, 14.9 or 55, 260), the
  west balcony's south part; [69] (cls 0x88 at (320, 29.9, 342.5)) is its console. Use at [69]
  starts script 0x82BD60 (counter 0x65 = 1); [68] then runs 0x82C060 (D_008107BD 0 -> 1, up) or
  0x82C120 (down) and sets counter 0x65 = 2. Measured (a04e_s0_console): Use facing -x from
  (325.5, 342) took at f630 (counter 0x65 0 -> 1), flag 0x0D = 0xFF at f674, flag 0x65 0 -> 1 and
  counter 0x65 = 2 at f844: the platform was down at the start and went up. How the player rides it
  (the console is 82 units from the platform) was not found.
- The hazard 001BECC0 (model 0x11) at (338, 15.7, 317) strikes inside its zone (D_00275628: radius
  17, height 2; 001BE5F0, BM). Both the walk south and the console approach pass it.

**a04e_00 (the north room to the hall).** Back round the bar (430, 20) .. (459, 92), the slider
[51] (001BB860 model 0x16, id 5, D_00810845 bit 5) from (440, 102.5) facing +z: entry 8.

**a04e_01 (the hall to AREA02).** The east side is closed south of z 217 (the pocket x 502..545
behind walls), so the west side: the north passage, the water floor 0x5B (node 593, y 13.2) at
x 365..410, round the box [40] (001551B0 at (368.1, 224.6), whole in this load) by (345, 215),
south along x 326..332, east north of the box stack at z 324..335, the slider [41] (001BB860 model
9, id 8|0x80, no lock) from (440, 374.5) facing +z: AREA02 entry 4 (439, 15, 99) sub 2
(D_00810730[2] = 0x82). Hits at (328.1, 310.1) f326 and (345.5, 327.7) f423, -5 each, both
inside the hazard's zone.

**a02c_00 (AREA02 sub 2, entry 4 to the seal).**

- Item 0x1E used at once (the HEALING page, 45 -> 75).
- The platform (y 15, x 285..470) is split by a wall at z 100; entry 4 lies in its doorway; the
  south part is closed from it. The channel north of the platform (z -26..33, x 80..612, between
  walls z -30 and the platform's side) carries: two 00147390 creatures (+0x34 180; drainA at (418,
  -26) moved to (433, -13), drainB at (162, -16)), the pod 001BECC0 at (265.8, 0.8, -10.5) (zone 17),
  three flames 001E3D90 along its south side (201..260, 33..39), the 001546C0 tendril fields at
  (153.9, -22.4) and on the platform at (361.9, 15, 62.9), and a generator 0015A2C0 at (298.4, 0,
  33.8).
- Off the platform's north edge at x 450: a drop of 15 that costs 10 (f567, every try; the fall
  reaction 0x0B). West along z 8 .. -23: the pod's north side, one strike (-5, f859 at (272.5,
  -23.2)); (215, -12), (196, -3) round the wall stub at x 180..192, z -27..-9; on west at z 0..4,
  (75, -10), the corridor x -38..40 north on its east side (x 32).
- The seal [21] (001582E0 model 0x0E, at (-38.6, 7.9, -187.2)) sits in the corner of the corridor's
  west wall and the door's wall: from (-34.1, -182.6) facing it (yaw -2.41), the light melee
  (Circle) broke it at f1945: D_00810843 0x08 -> 0x09, D_00810842 0x00 -> 0x02 (AREA01's [11]),
  as 001582E0 (BM) states.

**a02c_01 (door [22]).** 001BC350 model 0x15, id 0|0x80, D_00810843 bit 0: Use facing -z from
(-30.5, -181.5): AREA01 entry 3 (-25, 0, -197) sub 1 (D_00810730[1] = 0x81). A bite at the door
(f10, infection 85 -> 95).

## 3. Route table

| Beat | Source | Frames | Counters | End |
|---|---|---:|---|---|
| `a04e_00_hall` | a07b_07_lift | 944 | 275032..275976 | AREA04 sub 1 entry 8 (440.5, 14.9, 134.1); health 55 |
| `a04e_01_door41` | a04e_00 | 1,374 | 275976..277350 | **AREA02** sub 2 entry 4 (439, 15, 85.9); health 45 |
| `a02c_00_seal` | a04e_01 | 1,982 | 277350..279332 | D_00810843 bit 0, D_00810842 bit 1; (-34.1, 0.1, -182.6); health 60 |
| `a02c_01_door22` | a02c_00 | 839 | 279332..280171 | **AREA01** sub 1 entry 3 (-25, 0, -210.1); health 60, infection 95 |
| `a04e_s0_console` (side) | a04e_00 | 1,989 | 275976..277965 | counter 0x65 = 2, flag 0x65 = 1, flag 0x0D = 0xFF; health 5, infection 100 |

Line a07b_07 -> a04e_00..01 -> a02c_00..01: 5,139 trace frames, counters 275032..280171. Every
beat passed the recorder's resume check. The side beat is evidence for section 2 only (its health
is not a route state).

## 4. Story bytes, health and items (measured)

| Byte | Before | After | Where |
|---|---|---|---|
| D_00810843 (AREA02 locks) | 0x08 | 0x09 | a02c_00 f1945 |
| D_00810842 (AREA01 locks) | 0x00 | 0x02 | a02c_00 f1945 |
| D_0081083D (counter 0x65) | 0 | 2 | side beat only: 1 f630, 2 f844 |
| D_008107BD (flag 0x65), D_00810765 (flag 0x0D) | 0 | 1 / 0xFF | side beat only |
| items C64 | | | 0x1E -1 a02c_00 f211 (none held after) |
| health | 55 | 60 | 50 a04e_01 f326; 45 f423; 75 a02c_00 f302 (item 0x1E); 65 f567 (the drop); 60 f859 |
| infection | 85 | 95 | a02c_01 f10 |
| rounds | 51 | 51 | magazine 21, no shot in the kept beats |

## 5. Census

Not run (`route_census.py` knows only the earlier groups).

## 6. Tried and not kept

Exploration attempts of the same beat names in the same set (a failed beat keeps no snapshot; its
trace is in `_failed/`, overwritten by the next try). Leads, not ground truth.

- **The conveyor in sub 1** (a04e_01, first version, toward the lift [57]): stopped at (419.5, 31.6,
  259.9) against the 0x51 walls; health 55 -> 35 and infection 100.
- **The hall's east side** (x 520..525, z 212..232): stopped at (526.7, 214.3) by the pocket's walls;
  a bite (infection 100).
- **Straight past the box [40]**: stopped at (355, 220) against it; 55 -> 20.
- **South of the box stack** (z 338..345): stopped at (340, 343.8); 55 -> 25.
- **AREA02, the platform's north part to its west edge (z 40) and the channel's south side**:
  hits near the platform's tendril field (390, 40.7) and (345.6, 43.7), -10 each, the drop at (279.6,
  39) -10, then the flames (-5s): dead at (199.8, 29.9) f681.
- **The platform's south part**: not reachable from entry 4 (the wall z 100, the slider's frame).
- **The channel's middle (z 0..21)**: the pod's strikes at z 0..6 (-5 per about 100 frames), and the
  walk blocked at x 257..262 for z 0..21 by something that is neither in the grid nor in the movable
  object table (D_00275B8C held one entry, drainA); not identified. The north side passed.
- **Shooting the pod** from (290.9, 1.6): R1 and Circle, 34 rounds, its +0x34 stayed 60 (the lock
  did not take it); -10 from behind.
- **The seal from in front of the door** ((-33, -180) facing -z; (-38.6, -182.6) not reachable):
  eight light melees, no break; infection 85 -> 100 from bites. The corner position kept.

## 7. Open

1. **How the platform [68] is ridden** (or whether the console's script carries the player): the
   way from the hall to AREA03's door [44], and with it to AREA19 (AREA03 sub 1 [25] -> sub 0 [15]
   -> AREA19 entry 0 on the y-265 deck -> the ladder 694 -> the tower), if the platform is the
   designed way. 0x825510's script 0x82BD60 and the platform's motion were not read further.
2. **Whether the sub-1 conveyor ever opens** (the mound's actors 001BF6B0 at (438, 40.7, 254.6) and
   (442.1, 60, 268), 001C06E0 at (506.3, 55, 239.2), +0x34 80 each; walls 741 / 744). If it does,
   the east balcony leads to AREA13 by the lift [57] and on to AREA19 by the hatch [63] line of the
   twelfth to fourteenth levels.
3. **The channel's blocker at x 257..262** (AREA02 sub 2) and **00147390's attack** at a distance.
4. **The fall cost**: a drop of 15 in AREA02 cost 10 on every try, while AREA04's 40-unit drop
   (a04c_00) cost nothing. Not read.
5. AREA01 sub 1 from entry 3: the room x -50..50, z -320..-190 at y 0; door [9] (AREA00 entry 0,
   sub 2) is at z -1277 through the lower level. AREA01's upper floor (door [14] to AREA22, e6 at y
   60) showed no stair or ladder from the lower level in the grid (one ladder, attribute 0x32 at
   (-70, -719), climbs to y 25 only).

## 8. Where the recording stops, and what the next run needs

a02c_01 ends in AREA01 sub 1 at entry 3 (-25, 0, -210.1), health 60, infection 95, 21 in the gun
and 51 held, no healing item. It stops there because the derived segment needs item 0x24 first,
and the next steps toward it are open (section 7 items 1 and 2).

**The order the code now gives** (WORLD_GRAPH.md section 7 step 7, revised): item 0x24 from AREA19
sub 1 [53] (the tower's y-450 landing, flag 0x25 set) -> AREA00 sub 2: the reader [19] at (-185,
-45, -1499.1) facing -x (item 0x24; D_00810841 bit 4), the slider [20] to entry 11, g[3] (item 0x26)
at (-214.3, -48.9, -1477.9) -> [28] (0x826790) at (50, -59, -1590): item 0x26 used from the status
screen's EVENT page (00184D20 accepts event 0x26 for class 4 model 0x49 and sets +0x0B = 5), then
Use at the Use point (33.9, -47, -1630.4), r 10, h 8, player yaw about 1.92 (the raised surface,
not the -60 floor) -> script 0x82D070 -> AREA14 entry 0. In AREA14, [6] (0x825FD0 at (229.5, 680,
1202.8)) is usable only while item 0x28 is held; item 0x28 is AREA18 g[1] at (521.3, 153.1, 184.4),
and AREA18 is entered by 00190F20's quad D_0024A4B0 ((754.1, 829.6), (767.6, 795.0), (809.8,
814.3), (791.2, 840.5)) and left at x <= 285 to AREA14 entry 1. [6]'s Use point (222.5, 680.1,
1185.1), r 7, h 20, yaw about -1.85: script 0x828120, callback 0x825C40, AREA17 entry 0. AREA17:
[30]'s boss (00133E30) must be killed; [33], [34] (item 0x28 taken, item 0x2B, CC3 0x19, item 0x18
dropped at (511.5, 220.3, 316.5)); counter 0x5D (D_00810835) is written only by 0016BC40 (BM), the
cable ride's state 22, entered with item 0x18 on a floor of attribute 0x20 under a cable of
attribute 0x3C; then [38] (0x825470) and AREA11 entry 3 (465.2, 270.5, 426.4). Read by this lane's
code reading (BM / C files named in decomp `src/` and `src/overlays/AREA00, 14, 17, 18, 19`); none
of it played.

**What the port lanes need.** The recordings above; cards `a04e_00` .. `a02c_01`; chains `a04e`,
`a02c`. New overlay code on the live path: AREA04 0x825510 (the console and platform; side beat
only), none in AREA02 beyond its boot behaviours. Boot behaviours seen in these loads: 001BECC0
(the pod / hazard, with 001BE6C0 and 001BF3C0 children), 001546C0 (tendril fields), 0015A2C0
(generators), 001E3D90 (flames), 00147390, 0012A5D0, 001582E0 (the seal), 001BB860 / 001BC350.
Asset exports: AREA02 sub 2 (with the channel's actors) and AREA01 sub 1 (entry 3's room); AREA04
sub 1 is already exported (`assets/levels/04/sub1`).
