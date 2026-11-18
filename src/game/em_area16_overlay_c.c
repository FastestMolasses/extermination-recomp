/* AREA16's own overlay owners (em_area16_overlay.h), group c: 0x825800
 * (sub 0's placement owner [10], the player's height mover), its helper
 * 0x826A50 and the op09 callback 0x826C20 of its scripts. Each function is
 * one original function; the comment above it says what the code does. The
 * order of the memory accesses between calls is the original's (the test
 * compares it one for one). Ground truth: the decomp's byte-identical C
 * under src/overlays/AREA16/; evidence:
 * tools/test_area16_overlay_reference.py. */
#include "em_area16_overlay_internal.h"

/* globals */
#define D_008102B5 0x008102B5u   /* a byte the height moves compare with 1 and 2 */
#define D_00810350 0x00810350u   /* the player's position x */
#define D_00810354 0x00810354u   /* ... y */
#define D_00810358 0x00810358u   /* ... height */
#define D_00810360 0x00810360u   /* a vector copied to D_008105E0 / D_00810200 */
#define D_00810200 0x00810200u
#define D_008105E0 0x008105E0u
#define D_008103C0 0x008103C0u   /* three node pointers (their +0xC0 / +0xC4 is a place) */
#define D_008103CC 0x008103CCu
#define D_008103DC 0x008103DCu
#define D_0081078B 0x0081078Bu   /* 0xFF: the height move is done */
#define D_0081080B 0x0081080Bu   /* AREA16's event bits */
#define D_700031F0 0x700031F0u   /* scratchpad: "height moved this frame" */
#define D_70003B68 0x70003B68u   /* scratchpad: the frame counter */
#define D_700038A0 0x700038A0u   /* scratchpad: a 4-float vector argument */

/* overlay data: the original's own pointers (these four are where the
 * decomp's data symbols name them, no 0x40 shift; the oracle shows them) */
#define A16_GROUP_828C90 0x00828C90u   /* the 001B6660 record (decomp D_overlay_AREA16_00828C90) */
#define A16_SCRIPT_82A1C0 0x0082A1C0u  /* the script of state 0xC9 (decomp D_overlay_AREA16_0082A1C0) */
#define A16_SCRIPT_82A3C0 0x0082A3C0u  /* the script of state 0xC8 (decomp D_overlay_AREA16_0082A3C0) */
#define A16_PLACE_82A5E0 0x0082A5E0u   /* two vectors filled at frame 79 (decomp D_overlay_AREA16_0082A5E0) */

/* float constants (bit patterns) */
#define F_0 0x00000000u
#define F_HALF 0x3F000000u
#define F_MHALF 0xBF000000u
#define F_1 0x3F800000u
#define F_2 0x40000000u
#define F_MPI2 0xBFC90FDBu     /* -pi/2 */
#define F_M0_8 0xBF4CCCCDu
#define F_0_81 0x3F4F5C29u
#define F_0_18 0x3E3851ECu
#define F_0_1 0x3DCCCCCDu
#define F_M0_055 0xBD6147AEu
#define F_M1_35 0xBFACCCCDu
#define F_M8_4 0xC1066666u
#define F_19 0x41980000u
#define F_46_8 0x423B3333u
#define F_M120 0xC2F00000u
#define F_188 0x433C0000u
#define F_200 0x43480000u
#define F_205 0x434D0000u
#define F_297 0x43948000u
#define F_300 0x43960000u
#define F_304 0x43980000u
#define F_311 0x439B8000u
#define F_400 0x43C80000u
#define F_430 0x43D70000u
#define F_452 0x43E20000u
#define F_455 0x43E38000u
#define F_458 0x43E50000u
#define F_459 0x43E58000u
#define F_466 0x43E90000u
#define F_472 0x43EC0000u
#define F_475 0x43ED8000u
#define F_480 0x43F00000u
#define F_4096 0x45800000u

/* ----------------------------------------------------------------------
 * 0x826A50 (decomp func_overlay_AREA16_00826A10) — (self, pos, speed):
 * nothing unless speed >= 0.5 (speed > 0) or speed <= -0.5 (speed <= 0).
 * Then, with the rotation v = (-pi/2, 0, 0, 1) at sp - 0x10 and the place
 * p at sp - 0x20: p = (D_008103DC's +0xC0, +0xC4, pos[2]) and
 * 001EFD90(0x8000001D, p, v); the same with D_008103CC; then p = the
 * midpoint of D_008103CC's and D_008103C0's places ((a + b) / 2), pos[2]
 * and once more 001EFD90. Last, sound 001FBD50(self, 0x926, 0, 300) when
 * speed >= 0, else 0x927. Frame 0x80 bytes.
 * ---------------------------------------------------------------------- */
static void a16_826A50(A16 *o, uint32_t self, uint32_t pos, uint32_t speed, uint32_t sp)
{
    const uint32_t frame = sp - 0x80u;
    const uint32_t p = frame + 0x60u;
    const uint32_t v = frame + 0x70u;
    uint32_t n, n2;
    if (!A16_LE(speed, F_0)) {
        if (A16_LT(speed, F_HALF)) return;
    } else if (!A16_LE(speed, F_MHALF)) {
        return;
    }
    a16_w32(o, v, F_MPI2);
    a16_w32(o, v + 4, F_0);
    n = a16_u32(o, D_008103DC);
    a16_w32(o, v + 8, F_0);
    a16_w32(o, v + 12, F_1);
    a16_w32(o, p, a16_u32(o, n + 0xC0));
    a16_w32(o, p + 4, a16_u32(o, n + 0xC4));
    a16_w32(o, p + 8, a16_u32(o, pos + 8));
    if (a16_c_001EFD90(o, (int32_t)0x8000001Du, p, v) < 0) return;
    n = a16_u32(o, D_008103CC);
    a16_w32(o, p, a16_u32(o, n + 0xC0));
    a16_w32(o, p + 4, a16_u32(o, n + 0xC4));
    a16_w32(o, p + 8, a16_u32(o, pos + 8));
    if (a16_c_001EFD90(o, (int32_t)0x8000001Du, p, v) < 0) return;
    n = a16_u32(o, D_008103CC);
    n2 = a16_u32(o, D_008103C0);
    {
        const uint32_t ax = a16_u32(o, n + 0xC0);
        const uint32_t bx = a16_u32(o, n2 + 0xC0);
        a16_w32(o, p, A16_DIV(A16_ADD(ax, bx), F_2));
    }
    {
        const uint32_t ay = a16_u32(o, n + 0xC4);
        const uint32_t by = a16_u32(o, n2 + 0xC4);
        a16_w32(o, p + 4, A16_DIV(A16_ADD(ay, by), F_2));
    }
    a16_w32(o, p + 8, a16_u32(o, pos + 8));
    if (a16_c_001EFD90(o, (int32_t)0x8000001Du, p, v) < 0) return;
    (void)a16_c_001FBD50(o, self, A16_LT(speed, F_0) ? 0x927 : 0x926, 0, F_300);
}

/* D_008105E0 and D_00810200 = D_00810360 (x, y, z loaded, then stored
 * pairwise). */
static void a16_copy_place(A16 *o)
{
    const uint32_t x = a16_u32(o, D_00810360);
    const uint32_t y = a16_u32(o, D_00810360 + 4);
    const uint32_t z = a16_u32(o, D_00810360 + 8);
    a16_w32(o, D_008105E0, x);
    a16_w32(o, D_00810200, x);
    a16_w32(o, D_008105E0 + 4, y);
    a16_w32(o, D_00810200 + 4, y);
    a16_w32(o, D_008105E0 + 8, z);
    a16_w32(o, D_00810200 + 8, z);
}

/* D_700038A0 = (x, y, 459) of the player and 0x826A50(self, D_700038A0,
 * speed) (the helper's frame below this owner's 0x40). */
static void a16_825800_splash(A16 *o, uint32_t self, uint32_t speed_from, uint32_t speed, uint32_t sp)
{
    const uint32_t x = a16_u32(o, D_00810350);
    const uint32_t y = a16_u32(o, D_00810354);
    a16_w32(o, D_700038A0, x);
    a16_w32(o, D_700038A0 + 4, y);
    a16_w32(o, D_700038A0 + 8, F_459);
    if (speed_from) speed = a16_u32(o, speed_from);
    if (a16_failed(o)) return;
    a16_826A50(o, self, D_700038A0, speed, sp - 0x40u);
}

/* ----------------------------------------------------------------------
 * 0x825800 (decomp func_overlay_AREA16_008257C0) — sub 0's placement owner
 * [10], the player's height mover. Frame 0x40 bytes.
 * State 0: when 001B0FD0(self) is 0: 001C6380(self); unless D_0081078B is
 *  0xFF: +0 = 1, state 1, 0019C6F0(0x28, 1); else state 2 and
 *  001B6660(0x828C90). Then +0x2EC = the height, +0x2E4 = +0x2E8 = 0,
 *  001CA5F0(self, 3), 001D8BF0(self, 1), +0x88 = +0x84 = +0x80 = -120,
 *  +0x8C = 0, 001DAFA0, 001DB240(0), +0x2D8 = -1.
 * State 1: 001DB250, 001DB480, the +0x4C method; with (D_0081080B & 7) > 4
 *  001FC520(+0x2D8), else 001FC3C0(self, +0x2D8, 0x924, 200, 4096). When
 *  !(+0x2EC < 459) and height <= 459: the splash with speed +0x2E4. When
 *  y < 205 and height > 400: below 430 height = 430 - (speed - 0.81) and
 *  D_700031F0 = 1; below 466 001DB240(1), then with D_008102B5 < 2 height
 *  -= speed - 0.81 and x moves 0.1 back into [297, 311], else height -=
 *  speed - 0.18, and D_700031F0 = 1; else 001DB240(0). Then +0x2E8 =
 *  +0x2EC - height, speed = (+0x2E8 + speed) / 2, +0x2EC = height.
 *  Then with (D_0081080B & 7) >= 3: 001EFD20(1, D_700038A0) unless bit 2,
 *  001DB240(3), sound 0x929 when (bits & 7) < 4, state 0xC8, +0x28 = 0,
 *  +5 = 0, 0019C6F0(0x28, 0), D_0081080B |= 4, and below height 475
 *  001BA1A0(+0x1F0, script 0x82A3C0) else +5 = 1. Otherwise below height
 *  455, when speed < -0.055 with D_008102B5 == 1, or speed < -1.35: state
 *  0xC9 and 001BA1A0(+0x1F0, script 0x82A1C0).
 * State 0xC9: below height 472 (the splash with speed -0.8 when height is
 *  in [458, 459]) height += 1; when 001BA1F0(self) is set state 1 and
 *  001DB240(0), else the D_00810360 copy; then 001DB250, 001DB480, method.
 * State 0xC8, +5 0: below 480 D_700031F0 = 1 and height += 1; when
 *  001BA1F0 is set +5 = 1, +0x36 = 0; else ++(+0x28) and below 80 the
 *  copy, and at 79 0x82A5E0 = (x, y + 19, z + 46.8), 0x82A5F0 = (x, y, z)
 *  of D_00810360. +5 1: when +0x36 is set state 2, D_0081080B =
 *  D_0081078B = 0xFF, 001B6660(0x828C90), sound 0x92A, +0x2E0 = 452,
 *  +0x2DC = 5. Any +5: 001B1B70(self), 001DB250, 001DB480.
 * State 2: while +0x2DC > 0: on frames with (D_70003B68 & 7) == 0:
 *  D_700038A0 = (304, 188, +0x2E0, 1), 001EFD20(0x8000003F, D_700038A0),
 *  001EFD20(3, D_700038A0), +0x2DC -= 1, +0x2E0 += -8.4; then while
 *  +0x2DC > 3: 001DB250, 001DB480, method.
 * State 3: 001AFC10(self).
 * ---------------------------------------------------------------------- */
static void a16_825800(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t blk = self + 0x1F0u;
    int32_t r = 0;
    uint32_t ignored = 0;
    uint32_t z, speed, bits;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r != 0) break;
        if (a16_c_001C6380(o, self) < 0) return;
        if (a16_u8(o, D_0081078B) != 0xFFu) {
            a16_w8(o, self, 1);
            a16_w8(o, self + 4, 1);
            if (a16_c_0019C6F0(o, 0x28, 1) < 0) return;
        } else {
            a16_w8(o, self + 4, 2);
            if (a16_c_001B6660(o, A16_GROUP_828C90, &ignored) < 0) return;
        }
        a16_w32(o, self + 0x2EC, a16_u32(o, D_00810358));
        a16_w32(o, self + 0x2E4, 0);
        a16_w32(o, self + 0x2E8, 0);
        if (a16_c_001CA5F0(o, self, 3) < 0) return;
        if (a16_c_001D8BF0(o, self, 1) < 0) return;
        a16_w32(o, self + 0x88, F_M120);
        a16_w32(o, self + 0x84, F_M120);
        a16_w32(o, self + 0x80, F_M120);
        a16_w32(o, self + 0x8C, 0);
        if (a16_c_001DAFA0(o) < 0) return;
        if (a16_c_001DB240(o, 0) < 0) return;
        a16_w32(o, self + 0x2D8, 0xFFFFFFFFu);
        break;
    case 1:
        if (a16_c_001DB250(o) < 0) return;
        if (a16_c_001DB480(o) < 0) return;
        if (a16_method(o, self) < 0) return;
        if ((a16_u8(o, D_0081080B) & 7u) > 4u) {
            if (a16_c_001FC520(o, blk + 0xE8u) < 0) return;
        } else {
            if (a16_c_001FC3C0(o, self, blk + 0xE8u, 0x924, F_200, F_4096) < 0) return;
        }
        if (!A16_LT(a16_u32(o, self + 0x2EC), F_459) && A16_LE(a16_u32(o, D_00810358), F_459)) {
            a16_825800_splash(o, self, self + 0x2E4, 0, sp);
            if (a16_failed(o)) return;
        }
        if (A16_LT(a16_u32(o, D_00810354), F_205)) {
            z = a16_u32(o, D_00810358);
            if (!A16_LE(z, F_400)) {
                if (A16_LT(z, F_430)) {
                    speed = A16_SUB(a16_u32(o, self + 0x2E4), F_0_81);
                    a16_w32(o, D_700031F0, 1);
                    a16_w32(o, D_00810358, A16_SUB(F_430, speed));
                } else if (A16_LT(z, F_466)) {
                    if (a16_c_001DB240(o, 1) < 0) return;
                    if (a16_u8(o, D_008102B5) < 2u) {
                        speed = a16_u32(o, self + 0x2E4);
                        z = a16_u32(o, D_00810358);
                        a16_w32(o, D_00810358, A16_SUB(z, A16_SUB(speed, F_0_81)));
                        uint32_t x = a16_u32(o, D_00810350);
                        if (!A16_LE(x, F_311)) a16_w32(o, D_00810350, A16_SUB(x, F_0_1));
                        x = a16_u32(o, D_00810350);
                        if (A16_LT(x, F_297)) a16_w32(o, D_00810350, A16_ADD(x, F_0_1));
                    } else {
                        speed = a16_u32(o, self + 0x2E4);
                        z = a16_u32(o, D_00810358);
                        a16_w32(o, D_00810358, A16_SUB(z, A16_SUB(speed, F_0_18)));
                    }
                    a16_w32(o, D_700031F0, 1);
                } else {
                    if (a16_c_001DB240(o, 0) < 0) return;
                }
                z = a16_u32(o, D_00810358);
                const uint32_t d = A16_SUB(a16_u32(o, self + 0x2EC), z);
                a16_w32(o, self + 0x2E8, d);
                a16_w32(o, self + 0x2E4, A16_DIV(A16_ADD(d, a16_u32(o, self + 0x2E4)), F_2));
                a16_w32(o, self + 0x2EC, a16_u32(o, D_00810358));
            }
        }
        bits = a16_u8(o, D_0081080B);
        if ((bits & 7u) >= 3u) {
            if (!(bits & 4u)) {
                if (a16_c_001EFD20(o, 1, D_700038A0, &ignored) < 0) return;
            }
            if (a16_c_001DB240(o, 3) < 0) return;
            if ((a16_u8(o, D_0081080B) & 7u) < 4u) {
                if (a16_c_001FBD50(o, self, 0x929, 0, F_300) < 0) return;
            }
            a16_w8(o, self + 4, 0xC8);
            a16_w16(o, self + 0x28, 0);
            a16_w8(o, self + 5, 0);
            if (a16_c_0019C6F0(o, 0x28, 0) < 0) return;
            bits = a16_u8(o, D_0081080B);
            z = a16_u32(o, D_00810358);
            a16_w8(o, D_0081080B, bits | 4u);
            if (A16_LT(z, F_475)) {
                (void)a16_c_001BA1A0(o, blk, A16_SCRIPT_82A3C0);
            } else {
                a16_w8(o, self + 5, 1);
            }
        } else if (A16_LT(a16_u32(o, D_00810358), F_455)) {
            speed = a16_u32(o, self + 0x2E4);
            if ((A16_LT(speed, F_M0_055) && a16_u8(o, D_008102B5) == 1u) || A16_LT(speed, F_M1_35)) {
                a16_w8(o, self + 4, 0xC9);
                (void)a16_c_001BA1A0(o, blk, A16_SCRIPT_82A1C0);
            }
        }
        break;
    case 0xC9:
        z = a16_u32(o, D_00810358);
        if (A16_LT(z, F_472)) {
            if (A16_LE(z, F_459) && !A16_LT(z, F_458)) {
                a16_825800_splash(o, self, 0, F_M0_8, sp);
                if (a16_failed(o)) return;
            }
            a16_w32(o, D_00810358, A16_ADD(a16_u32(o, D_00810358), F_1));
        }
        r = 0;
        if (a16_c_001BA1F0(o, self, &r) < 0) return;
        if (r != 0) {
            a16_w8(o, self + 4, 1);
            if (a16_c_001DB240(o, 0) < 0) return;
        } else {
            a16_copy_place(o);
        }
        if (a16_c_001DB250(o) < 0) return;
        if (a16_c_001DB480(o) < 0) return;
        (void)a16_method(o, self);
        break;
    case 0xC8:
        switch (a16_u8(o, self + 5)) {
        case 0:
            z = a16_u32(o, D_00810358);
            if (A16_LT(z, F_480)) {
                a16_w32(o, D_700031F0, 1);
                a16_w32(o, D_00810358, A16_ADD(z, F_1));
            }
            r = 0;
            if (a16_c_001BA1F0(o, self, &r) < 0) return;
            if (r != 0) {
                a16_w8(o, self + 5, 1);
                a16_w16(o, self + 0x36, 0);
            } else {
                a16_w16(o, self + 0x28, (uint32_t)(a16_s16(o, self + 0x28) + 1));
                if (a16_s16(o, self + 0x28) < 80) {
                    const uint32_t x = a16_u32(o, D_00810360);
                    const uint32_t y = a16_u32(o, D_00810360 + 4);
                    const uint32_t zz = a16_u32(o, D_00810360 + 8);
                    a16_w32(o, D_008105E0, x);
                    a16_w32(o, D_00810200, x);
                    a16_w32(o, D_008105E0 + 4, y);
                    a16_w32(o, D_00810200 + 4, y);
                    a16_w32(o, D_008105E0 + 8, zz);
                    a16_w32(o, D_00810200 + 8, zz);
                    if (a16_s16(o, self + 0x28) == 79) {
                        a16_w32(o, A16_PLACE_82A5E0, x);
                        a16_w32(o, A16_PLACE_82A5E0 + 4, A16_ADD(F_19, y));
                        a16_w32(o, A16_PLACE_82A5E0 + 8, A16_ADD(F_46_8, zz));
                        a16_w32(o, A16_PLACE_82A5E0 + 0x10, x);
                        a16_w32(o, A16_PLACE_82A5E0 + 0x14, y);
                        a16_w32(o, A16_PLACE_82A5E0 + 0x18, zz);
                    }
                }
            }
            break;
        case 1:
            if (a16_s16(o, self + 0x36) != 0) {
                a16_w8(o, self + 4, 2);
                a16_w8(o, D_0081080B, 0xFF);
                a16_w8(o, D_0081078B, 0xFF);
                if (a16_c_001B6660(o, A16_GROUP_828C90, &ignored) < 0) return;
                if (a16_c_001FBD50(o, self, 0x92A, 0, F_300) < 0) return;
                a16_w32(o, self + 0x2E0, F_452);
                a16_w32(o, self + 0x2DC, 5);
            }
            break;
        }
        if (a16_c_001B1B70(o, self) < 0) return;
        if (a16_c_001DB250(o) < 0) return;
        (void)a16_c_001DB480(o);
        break;
    case 2:
        if (a16_s32(o, self + 0x2DC) > 0) {
            if ((a16_u32(o, D_70003B68) & 7u) == 0) {
                a16_w32(o, D_700038A0, F_304);
                a16_w32(o, D_700038A0 + 4, F_188);
                a16_w32(o, D_700038A0 + 8, a16_u32(o, self + 0x2E0));
                a16_w32(o, D_700038A0 + 12, F_1);
                if (a16_c_001EFD20(o, (int32_t)0x8000003Fu, D_700038A0, &ignored) < 0) return;
                if (a16_c_001EFD20(o, 3, D_700038A0, &ignored) < 0) return;
                a16_w32(o, self + 0x2DC, a16_u32(o, self + 0x2DC) - 1u);
                a16_w32(o, self + 0x2E0, A16_ADD(a16_u32(o, self + 0x2E0), F_M8_4));
            }
            if (a16_s32(o, self + 0x2DC) > 3) {
                if (a16_c_001DB250(o) < 0) return;
                if (a16_c_001DB480(o) < 0) return;
                (void)a16_method(o, self);
            }
        }
        break;
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00825800(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825800(&o, a0, sp);
    return a16_end(&o);
}

int em_area16_overlay_00826A50(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, float f12, uint32_t sp,
                               EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_826A50(&o, a0, a1, a16_bits(f12), sp);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826C20 (decomp func_overlay_AREA16_00826BE0) — the op09 callback of
 * scripts 0x82A1C0 / 0x82A3C0 (self, record, work): sound
 * 001FBD50(self, 0x13D, 0, 300); returns 1.
 * ---------------------------------------------------------------------- */
int em_area16_overlay_00826C20(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea16OverlayFault *fault)
{
    A16 o;
    (void)a1;
    (void)a2;
    if (!result || a16_begin(&o, h, fault)) return -1;
    if (a16_c_001FBD50(&o, a0, 0x13D, 0, F_300) < 0) return -1;
    *result = 1;
    return 0;
}
