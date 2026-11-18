/* AREA21's Gatling [62] and its overlay callees (em_area21_overlay.h); the
 * cannon [61] group is em_area21_overlay_cannon.c. Each function is one
 * original function; the comment above it says what the code does. The
 * order of the memory accesses between calls is the original's (the test
 * compares it one for one). */
#include "em_area21_overlay_internal.h"

/* overlay data (runtime addresses) */
#define A21G_STEP_TABLE 0x0082DB10u   /* 0x829310's step limits, one float per key level */
#define A21G_PACKET_82DB20 0x0082DB20u /* 0x829F40's packet group (001CFBE0) */

/* globals */
#define D_008101F0 0x008101F0u
#define D_00810200 0x00810200u
#define D_008102B0 0x008102B0u
#define D_008105D0 0x008105D0u
#define D_008105E0 0x008105E0u
#define D_00810E64 0x00810E64u
#define D_00810E65 0x00810E65u
#define D_00810E70 0x00810E70u
#define D_70003400 0x70003400u
#define D_70003430 0x70003430u
#define D_70003600 0x70003600u
#define D_70003610 0x70003610u
#define D_700036A0 0x700036A0u
#define D_700036D0 0x700036D0u
#define D_700038A0 0x700038A0u
#define D_70003B68 0x70003B68u
#define D_70003B74 0x70003B74u
#define D_70003B78 0x70003B78u

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_0_0001 0x38D1B717u
#define F_0_001 0x3A83126Fu
#define F_0_01 0x3C23D70Au
#define F_0_1 0x3DCCCCCDu
#define F_0_15 0x3E19999Au
#define F_0_2 0x3E4CCCCDu
#define F_HALF 0x3F000000u
#define F_0_5235988 0x3F060A92u   /* pi / 6 */
#define F_0_69813174 0x3F32B8C3u  /* the Gatling's elevation limit */
#define F_M0_69813174 0xBF32B8C3u
#define F_ONE 0x3F800000u
#define F_HALF_PI 0x3FC90FDBu
#define F_TWO 0x40000000u
#define F_2_323 0x4014AC08u
#define F_2_5 0x40200000u
#define F_THREE 0x40400000u
#define F_PI 0x40490FDBu
#define F_FOUR 0x40800000u
#define F_FIVE 0x40A00000u
#define F_6_5 0x40D00000u
#define F_SEVEN 0x40E00000u
#define F_EIGHT 0x41000000u
#define F_12 0x41400000u
#define F_13 0x41500000u
#define F_16 0x41800000u
#define F_M8 0xC1000000u
#define F_M8_2 0xC1033333u
#define F_160 0x43200000u
#define F_900 0x44610000u
#define F_1800 0x44E10000u
#define F_65535 0x477FFF00u
#define F_2_31 0x4F000000u        /* 2147483648.0 */

static void a21g_vec(A21G *o, uint32_t at, uint32_t x, uint32_t y, uint32_t z, uint32_t w)
{
    a21g_w32(o, at, x);
    a21g_w32(o, at + 4, y);
    a21g_w32(o, at + 8, z);
    a21g_w32(o, at + 12, w);
}

static uint32_t a21g_model(A21G *o, uint32_t self) { return a21g_u32(o, self + 0x118); }

/* ----------------------------------------------------------------------
 * 0x829310 (decomp func_overlay_AREA21_008292D0). n = 001B5DC0(key);
 * nonzero: step 0.001 * n towards the limit +-table[n] (+ when key & 0xFF
 * is 0x80 or more), stopping at the limit (re-read from the table); zero:
 * a nonzero value decays: |v| - 0.01 (0011DF78), 0 once that is not
 * positive, else with v's sign.
 * ---------------------------------------------------------------------- */
void a21g_829310(A21G *o, uint32_t v, int32_t key)
{
    int32_t n = 0;
    (void)a21g_c_001B5DC0(o, key, &n);
    if (n != 0) {
        const uint32_t step = A21G_MUL(F_0_001, A21G_CVT_S_W(n));
        const int up = ((uint32_t)key & 0xFFu) >= 0x80u;
        const uint32_t t = A21G_STEP_TABLE + ((uint32_t)n << 2);
        uint32_t x = a21g_u32(o, v);
        const uint32_t lim = up ? a21g_u32(o, t) : A21G_NEG(a21g_u32(o, t));
        if (A21G_LE(x, lim)) {
            x = A21G_ADD(x, step);
            a21g_w32(o, v, x);
            if (!A21G_LE(x, lim)) a21g_w32(o, v, up ? a21g_u32(o, t) : A21G_NEG(a21g_u32(o, t)));
        } else {
            x = A21G_SUB(x, step);
            a21g_w32(o, v, x);
            if (A21G_LT(x, lim)) a21g_w32(o, v, up ? a21g_u32(o, t) : A21G_NEG(a21g_u32(o, t)));
        }
        return;
    }
    const uint32_t x = a21g_u32(o, v);
    if (A21G_EQ(x, F_ZERO)) return;
    uint32_t d = 0;
    (void)a21g_c_0011DF78(o, x, &d);
    d = A21G_SUB(d, F_0_01);
    if (A21G_LE(d, F_ZERO)) {
        a21g_w32(o, v, F_ZERO);
    } else if (A21G_LT(a21g_u32(o, v), F_ZERO)) {
        a21g_w32(o, v, A21G_NEG(d));
    } else {
        a21g_w32(o, v, d);
    }
}

int em_area21_overlay_00829310(const EmArea21OverlayHooks *h, uint32_t a0, int32_t a1, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_829310(&o, a0, a1);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * The points 0x829590 and 0x828410 share (one stretch of each original):
 * the model's (0, 4, 0) and (0, 4, -8.2) through its matrix (+0x90); the
 * second into out (001031E0), their difference (001028D0) normalised into
 * +0x70 (00102760); (0, 6.5, 5) into D_008101F0 and D_008105D0, (0, 5, -8)
 * into D_00810200 (00102948) and D_008105E0.
 * ---------------------------------------------------------------------- */
void a21g_points(A21G *o, uint32_t self, uint32_t out)
{
    a21g_vec(o, D_70003600, F_ZERO, F_FOUR, F_ZERO, F_ONE);
    (void)a21g_c_001026A0(o, D_70003600, a21g_model(o, self) + 0x90, D_70003600);
    a21g_vec(o, D_70003610, F_ZERO, F_FOUR, F_M8_2, F_ONE);
    (void)a21g_c_001026A0(o, D_70003610, a21g_model(o, self) + 0x90, D_70003610);
    (void)a21g_c_001031E0(o, out, D_70003610);
    (void)a21g_c_001028D0(o, D_70003600, D_70003610, D_70003600);
    (void)a21g_c_00102760(o, self + 0x70, D_70003600);
    a21g_vec(o, D_70003600, F_ZERO, F_6_5, F_FIVE, F_ONE);
    (void)a21g_c_001026A0(o, D_70003600, a21g_model(o, self) + 0x90, D_70003600);
    (void)a21g_c_001031E0(o, D_008101F0, D_70003600);
    (void)a21g_c_001031E0(o, D_008105D0, D_008101F0);
    a21g_vec(o, D_70003610, F_ZERO, F_FIVE, F_M8, F_ONE);
    (void)a21g_c_001026A0(o, D_70003610, a21g_model(o, self) + 0x90, D_70003610);
    (void)a21g_c_00102948(o, D_00810200, D_70003610);
    (void)a21g_c_001031E0(o, D_008105E0, D_00810200);
}

/* ----------------------------------------------------------------------
 * 0x829590 (decomp func_overlay_AREA21_00829550): with the model's recoil
 * +0x7C..+0x84 zeroed for the duration (saved, restored at the end):
 * 001C6380, then the shared points (a21g_points) into out.
 * ---------------------------------------------------------------------- */
static void a21g_829590(A21G *o, uint32_t self, uint32_t out)
{
    uint32_t m = a21g_model(o, self);
    const uint32_t x = a21g_u32(o, m + 0x7C);
    const uint32_t y = a21g_u32(o, m + 0x80);
    const uint32_t z = a21g_u32(o, m + 0x84);
    a21g_w32(o, m + 0x7C, F_ZERO);
    a21g_w32(o, a21g_model(o, self) + 0x80, F_ZERO);
    a21g_w32(o, a21g_model(o, self) + 0x84, F_ZERO);
    (void)a21g_c_001C6380(o, self);
    a21g_points(o, self, out);
    a21g_w32(o, a21g_model(o, self) + 0x7C, x);
    a21g_w32(o, a21g_model(o, self) + 0x80, y);
    a21g_w32(o, a21g_model(o, self) + 0x84, z);
}

int em_area21_overlay_00829590(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_829590(&o, a0, a1);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8294C0 (decomp func_overlay_AREA21_00829480): (0, 1, 8) through the
 * matrix of +0xC4's turn (001029C0, 00102BB0) moved to +0xB0 (00102958 into
 * its last row) goes to 00182F90(ob, point); ob +0xC4 = 001B1470(pi +
 * +0xC4).
 * ---------------------------------------------------------------------- */
static void a21g_8294C0(A21G *o, uint32_t self, uint32_t ob)
{
    uint32_t r = 0;
    a21g_vec(o, D_70003600, F_ZERO, F_ONE, F_EIGHT, F_ONE);
    (void)a21g_c_001029C0(o, D_70003400);
    (void)a21g_c_00102BB0(o, D_70003400, D_70003400, a21g_u32(o, self + 0xC4));
    (void)a21g_c_00102958(o, D_70003430, self + 0xB0);
    (void)a21g_c_001026A0(o, D_70003600, D_70003400, D_70003600);
    (void)a21g_c_00182F90(o, ob, D_70003600);
    (void)a21g_c_001B1470(o, A21G_ADD(F_PI, a21g_u32(o, self + 0xC4)), &r);
    a21g_w32(o, ob + 0xC4, r);
}

int em_area21_overlay_008294C0(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_8294C0(&o, a0, a1);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8260A0 (decomp func_overlay_AREA21_00826060): 001D1F80(1, 2, 1),
 * 00207F80(1, 0x70F0, 0x84B0, 0x7B10, 0x8540, 0x30202020), then
 * 00207F80(1, 0x7100, 0x84C0, w, 0x8540, 0x80202080) with w =
 * 001281C0(16 * u) where u is the unsigned 001281C0(16 + 160 * t) + 0x700
 * as a float (above 2^31-1: half of it, its low bit kept, converted and
 * doubled).
 * ---------------------------------------------------------------------- */
static void a21g_8260A0(A21G *o, uint32_t t)
{
    int32_t i = 0;
    int32_t j = 0;
    uint32_t f;
    (void)a21g_c_001D1F80(o, 1, 2, 1);
    (void)a21g_c_00207F80(o, 1, 0x70F0, 0x84B0, 0x7B10, 0x8540, 0x30202020u);
    (void)a21g_c_001281C0(o, A21G_ADD(F_16, A21G_MUL(F_160, t)), &i);
    const uint32_t u = (uint32_t)i + 0x700u;
    if ((int32_t)u < 0) {
        f = A21G_CVT_S_W((u >> 1) | (u & 1u));
        f = A21G_ADD(f, f);
    } else {
        f = A21G_CVT_S_W(u);
    }
    (void)a21g_c_001281C0(o, A21G_MUL(F_16, f), &j);
    (void)a21g_c_00207F80(o, 1, 0x7100, 0x84C0, j, 0x8540, 0x80202080u);
}

int em_area21_overlay_008260A0(const EmArea21OverlayHooks *h, float a0, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_8260A0(&o, a21g_bits(a0));
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x829F10 (decomp func_overlay_AREA21_00829ED0): +0x240 = 0, +0x244 =
 * 00122BB8().
 * ---------------------------------------------------------------------- */
static void a21g_829F10(A21G *o, uint32_t self)
{
    int32_t r = 0;
    a21g_w32(o, self + 0x240, 0);
    (void)a21g_c_00122BB8(o, &r);
    a21g_w32(o, self + 0x244, (uint32_t)r);
}

int em_area21_overlay_00829F10(const EmArea21OverlayHooks *h, uint32_t a0, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_829F10(&o, a0);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x829F40 (decomp func_overlay_AREA21_00829F00), frame 0xF0: the matrix m
 * at sp - 0xA0, the packet at sp - 0x60. With an odd 00122BB8() and
 * D_70003B68 % 3 == 0: m = the turn by pi/2 times the +0x114 model's +0xC0
 * (001029C0, 00102B08, 00102918), its row 3 moved by (5 r / 2^31 - 2.5, 13,
 * 5 r / 2^31 - 2.5) with two more 00122BB8() draws, effect 0x8000003B there
 * (001EFEB0) and, when one came back, its +5 = 1, +0x1F0 = 8, +0x1F4 = 12,
 * +0x1F8 = 0.2. Then m = the +0x118 model's matrix with y + 1; the packet
 * 001CFB50(packet, 0, m, +0x240, (+0x244 >> 16 & 0xFFFF) / 65535 + 0.0001,
 * +0x22C / 1800, 0.1, 7) queued by 001CFBE0(001CCF70(m row 3), 1, 0x82DB20,
 * packet, 0); +0x240 += 0.01, less 1 once it is above 2.
 * ---------------------------------------------------------------------- */
static void a21g_829F40(A21G *o, uint32_t self, uint32_t sp)
{
    const uint32_t frame = sp - 0xF0u;
    const uint32_t m = frame + 0x50u, packet = frame + 0x90u;
    const uint32_t w = self + 0x1F0;
    int32_t r = 0;
    uint32_t x;
    (void)a21g_c_00122BB8(o, &r);
    if ((r & 1) != 0 && a21g_s32(o, D_70003B68) % 3 == 0) {
        uint32_t ob = 0;
        (void)a21g_c_001029C0(o, m);
        (void)a21g_c_00102B08(o, m, m, F_HALF_PI);
        (void)a21g_c_00102918(o, m, m, a21g_u32(o, self + 0x114) + 0xC0);
        a21g_w32(o, m + 0x34, A21G_ADD(a21g_u32(o, m + 0x34), F_13));
        r = 0;
        (void)a21g_c_00122BB8(o, &r);
        x = A21G_DIV(A21G_CVT_S_W(r), F_2_31);
        x = A21G_SUB(A21G_MUL(F_FIVE, x), F_2_5);
        a21g_w32(o, m + 0x30, A21G_ADD(a21g_u32(o, m + 0x30), x));
        r = 0;
        (void)a21g_c_00122BB8(o, &r);
        x = A21G_DIV(A21G_CVT_S_W(r), F_2_31);
        x = A21G_SUB(A21G_MUL(F_FIVE, x), F_2_5);
        a21g_w32(o, m + 0x38, A21G_ADD(a21g_u32(o, m + 0x38), x));
        (void)a21g_c_001EFEB0(o, 0x8000003Bu, m, &ob);
        if (ob != 0) {
            a21g_w8(o, ob + 5, 1);
            a21g_w32(o, ob + 0x1F0, 8);
            a21g_w32(o, ob + 0x1F4, F_12);
            a21g_w32(o, ob + 0x1F8, F_0_2);
        }
    }
    (void)a21g_c_00102958(o, m, a21g_model(o, self) + 0x90);
    a21g_w32(o, m + 0x34, A21G_ADD(a21g_u32(o, m + 0x34), F_ONE));
    const uint32_t heat = a21g_u32(o, w + 0x3C);
    const uint32_t seed = a21g_u32(o, w + 0x54);
    const uint32_t t = A21G_DIV(heat, F_1800);
    int32_t handle = 0;
    (void)a21g_c_001CCF70(o, m + 0x30, &handle);
    uint32_t s = A21G_CVT_S_W((seed >> 16) & 0xFFFFu);
    const uint32_t phase = a21g_u32(o, w + 0x50);
    s = A21G_ADD(A21G_DIV(s, F_65535), F_0_0001);
    (void)a21g_c_001CFB50(o, packet, 0, m, phase, s, t, F_0_1, F_SEVEN);
    (void)a21g_c_001CFBE0(o, handle, 1, A21G_PACKET_82DB20, packet, 0);
    x = A21G_ADD(a21g_u32(o, w + 0x50), F_0_01);
    a21g_w32(o, w + 0x50, x);
    if (!A21G_LE(x, F_TWO)) a21g_w32(o, w + 0x50, A21G_SUB(a21g_u32(o, w + 0x50), F_ONE));
}

int em_area21_overlay_00829F40(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_829F40(&o, a0, sp);
    return a21g_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8297C0 (decomp func_overlay_AREA21_00829780) — the Gatling [62], frame
 * 0x50. Work w = +0x1F0. State 0 (001B0FD0 zero): 001C6380, +0 = 1, +0x22C
 * = 0, +0x20 = 001C5570(self, (0, 1, 0, 1), 0x15, 2). State 1, +5:
 *  0 — +5 1 and +6 0 once D_008102B0 +0x230 is 0x28 and +0x1F0 0x38; a
 *      nonzero +0x22C decays by 0.5.
 *  1 — D_008102B0 +0x230 not 0x28: +5 0, the steps and the model's +0x70
 *      / recoil zeroed, 001C6380, 001A2370. Else +6 0: with D_008102B0
 *      +0x1F1 1, +6 1, +7 0, steps zeroed, 0x829590. +6 1: 0x829310 on
 *      D_00810E64 moves +0xC4 (001B1470) and on D_00810E65 the model's +0x70
 *      (clamped to +-0.69813174, the step zeroed there), 0x829590; a set
 *      D_00810E70 & (D_70003B74 | D_70003B78) bit adds 1 to +0x22C and with
 *      +7 0 starts a shot (001B1E20(0xA, 0), +7 1, +0x28 = 6, +0x228 = pi/2,
 *      sound 0x451, effect 0xA, a 001F4F40(2) child placed at the muzzle and
 *      its 001F4010(3) flash); unset, a nonzero +0x22C decays by 0.5. While
 *      +7 is set, +0x28 counts down: at 0 +7 0 and the recoil zeroed, else
 *      +0x228 -= pi/6 and the recoil is (0, 0, 0.15 cos +0x228) turned by
 *      +0x70. Then 001C6380, 001A2370, 0x8294C0(self, D_008102B0), and above
 *      900 the overheat: +0 = 2, D_008102B0 +0x302 = 1, +0x22C = 1800, +5 2,
 *      all zeroed, 0x829F10; else 0x8260A0(+0x22C / 900).
 *  2 — +0x22C -= 1 (at 0: +0 = 1, +5 0), 0x829F40.
 * Every state 1 frame: 001B17A0, the +0x4C method, the +0x20 child's +0xC4
 * = +0xC4, 001F5940(7, (0, 0, 2.323) through the model). States 2 / 3:
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21g_8297C0(A21G *o, uint32_t self, uint32_t sp)
{
    const uint32_t pl = D_008102B0, w = self + 0x1F0;
    const uint32_t state = a21g_u8(o, self + 4);
    uint32_t m, x, r = 0;

    if (state == 3 || state == 2) {
        (void)a21g_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        int32_t busy = 0;
        uint32_t child = 0;
        (void)a21g_c_001B0FD0(o, self, &busy);
        if (busy != 0) return;
        (void)a21g_c_001C6380(o, self);
        a21g_w8(o, self, 1);
        a21g_w32(o, w + 0x3C, F_ZERO);
        a21g_vec(o, D_700038A0, F_ZERO, F_ONE, F_ZERO, F_ONE);
        (void)a21g_c_001C5570(o, self, D_700038A0, 0x15, 2, &child);
        a21g_w32(o, self + 0x20, child);
        return;
    }
    if (state != 1) return;
    const uint32_t step = a21g_u8(o, self + 5);
    if (step == 2) {
        x = A21G_SUB(a21g_u32(o, w + 0x3C), F_ONE);
        a21g_w32(o, w + 0x3C, x);
        if (A21G_EQ(x, F_ZERO)) {
            a21g_w8(o, self, 1);
            a21g_w8(o, self + 5, 0);
        }
        a21g_829F40(o, self, sp - 0x50u);
    } else if (step == 1) {
        if (a21g_s32(o, pl + 0x230) != 0x28) {
            a21g_w8(o, self + 5, 0);
            a21g_w32(o, w + 0x30, F_ZERO);
            a21g_w32(o, w + 0x34, F_ZERO);
            a21g_w32(o, w + 0x38, F_ZERO);
            a21g_w32(o, a21g_model(o, self) + 0x70, F_ZERO);
            a21g_w32(o, a21g_model(o, self) + 0x7C, F_ZERO);
            a21g_w32(o, a21g_model(o, self) + 0x80, F_ZERO);
            a21g_w32(o, a21g_model(o, self) + 0x84, F_ZERO);
            (void)a21g_c_001C6380(o, self);
            (void)a21g_c_001A2370(o, self, self + 0xD0);
        } else {
            const uint32_t sub = a21g_u8(o, self + 6);
            if (sub == 1) {
                a21g_829310(o, w + 0x34, (int32_t)a21g_u8(o, D_00810E64));
                x = a21g_u32(o, w + 0x34);
                x = A21G_SUB(a21g_u32(o, self + 0xC4), x);
                a21g_w32(o, self + 0xC4, x);
                (void)a21g_c_001B1470(o, x, &r);
                a21g_w32(o, self + 0xC4, r);
                a21g_829310(o, w + 0x30, (int32_t)a21g_u8(o, D_00810E65));
                m = a21g_model(o, self);
                x = a21g_u32(o, w + 0x30);
                a21g_w32(o, m + 0x70, A21G_ADD(a21g_u32(o, m + 0x70), x));
                r = 0;
                (void)a21g_c_001B1470(o, a21g_u32(o, a21g_model(o, self) + 0x70), &r);
                a21g_w32(o, a21g_model(o, self) + 0x70, r);
                m = a21g_model(o, self);
                x = a21g_u32(o, m + 0x70);
                if (!A21G_LE(x, F_0_69813174)) {
                    a21g_w32(o, m + 0x70, F_0_69813174);
                    a21g_w32(o, w + 0x30, F_ZERO);
                } else if (A21G_LT(x, F_M0_69813174)) {
                    a21g_w32(o, m + 0x70, F_M0_69813174);
                    a21g_w32(o, w + 0x30, F_ZERO);
                }
                a21g_829590(o, self, w);
                const uint32_t b74 = a21g_u16(o, D_70003B74);
                const uint32_t b78 = a21g_u16(o, D_70003B78);
                if ((a21g_u16(o, D_00810E70) & (b74 | b78)) != 0) {
                    a21g_w32(o, w + 0x3C, A21G_ADD(a21g_u32(o, w + 0x3C), F_ONE));
                    if (a21g_u8(o, self + 7) == 0) {
                        uint32_t ob = 0;
                        (void)a21g_c_001B1E20(o, 0xA, 0);
                        a21g_w8(o, self + 7, 1);
                        a21g_w16(o, self + 0x28, 6);
                        a21g_w32(o, w + 0x38, F_HALF_PI);
                        (void)a21g_c_001FB9F0(o, 0x451, 0x1000, 0x1000, 0x1000);
                        (void)a21g_c_001EFD90(o, 0xA, w, self + 0x70);
                        (void)a21g_c_001F4F40(o, 2, &ob);
                        if (ob != 0) {
                            (void)a21g_c_00102948(o, ob + 0xB0, w);
                            (void)a21g_c_00102958(o, ob + 0xD0, a21g_model(o, self) + 0x90);
                            (void)a21g_c_00102948(o, ob + 0x100, w);
                            a21g_w32(o, ob + 0x10C, F_ONE);
                            (void)a21g_c_00102958(o, D_700036A0, a21g_model(o, self) + 0x90);
                            (void)a21g_c_001029C0(o, D_70003400);
                            (void)a21g_c_00102BB0(o, D_70003400, D_70003400, F_HALF_PI);
                            (void)a21g_c_001026D0(o, D_700036A0, D_700036A0, D_70003400);
                            a21g_vec(o, D_70003600, F_THREE, F_FOUR, F_HALF, F_ONE);
                            (void)a21g_c_001026A0(o, D_700036D0, D_700036A0, D_70003600);
                            (void)a21g_c_001F4010(o, 3, D_700036A0);
                        }
                    }
                } else {
                    x = a21g_u32(o, w + 0x3C);
                    if (!A21G_EQ(F_ZERO, x)) a21g_w32(o, w + 0x3C, A21G_SUB(x, F_HALF));
                }
                if (a21g_u8(o, self + 7) != 0) {
                    const uint32_t n = (uint32_t)(a21g_s16(o, self + 0x28) - 1);
                    a21g_w16(o, self + 0x28, n);
                    if ((int16_t)n == 0) {
                        a21g_w8(o, self + 7, 0);
                        a21g_w32(o, a21g_model(o, self) + 0x7C, F_ZERO);
                        a21g_w32(o, a21g_model(o, self) + 0x80, F_ZERO);
                        a21g_w32(o, a21g_model(o, self) + 0x84, F_ZERO);
                        a21g_w32(o, w + 0x38, F_HALF_PI);
                    } else {
                        uint32_t c = 0;
                        a21g_w32(o, w + 0x38, A21G_SUB(a21g_u32(o, w + 0x38), F_0_5235988));
                        a21g_w32(o, D_70003600, F_ZERO);
                        a21g_w32(o, D_70003600 + 4, F_ZERO);
                        (void)a21g_c_0011DE90(o, a21g_u32(o, w + 0x38), &c);
                        a21g_w32(o, D_70003600 + 8, A21G_MUL(F_0_15, c));
                        a21g_w32(o, D_70003600 + 12, F_ONE);
                        (void)a21g_c_001029C0(o, D_70003400);
                        (void)a21g_c_00102B08(o, D_70003400, D_70003400, a21g_u32(o, a21g_model(o, self) + 0x70));
                        (void)a21g_c_001026A0(o, D_70003600, D_70003400, D_70003600);
                        x = a21g_u32(o, D_70003600);
                        a21g_w32(o, a21g_model(o, self) + 0x7C, x);
                        x = a21g_u32(o, D_70003600 + 4);
                        a21g_w32(o, a21g_model(o, self) + 0x80, x);
                        x = a21g_u32(o, D_70003600 + 8);
                        a21g_w32(o, a21g_model(o, self) + 0x84, x);
                    }
                }
                (void)a21g_c_001C6380(o, self);
                (void)a21g_c_001A2370(o, self, self + 0xD0);
                a21g_8294C0(o, self, pl);
                x = a21g_u32(o, w + 0x3C);
                if (!A21G_LE(x, F_900)) {
                    a21g_w8(o, self, 2);
                    a21g_w8(o, pl + 0x302, 1);
                    a21g_w32(o, w + 0x3C, F_1800);
                    a21g_w8(o, self + 5, 2);
                    a21g_w32(o, w + 0x30, F_ZERO);
                    a21g_w32(o, w + 0x34, F_ZERO);
                    a21g_w32(o, w + 0x38, F_ZERO);
                    a21g_w32(o, a21g_model(o, self) + 0x70, F_ZERO);
                    a21g_w32(o, a21g_model(o, self) + 0x7C, F_ZERO);
                    a21g_w32(o, a21g_model(o, self) + 0x80, F_ZERO);
                    a21g_w32(o, a21g_model(o, self) + 0x84, F_ZERO);
                    a21g_829F10(o, self);
                } else {
                    a21g_8260A0(o, A21G_DIV(x, F_900));
                }
            } else if (sub == 0) {
                if (a21g_u8(o, pl + 0x1F1) == 1) {
                    a21g_w8(o, self + 6, sub + 1);
                    a21g_w8(o, self + 7, 0);
                    a21g_w32(o, w + 0x30, F_ZERO);
                    a21g_w32(o, w + 0x34, F_ZERO);
                    a21g_829590(o, self, w);
                }
            }
        }
    } else if (step == 0) {
        if (a21g_s32(o, pl + 0x230) == 0x28 && a21g_u8(o, pl + 0x1F0) == 0x38) {
            a21g_w8(o, self + 5, step + 1);
            a21g_w8(o, self + 6, 0);
        }
        x = a21g_u32(o, w + 0x3C);
        if (!A21G_EQ(F_ZERO, x)) a21g_w32(o, w + 0x3C, A21G_SUB(x, F_HALF));
    }
    (void)a21g_c_001B17A0(o, self);
    (void)a21g_method(o, self);
    const uint32_t child = a21g_u32(o, self + 0x20);
    if (child != 0) a21g_w32(o, child + 0xC4, a21g_u32(o, self + 0xC4));
    a21g_vec(o, D_700038A0, F_ZERO, F_ZERO, F_2_323, F_ONE);
    (void)a21g_c_001026A0(o, D_700038A0, a21g_model(o, self) + 0x90, D_700038A0);
    (void)a21g_c_001F5940(o, 7, D_700038A0, 0);
}

int em_area21_overlay_008297C0(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea21OverlayFault *fault)
{
    A21G o;
    if (a21g_begin(&o, h, fault)) return -1;
    a21g_8297C0(&o, a0, sp);
    return a21g_end(&o);
}
