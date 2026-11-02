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
- **Constructed starts (section 7c)**: k00 / k01 (item 0x24, and k01 also the reader / g[3]
  results) loaded by the original into AREA00 sub 2; from there [28] with item 0x26 was played
  into AREA14 (k01_02). Lane CAP3 (section 7d) played on through AREA14: the bench and item 0x19
  (k01_03), the attribute-0x33 face to the top and its script (k01_04), the plateau to (650, 1015)
  (k01_05). **Not entered**: AREA17, AREA18, AREA19 sub 1's tower.
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
- Side beat `a04e_s1_hoist` (section 7b item 3) never passed; its tries are in `_failed/`.
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

## 7b. Round 2: the way to item 0x24 (read, then tried)

The coordinator's order of attack, item by item. Marks as in WORLD_GRAPH.md (BM, NM, C; "grid" =
the collision grid D_0028A598[0] of a beat's snapshot, a lead until walked).

1. **The platform [68] cannot carry the player up from the hall (C, grid, measured).** 0x825510:
   subtype 0x11 is the platform, at y 14.9 when flag 0x65 (D_008107BD) is 0 and 55 when not; it
   moves only when counter 0x65 (D_0081083D) is 1, by script 0x82C060 (up) or 0x82C120 (down), each
   ending with counter 0x65 = 2; its collision is an actor-attached hull (001A2370, NM), not an
   entry of the movable-object table D_00275B8C. The console [69] (subtype 0, descriptor 0x82C1E0:
   radius 10, height 20, facing +x at (320, 29.9, 342.5)) starts script 0x82BD60, which places the
   player at the console, waits, sets counter 0x65 = 1 (measured at f630, 164 frames after the
   press) and ends with flag 0x0D = 0xFF (f674). The platform rose f630..f844. Its rest is a pit at
   x 317..350, z 245..275 in the hall floor, 82 units from the console's Use point: the player cannot
   be on it when it starts, and by the time a run reaches it (about f765) it is about 25 above the
   floor. Even up, it joins only the balcony's north part (x 320..350, z 140..245, and the strip z
   140..170 to x 405): the south ledge with door [44] (x 320..350, z 335..380) is 60 units further
   south at the same height, and the north strip ends at x 405 over the north room's corridor (x 410
   ..470), 65 units short of the east part (x 470..560).
2. **The sub-1 conveyor's walls are static.** Nodes 741..745 (attribute 0x51, a box x 424..458, z
   245..275, y 12.9..97.7) are in sub 1's own grid (the port's `assets/levels/04/sub1/area04.emcl`
   has them; sub 0's `assets/area04/sub0/area04.emcl` has other 0x51 posts, not these). No AREA04
   overlay function calls the cell-directory toggle 0019C6F0. The mound's actors are deferred g[19],
   g[20] (001BF6B0) and g[21] (001C06E0), condition 1. Nothing found that removes the walls.
   **AREA07's lift arrival** (entry 10) is the north room; its only ways out are the slider [51] and
   the lift. **AREA01's upper floor**: the grid of a02c_01's snapshot has no floor between y 26 and
   59 anywhere (x -400..400, z -1400..200); the one ladder (attribute 0x32 at (-70, -719)) ends at
   y 25, and the walkway at y 60 (z -680..-665) passes over the lower floor. Not walked.
3. **The rest of the hall** (grid; one try, a04e_s1_hoist, `_failed/`): the north strip's two
   machine blocks (x 368..383.7 and 389.4..405, z 139..169.6, top 47, under the balcony's 49.9),
   the pillar x 390..404, z 169.5..183.5 (top 53.9, at the balcony's unfenced edge x 390..404), the
   console [37] (0x825880, C: script 0x82C1F0, then after 120 frames the lift [36] 0x825B00 moves
   +10, to y 24.9). The walk to [37] was stopped by the pillar and by the hall's creatures (55 -> 30
   in one try, 55 -> 0 in another). Whether [36] leads onto the pillar was not seen.
4. **Item 0x24 before AREA07 (code).** [53] (0x827550, C) needs flag 0x25 == 0xFF (001BA1C0, BM)
   and flag 0x26 clear. Flag 0x25 has two writers and both need the other lab first: AREA07's [17]
   (0x823990, C) runs 0x823C20 (script 0x826930: flags 0x15 and 0x25) only when flag 0x18 is 0xFF,
   else 0x823B40 (flag 0x15 only); AREA08's sub-3 [2] (0x8245E0, C) runs 0x824870 (script 0x8268E0:
   flags 0x18 and 0x25, items 0x13 and 0x14) only when flag 0x15 is 0xFF, else 0x824790 (flag 0x18
   only). So flag 0x25 is set by whichever lab comes second. AREA07 is reached only from AREA04
   sub 1's north room (the socket [61] needs item 0x29, which only sub 1's [1] 0x825040 gives, in
   the hall), and the hall is entered only from AREA03's door [24] onto the south ledge (one way down)
   or from AREA02 sub 2. From the hall no way back to AREA03 (and so to AREA08 or AREA19) was found.
   **No branch takes item 0x24 before AREA00 on the ways found**: with AREA07 first, AREA08 is
   behind the hall; with AREA08 first, flag 0x25 is set inside the hall's cluster.
5. **The cluster after AREA07** (grid and code): the hall and north room, AREA07, AREA02 sub 2 (doors
   [22], [25]), AREA01 sub 1's room at entry 3 (doors [11] and [12] to AREA02, the bridge south), and
   AREA02 sub 1. AREA01's bridge (0x8261A0 -> 0x826200, C): the half's tilt is 10 degrees when flag
   0x06 is set, 60 when flag 0x08 is set (the fourth level's crossing set it, a01r_02), and 0 when
   flag 0x2C is set (AREA00 [28]'s script, AREA14 [5]): the way south from entry 3 to AREA01's door
   [9] (AREA00) is the raised half until then (read, not walked). If that holds, AREA00 itself is
   out of reach from this cluster, and the second half's first step could not be made by any player
   who did AREA07, which a shipped game would not allow. The likeliest gap is one of the reads in
   items 1, 3 and 5 (the bridge's raised half, the pillar and lift [36], the platform), each a lead
   to settle on foot with more health.

## 7c. Round 3: the bridge, then constructed starts

**AREA01's bridge (side beat `a01w_s0_bridge`, played, one try).** From entry 3 south along x 0
over the bridge's north half: its end is at z -430, the south half stands raised (flag 0x08), and
the walk off the end fell to y -280 (health 0). No way past it to door [9].

**Constructed starts (orchestrator's decision, 2026-11-02).** The segment's code-gated steps that
no route found reaches are entered from a *constructed* progress block: the latest real one (the
end state of `a02c_01_door22`) plus exactly the bytes the missing steps' code writes, each listed
below with the definition it comes from. The original's own save writer (00228A90, run by port
`tools/build_start_saves.py` `CONSTRUCTED`, with its save and read-back checks) makes the save; decomp
`tools/ps2_card.py <card.ps2> <save dir>` writes it into a card image (its file table read back
equal by port `tools/compare_card_image.py`); `record_level_routes.py --card` puts the image in slot
1 of the fork session (a copy per run under the set's `_cards/`, never the user's cards); the beat
then LOADs it through the in-game options' load row (SELECT, row 6, Cross, the card, the save, as
opt_07 does), so the original places the player. **Only the entry state is ours; everything after
the load is the original running.** Beats from a constructed start are named `k00_`, `k01_`.

The image path was proven first with a real block (side beat `k00_s0_loadtest`: the a02c_01 start
card loaded from AREA01, the player placed at its load pose (-34.06, 0.05, -182.64) in AREA02 sub 2,
health 60 and infection 85 as saved).

| Card | Base | Changed bytes (address = value: why) |
|---|---|---|
| k00 | a02c_01's end (AREA01 sub 1 entry 3) | D_00810700..03 = 00 02 00 01: AREA00 sub 2 (D_00810730[0] = 0x82 from AREA15's 0x823C80), entry 0 (what AREA01 door [9], id 0x80, gives), previous area 1. D_00810C88 = 1: item 0x24 (AREA19 sub 1 [53] 0x827550 calls 001C47A0(0x24, 1) at its script's end). D_0081077E = 0xFF: flag 0x26 (that script 0x82D590: op06/0 sets 1, its op07/5 end 0xFF). The player record D_00810350 = (-40, -35, -1290), D_00810370 = (0, pi, 0): spawn entry 0, copied into the load pose by the terminal's step (00159B90's 00102948). |
| k01 | the same | k00's bytes, plus D_00810841 = 0x14: the reader [19] (00158810 model 0x13) with item 0x24, 001576E0 (BM) script 0x246C20, op09 001580C0 (BM) ORs bit 4; D_00810C8A = 1: item 0x26 from g[3] (0015AFA0) behind the slider [20]; D_00810861 bit 0: g[3]'s taken bit (001B1190(8), BM; tested by 001B11E0 for deferred condition 1). The player again at entry 0. |
| area14k | k01_02's end | none: AREA14 entry 0 after [28], a card for the next AREA14 beats. |

Why k01 constructs the reader / slider / g[3] too: in AREA00 sub 2 the arrival corridor (x -45..-35,
the floor y -35) is closed to the south by the player-only walls 1584..1586 (attribute 0x51, z -1346
..-1319) and to the west by wall 70 (x -45); its east edge (x -35, past a rail at z > -1303) drops 25
to the floor (y -60, -5); that floor is parted from the reader's floor (x -182..-130, z -1513..-1489)
by a hive mound (attribute-8 walls and slopes at x -32..-18, z -1463..-1421, rising to y -15). Side
beat `k00_s1_reader` never passed (stopped at (-16.8, -60, -1442.1) against it). The way the
designers meant to the reader was not found.

**Played after the constructed starts:**

| Beat | Source | Frames | Counters | End |
|---|---|---:|---|---|
| `k00_00_load` (CONSTRUCTED k00) | a02c_01_door22 | 1,293 | 281155..281464 | AREA00 sub 2 entry 0 (-40.5, -35, -1290); item 0x24 held; health 60 |
| `k01_00_load` (CONSTRUCTED k01) | a02c_01_door22 | 1,293 | 281155..281464 | the same, with lock bit 4 and item 0x26 |
| `k01_01_slab` | k01_00 | 910 | 281464..282374 | off the corridor's east edge (f198, -5), the floor east of the mound south, the opening in the 0x50 walls at x 21.5 (z -1644..-1613): (26.1, -60, -1630); health 55 |
| `k01_02_item26` | k01_01 | 3,382 | 282374..285756 | the ledge climb onto [23] (001C4820 at (34.6, -60, -1628.9), top y -52); [28]'s Use point (33.9, -47, -1630.4) facing 1.92; START, ITEM, EVENT, the second entry, Cross, Left, Cross: item 0x26 taken (f511), script 0x82D070, **AREA14 entry 0** (f1633 area bytes 0E 00 00 00); [5]'s arrival script: flag 0x2C = 0xFF (f3313); (600.9, 390.5, 1354.3), health 55, infection 100 (a bite at f49) |

The EVENT page's list (00215870, BM, state 0): id 1 for item 0x24 (else 0 for item 0x23), then
(i - 0x23) for every held item 0x25..0x3B: here 0x24, 0x26, 0x27, 0x29, 0x2A, so item 0x26 is the
second entry (a first try on the third entry, item 0x27, ended in the page's refusal banner).

Port side: chains `k01` (k01_01, k01_02) in `tools/mid_route_beats.py`; cards `k01_01`, `k01_02`
and `area14k` built with `build_start_saves.build` (save and read-back checks passed; `k00` / `k01`
are the constructed cards themselves). Per-row rand() tables: k01_02's replay matched (0.000);
k01_01's did not (largest position difference 230.7 at the 50-row samples: the re-drive left the
recording's path; not investigated), so its table is not to be used.

**AREA14 (read and tried; not recorded).** Entry 0 lies in a valley (y 390). The wide "stair" north
of it (x 499..609, z 1242..1300) is a slope of attribute 0x05 (normal y 0.66 above y 423.6): the
walk stopped at (545.9, 422.6, 1272.7). The way up is a climbing face of attribute 0x33 (nodes 1465..
1479 and 1618: x 371..457, z 1228..1274, y 432..679, normal (0.42, 0, 0.91)) from the base floor
1617 (x 376..395, y 432, z 1268..1286) to the top 1482 (x 409..425, y 679). 0015D4C0 (NM) starts it
(state 0x0D, actions 0x1B / 0x1C) only with item 0x19 held (D_00810C7D), and item 0x19 is AREA14's
g[1] at (394.7, 448.5, 1375.5) on the bench west of the valley (y 430..450). The climb's base is
inside [0]'s polygon (0x8238E0: y < 435, its script 0x826C10 and CC3 0x16 play on entry). The way
from the valley (y 390) up to the bench (y 430) was not found (the grid shows walls at x 484 and
x 522 and small pockets at y 410 / 425 between, leads for ledge climbs).

## 7d. Round 4 (lane CAP3): AREA14 from entry 0 to the plateau

Lane CAP3, 2026-11-02. Beats `k01_03` .. `k01_05` run on from k01_02 (so after the constructed
start k01; nothing in them is constructed). Same tool and set as section 7c
(`levels-recordings-20261102-1`, group `k01`). Exploration side beats `k01_s2_face`,
`k01_s3_plateau`, `k01_s4_fight` end on purpose with an error, so their traces stay in `_failed/`
(leads, overwritten by the next try). Positions marked *grid* were read from a snapshot's collision
grid (D_0028A598[0]) and then walked unless said otherwise. A public walkthrough (GameRevolution's
Extermination guide) was used as a lead for the order only; every step below is the code's and the
recording's.

**Health.** At infection 100 health falls by 2 about every 240 frames outside scripts (measured:
no loss during [0]'s and [2]'s scripts). The 0x3E reaction (+1F0 62) is a hit: the 00141D20
creatures' attacks cost 20 and 5..10 (k01_s4_fight); one 0x3E at f93 of k01_03 cost 5 with no
creature near (not identified). HEALING kinds (002160B0, C): item 0x1E +30, 0x1F full, 0x20 full and
infection 0, 0x21 +30 (and infection -30 when D_00810707 is not 1), 0x22 full; with D_00810707 = 1
(here) health is capped at 60 and infection is kept.

**How the 0x33 face works (read, then measured).** 0015D4C0 case 0x33 (NM) mounts the face with
item 0x19 (state 0xD, 00167C80 BM, then 0xE, 00168050 C). In state 0xE the stick picks a mode
(00174FD0: up / down / left / right). Up: 001806E0 (C) probes 10 units ahead at feet + 18; a miss
is the climb-over (states 0x1E..0x21, onto the floor above); else it moves while a probe at the
skeleton's hip + 6 still hits attribute 0x33, so the feet stop about 14 below a patch's top.
Sideways: 00180850 (C) needs attribute 0x33 at both side probes; at a patch's edge the player
reaches (0x46 / 0x50) and Use (D_00810E74 & the press mask) runs 001809B0 (C): a hang ledge 0x3D,
a bar 0x34, a ladder, or (00178080, C) a ledge whose top lies between feet + 9 and feet + 21 on the
wall 10 to that side (12 below y 560); the jump (0x5A..0x5C) ends in the hang (state 9), and the
stick toward the face pulls up. Measured rates: about 0.14 per frame up, 0.12 sideways.

**Beats.**

| Beat | Source | Frames | Counters | End |
|---|---|---:|---|---|
| `k01_03_item19` | k01_02 | 1,112 | 285756..286868 | the crate 1791 (grid floor y 404, x 485.5..499.5, z 1368.7..1382.7) climbed from the east facing -x (14 up), the block 1453 (top 431.6) from it facing -x (27.6 up: the hang, stick forward pulls up), the plateau y 432, the ramp 1002 onto the bench (y 448.5); g[1] taken (f1017, **item 0x19**); (402.1, 448.5, 1375.6), health 46 |
| `k01_04_face` | k01_03 | 16,130 | 286868..302998 | round the eggs (00156620 at (447..463, 432, 1338..1340) block the line z 1338) to the face; [0]'s polygon: script 0x826C10 f228..f2882 (flag 0x3D = 0xFF, CC3 0x16 = 1; the player put at (403, 432, 1281)); Use at (384, 1274.8) facing yaw -2.71: the face 1618 / 1479 to feet 478.6, right to 1475 (x 453.7), up to 543, left over 1469 to 1473 (x 425.3), up to 551.9, right to 1471 / 1470 (x 440), up to 572.4, right to 1472 (x 453.7), up to 587, the side jump left (Use) onto the shelf's front 1311: hang, pull-up onto the y-602.2 shelf (440.1, 1241.5); the ledge climb facing yaw -1.14 onto 1341 (y 618.7); west over the 0x39 ledge 1487 (the wall-side shuffle) to the 0x33 floor 1481; 1468 up to 647.9, right over 1466 to 1465 (x 418.7), up: the climb-over onto 1482 (y 678.9) at f7185 starts [2]'s script 0x826FC0 (f7185..f14758; flag 0x3E = 0xFF, item 0x0F, CC3 0x17; the item page closed with Triangle); control at (409, 680, 1209), health 12 -> 6 on the way; g[4] taken (f15584, item 0x1E) and used (f15973, 6 -> 36) at (471.3, 677, 1295.4) |
| `k01_05_plateau` | k01_04 | 1,490 | 302998..304488 | north along the plateau's west wall (attribute 0, x 450 -> 512 diagonal, y 680..953), the box 001551B0 at (546.1, 682.3, 946.6) broken with the light melee, g[6] taken (f721, item 0x1E), east along z 1015 and the item used at (649.2, 1015) (f1330, 28 -> 58); end (658.3, 682, 1015.3), health 58, 21 + 51 rounds |

Line k01_02 -> k01_03..05: 18,732 trace frames, counters 285756..304488. Every beat passed the
recorder's resume check.

**Port side.** Chain `k01` in `tools/mid_route_beats.py` now k01_01 .. k01_05; cards `k01_03`,
`k01_04`, `k01_05` in `tools/build_start_saves.py` (built into main's `build/start_saves/`, save and
read-back checks passed). Rand tables (`tools/record_beat_rands.py`): k01_04 and k01_05 replayed
exactly (0.000); **k01_03 did not** (largest position difference 7.016 at the 50-row samples, like
k01_01's), so its table is not to be used until that is understood.

**Tried and not kept (AREA14).**

- The face's patches are separate: 1475's top (556.5) and 1472's foot (568.5) do not meet, nor do
  1476 (top 505.5) and 1473 (foot 523.5); from 1472's top (feet 595.8) the side jump finds no
  target (the reach, Cross four times, no jump), and Use on the face does nothing. The jump that
  works is from 1472 at feet 587 (the shelf's top 602.2 inside feet + 9 .. feet + 21).
- The bench's walk east at z 1338 was stopped by the eggs; the first face_move runs lost 45 frames
  per stall and the first k01_04 runs died of the drain 6 units short of g[4] (health 4 at the top);
  the kept run trims the stalls and the shelf walk (health 12 at the top, 6 at control).
- **The plane (00190F20's quad).** The quad D_0024A4B0 ((754.1, 829.6), (767.6, 795), (809.8,
  814.3), (791.2, 840.5)) lies over the plane's cargo ramp 1530 (attribute 4, slope from 658.5 at
  z 798..820 up to the plane floor 705 at z 827..849). Over it leans a player-only panel 1531
  (attribute 0x50, normal y 0.71; from y 664.6 along (739.7, 814.3)-(805.9, 849.4) up to 729.6 along
  (771.5, 757.6)-(834.9, 791.1)); below both there is no floor (the grid's lowest is y 348). The
  plane's body is boxed by player-only walls 1570..1577 / 1667..1671 (attribute 0x51 / 0x50, y 674
  ..705); the creatures walk inside it at y 705, the player cannot. The pit round the ramp is walled
  by 1493 / 1495 (west, x 742..760) and 1505 / 1509 (north-east), attribute 4: walked into from the
  west ground (741, 833) and from the east corridor (789, 859), both stopped. The ridge east of the
  corridor (x 816..849, y 712..736) has slopes of normal y 0.45..0.55 at its foot: the walk east from
  (806, 868) stopped. South of z 780 there is no ground. Not found: how the player gets onto the
  panel or the ramp (the walkthrough's "slide through the cargo doors").
- **The two 00141D20 creatures** ([100] / [101] of the area's actors, cls 2 model 6, +0x34 300)
  roam the plateau round the plane and walk into it. Shots (R1 held, Circle every 14 frames): no
  lock (the lock bytes stayed 0), 2 hits in 51 rounds (-10 each). When one stands next to the
  player its attacks cost 20 and then 5..10 each (k01_s4_fight: 48 -> 4 in about 800 frames); the
  first probe died at (716.8, 918.2) on the way from (700, 975) to (720, 930). Shooting them down
  is not possible with the ammunition held (600 hit points, 72 rounds).
- **k01_05's end is next to a creature**: one 00141D20 stands about 42 units south (657.9, 686.5,
  972.9) at the last frame. A probe that walked east along z 1000 toward the block 2086 (a ledge
  climb onto its top at about 710 from its west face x 754..765, then the ridge from the north) was
  caught at (677.8, 1008.2): health 58 -> 16 by f580 and 0 by f980, so the block and the ridge from
  the north were not tried. A probe that went south-west first ((690, 960), then round the plane's
  west and north sides) was not attacked until the east corridor. The next run should leave the
  creature's side first.
- The four turrets (overlay 0x823FC0, the AREA19 0x827DD0 family; at (509.1, 725.2, 1050.5),
  (482.2, 725.2, 1111), (514.8, 725.2, 1005.4) and (421, 494.2, 1362.8)) did not hit the player on
  any walk past them (read: not further).

## 8. Where the recording stops, and what the next run needs

**The kept line now stops at k01_05's end: AREA14's plateau at (658.3, 682, 1015.3), health 58,
infection 100, item 0x19 held, 21 + 51 rounds, after the constructed start k01 (sections 7c, 7d).**
Next: onto the plane's cargo panel / ramp and into 00190F20's quad (section 7d "The plane"; the
lead is the walkthrough's slide, and whether a script or the creatures change the panel 1531 or the
walls 1493..1509), keeping clear of the two 00141D20 creatures; AREA18 (item 0x28, g[1] at (521.3,
153.1, 184.4); the slider [18] and the panel [21] lock bit 0; back at x <= 285 to AREA14 entry 1
(757.7, 705.1, 860.5), where [4] plays with item 0x28), [6] (0x825FD0, Use point (222.5, 680.1,
1185.1), r 7, yaw about -1.85) to AREA17, the boss (00133E30), [34], item 0x18 and the cable floor
(attribute 0x20 under 0x3C), [38] to AREA11 entry 3. Health: the drain is 2 per 240 frames; AREA14's
remaining heals are g[2] (item 0x21, (327.6, 679.3, 1358.2), the top's west) and g[8] (item 0x21,
(502.2, 515.2, 1210.4), a ledge not reached); AREA18 has g[2] (item 0x1E). Cards: `area14k` (=
`k01_03`), `k01_04`, `k01_05`.

The real line (no constructed state) still ends at a02c_01: AREA01 sub 1 at entry 3 (-25, 0,
-210.1), health 60, infection 95, 21 in the gun and 51 held, no healing item. It stops there because the derived segment needs item 0x24 first,
and no way to it from this state was found (section 7b).

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

**What the port lanes need (AREA14, section 7d).** The k01_03..05 recordings and cards. Overlay
code on the live path: AREA14 0x8238E0 ([0]: polygon, script 0x826C10), 0x823B30 ([2]: script
0x826FC0, items 0x0F / CC3 0x17), 0x823760 ([5]: the arrival, k01_02), the turrets 0x823FC0 with
0x825AA0 (deferred group 0x826180) and the effects 0x823580 ([35] / [36]). Boot behaviours on the
path: the face climb 0015D4C0 (case 0x33), 00167C80, 00168050 with 001806E0 / 00180850 / 001809B0 /
00178080 / 00180300, the ledge and hang climbs (0015DF10, 00161790, state 9), the 0x39 shuffle, the
item page and the HEALING page (002160B0), 00156620 (eggs), 001551B0 (boxes), 00219550 / 0015AFA0
(pickups), 00141D20 (the creatures), 001E3D90. Assets: AREA14 sub 0 (whole: the valley, the face,
the plateau and the plane).

**What the port lanes need (earlier beats).** The recordings above; cards `a04e_00` .. `a02c_01`; chains `a04e`,
`a02c`. New overlay code on the live path: AREA04 0x825510 (the console and platform; side beat
only), none in AREA02 beyond its boot behaviours. Boot behaviours seen in these loads: 001BECC0
(the pod / hazard, with 001BE6C0 and 001BF3C0 children), 001546C0 (tendril fields), 0015A2C0
(generators), 001E3D90 (flames), 00147390, 0012A5D0, 001582E0 (the seal), 001BB860 / 001BC350.
Asset exports: AREA02 sub 2 (with the channel's actors) and AREA01 sub 1 (entry 3's room); AREA04
sub 1 is already exported (`assets/levels/04/sub1`).
