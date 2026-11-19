/* em_sight.h - the weapon sights the aim camera's sight switch 001999C0
 * draws (docs/CAMERA_LIVE.md section 7, docs/AIM_FIRE.md). Prefix em_sight_.
 *
 * Which drawers run (001999C0, by the equipment selectors): D_00810CA4 0,
 * the DELTA auto sight (item 0x0A): 001DB9D0, 001DBF00, 001DC020 (zoom);
 * D_00810CA4 1, the interceptor unit (item 0x0B): 001DB9D0, 001DC610,
 * 001DC890 (zoom); D_00810CA4 2, the missile launcher (items 0x0C..0x0E):
 * 001DB830, 001DBE20, 00199770 (player); otherwise D_00810CA7 8, the zoom
 * scope (item 0x08): 001DB9D0, 001DC960, 001DBCB0 (D_00810248); D_00810CA7
 * 9, the night vision scope (item 0x09): 001DB830, 001DBD50. Every drawer
 * gets a0 = 1 (render-context slot 1, the 2D layer); the f12 the switch
 * passes is 1.0 except where a parameter is named below.
 *
 * Translated here, from the original instructions (the decomp's C is
 * byte-matched for all but 001DB9D0, a NEARMISS: its instructions were
 * followed; they differ from the C only in register colouring):
 *
 *   001DB830(slot, a1)  the scope mask: four 001D64A0 sprites over the
 *                       field's edges, colour quadword D_00253240 with its
 *                       fourth word = a1 (0xFF from the switch)
 *   001DB9D0(slot, dt)  the vignette ring: one 0x870-byte packet at the
 *                       slot cursor (33 vertex pairs of a strip, the radius
 *                       768 - 568 dt rotated by pi / 32 per step by a
 *                       second-order recurrence), templates D_00253250..7F
 *   001DBCB0(slot, z)   the zoom scope's zoom bar: one 001D64A0 sprite whose
 *                       right edge is 29056 + 2400 z (D_00810248)
 *   001DBD50(slot)      the night vision scope's frame: 001DCD40 and
 *                       001DCF40 in 0x80B9E1A5, three text runs
 *   001DBE20(slot)      the missile launcher's frame: 001DCD40, two
 *                       rectangles, two text runs (0x8091CDFF)
 *   001DBF00(slot)      the DELTA auto sight's frame: 001DCFF0, three
 *                       rectangles, three text runs (0x801996FF)
 *   001DC020(slot, z)   the DELTA auto sight's two zoom boxes (eight
 *                       2-pixel bars; t = 13/23 + 10/23 z)
 *   001DC610(slot)      the interceptor unit's frame: 001DCFF0, four
 *                       rectangles, four text runs, four 002081A0 line
 *                       packets from the templates D_00253290..D_00253350
 *   001DC890(slot, z)   the interceptor unit's zoom ring (002082B0, radius
 *                       142 (0.5 + 0.5 z), width 3)
 *   001DC960(slot)      the zoom scope's frame: 001DCFF0, 001DCF40, 14
 *                       rectangles, nine text runs, one 002081A0 line packet
 *                       (D_00253390)
 *   001DCD40(slot, c)   001D1F80(slot, 0, 1) and 16 corner bars in colour c
 *   001DCF40(slot, c)   001D1F80(slot, 0, 1) and four tick bars in colour c
 *   001DCFF0(slot, c)   the fixed scope ring (radius 200..203, colour c's
 *                       bytes as floats), after 001D1F80(slot, 0, 1)
 *   00199770(player)    the missile launcher's lock marks: for each class-2
 *                       list entry (D_00275B90, D_00275B98 of them) whose
 *                       +0 byte is set, which 00183B80 accepts and whose
 *                       +0x34 is set, its position (00183C40) within 360 of
 *                       the player, projected by 0x70003AC0 inside 180 x 90
 *                       of the centre: the reticle 001DD170(1, 0x700038B0,
 *                       1, 0x80808080, distance)
 *   0022E7F0(player)    the radar of D_00810CA5 == 6 (the UPPER U.R.S., item
 *                       0x06; 001999C0's tail and 00197D20's R1 state 2):
 *                       00207D00(1, 0); 0022E630(player, e) for each entry
 *                       of the radar list (D_00275B54, D_00275B58 of them)
 *                       whose +0 byte is set; the heading fan: 0x700036E0 =
 *                       diag(0.8, 1, 0.5, 0) turned by the wrapped angle
 *                       between the flattened row +0x90 of the player's
 *                       node *(player + 0x12C) (its +0x110 word 7) and the
 *                       player's +0xF0, 0x700036A0 = its
 *                       product, 001031E0(0x700036D0, (96, 0, 340, 1)), the
 *                       corners D_0026A990/A0/B0 turned by it and scaled to
 *                       16 (1792 + x), 16 (1936 + z / 2), 00208040(1, the
 *                       three, 0x8000FFFF); then 002082B0(1, D_0026A9C0 /
 *                       D_0026AA20 / D_0026AA80)
 *   0022E630(player, e) one blip: e's +0xB0 / +0xB8 within 200 of the
 *                       player's +0xA0 / +0xA8, turned by 001B1470(2 pi -
 *                       (pi + player +0xC4)) into 0x700038B0, the sprite
 *                       00207E40(1, 16 (1888 + 0.39375 x), 16 (1936 + (340
 *                       + 0.45 z) / 2), 8, 8, 0x80008080, tex0
 *                       0x20041605113222CD)
 *   002082B0(slot, rec) the ring builder: one packet at the slot cursor,
 *                       a strip of 16-degree columns between rec's two
 *                       radii, colours ramped through 0x70003600..0x7000363F
 *   002081A0(slot, buf, prim, n, rgba)  the line-list builder: one packet
 *                       of n vertices, each buf entry's first two words
 *
 * The leaf callees go through `call` by original address with the
 * original's argument registers (a[0..6] = a0..a3, t0..t2): 001D1F80,
 * 001D64A0, 00207F80, 001CBA50, float_to_int 001281C0, 0011E2A8, 0011DE90,
 * 00102948, 00102850, 001028B8, 001028D0, 001026A0, 00183B80, 00183C40,
 * 0011E748, 0011DF78, 001DD170, and for the radar 00207D00, 00207E40,
 * 00208040, 001029C0, 00102760, 00102BB0, 001026D0, 001031E0, 0011E620
 * (f12, f13), 001B1470. Calls between the functions above are
 * direct. The module never simulates a callee.
 *
 * Memory: every load and store goes through `map` by original address, the
 * caller's stack included: each function's locals sit at its original
 * sp-relative offsets below the caller's sp (EmSight.sp for an entry), and
 * the pointers the callees get name them (001D64A0's two 3-word groups and
 * its colour quadword, 001CBA50's style record, 002082B0's 0x60-byte ring,
 * 002081A0's buffers, 00102948's quadwords). The register saves have no
 * native representation, with one exception: 001DB9D0 stores the upper
 * doubleword of its $ra save (zero: the EE's jal writes only $ra's lower
 * doubleword, and nothing writes the upper one), because the next drawer at
 * the same depth (001DBF00, 001DC610, 001DC960) passes 001CBA50 a style
 * record whose bytes +6 / +7 it never writes: they are the top of that save
 * (001CBC20 ORs byte +7 into its vertices). After 001DB830 they are its
 * colour quadword's a1 bytes.
 *
 * Fail-stop (the EmAimFireTarget contract): a missing map or call (code 1),
 * an unmapped range (2) or a failed callee (3) latches the fault with the
 * root entry and the address; nothing runs after it and the entry returns
 * -1. A fault already latched makes every entry return -1 at once. A NULL
 * call faults only when a callee is reached.
 *
 * Verified by tools/test_sight_reference.py (the original instructions of
 * all sixteen over designed memory: every callee entry with its arguments,
 * stack pointer and the bytes behind its pointers, in order; RAM, scratchpad
 * and the stack's locals at the end; every reachable instruction word
 * executed). stdint only; float arithmetic em_ee_float.h, operands in the
 * instructions' order. */
#ifndef EM_SIGHT_H
#define EM_SIGHT_H

#include <stdint.h>

#include "game/em_aim_fire_target.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef EmAimFireTarget EmSight;

/* The functions by original address; f12 arguments are binary32 bits.
 * Each returns 0, or -1 on a latched fault. */
int em_sight_001DB830(EmSight *h, int32_t slot, uint32_t a1);
int em_sight_001DB9D0(EmSight *h, int32_t slot, uint32_t dt);
int em_sight_001DBCB0(EmSight *h, int32_t slot, uint32_t zoom);
int em_sight_001DBD50(EmSight *h, int32_t slot);
int em_sight_001DBE20(EmSight *h, int32_t slot);
int em_sight_001DBF00(EmSight *h, int32_t slot);
int em_sight_001DC020(EmSight *h, int32_t slot, uint32_t zoom);
int em_sight_001DC610(EmSight *h, int32_t slot);
int em_sight_001DC890(EmSight *h, int32_t slot, uint32_t zoom);
int em_sight_001DC960(EmSight *h, int32_t slot);
int em_sight_001DCD40(EmSight *h, int32_t slot, uint32_t colour);
int em_sight_001DCF40(EmSight *h, int32_t slot, uint32_t colour);
int em_sight_001DCFF0(EmSight *h, int32_t slot, uint32_t colour);
int em_sight_00199770(EmSight *h, uint32_t player);
int em_sight_0022E7F0(EmSight *h, uint32_t player);
int em_sight_0022E630(EmSight *h, uint32_t player, uint32_t entry);
int em_sight_002082B0(EmSight *h, int32_t slot, uint32_t ring);
int em_sight_002081A0(EmSight *h, int32_t slot, uint32_t buf, int32_t prim, int32_t count, uint32_t rgba);

/* A call record of one of the sixteen (function, a[], f[0] as the original
 * passes them; h->sp is taken from c->sp): runs it. 1 when the function is
 * not one of them (nothing done), else as the entry. */
int em_sight_call(EmSight *h, const EmAimFireTargetCall *c);

#ifdef __cplusplus
}
#endif

#endif /* EM_SIGHT_H */
