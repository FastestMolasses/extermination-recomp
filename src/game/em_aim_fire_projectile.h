/* em_aim_fire_projectile.h - the sub-weapon's guided projectile: the pool
 * behaviour 0018AF50 that 001869A0 (the equipment one-shot of fire modes
 * 1 / 2) gives the class-1 record it allocates (model byte 3, +0xB0..+0xB8
 * and +0xC0..+0xC8 from the gun, +0x24 the locked target D_008106E0 when
 * D_008104E0 is 0xC and D_00810CA4 is 1, else 0).
 *
 * 0018AF50(node), by its state +4:
 *   0  +4 + 1; +0x28 = 0x32; +0xA0 = +0xC0 * 6 (00103230), +0xA0 = +0xB0 -
 *      +0xA0 (001028D0), +0xC0 *= 4 (00103230); +0x20 = 001EFE00(0x80000037,
 *      node) (the smoke trail); +0x38 = 0; +0x2A = 0x12C.
 *   1  when the byte 0x70003B8D is set: the trail's +4 = 2, +4 = 3. Else
 *      the halfword +0x2A counts down (at 0: +4 = 2). By +5:
 *      0  the halfword +0x28 counts down; when it reaches 0, +5 + 1 and,
 *         when +0xC4 < 0, +0x38 = 0011DF78(+0xC4). With a target +0x24:
 *         0x700038B0 = its point (00183C40), 0x700038A0 = the unit vector
 *         from +0xB0 to it (001028D0, 00102760), +0xC0 = the unit blend
 *         0.2 of it with +0xC0 (00103200, 00102760) times 4 (00103230).
 *         Then +0xB0..+0xB8 += +0xC0..+0xC8.
 *      1  +0xB0 += +0xC0 / 3, +0xB8 += +0xC8 / 3, +0x38 = min(+0x38 +
 *         0.03, 4), +0xB4 -= +0x38 (the fall).
 *      Then the segment +0xA0 -> +0xB0 against the grid (0019A570(.., 7,
 *      0x20)): a hit puts the node at the hit point (0x700031B0, w 1.0;
 *      001031E0, 00102948) and, for hit kinds 1 and 2, the record
 *      *0x700031D4 (when set and its +0 is not 0): class 2 with +0 == 1
 *      takes 001B41F0(rec, 0x700038A0, +0xC0, (*0x700031D0)+0x1C, 0x2000,
 *      0x50); another class gets +0x36 = 0x50 and +0x70 = +0xC0; +4 = 2.
 *      No hit: 0019B6C0 (the actor-less probe); a hit puts the node at
 *      0x700031B0, a water surface ((*0x700031D0)+0x1A == 0x5B) ripples
 *      (001E8B90(+0xB0, 4.0)), +4 = 2. Last: +0xA0..+0xAC = +0xB0..+0xBC;
 *      0x700038A0..AC = (0, 0, 0, 1.0); +0xD0 = build_trs_matrix(+0xB0,
 *      0x700038A0, 0x700038A0) (001C94B0).
 *   2  +4 + 1, the trail's +4 = 2, 001EFD20(0x80000069, +0xB0) (the
 *      blast), 001FBD50(node, 0x5DE, 0, 300.0).
 *   3  001AFC10(node) (the free).
 * Read from the original instructions (the decomp's C is byte-matched).
 *
 * Memory by original address through the EmAimFireTarget host (map / call /
 * store), as em_aim_fire_marker; every callee goes through `call`.
 * Fail-stop as there. Verified by tools/test_aim_fire_projectile_reference.py.
 * Bound by em_aim_fire_runtime beside the marker (AIM_FIRE.md section 3). */
#ifndef EM_AIM_FIRE_PROJECTILE_H
#define EM_AIM_FIRE_PROJECTILE_H

#include <stdint.h>

#include "game/em_aim_fire_target.h"

#ifdef __cplusplus
extern "C" {
#endif

int em_aim_fire_projectile_0018AF50(EmAimFireTarget *h, uint32_t node);

#ifdef __cplusplus
}
#endif

#endif /* EM_AIM_FIRE_PROJECTILE_H */
