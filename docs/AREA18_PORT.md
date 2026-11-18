# AREA18's overlay (lane PORT-A17A18)

Lane PORT-A17A18, 2026-11-17, branch `a17a18`. AREA18 (area byte 0x12,
overlay id 0x0F, `OVERLAY/AREA18.BIN`) is the transport plane entered from
AREA14 by 00190F20 and left back to AREA14 entry 1 the same way: item 0x28
(g[1], which AREA17's [34] takes), the panel [21] and one room move [18]
(SECOND_HALF_AREAS.md section 4).
Every behaviour its placements and groups name is a boot function; the
overlay holds only the entry pad and the area init.

## The overlay has nothing more to translate

- **The init** at 0x823580 (decomp `overlay_AREA18_func_00823540.c`) is
  em_area01_state's: 001E7780's key 0x1200 runs it, and
  `make test-second-half-state-reference` runs the ORIGINAL init over
  AREA18's own overlay file and compares the six globals, the data and the
  whole BSS byte for byte (its AREA18 row).
- **Nothing else**: `make test-area18-overlay-reference`
  (`tools/test_area18_overlay_reference.py`, under a second) checks against
  the user's file that the header equals the state test's AREA18 row, that
  the text's only nonzero words are the init's 8 words (one return; the pad
  and the tail are zero), and that no header or data word points into the
  text; and, in the two guided-run states with AREA18 loaded (decomp
  `build/guided_play/g41_area14_area18` and `g43_area14_area18`, end.p2s,
  extracted once into ignored `build/area18_overlay/states/`), that the
  resident header and text equal the file, D_00810700 is 0x12, and no live
  pool node's behaviour lies in the overlay text (68 / 69 live nodes, 21 boot
  behaviours).

## Not done

AREA18's boot behaviours (section 4: 0012A5D0, 00147390, 001551B0, 00159210,
00159970, 00159B90, 0015AFA0, 001BB860, 001C4820, 001E3D90, 00219550) are
each in a census ledger or defined by an earlier lane; none has run in
AREA18. AREA18 has no `SUBS` row, asset export, start card or live binding
(no script chain, no message table). The guided run's takes g41 .. g44
(AREA14 -> AREA18 -> AREA14, twice) are the recordings to bind it from.
