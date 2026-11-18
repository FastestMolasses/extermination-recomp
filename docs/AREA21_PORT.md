# AREA21's overlay owners in the port (lane PORT-A21)

Lane PORT-A21, 2026-11-17, branch `a21`. AREA21 is the final boss area
(overlay id 0x12, area byte 0x15; decomp docs/AREA21_OVERLAY.md). This lane
translated its overlay owners from the decomp's C. AREA21 has not loaded
live in the port: no recording of it exists yet (the guided run's AREA21
stretches are still being recorded), so its assets, SUBS row, capture kinds
and the 001A2370 / 0019C6F0 rules of docs/SECOND_HALF_AREAS.md section 2
are not done. Nothing here has run in a frame of the game.

AREA21's boot behaviours (00137830, 0014EC00, 00151590 and their helpers)
and the twins 0x823A10 / 0x823AB0 were done earlier by lane BOOT2
(SECOND_HALF_AREAS.md section 6).

## Modules

Each function is one original function, translated from the decomp C
(byte-identical C, but 0x8273C0, NEARMISS: the oracle runs the original
words, so it decides). Same contract as em_area14_overlay: callees are hooks
by original address, memory goes through `bytes`, a NULL or failing hook or
an unmapped address latches a fault and the entry returns -1; a node's +0x4C
method goes through `w_callback(function, self)`. Each module has a hook
generator (`tools/gen_area21_<module>.py`, the HOOKS table the test also
reads).

| Module | Entries (runtime addresses) | Frame locals |
|---|---|---|
| `em_area21_overlay` (+ `_cannon.c`) | the Gatling [62] 0x8297C0 with 0x8260A0, 0x829310 (the rate-limited step on the table 0x82DB10, shared with the cannon), 0x8294C0, 0x829590, 0x829F10, 0x829F40; the cannon [61] 0x828700 with 0x828410, 0x828E80 and the projectile 0x828F30 (the code pointer 0x828E80 installs) | 0x8297C0 takes the entry sp: 0x829F40 keeps a matrix and a packet in its frame |
| `em_area21_owners` | [42] 0x8261A0; [43] / [44] 0x8262D0 with 0x826310 and 0x8264D0; [45] 0x8266F0 with 0x826840 and 0x8268F0; script 0x82BEC0's op09 callbacks 0x8269A0 and 0x8269F0; [53] 0x827040 (the ending request) with script 0x82CBD0's op09 callback 0x827250; [58] 0x827CD0 | none |
| `em_area21_doors` | the doors [30] 0x826BB0 and [27] / [33] 0x826E60; [54] / [55] 0x82A1D0; the rounds 0x825C70 with its code pointer 0x825C60; [47] 0x8272C0, [49] / [50] 0x8273C0, [51] 0x827640, [56] 0x827880, [57] 0x827A30, [59] / [60] 0x827E70, the deferred group's 0x828350 | none; the four jump-table dispatchers (tables 0x82DC00, 0x82DC20, 0x82DC40, 0x82DC60) are switches |
| `em_area21_groups` | the members of groups 0x82B500 / 0x82BA70: 0x823580, 0x8237B0, 0x823860, 0x8238E0, 0x823AA0; the sub 0 placement [52] 0x8243B0 (lane PORT-A21-52); the boss fight's effect nodes 0x824CF0 and 0x824FE0 (no static reference, reached in g77: lane PORT-A21-BOSS), 0x825B00 (lane PORT-A21-PHASES) and 0x823B40 (lane PORT-A21-3B40) | 0x823580 (a position at sp - 0x10), 0x8243B0, 0x824CF0, 0x824FE0 and 0x825B00 (a packet at sp - 0x60) and 0x823B40 (a packet at sp - 0x70, a blend pair at sp - 0x10) take the entry sp |

[52] 0x8243B0 (decomp func_overlay_AREA21_00824370, NEARMISS C at 99.48%:
two loop-label padding nops and integer temporaries two argument registers
lower, neither a behaviour) is in em_area21_groups. Its casts are the
compiler's float -> unsigned helper 001281C0 (hook, run as original code in
the oracle); the packet lives at sp - 0x60 (frame 0x120), the tables and
groups it names are at their runtime addresses (data is not shifted by the
0x40 of the decomp's code names: the oracle caught a first try that did).
0x824FE0 is translated since lane PORT-A21-BOSS, 0x825B00 since lane
PORT-A21-PHASES and 0x823B40 since lane PORT-A21-3B40 (below): no table
names them statically, the boss fight's effect nodes run them.

**0x823B40** (decomp func_overlay_AREA21_00823B00, NEARMISS 99.33%: only
register choices around the 001281C0 call) is the behaviour of the effect
node 0x824FE0's 001EFD20 starts (g77 row 12708). Its three kinds by +0xD:
0 and 2 queue one 001CFB50 packet a frame (groups 0x82A660; 0x82A6F0 when
+0x38 is 1, else 0x82A780) until the phase passes 2.0 / 1.2; 1 draws ten
particles a frame from a seed read once (never stored back), each placed
through the node's +0xD0 matrix and sized by a blend of two scratchpad rows
(both VU0 inline sequences computed with em_vu_vec_bits / em_vu_max_bits /
em_vu_min_bits on bit patterns; the VU registers are locals of each
sequence) and drawn by 001CA7B0 / 001C7900 / 001C6120 / 001CA940, until
the phase passes 1.5. Frame 0xE0: the packet at sp - 0x70, the blend pair
at sp - 0x10 (its last two words are read with the quadword, never
written). The oracle (groups test, 260 of its cases) runs the original
words with sinf 0011E2A8 and the row scale 00102900 as original code:
states 0..3 and others, kinds 0 / 1 / 2 and others, +0x38 1 and not, each
phase bound one ulp either side, the first particle's life at exactly 0,
0.5 and 1 with phases one and two ulps either side, lives below 0, in 0..1
and above 1, 001CA7B0 negative / zero / positive, a matrix with a -0 y
column (the zero gravity term), saturating colours and matrices, and
Scribbles (the phase, life, colour, size rows, matrix and D_0028A56C
rewritten by draw callees; the seed is not reread). 474 of 474 reachable
words. Mutants (20 of 20 caught): the steps 0.015 / 0.025 / 0.01 (one ulp,
each kind), 1/65535 and 0.0001 (one ulp), the seed step 37 / 11, 0x3F5,
the 255 clamp (one ulp), the 0 clamp dropped, the zero gravity term
dropped, the 0.5 / 0 / 1 life compares with <= for <, h < 0 as <= 0, the
cull radius 10.0, the seed stored back, the fptoui part dropped, +0x38 == 1
as != 0.

## Evidence

`make test-area21-overlay-reference` runs the four oracles
(`tools/test_area21_overlay_reference.py`, `_owners_`, `_doors_`,
`_groups_`), each on the shared harness tools/test_level14_port_reference.py.
No AREA21 capture exists, so each test loads the whole AREA21.BIN (header,
text and data) from the user's extract over the arena of capture
a03b_03_item2a (its boot text checked against the pinned ELF) and runs the
ORIGINAL instructions over designed per-owner states at 0x1E61000 (zero in
that capture): every state and step byte, the flag, counter and lock bytes
each function reads, the D_008102B0 keys and the fire test of the guns,
counter and clamp boundaries (one ulp past each float limit), hook results
and Scribble cases where a hook writes a field the owner rereads. The pure
vector, matrix and float helpers run as original code (their writes
replayed); every other callee is a scripted hook. Compared per case: calls
and arguments, memory at every call, the memory accesses between calls one
for one in order (stack locals at their original stack addresses), memory
after the last store, a poisoned re-run, coverage of every reachable word,
the callee inventory and the fail-stop contract.

| Test | Quick / EM_TEST_FULL=1 | Words |
|---|---|---|
| test_area21_overlay_reference | 518 / 617 cases | every reachable word of the 11 entries (1643) |
| test_area21_owners_reference | 329 / 581 | every word of the 12 entries (635) |
| test_area21_doors_reference | 860 / 1112 | every word but three dead delay slots named in the test (0x82744C: 0x8273C0's table bound after `& 7` never fails; 0x827B70 / 0x827F8C: an effect loop index only 0..2) |
| test_area21_groups_reference | 1511 / 1721 | every word of the 10 entries but one dead delay slot named in the test (0x825544: 0x824FE0's mode 10 switch on its loop index, only 0 or 1); 0x8243B0's 513, 0x824FE0's 609, 0x823B40's 474 |

Lane PORT-A21-52 ran five mutants of 0x8243B0 (the 0.75 scale, a step list, a phase increment, the handle compare, the fourth random's source): all were caught. Each sub-lane also ran a few mutants (load order, limits off by one ulp,
frame offsets, constants): each was caught but equivalent rewrites (a
float add's operand order, `r & 1` for `r % 2 != 0`, a bit below the
24-bit significand in 0x8260A0's unsigned conversion, which the translation
keeps as the original does).

The guided run's AREA21 notes (decomp docs/FULL_RUN_PLAN.md sections
54-60) are behaviour notes, not evidence, and were not compared here; a
recording of AREA21 is still needed to show the owners in a run.

## Binding

- `src/game/em_area01_runtime_second_half.inc`: `dispatch_area21`
  (area 0x15) runs each placement's, group's and spawn's owner and the op09
  callbacks, with each function's frame from its first instruction (0x8297C0
  and 0x823580 get the entry sp). The internal callees run only inside
  their modules.
- `tools/gen_mid_route_bridge.py`: the four hook tables (`b21g`, `b21w`,
  `b21d`, `b21p`) re-enter em_area01_runtime_call by address; the boot
  callees they name are the runtime's (an unbound one faults there).
- `src/game/em_scene_bindings.c`: AREA21's translated overlay behaviours
  bind (0x8243B0 included); any other overlay address of AREA21 does not.

## Live (lane PORT-LIVE-A2021, 2026-11-18, branch `live2021`)

AREA21 is exported from the guided run and loads live through the real area
change from AREA20 (chain g20, AREA20_PORT.md "Live"); it does not play yet.

- **Exports**: SUBS[('area21', 0)] = g75 (entry 3 after the movie and the
  arrival script 0x82B240), g76 (the prep), g77 / g78 (phases 1 and 2), one
  load (cursor 0x1351F80 in all), previous g74. g79's end is left out: its
  background asset differs from the other four (not modelled).
  CREATURES 0x8297C0 and OWNER_MATRIX 0x82A1D0 (both self + 0xD0, byte-
  identical C); FLAG_RULES `area21_flag_calls` from the C of [58] 0x827CD0
  (nine (n, 1) calls once its +5 is 2) and [45]'s 0x826840 ((0x1C, 0) on the
  way past D_0081080E 1: 0x10, 0x11 or 0xFF in the capture). EXPLICIT_MODELS
  0x827E70 state 1 (model 9 from +5 4); ORPHANS_UNPROVEN drum uid 19 (freed in
  the fight); WRITERS: [52] 0x8243B0's eight data words (each pair holds one
  of its products 48c / 8a / 20b / 10b). Shared fixes: export_area15_level
  reads the cell directory across the two files id 0x46 spans (chunk25 f10's
  tail, f11's head); export_render_context.RUNTIME adds D_0027569C (001E1E60's
  handle) and D_002512E0's fields 001CEEE0 stores (AREA21's rounds 0x825C70).
  `--kind all` / `place` pass (692 objects, 3788 level and 116 dynamic kicks,
  cells 69 uids, roster 63, 7 chains, 33 world models, sound); profile 21,
  sfx scope (0x15, 0), stream cues (D_00264DD0[0x16]).
- **Runtime**: area 0x15 as a mid / later area (boot owners bound), data
  window 0x82A380 .. 0x82DC80, receivers, textures, arrival scene list.
- **Blocker**: the far light (Far light) and the grid (Grid) run; chain
  g20 now stops in g76 at 001AAC00 (Grid, last paragraph).

## Far light (lane PORT-A21-RENDER, 2026-11-18, branch `a21render`)

001E1E60's flag-0x23 branch (STATIC_WORLD.md 2.3 step 9 calls it a "sound"
branch: that label is wrong, the code builds a far point's screen sprite,
queues a GS read-back probe at its pixel and draws a flare) is translated in
`em_far_light` (header: every function's behaviour) from the decomp C:
001D73A0 (C), 001D72D0 and 001D75E0 (byte-identical C with VU0 asm), their
callee 001D7200, 001CEFD0 (NEARMISS 95.71%: the original instructions were
followed; the sqrt argument is MULA then MADD, the clamp `c.le` against 1)
and its callee 001CF320. 00122BB8 is the shared rand (em_random_next,
test_random_reference). They bind in em_render_context_live's swc_setup
(views: the read-back list D_008172C0..+0x900, scratch 0x70003600..7F and
0x70003A20..3F; the sqrt is the binder's 0011E748; 001CB5F0 / 001CB900 the
packet chain over R.pc).

- **Evidence**: `make test-far-light-reference`
  (tools/test_far_light_reference.py): the original instructions of all six
  over designed states (camera, clip matrix and fog ramp built to put the
  point inside, at the edges of and outside the window, behind the camera,
  exactly at the clip bound, at 001D7200's clamps and at the fog bounds
  0 / 0x100; lists full or not, both banks); sqrt, 001CB5F0 and 001CB900
  scripted on both sides. Compared: return values, every byte of the
  windows (and no original store outside them), the scripted calls, and
  vf1, vf2, vf23..vf31, ACC, Q, CLIP. Quick 200 cases, EM_TEST_FULL=1 1600.
  Mutants caught: the 0.8 scale, the fog min lane, the 393.84616 divisor
  (one ulp), the clip compare (>= for >), the read-back word offset, the
  extent operand, both fog bounds; equivalent (not catchable): 001D7200's
  clamps written as < 0xDF / < 511.5.
- **vf23**: these functions read vf23 as their caller left it. A fork watch
  table at every 001D72D0 entry over 1804 frames from the guided run's
  states g76 end and g77 end found vf23 equal to the fog quadword ctx +0xA0
  every time (the last writers before 001C1D00 are the fog loaders
  001CAAC0 / 001CCF70 / 001CD520 / 001F4A10; 001D0540, which loads a matrix
  row into vf23, also runs in those frames but never last). The binding
  starts each call from ctx +0xA0.
- **Not modelled: 001D7410** (main-loop step K, NEARMISS
  gs_readback_queue_drain): the original reads back an 8 x 8 PSMZ24 block of
  the Z buffer at each queued point into the entry's +8. The port does not,
  so the list keeps 001D7200's floats and zeros, 001D73A0 returns 0 and the
  flare (001CEFD0) is built whenever its clip and fog tests pass. In the
  original over g76 end and g77 end (1800 frames) 001CEFD0 never ran: the
  read-back word was non-zero. This only changes packets (no game byte
  reads them); exact behaviour needs the GS world's Z buffer at step K
  (renderer work, decision for the orchestrator).

## Grid (lane PORT-A21-GRID, 2026-11-18, branch `a21grid`)

AREA21 also sets render flag 0x22, so 001E0CF0 runs 001E1AD0 (the 32 x 32
grid, em_static_world) after 001E1E60. Its missing pieces are bound:

- **D_00275C0C** (001E1AD0's phase, .bss): a writable view in
  em_render_context_live, zero at boot. In the original it is 0.0 at the
  end of every guided take before AREA21 (g71..g74 end states) and only
  001E1AD0 writes it, so a run that enters AREA21 through the area change
  (chain g20, card g73) starts it exactly. A start from a card inside
  AREA21 would need its session value (g75 end 0.5001251, g76 end
  0.9912671, ...): not modelled.
- **D_008105D0** (the camera eye; 001E0E80 reads its y, D_008105D4): an
  optional external view of em_rcl_bind (em_scene_bindings hands the
  camera pool's bytes).
- **001E1760** (C) and **001E17E0** (NEARMISS, VU0; register choices only,
  the original instructions were followed) in `em_grid_water` (header:
  every function's behaviour), with 001E1760's callees 001E10A0 (C) and
  001E13E0 (NEARMISS, one register move; behaviour as the C). 001D6E60,
  001D1F80, 001D1FF0 and 001D1F20 are em_load_veil_particles' (R.veil),
  001D6F60 em_static_world's, rand the shared em_random. The two VU0
  sequences 001E17E0 shares with the far light (clip test, projection) are
  em_far_light_vu_clip / em_far_light_vu_project (test_far_light_reference
  re-passed).
- **vf23**: 001E17E0 reads it as its caller left it (the fog lane). No
  function between 001E0CF0's entry and 001E17E0 writes vf23 (their
  original instructions read it only as a source), so it is the value
  measured at 001D72D0 in the same frame (Far light): ctx +0xA0.
- **Evidence**: `make test-grid-water-reference`
  (tools/test_grid_water_reference.py): the original instructions of
  001E1760 (with 001E10A0 / 001E13E0 running as original code), 001E10A0,
  001E13E0 and 001E17E0 over designed states (surface tables from small to
  clamping magnitudes and signed zeros; camera and clip matrices that put
  the grid inside, across and outside the clip volume and behind the
  camera; fog ramps); rand and 001E1760's translated callees scripted on
  both sides. Compared: v0, every byte of the windows (no original store
  outside them), the scripted calls with their arguments, and the VU0
  registers vf1 to vf2 and vf23 to vf31, ACC, Q, CLIP (unchanged by the
  scalar ones). Quick 40 cases,
  EM_TEST_FULL=1 240. Mutants caught: the 0.961 damping (one ulp), the
  laplacian's height operand, the rand shift, the clip flag bit, the wrap
  column's x 0x8800, the last row's y bias, the grid height 10 (one ulp);
  equivalent: the laplacian's starting zero's sign.
- **001E67C0 in AREA21** (em_snow_runtime): the area switch's key 0x1500
  sets the count D_002551F0 to 6; with camera top mode 3 (D_008101E4) the
  scene id D_0081024E 0x1F returns before any fog call or tile, 0x1E sets
  3, 0x20 sets 9 (decomp C, byte-identical). The reversed row (flags & 2)
  still faults; AREA21's weather flags in g75..g79 (0x22070001,
  0x30070010) do not set it.
- **Background reader** (em_static_world_live, renderer side): a DIRECT
  packet in 001E1E60's list that is not an A+D one (the far light's sprite,
  001D75E0) is skipped instead of refused.

Chain g20 (card g73): g73 4191 / 4192 compared rows on the row (heading
0.0012 off from row 1130, as before), g74 3736 / 3736, **g75 3864 / 3864
to its last row 4246**, g76 1346 / 1346 to row 1372, where the Use press
reaches 001AAC00 (area 0x15's classifier, `use_classify` in
em_player_closure_live: unbound, fail-stop). Next: 001AAC00 (C) over the
class-4 list D_00275B7C / D_00275B84 (gate D_0028A9A0) with its callees
001AA8D0 (C: the 0x4A placement test, returns 2 or 3 with a point and an
angle) and 001AAA90 (C: the 0x4B mount test through the object's matrix
+0xD0, returns 1), their 0011E748 / 0011DF78 / 001B1470 / 001026A0 (the
distance argument is MULA then MADD in both), then the state the press
starts. In the take the press was a placement: at row 1374 x goes from
134.00 to 133.50 (an object at x 146.1 or 120.9, 12.6 off: 001AA8D0's
point), then from about row 1450 y rises (35.01 to 39.80): a climb.

## Use and the boss effects (lane PORT-A21-USE, 2026-11-18, branch `a21use`)

- **001AAC00** (area 0x15's Use classifier) with **001AA8D0** (the 0x4A
  placement) and **001AAA90** (the 0x4B mount), all byte-identical C, in
  `em_area21_use` (header: every function's behaviour). The two distance
  sums are MULA then MADD (em_ee_madd_bits). Bound in the runtime's AREA21
  dispatch (`em_area01_runtime_second_half.inc`, the four callees re-enter
  it by address) and reached through `em_area01_live_classify`: the Use
  dispatcher's player record is borrowed read-only in the player stage, so
  the outputs (+0x290..+0x29F, +0x218) land in call locals and are copied
  to the record after the call (the original stores them only on a
  non-zero result, every word, and nothing in the call reads them back).
  `em_player_closure_live_set_classify` (em_scene_bindings: the later
  areas' binding) replaces the fail-stop.
- **The g76 press was not a placement.** With the classifier bound, both
  Use presses of g76 return 0 (no 0x4A / 0x4B object in range) and the
  dispatcher goes on to its ledge path: the port climbs exactly as the take
  (g76 6383 / 6383 rows on the row). The earlier "x 134.00 -> 133.50 at
  001AA8D0's point" reading was a coincidence of numbers (a label is not
  evidence). 001741D0 (+5 0x23, the placement / mount state) is still not
  translated: no take reaches it so far.
- **001EB440 / 001EB250** (two effect handlers of the boss fight, g77):
  byte-identical C, one translation `em_effect_001EB440` (001EB250 also
  lowers the node matrix's translation z by 2 while the work block's +0x54
  is above 0.8), bound in em_effects_live's handler switch over the node's
  work block views. Their 001CFBE0 sources D_00255B30..D_00255CE0 are
  exported by export_area00_runtime_tables.py AREA21_EFFECT_WINDOWS.
- **001B6660 in AREA21** (the deferred spawn of [43] / [44] 0x8262D0):
  em_scene_bindings' area_spawn_deferred with module id 0x12 over the data
  window 0x82A380..0x82DC80.
- **Evidence**: `make test-area21-use-reference` runs
  tools/test_area21_use_reference.py (the original instructions of the
  three Use functions over designed lists: gate values, counts 0..4, tags,
  kinds 0x4A / 0x4B / others, fabs and sqrt results at and one ulp past
  3.0 / 3.5 / 4.0 / 8.0, far players; hooks 0011DF78 / 0011E748 / 001B1470
  / 001026A0 scripted on both sides; compared: result, hook calls with
  argument bits and 001026A0's vector bytes, low 8 MB of RAM and the
  scratchpad, the fault contract at every hook; quick 300 / full 4000
  cases, 258 of 258 reachable words, two dead argument moves named) and
  tools/test_effect_001EB440_reference.py (both handlers: seeds over the
  16-bit range, +0x54 at 0.8 and one ulp past, fault contract; quick 100 /
  full 1200, 234 of 234 words). Mutants caught: 12.6 one ulp, the 4.0
  compare (< for <=), the tag test, the scratch seed 8.0, the fraction bias
  one ulp, the seed step, the fraction shift, the second source, the 0.8
  limit (one ulp and < for <=), the lowered word. Equivalent: MADD written
  as MUL then ADD (both products are squares, so a saturated sum is the
  same).
- **Chain g20** (card g73): g73 4191 / 4192, g74 3736 / 3736, g75 3864 /
  3864, g76 6383 / 6383 (to its last row 6499), **g77 2859 / 2859 to row
  2858**, where [43] / [44]'s spawn starts a node whose behaviour is
  **0x824FE0** (decomp func_overlay_AREA21_00824FA0, 0xB18 bytes,
  byte-identical C since lane MATCH-OVL2): untranslated, fail-stop. So
  0x824FE0 is reached (AREA21_OVERLAY.md's "unreached" is wrong for it:
  its only reference is spawn data, not code). g76 also shows a rand()
  word difference at row 6386 (the frame before drew differently; positions
  stay on the row).

## The boss fight (lane PORT-A21-BOSS, 2026-11-18, branch `a21boss`)

- **0x824FE0** (decomp func_overlay_AREA21_00824FA0, byte-identical C) in
  `em_area21_groups` (its comment says what every mode does): the effect
  node 0014EC00's 001EFD20 starts when [43] / [44]'s deferred spawn runs.
  It watches 7 or 12 of its parent's child objects (+0x24's +0x110 slots
  through the tables 0x82B090 / 0x82B0B0), spawns 001EFD20 effects by its
  mode byte +5 when an object crosses a height, and queues one 001CFB50 /
  001CFBE0 packet per object (group 0x82B000). Its 001EFD20 result (the
  new node, whose +0x38 mode 10 clears) made the groups hook return a
  value (gen_area21_groups.py: 001EFD20 'u'; the other members ignore
  it); 001026A0 and 00102BB0 joined the hooks (run as original code in
  the oracle). Bound in dispatch_area21 (frame 0xF0, `sp` for the packet
  at sp - 0x60) and em_scene_bindings' AREA21 list.
- **0x824CF0** (translated and tested by lane PORT-A21 as "unreached"):
  also a 001EFD20 effect node of the fight, reached at g77 row 3024; bound
  the same way (frame 0xA0). A label is not evidence: "no static
  reference" never meant unreached.
- **001EB600** (byte-identical C; an effect handler of the fight, g77 row
  3622) in `em_effect_001EB440`: unless the work block's +0x54 is at most
  0, the node matrix's translation z (+0x38) falls by 3; the two fog
  calls; translation y (+0x34) = 20; the two passes with the sources
  D_00255D70 / D_00255E00 (export_area00_runtime_tables.py
  AREA21_EFFECT_WINDOWS widened to 0x360 bytes). Bound in em_effects_live
  beside 001EB440 / 001EB250.
- **Music cue 0xC** (export_streams.py MUSIC_EXTRA): [43] / [44]'s
  0x826310 calls 001FB0B0(0xC) at the end of script 0x82B5E0 (the fight's
  start); its sectors were not exported, so step H (001FB100) faulted at
  00112610 code 8 one frame after 0x824FE0 was bound. The stream fault
  was this separate gap, not a consequence of the 0x824FE0 stop.
- **Evidence**: `make test-area21-overlay-reference` (the groups oracle
  now runs 0x824FE0's original instructions over designed parent nodes
  with 32 child objects: every mode 0..4, 10, 11, 12, 0x63 and others,
  states 0..3 and above, both tables and the 7-entry table's mode 10
  reading past its end, the -40 / 50 / -10 / 20 / 46 / 10 bounds at and
  one ulp past, frames around multiples of 12, the 2.0 phase wrap one ulp
  either side, seeds, n 0 / 1 / -1, the 001EFD20 result 0 or a node,
  Scribbles from 001EFD20 / 001CFBE0 / 001CCF70 / 0021B9A0 / 00122BB8;
  memory accesses compared one for one; 609 of 609 reachable words) and
  `make test-area21-use-reference` (test_effect_001EB440_reference now
  covers 001EB600: +0x54 at +0, -0 and the least positive float; 358 of
  358 words). Mutants of 0x824FE0 caught: the 0.015 phase step (one ulp),
  mode 1's `<` as `<=`, mode 2's 0x80000059 entry, the 46 bound (one
  ulp), the seed step, the fire slot choice 7 / 19, the +20 on z instead
  of y.
- **Chain g20** (card g73): g73 4191 / 4192, g74 3736 / 3736, g75 3864 /
  3864, g76 6383 / 6383 (rand() word at row 6386 as before), **g77 4355 /
  4355 to row 4354**, the original's hit at row 3533 (95.0) matched by
  the port. At row 4354 the player enters +5 0x23 with +1F0 0x38 (the
  Gatling mount: the guided run's g77 mounts [62]) and 0015B130's
  state[0x23], **001741D0**, is unbound (fail-stop; the scene's fault
  address reads 00198AF0, the camera action of that frame, but the
  failing worker is the state callback). g78 .. g80 not reached.
- **g76's rand() word at row 6386** (diagnosed, not fixed): the port's
  word is the original's advanced by exactly one LCG step, so the port
  drew once more in row 6385. EM_RAND_TRACE over the chain names the
  extra call: the status pages' background step 0020A7A0
  (em_status_background_step, from the page render) drawing from the
  original row 6386 word itself, i.e. one draw after the original's frame
  ended. The page is open there (the take uses the status screen before
  the fight); 0020A7A0 draws only on some frames (its own timer in
  D_002655A0), so the port's background timer or the page's open / close
  frame is one frame off from the original's there. Positions stay on the
  row (the chain reseeds each row). Next: compare D_002655A0 with the
  take's rows around that page (EM_START_AT_STATUS_BG restores it only at
  the card).

## The Gatling seat (lane PORT-A21-GATLING, 2026-11-18, branch `a21gatling`)

- **001741D0** (0015B130's state[0x23], byte-identical C) in
  `em_player_running_jump` beside 001747F0: with +1F0 0x38 the Gatling
  seat (walk to +290 at 0.4 a frame along the blend of the approach and the
  record's facing, snap within 1.0, turn to +218 by 12 degrees a frame
  through 001B12B0, sit, wait for the dismount press D_00810E74 &
  0x70003B76 or +302, stand up, back to +5 0 through 00174A50(16)); any
  other +1F0 the 0x4A placement's step-on / step-off. Its leaves 001028D0,
  00102760, 00103200 and 001B12B0 run inside the module; new workers
  `row_request` (00174A50), `root_word0` (*(*D_00275B40 + 0)) and `buttons`.
  Bound as state[0x23] in em_player_closure_live.
- **The rest of the seated frame**, each the original's own: camera
  action 14 (00198AF0, em_camera_aim) now binds under the area runtime's
  host too (it was bound only outside it, so the seat's first frame
  faulted there: the "00198AF0" of the earlier stop was this, not the
  state callback); 001B5DC0 (the stick ring, em_effect_original_001B5DC0)
  and 00207F80 (the gauge bars into the slot-1 packets, em_sgp_00207F80)
  in the runtime's worker switch; the muzzle node 001F5040 is allowed in
  AREA21 (em_scene_bindings); 001F4010 routes to the revisit's ring-slot
  translation in AREA21; 001CEEE0 (the rounds' draw, byte-identical C: the
  D_002512E0 stores, then 001026A0 / 001CCF70 / 001CFA60 / 001CFBE0 by
  address) and 001B41F0 (em_area00_world's hit application, new view
  entry) in em_area01_runtime; the hit spray handler 001EE4E0 in
  `em_effect_001EB440` with a new exported window (0x2576C0, 0x1B0:
  export_area00_runtime_tables.py AREA21_EFFECT_WINDOWS).
- **Evidence**: `make test-player-running-jump-reference` (001741D0 joins
  the oracle as the entry `state23`: designed modes, steps 0..6 and past,
  seats around the 1.0 bound, headings within a turn step, pad words,
  +224 / +22C / +F / +302; every branch of 001741D0 both ways; the leaves
  run as original code; 857 of 7706 quick cases); `make
  test-stick-ring-reference` (new: 001B5DC0 over every low byte under six
  high patterns); `make test-area21-use-reference`
  (test_effect_001EB440_reference covers 001EE4E0: step at 0.02 and one
  ulp either side; 570 of 570 words). Mutants caught: the turn step (one
  ulp), the 1.0 bound (< for <=), the 0.4 lane, the 0x174 clip, the -0.2
  sign, the 0x38 test, the 001EE4E0 kinds, floor and a row value.
  Equivalent: swapping the blend's operands (t = 0.5); a 0.02 + 1 ulp in
  001EE4E0's ease survives the full sweep. Not oracle-tested: the
  runtime's 001CEEE0 composition (its callees are the existing
  translations) and the 00207F80 / 001F4010 / 001B41F0 routing.
- **Chain g20** (card g73): g73 4191 / 4192, g74 3736 / 3736, g75 3864 /
  3864, g76 6383 / 6383, **g77 5894 / 5894 to row 5893** (hit at row 3533
  matched). It stops at row 5893: 0014EC00's 001EFD90 starts an effect
  node whose behaviour is the overlay's **0x825B00** (decomp
  func_overlay_AREA21_00825AC0; listed above as "not translated, no static
  reference"): untranslated, fail-stop. g78 .. g80 not reached.

## Haze ring (lane PORT-A21-RING, 2026-11-18, branch `a21ring`)

The effect node behaviour **001E76A0** (a word of 001EF9D0's kind table;
the boss fight starts one at g77 row 5922, node 0x7BCB50) is translated in
`em_haze_ring` (header: every function's behaviour) from the decomp C:
001E76A0 and its ring **001E0AC0** (byte-identical C) and the sprite
**001E0750** (NEARMISS 99.75%, VU0; register choices only, the original
instructions were followed: x and y are scaled by Q, then w loses vf3.x
(the GPR the zero float went into), Q is taken again and only then z is
scaled). The node runs 300 frames (+0x20 from 300, state 3 at <= 0, then
001AFC10); each frame 001028B8 moves its point by +0xC0 and 001E0AC0 draws
64 screen-sampling sprites on an ellipse (75 by 30) around it on channel 3
(001D1F80 / 001D7510 / 001D1FF0 first, a closing tag, then 001CAAC0). Each
drawn sprite takes two rand() words (00122BB8) per vertex, 12 in all.

- **Binding**: `em_haze_ring_live` over the area runtime, from
  dispatch_area21 (em_area01_runtime_second_half.inc). Every hook re-enters
  em_area01_runtime_call by address; 001D7510 is newly exported
  (em_rcl_001D7510) and routed with 001D1F80 / 001D1FF0 in
  em_area01_rcl_workers; D_0026E9A0 and D_00253530 come from the boot
  constants; rand is the shared em_random.
- **Evidence**: `make test-haze-ring-reference`
  (tools/test_haze_ring_reference.py): the original instructions of the
  three functions (001E0AC0 running 001E0750, 001E76A0 running both;
  001CD370 and float_to_int as original code) over designed states (node
  states 0..3 and others, the timer at 1.0, one ulp either side, 0, -0;
  points inside, across and outside the clip volume and behind the camera;
  the ELF's corner pairs or designed ones; fog ramps over both bounds; rand
  words and sin / cos results true or extreme). Hooks scripted on both
  sides with arguments and stack pointers compared. Compared: every byte of
  the windows (no original store outside them), the calls, vf1, vf2,
  vf3.x / .y, vf23..vf31, ACC, Q, CLIP. Quick 154 cases (~4 s),
  EM_TEST_FULL=1 744; 363 of 363 words executed. EM_HAZE_RING_MUTANTS=1:
  12 of 12 caught (300 one ulp, the s jitter span one ulp, the t jitter
  base one ulp, the jitter scale, a corner offset index, the closing tag
  0x60, the ring's fy, the step count 64, the w reduction's lane, the timer
  compare < for <=, the fog max bound, the scale factor 8 one ulp).
- **vf23 (open item)**: 001E0750 reads it as the logic walk left it.
  Measured in the original (fork, hidden replay of g77's pad log from its
  start state, a watch table at every 001E0750 entry over rows
  5922..6281: 57600 calls, all from 001E0AC0, 900 rings, up to three rings
  a frame): constant within a ring; equal to the fog quadword ctx +0xA0 in
  676 rings, another owner's leftover (it looks like a matrix row, 184
  distinct values) in 224, in no fixed position within the frame. The port
  starts every ring from ctx +0xA0. vf23 shapes only the fog lane: ACC.w,
  vf2.w and the scratch word 0x7000360C (the packets take x, y, z only).
  Exact values need the last vf23 writer of the logic walk modelled.
- vf3.z / .w (the upper half of that GPR, never set by 001E0750) are left
  as they were.

Chain g20 (card g73): g77 now 12090 / 12090 compared rows on the row
(rand() word included) to row 12089, where 0x8266F0's 001BA1F0 faults at
0x8269F0 (record 0x82C340; em_area11 script host). g73..g76 as before.

Chain g20 after 0x823B40 (lane PORT-A21-3B40): g77 12864 / 12864 compared
rows on the row (to its end); g78 stopped at row 12749, where [45]
0x8266F0's script (record 0x82C740) runs the boot worker 001B7670 (the
0x70003B91 latch, em_area00_low's oracle-checked translation): the boot
script workers now serve AREA21 too (em_area01_script_workers' area gate).
Then g78 15530 / 15530 on the row; g79 578 compared, 429 on the row: the
rand() word differs at row 418, the position leaves at row 519 (0.8 away)
and the port stops at row 667, where 001869A0 (player equipment,
em_equipment_live) faults at address 0x7A6DE4. g80 not reached.

## Next

0. Next: **0x825B00** (g77 row 5893, the effect node 0014EC00 starts),
   then chain g20 on through g78 .. g80; the rand() word at g76 row 6386.
1. (done) 001741D0 and the seated frame's workers (The Gatling seat).
   (done) 001E1E60's flag-0x23 workers (Far light); (done) 001E1AD0's
   flag-0x22 path (Grid); (done) 001AAC00 with its tests, 001EB440 /
   001EB250 (Use and the boss effects); (done) 0x824FE0, 0x824CF0,
   001EB600 and cue 0xC (The boss fight). Next: **001741D0** (+5 0x23,
   byte-identical C; g77 row 4354 takes its +1F0 0x38 Gatling branch, a
   six-step table switch; the other branch is the 0x4A placement), most
   naturally in em_player_running_jump beside 001747F0 (its workers
   already cover 001749A0, 00178B90, 00175900, sin / cos / atan2 / sqrt
   and *(*D_00275B40 + 8)); new: 00174A50, *(*D_00275B40 + 0), the pad
   test D_00810E74 & 0x70003B76, 001028D0 / 00102760 / 001026A0 /
   00103200 into 0x700038A0..DC, 001B12B0, 001B1470. Then the extra
   0020A7A0 draw at g76 row 6385 (The boss fight, last item), then chain
   g20 on through g78 .. g80.
1b. 001D7410's Z read-back (Far light, "Not modelled").
1c. The haze ring's vf23 (Haze ring, open item): the logic walk's last
    vf23 writer before each ring.
2. Run the live owners of that recording through these oracles (as
   test_area14_overlay_reference's `live/` cases).
3. (done) 0x8243B0, [52], lane PORT-A21-52.
4. The SDK reach of 0x827040 (001FBC50 -> the sound library) and the
   ending: SECOND_HALF_AREAS.md section 6.
