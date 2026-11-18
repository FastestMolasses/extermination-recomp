# The attract demo (001ACA20, 001ACE70, 001B5BC0)

Lane PORT-DEMO, 2026-11-18. When the title is left idle, the original plays a
gameplay demo driven by recorded pads. This file says what the code does, how
the port runs it, and how it was checked against the original.

## 1. What the original does

- **Title idle.** 001AC480 counts 1200 idle ticks, fades out and returns 3.
  001AC070 state 2 then alternates on its record's +0xE: the intro movie
  (state 1, 001AC3B0) and the demo (state 3). After a cold boot the intro has
  just played, so the first idle plays a demo.
- **Which demo.** The demo counter D_00275BD4 picks it. 001AC070 state 0 sets
  the counter to 0. Each time state 3 ends, the counter steps 0, 1, 2, then
  wraps to 0. Over a long idle the title plays demo 0, the movie, demo 1, the
  movie, demo 2, and so on.
- **001ACA20** (anim_frame_top_a, byte-matched C) runs once per tick from
  001AC070 state 3, over the title task's own record. Its state is +9.
  - State 0 runs 001AF2C0 (the New Game reset of the progress block), then
    sets D_00275BE0 = 2 and D_00275BD8 = 1, then loads module 3 with
    001FF080(0, 3).
  - State 1 waits for the busy byte. It then runs 00200830(D_0028A564) and
    001D19D0, and writes the demo's seeds:
    - demo 0: AREA06 sub 0 (D_00810700 = 6);
    - demo 1: AREA13 entry 8 (0x0D / 702 = 8), with item bytes C7E and C68 = 1,
      CAC = 10, equipment CA6 = 4, and progress bytes 774 / 7F4 / 79A = 0xFF;
    - demo 2: AREA03 sub 1, with C65 = 1, CA6 = 1 and CA8 = 40.

    All three demos set C61 = 1.
  - State 2 runs 001ADF50, the area load (the game task's own load). When it
    is done, it loads module 0x36 with 001FF080(0, 0x36). That module holds
    the recorded pads: the loader's slot 0x87, D_0028A6AC, points at them.
  - State 3 waits for module 0x36 to finish loading.
  - State 4, sub 0, seeds rand() with 0x45 (00122BA8) and clears 0x70003B68
    and the main-loop counter 0x70003B64. It then rebuilds the world as
    0x1AE040 state 0 does: 001AFCA0, 001AFCF0, 001B07C0(0) (placement:
    D_00275BE0 = 2 is not a loaded game), 001B6990, 001D19E0, 001C1DC0 and
    00199C50. Then come the fade-in 001AEE10(4, 0), 001C5C50 and 001D1EF0.
  - State 4, sub 1, runs once per tick:
    1. It returns 3 once the record counter +0x16 reaches 0xE10.
    2. 001B5BC0(n) overwrites the pad block with record n.
    3. +0x16 steps by one.
    4. It returns 2 when 001ACE70 is nonzero.
    5. It runs the world frame (001AE5E0, or 001AE6B0 by 3B8D).
    6. It returns 3 when the death request (B9) or the fade request (B8) is
       set and the fade D_0028A9A0 is 2.
  - Every state except 2 first tests the real held word D_00810E70 against
    0x9F0 (START, SELECT and the four face buttons). Any of those buttons
    returns 2. Sub 1 tests the real pad before 001B5BC0 overwrites it.
- **001B5BC0** reads record n of demo D_00275BD4 from the table at
  D_0028A6AC[0]. Each demo has 0x4800 bytes; each record has 5 bytes: the
  buttons (big-endian, active-low), stick x, stick y and the gait byte. It
  stores:
  - E72 = 0 for n = 0, else the previous record's buttons;
  - E70 = this record's buttons;
  - E76 = E74;
  - E74 = E70 & ~E72;
  - E64, E65 and E57 from the stick and gait bytes.
- **001ACE70** returns 3 when D_008106CE is set, else 2 when D_008106C5 or
  D_008106B0 is set, else 0.
- **The exit.** When 001ACA20 returns 2 or 3, 001AC070 runs 001FBC50,
  001D2880 and 001AEDB0(0) (full black), steps the counter, and goes back to
  the title (state 2).
- **The capture.** All three captured demos ran their full 0xE10 records and
  returned 3. A demo is about 60 s of play. It is not a movie.

## 2. The original capture

`tools/capture_attract_demo.py --demos 3 --snap` writes the capture to the
decomp's ignored `build/demo_ref/run1/`. It runs a hidden fork session from the
`slot01_title` state, with the pad released, under the run lock. The session
took 340 s and was shut down afterwards.

- `rows.jsonl` holds one row per main-loop top from each demo's start: the task
  record, fade, scratchpad words, D_00275BD4..BE0, the area block, the pad block
  D_00810E50..7F, the request block, the player's position, yaw and action, the
  rand() state and D_0028A6AC.
- `snap_<n>_sub1/` holds each demo's RAM at the start of sub 1.

What the capture shows for each demo:

| Demo | Starts at session frame | Area | Sub 1 starts at row | Rows |
|------|-------------------------|------|---------------------|------|
| 0 | 1278, after 1277 idle ticks | 06 00 00 | 354 | 3956 |
| 1 | 7970 | 0D 00 08 | 422 | 4024 |
| 2 | 14728 | 03 01 00 | 375 | 3977 |

D_0028A6AC = 0x0176EE80, which is module 0x36's data.

## 3. The port

- **Cores.** `em_sf_001ACA20` and `em_sf_001ACE70` are in
  `src/game/em_scene_task.c`. `make test-scene-task-reference` executes the
  original instructions of 001ACA20, with 001ADF50, 001AFCF0 and 001ACE70
  inside it. It checks every arm, the events, the trace, the final state and
  the fail-stop behaviour: 616 compared runs of 001ACA20, beside the existing
  cores.
- **001B5BC0.** `em_pad_record_001B5BC0` is in `src/em_input.c`.
  `em_frame_pad_record_001B5BC0` applies it to the step-C pad block (the one
  storage) and refreshes the native view. It has no original-instruction
  oracle of its own; the capture comparison below checks it record by record.
- **Workers.** These are in `src/game/em_scene_bindings.c`, and
  `em_scene_attract_001ACA20` runs the core over the title task's record.
  - New workers: 00122BA8 (em_random_seed), the D_0028A564 reader, 00200830
    and 001D19D0 (as 001AD1A0's), 001B5BC0 (it reads the loader's slot 0x87
    and module 0x36's bytes), and the 0x70003B64 store
    (em_frame_store_counter_70003B64).
  - 001AF2C0 and 001FF080 (0, 3), (1, 0) and (0, 0x36) accept the demo's
    states.
  - Sub 0 runs with s_entry_state 0 and sub 1 with s_entry_state 1, so the
    world workers bind as they do in the frame machine.
  - The crawl node 00188340 is now bound in AREA06 sub 0. Demo 0 crawls into
    a duct there.
- **Data.** `tools/export_module_loader.py` exports module 0x36 (DEMO_MODULES).
  The three areas were already in the pack.
- **Frontend** (`src/game/em_frontend.c`). `attract_tick` makes one 001ACA20
  call per tick (the first inside the ATTRACT event).
  - A result of 2 or 3 completes the title's state 3. Its exit runs 001FBC50
    and 001D2880, then em_startup's FADE_FULL.
  - The black overlay is not drawn while a live demo runs.
  - Test mode: `EM_STARTUP_TEST=attract`, with `EM_ATTRACT_EXIT_AT=N`,
    `EM_ATTRACT_DEMOS=N`, `EM_ATTRACT_DEMO_ID=N` (constructed) and
    `EM_ATTRACT_ALL=1`.

## 4. Comparison with the original

`tools/compare_attract_demo.py` compares port and original record by record.
For each record it checks the fed pad, the area block, the player's position
and yaw, the fade and the rand() state. Rows are aligned on the record
counter, because the loads run at host speed in the port.

| Demo | Equal records | Status |
|------|---------------|--------|
| 0 (AREA06) | 2429 of 3600 | Plays to its end and returns to the title. rand() first differs at record 2430, during the duct crawl. Yaw first differs at 2680 and position at 2767 (by 0.07). |
| 1 (AREA13) | 1436 of 1436 | At record 1436 the aim/fire composition faults: original 00185A10 reads 007AB1EF from inside 00183AC0. |
| 2 (AREA03) | 3600 of 3600 | Plays to its end and returns to the title. This was checked in a constructed run (`EM_ATTRACT_DEMO_ID=2`); each demo resets the progress block and seeds rand(), so the earlier demos do not change it. |

Demos 0 and 2 are live. Demo 1 keeps the stand-in: a black screen that
serves only the button exit, so an idle title never faults. Its
`attract_live` gate names the open item.

`make test-attract-demo` takes about 7 s. Demo 0 idles in, records 1..300 must
equal the capture, CROSS at record 300 exits, and the title must come back.
`EM_TEST_FULL=1` also plays demos 0 and 2 to their end.

## 5. Open

1. **Demo 1's fault.** Find what 00183AC0 reads at 0x7AB1EF in AREA13: a
   pool node or view that the aim/fire composition does not model. Then make
   demo 1 live.
2. **Demo 0's divergence.** rand() first differs at record 2430 in the crawl.
   Tabulate the original's rand() callers for records 2425..2435 with a hidden
   fork replay that has log probes on 00122BB8 (CLAUDE pitfall: "a port drift
   that looks like wrong data is usually a missing input").
3. **No oracle for 001B5BC0.** It has no original-instruction oracle of its
   own. The capture's pads equal the port's on every compared record.
