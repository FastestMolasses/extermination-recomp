# The second half's sound against the original

Lane PORT-SND2, 2026-11-19. The second half's picture was compared with the
original's GS frame by the gspics lanes (DRAW_LIST_AUDIT.md section 4); this
does the same for the sound, over Claude's guided run (decomp
`build/guided_play/`, g01..g80), which the port plays tick-exact from the title.

## 1. The boundary: the EE sound state and the RPC 0x64 exchanges, tick by tick

Everything the game code decides about sound is on the EE: the SFX sequencer
and allocator (001152D8 and its callees: the track table D_0027E0C0, the voice
records D_0027CCC0, the cursor and serial D_0027F740 + 0x30 / + 0x34), the
stream lanes and cues (D_00282150..A7, the voice ring D_00281CF0, the delayed
cues D_00281F30, the looped-service tables D_00281B70 / D_00281C30), and the
command queue D_0027F7C0 that each field's exchange (001152D8's 001191F0(0x64,
...), the call at 0x001157B4) sends to the IOP driver: key-on / key-off masks,
pitch, volume words, ADSR, sample addresses, EON / NON, and the stream
library's commands (0x16 effect return, 0x40.., the lanes' reads). These are
game-code outputs, equal tick for tick when the port is the original, and the
fork samples them inline (watch tables), with no stops and no observer effect.
So this is the boundary used: exact and cheap (a window of 600 ticks costs
seconds on the fork and nothing in the port).

The mixed PCM was not compared. Below this boundary is the IOP driver and the
SPU2: the ENVX the IOP returns (whose sampling phase is hardware timing), the
voice rendering, reverb and the core mix, which SPU2_OUTPUT.md keeps open (no
permitted source for the core stage; the port's Original output keeps the
existing renderer). A PCM comparison only becomes meaningful where the
commands are equal, which this establishes first.

## 2. Tools

| Tool | Side | What |
|---|---|---|
| `EM_SOUND_DUMP` / `EM_SOUND_DUMP_AT` | port | per tick whose recorded engine counter is in a range: the tick log's `"snd"` object (now with `"tlr"`, the tracks' volume requests) and the fields' exchanges since the previous tick (`em_scene_bindings_sound_dump`, `em_stream_live_exchange_log`, `em_sfx_keep_exchange`). Comparison only; unset, nothing is read or written. Called from the replay tick (EM_INPUT_PLAY) and from a pad script's head hook (EM_START_AT_COUNTER0) |
| `tools/watch_takes.py --snd-dump TAKE:R0-R1,...` | port | the whole guided run from the title in one process, dumping the windows (quits after the last) |
| `tools/guided_chain.py --snd-dump TAKE:R0-R1,...` | port | the same from a chain's card start (only windows after the chain's first area load compare: a LOAD GAME restarts the music and leaves the history's voices and delayed cues behind) |
| `tools/capture_sound_original.py TAKE R0 R1` | original | replays the kept take on the fork (hidden, the run lock, Metal as the takes), watch tables at every loop top and at 0x001157B4; every loop-top row checked against take.rec (counter, position). Output `../Extermination/build/sndpics/orig/<take>/r<R0>_<R1>/` (a kept recording) |
| `tools/compare_sound_ticks.py PORT ORIG...` | both | pairs by engine counter and compares, per tick: lanes, cues, music clip, voice ring, D_00281B70 / C30, delayed cues, cursor and serial (by difference), every live voice record (17 fields), the reaper's ENVX test, the tracks' volume requests and each exchange (SFX commands through a voice map; stream commands in order) |
| `tools/test_sound_second_half_reference.py` | both | the pinned windows (quick: chain g20 to g73 row 2680; `EM_TEST_FULL=1`: every window from the continuous run) |

What the comparison normalises, and why (compare_sound_ticks.py's docstring
has the rules): voice numbers (00117428 starts at a cursor that the title's
sounds advanced in the original; the port's SFX side starts at the New Game's
registry load), so voices are mapped by the exchange's commands and the
key-on serial; the global serial and cursor by their difference; sample
addresses (the port names the sample) by a one-to-one map; EON / NON on the
mapped voices only; the stream voices' age and bank handle and the free
records (boot history). The ENVX words are compared as the reaper reads them
(`< 2`), not as values: the IOP samples the envelope at its own tick, whose
phase is hardware timing.

Reproduce (port root; macOS arm64, the fork x86_64 under Rosetta):

```sh
python3 tools/capture_sound_original.py g42_area18_area14 0 600     # ~10 s .. 3 min per window
python3 tools/watch_takes.py --check --root ../Extermination/build/guided_play g01_title_newgame \
    --snd-dump g42:0-600                                            # the port reaches g42 in ~1 h
python3 tools/compare_sound_ticks.py build/sndpics/watch_g01_title_newgame/port/sound.jsonl \
    ../Extermination/build/sndpics/orig/g42_area18_area14/r0_600
```

## 3. Results

Port at sndpics 6c334655 (all three fixes below) playing the whole guided run
from the title (`watch_takes.py --check --root .../guided_play g01_title_newgame
--snd-dump ...`); the original from each take's own start state on the fork
(every loop-top row on take.rec: counter and position). "Equal" means equal
at every paired tick after the normalisations of section 2.

| Window (take rows) | Area, what | Ticks paired | Equal at every tick | Differences (first counter) |
|---|---|---|---|---|
| g02 100..160 | AREA11 (first level), steps | 61 | every key | none |
| g12 16485..17085 | AREA15 arrival (load, scene, play) | 601 | cues, ring, D_00281B70 / C30, tracks' requests, **every exchange** (SFX and stream) | lane read phase 4 ticks (182585: a music refill read starts a tick early); D_00281F30 stale records (all 601, inactive); a voice reset a tick apart in the first 2 rows |
| g40 4000..4600 | AREA14 (play, booster) | 601 | cues, ring, B70 / C30, requests, **every exchange** | lane 4 + 4 ticks (412858); D_00281F30 stale; a voice reset a tick apart (412863, 3 ticks) and the allocator cursor after it (147 ticks) |
| g42 0..600 | AREA18 (play) | 601 | cues, ring, B70 / C30, 599 exchanges | the repeating sound 0x7E6 (two voices, every ~40 ticks) is reset a tick later in the port (419783) and its tracks freed a tick later (419803; requests 39 ticks); voice numbering follows; at 420285 two of a three-voice key-on find no voice in the port (the allocator with the previous voices still held): **two voices missing**; serial 47 ticks, voices 66 |
| g46 0..600 | AREA17 arrival (load, play) | 600 | cues, ring, requests until 438399 | the looped sound 0x411 starts on track 0 in the port, track 1 in the original (438399: 00119EA0's lowest free track; the original's track 0 still held), so D_00281B70 / C30 slots, the track's requests and the voices' owner differ from there (180 ticks); lane 4 ticks |
| g73 2440..2680 (chain g20 from card g73) | AREA20 arrival scene | 241 | every key but one | D_00281F30: the card start's (a LOAD GAME leaves the earlier areas' records behind) |
| g60 1334..1761, g73 2080..2680, g75 1884..2484, g77 1000..1600, g80 100..700 and 21000..21600 | AREA16 arrival, AREA20 arrival, AREA21 arrival and the Gatling phase, the ending scene and the credits | captured | | the port's continuous run had not reached them at hand-back (about 95 minutes from the title); `compare_sound_ticks.py` with section 2's command |

Before the fixes, every window above also differed in the tracks' requests
and the volume words of the player's steps (g02: 4865 / 4870 against 4878 /
4878; AREA20's arrival 3859 / 4089 against 4096 / 4096) and in the exchanges
after each area load (00116DB8's re-sends: AREA15 at 182486, AREA20 at
597785).

## 4. Fixes (this lane)

Three differences the original's code settles, each found by this
comparison and each first visible in every area (the first level included):

1. **The distance listener is the player record's +0xB0** (`em_render_frame.c`
   `sfx_d810360`). D_00810360 is not a copy of the hip: the player record
   sits at 0x008102B0, so D_00810360 *is* its +0xB0. Every sound the player
   record places at its own +0xB0 (001FBD50(p, ...): the steps, the stage's
   own sounds) is therefore at distance 0 in 001FBF50: full volume and
   centred, the track's request 0x1000 / 0x1000. The port read the pose
   host's hip there, a separate storage that differs from the record's +0xB0
   inside 0015BCF0 (where the steps play: (75, 190, 80.8) against the hip
   (75, 200.9, 80.8) at AREA20's arrival), so every step was a few per cent
   quieter and off centre: g02 AREA11 4865 / 4870 against the original's
   4878 / 4878, AREA20's arrival requests 3859 / 4089 against 0x1000 / 0x1000.
   While the live stage runs the listener now reads the record's +0xB0; the
   hip (or the position) stays the reader outside it.
2. **00116DB8's SFX-track arm** (`em_sfx_bank.c` `bank_volume`, the
   byte-matched decomp C). Each area load's bank chain calls 001199F0(handle,
   0x64), which leaves a pending volume in D_002819C0; the next tick, the
   first track of that bank to run re-sends every voice it owns its volume
   words (command 1). The port's driver never consumed D_002819C0, so those
   commands were missing (AREA15's arrival at counter 182486, AREA20's at
   597785). The words are the voice's current ones: the chain's 0x64 is the
   banks' own volume, so the re-sent words equal the key-on's, as the
   original's do. The swap of the pair's volume with the track's +0x6C is not
   modelled (the port keeps no +0x6C; the bank-sound test excepts
   D_002819C0's volume bytes as before).
3. **001FBF50 runs as the original computes it** (`em_sfx_request_words`,
   bound to `em_player_misc_001FBF50` with the SDK's own sine, square root,
   VU0 leaves and 001B1380 by `em_render_frame.c` `sfx_gains_001FBF50`). The
   host-float approximation (`sinf`, `atan2f`, `cosf`) left request words one
   off (the delayed cues' 3499 / 2782 against 3498 / 2781). All positional
   requests (001FBD50's plays, the looped service 001FC3C0, the boxes', the
   aim-fire break cue's and the effects' 001FBF50 calls) take the exact path;
   the host approximation remains only where the SDK math tables are absent
   (unit fixtures), counted at shutdown.

## 5. Open

1. **The IOP driver's tick phase on the SFX side (the reaper's input).**
   Every remaining voice difference above is one class: a voice's end (a
   sample's end, or a hard stop) reaches the EE's D_002817C0 one exchange
   apart, because the IOP driver ticks every 128 half-lines and the port's
   SFX clock starts at phase 0 at the New Game's registry load, while the
   original's follows the IOP's boot (SFX_SEQUENCER.md "Where the driver
   runs"; IOP_STREAM.md). The first level saw it once (a fence door voice,
   105 against 106 ticks). In the second half it reaches the game code: in
   AREA18 the repeating 0x7E6 is reset a tick late at every repeat, its
   tracks are freed a tick late, and at 420285 two voices of a three-voice
   key-on are refused by 00117428 (the previous instance still holds
   voices): an audible difference. AREA17's 0x411 lands on another track
   for the same reason (a track freed a tick apart). Not a rule the code
   shows: it is the IOP timer's phase, hardware timing (PORT_PROFILES.md).
   Proposed next step: fit the original's SFX-side tick phase from the
   captured reply timings (each sample end is a known function of the
   phase; the windows above hold dozens) and, if one phase reproduces all of
   them, start the port's SFX clock there (a decision for the user: it is a
   measured hardware phase, like EM_IOP_TICK_PHASE on the stream side).
2. **The music lanes' refill read phase** (lane bytes D_00282157 / 58: 4 to 8
   ticks per window, AREA14, 15, 17): a refill read starts a tick apart. The
   stream backend's IOP clock (counted from the boot) and the drive replay
   set when a refill is due; AREA20_PORT.md records the same family (the
   port's lane-0 refills a block behind at g73). The cues, the music clip,
   the voice ring and every stream command are equal.
3. **D_00281F30's inactive records.** The ten delayed-cue records hold the
   gains of the last cue each slot played, long after (cue -1). Slot 1
   (sounds 0x19F..0x1A1) holds 3499 / 2782 where the original holds 3498 /
   2781 in every window from AREA15 on: written before g12 by an overlay's
   001FC580 through `em_area01_audio_services` (its own exact 001FBF50,
   whose listener is that host's view of D_00810360). Inactive, never
   played; the write is not traced (not in a compared window).
4. **The original's never-keyed voice records hold bank handle 0** (and the
   stream voices' records too), the port's 0xFFFF (`em_sfx_driver_init`).
   The free records are left out of the comparison; 001195A8's scan reads
   +0x22 (em_sound_bank), so the difference is not proven harmless. Boot
   history (the title's sounds the port's driver does not key).
5. **The voice numbers, serials and cursor differ from the title on**: the
   original's title keys sounds through the driver; the port's SFX side
   starts at the New Game's registry load. Normalised (section 2).
6. **00116DB8's swap** of the bank pair's volume with the track's +0x6C is
   not modelled; nothing compared reads them.
7. **Not compared:** the mixed PCM and everything below the IOP command
   boundary (SPU2_OUTPUT.md).
