/* AREA21's cannon [61] group (em_area21_overlay.h): 0x828700, 0x828410,
 * 0x828E80 and the projectile 0x828F30. Each function is one original
 * function; the comment above it says what the code does. The order of the
 * memory accesses between calls is the original's (the test compares it
 * one for one). */
#include "em_area21_overlay_internal.h"

/* overlay code (runtime address) */
#define A21G_PROJECTILE 0x00828F30u   /* the behaviour 0x828E80 installs */

/* globals */
#define D_008102B0 0x008102B0u
#define D_0081080E 0x0081080Eu
#define D_00810E64 0x00810E64u
#define D_00810E65 0x00810E65u
#define D_00810E70 0x00810E70u
#define D_70003400 0x70003400u
#define D_70003600 0x70003600u
#define D_70003610 0x70003610u
#define D_700031B0 0x700031B0u
#define D_700031D0 0x700031D0u
#define D_700031D4 0x700031D4u
#define D_700038A0 0x700038A0u
#define D_700038B0 0x700038B0u
#define D_700038C0 0x700038C0u
#define D_70003B74 0x70003B74u
#define D_70003B78 0x70003B78u

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_0_05 0x3D4CCCCDu
#define F_M0_17453294 0xBE32B8C3u /* the barrel's lowest elevation */
#define F_0_31415927 0x3EA0D97Cu  /* pi / 10 */
#define F_0_6108653 0x3F1C61ABu   /* the barrel's highest elevation */
#define F_0_75049156 0x3F402037u  /* the turn limit */
#define F_M0_75049156 0xBF402037u
#define F_ONE 0x3F800000u
#define F_1_5 0x3FC00000u
#define F_M1_5 0xBFC00000u
#define F_HALF_PI 0x3FC90FDBu
#define F_TWO 0x40000000u
#define F_M3 0xC0400000u
#define F_PI 0x40490FDBu
#define F_3_6 0x40666666u
#define F_SEVEN 0x40E00000u
#define F_EIGHT 0x41000000u
#define F_8_5 0x41080000u
#define F_M8_5 0xC1080000u
#define F_10 0x41200000u
#define F_10_53 0x41287AE1u
#define F_11 0x41300000u
#define F_12_03 0x41407AE1u
#define F_M24 0xC1C00000u
#define F_300 0x43960000u
#define F_800 0x44480000u
#define F_1000 0x447A0000u

static void a21g_vec4(A21G *o, uint32_t at, uint32_t x, uint32_t y, uint32_t z, uint32_t w)
{
    a21g_w32(o, at, x);
    a21g_w32(o, at + 4, y);
    a21g_w32(o, at + 8, z);
    a21g_w32(o, at + 12, w);
}

static uint32_t a21g_mdl(A21G *o, uint32_t self) { return a21g_u32(o, self + 0x118); }

/* B0..BC into A0..AC (the previous position), word by word. */
static void a21g_keep_position(A21G *o, uint32_t self)
{
    for (uint32_t k = 0; k < 16; k += 4) {
        const uint32_t v = a21g_u32(o, self + 0xB0 + k);
        a21g_w32(o, self + 0xA0 + k, v);
    }
}

/* ----------------------------------------------------------------------
 * 0x828410 (decomp func_overlay_AREA21_008283D0): with the model's recoil
 * +0x7C..+0x84 zeroed for the duration (saved, restored at the end):
 * 001C6380, the muzzles (-1.5 / 1.5, 3.6, -24) through the model matrix
 * into out +0x10 / +0x20 (001031E0), then the points 0x829590 also makes
 * (a21g_points).
 * ---------------------------------------------------------------------- */
static void a21g_828410(A21G *o, uint32_t self, uint32_t out)
{
    const uint32_t m = a21g_mdl(o, self);
    const uint32_t x = a21g_u32(o, m + 0x7C);
    const uint32_t y = a21g_u32(o, m + 0x80);
    const uint32_t z = a21g_u32(o, m + 0x84);
    a21g_w32(o, m + 0x7C, F_ZERO);
    a21g_w32(o, a21g_mdl(o, self) + 0x80, F_ZERO);
    a21g_w32(o, a21g_mdl(o, self) + 0x84, F_ZERO);
    (void)a21g_c_001C6380(o, self);
    a21g_vec4(o, D_70003610, F_M1_5, F_3_6, F_M24, F_ONE);
    (void)a21g_c_001026A0(o, D_70003610, a21g_mdl(o, self) + 0x90, D_70003610);
    (void)a21g_c_001031E0(o, out + 0x10, D_70003610);
    a21g_vec4(o, D_70003610, F_1_5, F_3_6, F_M24, F_ONE);
    (void)a21g_c_001026A0(o, D_70003610, a21g_mdl(o, self) + 0x90, D_70003610);
    (void)a21g_c_001031E0(o, out + 0x20, D_70003610);
    a21g_points(o, self, out);
    a21g_w32(o, a21g_mdl(o, self) + 0x7C, x);
    a21g_w32(o, a21g_mdl(o, self) + 0x80, y);
    a21g_w32(o, a21g_mdl(o, self) + 0x84, z);
}

int em_area21_overlay_00828410(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_828410(&o, a0, a1);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x828E80 (decomp func_overlay_AREA21_00828E40): ob = 001AFA90(8); none:
 * 0. Else ob +3 = 6, +0x10 = 0x828F30, ob +0xB0..+0xB8 = rec +0x10..+0x18
 * (which 0) or +0x20..+0x28, ob +0xC0 = src +0x70 (00102948); 1.
 * ---------------------------------------------------------------------- */
static int32_t a21g_828E80(A21G *o, uint32_t src, uint32_t rec, int32_t which)
{
    uint32_t ob = 0;
    (void)a21g_c_001AFA90(o, 8, &ob);
    if (ob == 0) return 0;
    a21g_w8(o, ob + 3, 6);
    a21g_w32(o, ob + 0x10, A21G_PROJECTILE);
    const uint32_t from = rec + (which == 0 ? 0x10u : 0x20u);
    for (uint32_t k = 0; k < 12; k += 4) {
        const uint32_t v = a21g_u32(o, from + k);
        a21g_w32(o, ob + 0xB0 + k, v);
    }
    (void)a21g_c_00102948(o, ob + 0xC0, src + 0x70);
    return 1;
}

int em_area21_overlay_00828E80(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1, int32_t a2, int32_t *result,
                               EmArea21OverlayFault *fault)
{
    A21G o;
    if (!result) return -1;
    if (a21g_begin(&o, h, fault)) return -1;
    const int32_t r = a21g_828E80(&o, a0, a1, a2);
    if (a21g_end(&o)) return -1;
    *result = r;
    return 0;
}

/* The turn and barrel input of 0x828700's +7 0 (no shot): +0x230 counts
 * down; 0x829310 on D_00810E64 turns +0xC4 (001B1470, clamped to
 * +-0.75049156 with the step zeroed there) and on D_00810E65 lifts the
 * model's +0x70 (-0.17453294 .. 0.6108653); a nonzero step inside the
 * limits with +0x230 0 plays sound 0x455 and sets +0x230 = 50. */
static void a21g_828700_aim(A21G *o, uint32_t self, uint32_t w)
{
    uint32_t x, r = 0;
    const int32_t wait = a21g_s32(o, w + 0x40);
    if (wait != 0) a21g_w32(o, w + 0x40, (uint32_t)(wait - 1));
    a21g_829310(o, w + 0x34, (int32_t)a21g_u8(o, D_00810E64));
    x = a21g_u32(o, w + 0x34);
    x = A21G_SUB(a21g_u32(o, self + 0xC4), x);
    a21g_w32(o, self + 0xC4, x);
    (void)a21g_c_001B1470(o, x, &r);
    a21g_w32(o, self + 0xC4, r);
    if (!A21G_LE(r, F_0_75049156)) {
        a21g_w32(o, self + 0xC4, F_0_75049156);
        a21g_w32(o, w + 0x34, F_ZERO);
    } else if (A21G_LT(r, F_M0_75049156)) {
        a21g_w32(o, self + 0xC4, F_M0_75049156);
        a21g_w32(o, w + 0x34, F_ZERO);
    } else if (!A21G_EQ(a21g_u32(o, w + 0x34), F_ZERO) && a21g_s32(o, w + 0x40) == 0) {
        (void)a21g_c_001FB9F0(o, 0x455, 0x1000, 0x1000, 0x1000);
        a21g_w32(o, w + 0x40, 0x32);
    }
    a21g_829310(o, w + 0x30, (int32_t)a21g_u8(o, D_00810E65));
    uint32_t m = a21g_mdl(o, self);
    x = a21g_u32(o, w + 0x30);
    a21g_w32(o, m + 0x70, A21G_ADD(a21g_u32(o, m + 0x70), x));
    r = 0;
    (void)a21g_c_001B1470(o, a21g_u32(o, a21g_mdl(o, self) + 0x70), &r);
    a21g_w32(o, a21g_mdl(o, self) + 0x70, r);
    m = a21g_mdl(o, self);
    x = a21g_u32(o, m + 0x70);
    if (!A21G_LE(x, F_0_6108653)) {
        a21g_w32(o, m + 0x70, F_0_6108653);
        a21g_w32(o, w + 0x30, F_ZERO);
    } else if (A21G_LT(x, F_M0_17453294)) {
        a21g_w32(o, m + 0x70, F_M0_17453294);
        a21g_w32(o, w + 0x30, F_ZERO);
    } else if (!A21G_EQ(a21g_u32(o, w + 0x30), F_ZERO) && a21g_s32(o, w + 0x40) == 0) {
        (void)a21g_c_001FB9F0(o, 0x455, 0x1000, 0x1000, 0x1000);
        a21g_w32(o, w + 0x40, 0x32);
    }
}

/* ----------------------------------------------------------------------
 * 0x828700 (decomp func_overlay_AREA21_008286C0) — the cannon [61]. Work w
 * = +0x1F0. State 0 (001B0FD0 zero): 001C6380, +0 = 1, +0x248 = +0x24C = 0.
 * State 1, +5:
 *  0 — +5 1, +6 0, +0x230 0 once D_008102B0 +0x230 is 0x28 and +0x1F0 0x39.
 *  1 — D_008102B0 +0x230 not 0x28: +5 0, the work and +0xC4 and the
 *      model's +0x70 / recoil zeroed, 001C6380, D_008102B0 +0xC4 =
 *      001B1470(pi + +0xC4). Else +6 0: with D_008102B0 +0x1F1 1, +6 1, +7
 *      0, steps zeroed, 0x828410, +0x230 = 0. +6 1 by +7: 0 — a set
 *      D_00810E70 & (D_70003B74 | D_70003B78) bit fires the first barrel
 *      (+7 1, +0x2A = 20, +0x28 = 10, +0x228 = pi/2, steps zeroed, sound
 *      0x454, 0x828E80(self, w, 0), 001B1E20(9, 0)), else the aim input;
 *      then 0x828410 and D_008102B0 +0xC4 = 001B1470(pi + +0xC4). 1 / 3 —
 *      +0x2A counts down (at 0 the next +7); +0x28 0: recoil zeroed and
 *      +0x228 = pi/2, else +0x28 -= 1, +0x228 -= pi/10 and the recoil is
 *      (0, 0, 2 cos +0x228) turned by +0x70. 2 — the second barrel (+7 3,
 *      the same set-up, 0x828E80(self, w, 1)). 4 — +7 0. Then 001C6380.
 * Every state 1 frame: 001B17A0, the +0x4C method, three 001F5940(7)
 * markers at (0, 10.53, 12.03), (+-8.5, 11, -3) through the +0x110 model;
 * from D_0081080E 0x10 on (+0x248 = 1) +0x24C counts, and at 255 +4 = 3.
 * States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21g_828700(A21G *o, uint32_t self)
{
    const uint32_t pl = D_008102B0, w = self + 0x1F0;
    const uint32_t state = a21g_u8(o, self + 4);
    uint32_t r = 0;

    if (state == 3 || state == 2) {
        (void)a21g_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        int32_t busy = 0;
        (void)a21g_c_001B0FD0(o, self, &busy);
        if (busy != 0) return;
        (void)a21g_c_001C6380(o, self);
        a21g_w8(o, self, 1);
        a21g_w32(o, w + 0x58, 0);
        a21g_w32(o, w + 0x5C, 0);
        return;
    }
    if (state != 1) return;
    const uint32_t step = a21g_u8(o, self + 5);
    if (step == 1) {
        if (a21g_s32(o, pl + 0x230) != 0x28) {
            a21g_w8(o, self + 5, 0);
            a21g_w32(o, w + 0x40, 0);
            a21g_w32(o, w + 0x3C, 0);
            a21g_w32(o, w + 0x30, F_ZERO);
            a21g_w32(o, w + 0x34, F_ZERO);
            a21g_w32(o, w + 0x38, F_ZERO);
            a21g_w32(o, self + 0xC4, F_ZERO);
            a21g_w32(o, a21g_mdl(o, self) + 0x70, F_ZERO);
            a21g_w32(o, a21g_mdl(o, self) + 0x7C, F_ZERO);
            a21g_w32(o, a21g_mdl(o, self) + 0x80, F_ZERO);
            a21g_w32(o, a21g_mdl(o, self) + 0x84, F_ZERO);
            (void)a21g_c_001C6380(o, self);
            (void)a21g_c_001B1470(o, A21G_ADD(F_PI, a21g_u32(o, self + 0xC4)), &r);
            a21g_w32(o, pl + 0xC4, r);
        } else {
            const uint32_t sub = a21g_u8(o, self + 6);
            if (sub == 1) {
                const uint32_t shot = a21g_u8(o, self + 7);
                if (shot == 4) {
                    a21g_w8(o, self + 7, 0);
                } else if (shot == 2) {
                    a21g_w8(o, self + 7, 3);
                    a21g_w16(o, self + 0x2A, 20);
                    a21g_w16(o, self + 0x28, 10);
                    a21g_w32(o, w + 0x38, F_HALF_PI);
                    (void)a21g_c_001FB9F0(o, 0x454, 0x1000, 0x1000, 0x1000);
                    (void)a21g_828E80(o, self, w, 1);
                    (void)a21g_c_001B1E20(o, 9, 0);
                } else if (shot == 3 || shot == 1) {
                    const uint32_t n = (uint32_t)(a21g_s16(o, self + 0x2A) - 1);
                    a21g_w16(o, self + 0x2A, n);
                    if ((int16_t)n == 0) {
                        a21g_w8(o, self + 7, a21g_u8(o, self + 7) + 1);
                    } else {
                        const int32_t c = a21g_s16(o, self + 0x28);
                        if (c == 0) {
                            a21g_w32(o, a21g_mdl(o, self) + 0x7C, F_ZERO);
                            a21g_w32(o, a21g_mdl(o, self) + 0x80, F_ZERO);
                            a21g_w32(o, a21g_mdl(o, self) + 0x84, F_ZERO);
                            a21g_w32(o, w + 0x38, F_HALF_PI);
                        } else {
                            uint32_t cs = 0, x;
                            a21g_w16(o, self + 0x28, (uint32_t)(c - 1));
                            a21g_w32(o, w + 0x38, A21G_SUB(a21g_u32(o, w + 0x38), F_0_31415927));
                            a21g_w32(o, D_70003600, F_ZERO);
                            a21g_w32(o, D_70003600 + 4, F_ZERO);
                            (void)a21g_c_0011DE90(o, a21g_u32(o, w + 0x38), &cs);
                            a21g_w32(o, D_70003600 + 8, A21G_MUL(F_TWO, cs));
                            a21g_w32(o, D_70003600 + 12, F_ONE);
                            (void)a21g_c_001029C0(o, D_70003400);
                            (void)a21g_c_00102B08(o, D_70003400, D_70003400, a21g_u32(o, a21g_mdl(o, self) + 0x70));
                            (void)a21g_c_001026A0(o, D_70003600, D_70003400, D_70003600);
                            x = a21g_u32(o, D_70003600);
                            a21g_w32(o, a21g_mdl(o, self) + 0x7C, x);
                            x = a21g_u32(o, D_70003600 + 4);
                            a21g_w32(o, a21g_mdl(o, self) + 0x80, x);
                            x = a21g_u32(o, D_70003600 + 8);
                            a21g_w32(o, a21g_mdl(o, self) + 0x84, x);
                        }
                    }
                } else if (shot == 0) {
                    const uint32_t b74 = a21g_u16(o, D_70003B74);
                    const uint32_t b78 = a21g_u16(o, D_70003B78);
                    if ((a21g_u16(o, D_00810E70) & (b74 | b78)) != 0) {
                        a21g_w8(o, self + 7, 1);
                        a21g_w16(o, self + 0x2A, 20);
                        a21g_w16(o, self + 0x28, 10);
                        a21g_w32(o, w + 0x38, F_HALF_PI);
                        a21g_w32(o, w + 0x30, F_ZERO);
                        a21g_w32(o, w + 0x34, F_ZERO);
                        (void)a21g_c_001FB9F0(o, 0x454, 0x1000, 0x1000, 0x1000);
                        (void)a21g_828E80(o, self, w, 0);
                        (void)a21g_c_001B1E20(o, 9, 0);
                    } else {
                        a21g_828700_aim(o, self, w);
                    }
                    a21g_828410(o, self, w);
                    r = 0;
                    (void)a21g_c_001B1470(o, A21G_ADD(F_PI, a21g_u32(o, self + 0xC4)), &r);
                    a21g_w32(o, pl + 0xC4, r);
                }
                (void)a21g_c_001C6380(o, self);
            } else if (sub == 0) {
                if (a21g_u8(o, pl + 0x1F1) == 1) {
                    a21g_w8(o, self + 6, sub + 1);
                    a21g_w8(o, self + 7, 0);
                    a21g_w32(o, w + 0x30, F_ZERO);
                    a21g_w32(o, w + 0x34, F_ZERO);
                    a21g_w32(o, w + 0x3C, 0);
                    a21g_828410(o, self, w);
                    a21g_w32(o, w + 0x40, 0);
                }
            }
        }
    } else if (step == 0) {
        if (a21g_s32(o, pl + 0x230) == 0x28 && a21g_u8(o, pl + 0x1F0) == 0x39) {
            a21g_w8(o, self + 5, step + 1);
            a21g_w8(o, self + 6, 0);
            a21g_w32(o, w + 0x40, 0);
        }
    }
    (void)a21g_c_001B17A0(o, self);
    (void)a21g_method(o, self);
    a21g_vec4(o, D_700038A0, F_ZERO, F_10_53, F_12_03, F_ONE);
    (void)a21g_c_001026A0(o, D_700038A0, a21g_u32(o, self + 0x110) + 0x90, D_700038A0);
    (void)a21g_c_001F5940(o, 7, D_700038A0, 0);
    a21g_vec4(o, D_700038B0, F_8_5, F_11, F_M3, F_ONE);
    (void)a21g_c_001026A0(o, D_700038B0, a21g_u32(o, self + 0x110) + 0x90, D_700038B0);
    (void)a21g_c_001F5940(o, 7, D_700038B0, 0);
    a21g_vec4(o, D_700038C0, F_M8_5, F_11, F_M3, F_ONE);
    (void)a21g_c_001026A0(o, D_700038C0, a21g_u32(o, self + 0x110) + 0x90, D_700038C0);
    (void)a21g_c_001F5940(o, 7, D_700038C0, 0);
    if (a21g_u8(o, D_0081080E) == 0x10) a21g_w32(o, w + 0x58, 1);
    if (a21g_s32(o, w + 0x58) == 1) {
        a21g_w32(o, w + 0x5C, (uint32_t)a21g_s32(o, w + 0x5C) + 1u);
        if (a21g_s32(o, w + 0x5C) >= 0xFF) a21g_w8(o, self + 4, 3);
    }
}

int em_area21_overlay_00828700(const EmArea21OverlayHooks *h, uint32_t a0, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_828700(&o, a0);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x828F30 (decomp func_overlay_AREA21_00828EF0) — the projectile. State
 * 0: +4 = 1, +0x28 = 32, +0xC0 scaled to 8 (00103230), +0x20 =
 * 001EFE00(0x80000037, self), +0xA0 = +0xB0 (four words), +0x38 = 0.
 * State 1: +5 0 moves by +0xC0 (+0x28 counts down, at 0 +5 1); +5 1 moves
 * x / z by +0xC0 / 7 and falls by +0x38 (+0.05 a frame, at most 7). A
 * 0019A570(+0xA0, +0xB0, 7, 0x20) hit r: +0xB0 = the hit point
 * (D_700031B0 with w 1); for r 1 / 2 a live hit object D_700031D4 of kind 2
 * (+2 & 0x1F) with +0 1 gets 001B41F0(ob, point, +0xC0, D_700031D0 +0x1C,
 * 0x2000, 0x50), one of another kind +0x36 = 5 and +0x70 = +0xC0, either
 * with sound 0x456 (300); state 2, +5 0. No hit and y below 10: y = 10,
 * state 2, +5 1. Then +0xA0 = +0xB0 and 001C94B0(+0xD0, +0xB0, 0, 0). State
 * 2: +5 0 effect 0x80000069 and sound 0x456 (1000), +5 1 effect 0x8000005F
 * and sound 0xDB (800); the +0x20 object's +4 = 2; +4 += 1. State 3:
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21g_828F30(A21G *o, uint32_t self)
{
    const uint32_t state = a21g_u8(o, self + 4);
    uint32_t x, y;

    if (state == 3) {
        (void)a21g_c_001AFC10(o, self);
    } else if (state == 2) {
        const uint32_t kind = a21g_u8(o, self + 5);
        if (kind == 1) {
            (void)a21g_c_001EFD20(o, 0x8000005Fu, self + 0xB0);
            (void)a21g_c_001FBD50(o, self, 0xDB, 0, F_800);
        } else if (kind == 0) {
            (void)a21g_c_001EFD20(o, 0x80000069u, self + 0xB0);
            (void)a21g_c_001FBD50(o, self, 0x456, 0, F_1000);
        }
        a21g_w8(o, a21g_u32(o, self + 0x20) + 4, 2);
        a21g_w8(o, self + 4, a21g_u8(o, self + 4) + 1);
    } else if (state == 1) {
        const uint32_t phase = a21g_u8(o, self + 5);
        int32_t r = 0;
        if (phase == 1) {
            x = a21g_u32(o, self + 0xC0);
            a21g_w32(o, self + 0xB0, A21G_ADD(a21g_u32(o, self + 0xB0), A21G_DIV(x, F_SEVEN)));
            x = a21g_u32(o, self + 0xC8);
            a21g_w32(o, self + 0xB8, A21G_ADD(a21g_u32(o, self + 0xB8), A21G_DIV(x, F_SEVEN)));
            x = A21G_ADD(a21g_u32(o, self + 0x38), F_0_05);
            a21g_w32(o, self + 0x38, x);
            if (!A21G_LE(x, F_SEVEN)) a21g_w32(o, self + 0x38, F_SEVEN);
            x = a21g_u32(o, self + 0x38);
            a21g_w32(o, self + 0xB4, A21G_SUB(a21g_u32(o, self + 0xB4), x));
        } else if (phase == 0) {
            const uint32_t n = (uint32_t)(a21g_s16(o, self + 0x28) - 1);
            a21g_w16(o, self + 0x28, n);
            if ((int16_t)n == 0) a21g_w8(o, self + 5, a21g_u8(o, self + 5) + 1);
            for (uint32_t k = 0; k < 12; k += 4) {
                x = a21g_u32(o, self + 0xC0 + k);
                a21g_w32(o, self + 0xB0 + k, A21G_ADD(a21g_u32(o, self + 0xB0 + k), x));
            }
        }
        (void)a21g_c_0019A570(o, self + 0xA0, self + 0xB0, 7, 0x20, &r);
        if (r != 0) {
            (void)a21g_c_001031E0(o, D_700038A0, D_700031B0);
            a21g_w32(o, D_700038A0 + 12, F_ONE);
            (void)a21g_c_00102948(o, self + 0xB0, D_700038A0);
            if ((uint32_t)r - 1u < 2u) {
                const uint32_t ob = a21g_u32(o, D_700031D4);
                if (ob != 0) {
                    const uint32_t live = a21g_u8(o, ob);
                    if (live != 0) {
                        if ((a21g_u8(o, ob + 2) & ~0xE0u) == 2) {
                            if (live == 1) {
                                y = a21g_u32(o, D_700031D0);
                                const int32_t id = a21g_s32(o, y + 0x1C);
                                (void)a21g_c_001B41F0(o, ob, D_700038A0, self + 0xC0, id, 0x2000, 0x50);
                            }
                        } else {
                            a21g_w16(o, ob + 0x36, 5);
                            (void)a21g_c_00102948(o, ob + 0x70, self + 0xC0);
                        }
                        (void)a21g_c_001FBD50(o, self, 0x456, 0, F_300);
                    }
                }
            }
            a21g_w8(o, self + 4, 2);
            a21g_w8(o, self + 5, 0);
        } else if (A21G_LT(a21g_u32(o, self + 0xB4), F_10)) {
            a21g_w32(o, self + 0xB4, F_10);
            a21g_w8(o, self + 4, 2);
            a21g_w8(o, self + 5, 1);
        }
        a21g_keep_position(o, self);
        a21g_vec4(o, D_700038A0, F_ZERO, F_ZERO, F_ZERO, F_ONE);
        (void)a21g_c_001C94B0(o, self + 0xD0, self + 0xB0, D_700038A0, D_700038A0);
    } else if (state == 0) {
        uint32_t ob = 0;
        a21g_w8(o, self + 4, 1);
        a21g_w16(o, self + 0x28, 32);
        (void)a21g_c_00103230(o, self + 0xC0, self + 0xC0, F_EIGHT);
        (void)a21g_c_001EFE00(o, 0x80000037u, self, &ob);
        a21g_w32(o, self + 0x20, ob);
        a21g_keep_position(o, self);
        a21g_w32(o, self + 0x38, F_ZERO);
    }
}

int em_area21_overlay_00828F30(const EmArea21OverlayHooks *h, uint32_t a0, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_828F30(&o, a0);
    return a21g_end(&o);
}
