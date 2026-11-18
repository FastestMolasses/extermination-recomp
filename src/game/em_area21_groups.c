/* AREA21's group members (em_area21_groups.h). Each function is one
 * original function; the comment above it says what the code does. The
 * order of the memory accesses between calls is the original's (the test
 * compares it one for one). */
#include "em_area21_groups_internal.h"

/* overlay data (runtime addresses) */
#define A21P_GROUP_82AE50 0x0082AE50u   /* 0x824CF0's packet groups (+0xD 3 / 5) */
#define A21P_GROUP_82AEE0 0x0082AEE0u
#define A21P_GROUP_82AF70 0x0082AF70u   /* 0x824CF0's packet group (+0xD 4) */
#define A21P_GROUP_82B0E0 0x0082B0E0u   /* 0x825B00's packet group */

/* globals */
#define D_700038A0 0x700038A0u   /* a scratchpad vector (0x8238E0's quad position) */
#define D_70003B68 0x70003B68u   /* the frame counter */

/* the GIF tag 0x8238E0 passes to 001CD520 */
#define A21P_GIFTAG_8238E0 0x2004128555322090ull

/* 0x8243B0's globals: the detail byte and the stage short, the wrapped
 * phases, the random values, the packet matrix and the handle source */
#define D_008101E4 0x008101E4u
#define D_0081024E 0x0081024Eu
#define D_70003A20 0x70003A20u
#define D_70003A24 0x70003A24u
#define D_70003A30 0x70003A30u   /* four randoms, 4 bytes apart */
#define D_700036A0 0x700036A0u
#define D_700036D0 0x700036D0u
/* its tables (runtime addresses): 16 bytes a step, 14 steps */
#define A21P_TABLE_82AD70 0x0082AD70u   /* 00102C58's operand */
#define A21P_TABLE_82AC90 0x0082AC90u   /* 00102918's operand */

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_TWO 0x40000000u
#define F_5 0x40A00000u
#define F_10 0x41200000u
#define F_15 0x41700000u
#define F_20 0x41A00000u
#define F_0_1 0x3DCCCCCDu
#define F_0_5 0x3F000000u
#define F_0_75 0x3F400000u
#define F_8 0x41000000u
#define F_48 0x42400000u
#define F_100000 0x47C35000u
#define F_1000000 0x49742400u
#define F_0_005 0x3BA3D70Au
#define F_0_02 0x3CA3D70Au
#define F_PI 0x40490FDBu
#define F_2PI 0x40C90FDBu
#define F_2_31 0x4F000000u        /* 2147483648.0 */
#define F_65535 0x477FFF00u
#define F_0_0001 0x38D1B717u
#define F_1E_6 0x358637BDu
#define F_0_01 0x3C23D70Au
#define F_0_03 0x3CF5C28Fu
#define F_1_2 0x3F99999Au
#define F_1_60 0x3C888889u        /* 0.016666668 */
#define F_1_120 0x3C088889u       /* 0.008333334 */
#define F_1_5 0x3FC00000u

/* ----------------------------------------------------------------------
 * 0x823580 (decomp overlay_AREA21_func_00823540) — group 0x82B500 (x2).
 * +0x1F0 counts consecutive frames and +0x1F4 keeps the last frame
 * counter. State 0: count 0, last = D_70003B68. State 1: the count is
 * reset unless last + 1 is D_70003B68, else incremented; last =
 * D_70003B68; then by the half-word +0x94: 5: every 12th count 00122BB8()
 * (its result unused) and effect 0x80000047 at (+0x100, 15, 15 + +0x108);
 * 3: v = 001C6190(self), k = 20 (35 for 500 < v < 600, 15 for 1040 < v <
 * 1080); when v <= 1000 or v >= 1040 and the count is a multiple of k,
 * effect 0x80000059 at (+0x100, 15, +0x108 + 10 * 0011DE90(2pi * rand /
 * 2^31 - pi)). The position is the frame local at sp - 0x10. Other states:
 * nothing.
 * ---------------------------------------------------------------------- */
static void a21p_823580(A21P *o, uint32_t self, uint32_t sp)
{
    const uint32_t pos = sp - 0x10u;
    const uint32_t w = self + 0x1F0;
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 0) {
        a21p_w32(o, w, 0);
        a21p_w32(o, w + 4, a21p_u32(o, D_70003B68));
        return;
    }
    if (state != 1) return;
    const uint32_t next = a21p_u32(o, w + 4) + 1u;
    if (next != a21p_u32(o, D_70003B68)) {
        a21p_w32(o, w, 0);
    } else {
        a21p_w32(o, w, a21p_u32(o, w) + 1u);
    }
    a21p_w32(o, w + 4, a21p_u32(o, D_70003B68));
    const int32_t kind = a21p_s16(o, self + 0x94);
    if (kind == 5) {
        if (a21p_s32(o, w) % 12 != 0) return;
        int32_t unused = 0;
        (void)a21p_c_00122BB8(o, &unused);
        a21p_w32(o, pos + 4, F_15);
        a21p_w32(o, pos, a21p_u32(o, self + 0x100));
        const uint32_t z = a21p_u32(o, self + 0x108);
        a21p_w32(o, pos + 8, A21P_ADD(F_15, z));
        { uint32_t e_ = 0; (void)a21p_c_001EFD20(o, 0x80000047u, pos, &e_); }
    } else if (kind == 3) {
        int32_t v = 0;
        (void)a21p_c_001C6190(o, self, &v);
        int32_t k = 20;
        if (v > 500 && v < 600) k = 35;
        if (v > 1040 && v < 1080) k = 15;
        if (v > 1000 && v < 1040) return;
        if (a21p_s32(o, w) % k != 0) return;
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        const uint32_t unit = A21P_DIV(A21P_CVT_S_W(r), F_2_31);
        a21p_w32(o, pos + 4, F_15);
        a21p_w32(o, pos, a21p_u32(o, self + 0x100));
        uint32_t s = 0;
        (void)a21p_c_0011DE90(o, A21P_SUB(A21P_MUL(F_2PI, unit), F_PI), &s);
        const uint32_t z = a21p_u32(o, self + 0x108);
        a21p_w32(o, pos + 8, A21P_ADD(z, A21P_MUL(F_10, s)));
        { uint32_t e_ = 0; (void)a21p_c_001EFD20(o, 0x80000059u, pos, &e_); }
    }
}

int em_area21_groups_00823580(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_823580(&o, a0, sp);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8237B0 (decomp func_overlay_AREA21_00823770) — group 0x82BA70 (x12).
 * State 1: v = 001C6190(self), k = 25 (65 for 600 < v < 950); effect 0 at
 * +0x100 (001EFD20) when D_70003B68 is a multiple of k. Other states:
 * nothing.
 * ---------------------------------------------------------------------- */
static void a21p_8237B0(A21P *o, uint32_t self)
{
    if (a21p_u8(o, self + 4) != 1) return;
    int32_t v = 0;
    (void)a21p_c_001C6190(o, self, &v);
    int32_t k = 25;
    if (v > 600 && v < 950) k = 65;
    if (a21p_s32(o, D_70003B68) % k == 0) { uint32_t e_ = 0; (void)a21p_c_001EFD20(o, 0, self + 0x100, &e_); }
}

int em_area21_groups_008237B0(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_8237B0(&o, a0);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823860 (decomp func_overlay_AREA21_00823820) — group 0x82BA70 (x3).
 * State 1: effect 1 at +0x100 (001EFD20) every 30th D_70003B68 frame.
 * Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a21p_823860(A21P *o, uint32_t self)
{
    if (a21p_u8(o, self + 4) != 1) return;
    if (a21p_s32(o, D_70003B68) % 30 == 0) { uint32_t e_ = 0; (void)a21p_c_001EFD20(o, 1, self + 0x100, &e_); }
}

int em_area21_groups_00823860(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_823860(&o, a0);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8238E0 (decomp func_overlay_AREA21_008238A0) — group 0x82BA70 (x3).
 * State 0: +0x28 = 0 (before the call), +0x2A = 1 when 00122BB8() % 2 is
 * not zero, else -1. State 1: D_700038A0 = +0xB0 (00102948), its x +=
 * 0.03 * +0x28, the quad 001CD520(0, 1, D_700038A0, the GIF tag,
 * 0x40808080, 20, 15, 5), then +0x28 += +0x2A. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a21p_8238E0(A21P *o, uint32_t self)
{
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 0) {
        a21p_w16(o, self + 0x28, 0);
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        a21p_w16(o, self + 0x2A, (r % 2) != 0 ? 1u : 0xFFFFu);
    } else if (state == 1) {
        (void)a21p_c_00102948(o, D_700038A0, self + 0xB0);
        const int32_t n = a21p_s16(o, self + 0x28);
        const uint32_t x = a21p_u32(o, D_700038A0);
        a21p_w32(o, D_700038A0, A21P_ADD(x, A21P_MUL(F_0_03, A21P_CVT_S_W(n))));
        (void)a21p_c_001CD520(o, 0, 1, D_700038A0, A21P_GIFTAG_8238E0, 0x40808080u, F_20, F_15, F_5);
        const int32_t a = a21p_s16(o, self + 0x28);
        const int32_t b = a21p_s16(o, self + 0x2A);
        a21p_w16(o, self + 0x28, (uint32_t)(a + b));
    }
}

int em_area21_groups_008238E0(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_8238E0(&o, a0);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823AA0 (decomp func_overlay_AREA21_00823A60) — group 0x82BA70. An
 * empty function: it returns at once.
 * ---------------------------------------------------------------------- */
int em_area21_groups_00823AA0(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault)
{
    A21P o;
    (void)a0;
    if (a21p_begin(&o, h, fault)) return -1;
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8243B0 (decomp func_overlay_AREA21_00824370, NEARMISS C) — sub 0
 * placement [52]: a 14-step packet effect. The work block is +0x1F0: four
 * phases [0..3] and a seed [4]. State 0: each phase = 00122BB8() / 2^31
 * (stored one by one), the seed = 00122BB8(), state 1, on into state 1.
 * State 1: flags 7 and the scales a = b = c = 1.0; when D_008101E4 is 3 the
 * short D_0081024E picks: 29 / 31 clear every flag, 30 clears bit 0 and sets
 * b = c = 0.5, 32 sets b = c = 0.75, other values change nothing; with any
 * other D_008101E4 a = b = c = 0.5. Fog 0021B9A0(2, 0, 100000), (3, 0,
 * 1000000); the seed is read; sa = 48 c, sb = 20 b, sc = 10 b, sd = 8 a.
 * Each of the 14 steps: the matrix D_700036A0 (001029C0, then 00102C58 with
 * table 0x82AD70 + 16 n, then 00102918 with 0x82AC90 + 16 n), the handle h =
 * 001CCF70(D_700036D0), four randoms (bits 16..31 of the seed / 65535 +
 * 0.0001, the seed * 37 + 11 after each) stored at D_70003A30..3C; then by
 * step: 0, 1 (bit 1): the words +0x80 of the groups 0x82A810 / 0x82A8A0 =
 * the unsigned conversion (001281C0) of sa, and two packets (001CFB50 at
 * sp - 0x60 with phase [0] / first random, then phase [1] / the second
 * random read back from memory; 1.0, 0.1, 0.0) queued by 001CFBE0; 8, 9, 12,
 * 13 (bit 2): groups 0x82AA50 / 0x82AAE0 with sb, phases [2] / [3]; 10, 11
 * (bit 2): groups 0x82AB70 / 0x82AC00 with sc, phases [2] / [3]; the other
 * steps (bit 0 and h != 0xFFFFFF): D_70003A20 = [2] + first random and
 * D_70003A24 = [3] + second, each wrapped (above 2.0 less 1.0, the stored
 * value reread), the words of the groups 0x82A930 / 0x82A9C0 = sd, packets
 * with D_70003A20 / the third random and D_70003A24 (reread) / the fourth
 * (reread), 1.0, 1e-6, 0.0. After the loop fog (1, 0, 0) and the four
 * phases advance by 0.01 / 0.005 / 0.02 / 0.01, each wrapped the same way.
 * States 2 / 3: 001AFC10(self). Other states: nothing.
 * ---------------------------------------------------------------------- */
/* phase += step; stored, then above 2.0 less 1.0 (the store reread) */
static void a21p_8243B0_advance(A21P *o, uint32_t addr, uint32_t step)
{
    const uint32_t next = A21P_ADD(a21p_u32(o, addr), step);
    a21p_w32(o, addr, next);
    if (!A21P_LE(next, F_TWO)) a21p_w32(o, addr, A21P_SUB(a21p_u32(o, addr), F_ONE));
}

/* the random of a seed: ((seed >> 16) & 0xFFFF) / 65535 + 0.0001 */
static uint32_t a21p_8243B0_random(uint32_t seed)
{
    return A21P_ADD(A21P_DIV(A21P_CVT_S_W((seed >> 16) & 0xFFFFu), F_65535), F_0_0001);
}

/* one queued packet: 001CFB50(packet, 0, D_700036A0, phase, spread, 1.0,
 * tail, 0.0), then 001CFBE0(h, 1, group, packet, 1) */
static void a21p_8243B0_packet(A21P *o, uint32_t packet, int32_t h, uint32_t group, uint32_t phase,
                               uint32_t spread, uint32_t tail)
{
    (void)a21p_c_001CFB50(o, packet, 0, D_700036A0, phase, spread, F_ONE, tail, F_ZERO);
    (void)a21p_c_001CFBE0(o, h, 1, group, packet, 1);
}

/* a group's +0x80 word and the next group's, both the unsigned conversion
 * of v (the helper called twice) */
static void a21p_8243B0_words(A21P *o, uint32_t first, uint32_t second, uint32_t v)
{
    int32_t t = 0;
    (void)a21p_c_001281C0(o, v, &t);
    a21p_w32(o, first, (uint32_t)t);
    (void)a21p_c_001281C0(o, v, &t);
    a21p_w32(o, second, (uint32_t)t);
}

static void a21p_8243B0(A21P *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0x60u;
    const uint32_t w = self + 0x1F0;
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 3 || state == 2) {
        (void)a21p_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        for (uint32_t i = 0; i < 4; i++) {
            int32_t r = 0;
            (void)a21p_c_00122BB8(o, &r);
            a21p_w32(o, w + 4 * i, A21P_DIV(A21P_CVT_S_W(r), F_2_31));
        }
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        a21p_w32(o, w + 0x10, (uint32_t)r);
        a21p_w8(o, self + 4, 1);
    } else if (state != 1) {
        return;
    }
    uint32_t fa = F_ONE, fb = F_ONE, fc = F_ONE;
    uint32_t flags = 7;
    if (a21p_u8(o, D_008101E4) == 3) {
        const int32_t stage = a21p_s16(o, D_0081024E);
        if (stage == 29 || stage == 31) {
            flags = 0;
        } else if (stage == 30) {
            flags &= ~1u;
            fb = fc = F_0_5;
        } else if (stage == 32) {
            fb = fc = F_0_75;
        }
    } else {
        fa = fb = fc = F_0_5;
    }
    (void)a21p_c_0021B9A0(o, 2, F_ZERO, F_100000);
    (void)a21p_c_0021B9A0(o, 3, F_ZERO, F_1000000);
    uint32_t seed = a21p_u32(o, w + 0x10);
    const uint32_t sa = A21P_MUL(F_48, fc), sb = A21P_MUL(F_20, fb), sc = A21P_MUL(F_10, fb),
                   sd = A21P_MUL(F_8, fa);
    for (uint32_t n = 0; n < 14; n++) {
        (void)a21p_c_001029C0(o, D_700036A0);
        (void)a21p_c_00102C58(o, D_700036A0, D_700036A0, A21P_TABLE_82AD70 + 16u * n);
        (void)a21p_c_00102918(o, D_700036A0, D_700036A0, A21P_TABLE_82AC90 + 16u * n);
        int32_t handle = 0;
        (void)a21p_c_001CCF70(o, D_700036D0, &handle);
        uint32_t rnd[4];
        for (int k = 0; k < 4; k++) {
            rnd[k] = a21p_8243B0_random(seed);
            seed = seed * 37u + 11u;
        }
        for (int k = 0; k < 4; k++) a21p_w32(o, D_70003A30 + 4u * (uint32_t)k, rnd[k]);
        if (n == 0 || n == 1) {
            if (!(flags & 2)) continue;
            a21p_8243B0_words(o, 0x0082A890u, 0x0082A920u, sa);
            a21p_8243B0_packet(o, packet, handle, 0x0082A810u, a21p_u32(o, w), rnd[0], F_0_1);
            a21p_8243B0_packet(o, packet, handle, 0x0082A8A0u, a21p_u32(o, w + 4),
                               a21p_u32(o, D_70003A30 + 4), F_0_1);
        } else if (n == 8 || n == 9 || n == 12 || n == 13) {
            if (!(flags & 4)) continue;
            a21p_8243B0_words(o, 0x0082AAD0u, 0x0082AB60u, sb);
            a21p_8243B0_packet(o, packet, handle, 0x0082AA50u, a21p_u32(o, w + 8), rnd[0], F_0_1);
            a21p_8243B0_packet(o, packet, handle, 0x0082AAE0u, a21p_u32(o, w + 12),
                               a21p_u32(o, D_70003A30 + 4), F_0_1);
        } else if (n == 10 || n == 11) {
            if (!(flags & 4)) continue;
            a21p_8243B0_words(o, 0x0082ABF0u, 0x0082AC80u, sc);
            a21p_8243B0_packet(o, packet, handle, 0x0082AB70u, a21p_u32(o, w + 8), rnd[0], F_0_1);
            a21p_8243B0_packet(o, packet, handle, 0x0082AC00u, a21p_u32(o, w + 12),
                               a21p_u32(o, D_70003A30 + 4), F_0_1);
        } else {
            if (!(flags & 1) || handle == 0xFFFFFF) continue;
            a21p_w32(o, D_70003A20, A21P_ADD(a21p_u32(o, w + 8), rnd[0]));
            const uint32_t second = A21P_ADD(a21p_u32(o, w + 12), rnd[1]);
            uint32_t x = a21p_u32(o, D_70003A20);
            a21p_w32(o, D_70003A24, second);
            if (!A21P_LE(x, F_TWO)) x = A21P_SUB(x, F_ONE);
            a21p_w32(o, D_70003A20, x);
            uint32_t y = a21p_u32(o, D_70003A24);
            if (!A21P_LE(y, F_TWO)) y = A21P_SUB(y, F_ONE);
            a21p_w32(o, D_70003A24, y);
            a21p_8243B0_words(o, 0x0082A9B0u, 0x0082AA40u, sd);
            a21p_8243B0_packet(o, packet, handle, 0x0082A930u, x, rnd[2], F_1E_6);
            a21p_8243B0_packet(o, packet, handle, 0x0082A9C0u, a21p_u32(o, D_70003A24),
                               a21p_u32(o, D_70003A30 + 12), F_1E_6);
        }
    }
    (void)a21p_c_0021B9A0(o, 1, F_ZERO, F_ZERO);
    a21p_8243B0_advance(o, w, F_0_01);
    a21p_8243B0_advance(o, w + 4, F_0_005);
    a21p_8243B0_advance(o, w + 8, F_0_02);
    a21p_8243B0_advance(o, w + 12, F_0_01);
}

int em_area21_groups_008243B0(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_8243B0(&o, a0, sp);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x824CF0 (decomp func_overlay_AREA21_00824CB0) — no static reference;
 * a 001EFD20 effect node of the boss fight (g77 row 3024). The work block is +0x1F0: [0] a phase,
 * [1] a scale, [2] a seed, [3] a frame count. State 0: the +0xD0 matrix
 * from 001029C0 / 00102C58 (+0xC0) / 00102918 (+0xB0), count 0 (before
 * the call), seed = 00122BB8(), phase 0, scale 1.0, state 1, and on into
 * state 1: h = 001CCF70(+0x100); by +0xD: 3 / 5: the packet in the frame
 * (sp - 0x60) from 001CFB50(packet, 0, +0xD0, phase, (seed >> 16 & 0xFFFF)
 * / 65535 + 0.0001, 1.0, 1e-6, 5.0), queued twice by 001CFBE0(h, 1, group,
 * packet, 0) for the groups 0x82AE50 and 0x82AEE0; phase += 0.03 and state
 * 3 unless it is at most 1.2. 4: the same packet with the scale as the
 * third value, queued for 0x82AF70; phase += 0.01, less 1.0 unless it is at
 * most 2.0; the count + 1, and above 240 the scale falls by 1/60, state 3
 * below 0. Other kinds: nothing more. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21p_824CF0(A21P *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0x60u;
    const uint32_t w = self + 0x1F0;
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 3 || state == 2) {
        (void)a21p_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a21p_c_001029C0(o, self + 0xD0);
        (void)a21p_c_00102C58(o, self + 0xD0, self + 0xD0, self + 0xC0);
        (void)a21p_c_00102918(o, self + 0xD0, self + 0xD0, self + 0xB0);
        a21p_w32(o, w + 0xC, 0);
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        a21p_w32(o, w + 8, (uint32_t)r);
        a21p_w32(o, w, F_ZERO);
        a21p_w32(o, w + 4, F_ONE);
        a21p_w8(o, self + 4, 1);
    } else if (state != 1) {
        return;
    }
    int32_t handle = 0;
    (void)a21p_c_001CCF70(o, self + 0x100, &handle);
    const uint32_t kind = a21p_u8(o, self + 0xD);
    const uint32_t seed = a21p_u32(o, w + 8);
    const uint32_t spread =
        A21P_ADD(A21P_DIV(A21P_CVT_S_W((seed >> 16) & 0xFFFFu), F_65535), F_0_0001);

    if (kind == 4) {
        const uint32_t phase = a21p_u32(o, w);
        const uint32_t scale = a21p_u32(o, w + 4);
        (void)a21p_c_001CFB50(o, packet, 0, self + 0xD0, phase, spread, scale, F_1E_6, F_5);
        (void)a21p_c_001CFBE0(o, handle, 1, A21P_GROUP_82AF70, packet, 0);
        const uint32_t next = A21P_ADD(a21p_u32(o, w), F_0_01);
        a21p_w32(o, w, next);
        if (!A21P_LE(next, F_TWO)) a21p_w32(o, w, A21P_SUB(a21p_u32(o, w), F_ONE));
        a21p_w32(o, w + 0xC, a21p_u32(o, w + 0xC) + 1u);
        if (a21p_s32(o, w + 0xC) > 0xF0) {
            const uint32_t less = A21P_SUB(a21p_u32(o, w + 4), F_1_60);
            a21p_w32(o, w + 4, less);
            if (A21P_LT(less, F_ZERO)) a21p_w8(o, self + 4, 3);
        }
    } else if (kind == 5 || kind == 3) {
        const uint32_t phase = a21p_u32(o, w);
        (void)a21p_c_001CFB50(o, packet, 0, self + 0xD0, phase, spread, F_ONE, F_1E_6, F_5);
        (void)a21p_c_001CFBE0(o, handle, 1, A21P_GROUP_82AE50, packet, 0);
        (void)a21p_c_001CFBE0(o, handle, 1, A21P_GROUP_82AEE0, packet, 0);
        const uint32_t next = A21P_ADD(a21p_u32(o, w), F_0_03);
        a21p_w32(o, w, next);
        if (!A21P_LE(next, F_1_2)) a21p_w8(o, self + 4, 3);
    }
}

int em_area21_groups_00824CF0(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_824CF0(&o, a0, sp);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825B00 (decomp func_overlay_AREA21_00825AC0, byte-identical C) — no
 * static reference; the behaviour of an effect node 0014EC00's 001EFD90
 * starts (g77 row 5893). The AREA19 0x824A90's shape with another packet
 * builder. Work block +0x1F0: [0] a phase, [1] a seed fraction. State 0:
 * the +0xD0 matrix from 001029C0 / 00102C58 (+0xC0) / 00102918 (+0xB0),
 * phase 0, [1] = 00122BB8() / 2^31, state 1, and on into state 1: h =
 * 001CCF70(+0x100); the packet in the frame (sp - 0x60) from
 * 001CFAE0(packet, 0, +0xD0, phase, [1], 1.0, 0.1), queued by
 * 001CFBE0(h, 0, group 0x82B0E0, packet, 0); phase += 1/120 and state 3
 * unless it is at most 1.5. States 2 / 3: 001AFC10. Others: nothing.
 * ---------------------------------------------------------------------- */
static void a21p_825B00(A21P *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0x60u;
    const uint32_t w = self + 0x1F0;
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 3 || state == 2) {
        (void)a21p_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a21p_c_001029C0(o, self + 0xD0);
        (void)a21p_c_00102C58(o, self + 0xD0, self + 0xD0, self + 0xC0);
        (void)a21p_c_00102918(o, self + 0xD0, self + 0xD0, self + 0xB0);
        a21p_w32(o, w, F_ZERO);
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        a21p_w32(o, w + 4, A21P_DIV(A21P_CVT_S_W((uint32_t)r), F_2_31));
        a21p_w8(o, self + 4, 1);
    } else if (state != 1) {
        return;
    }
    int32_t handle = 0;
    (void)a21p_c_001CCF70(o, self + 0x100, &handle);
    const uint32_t phase = a21p_u32(o, w);
    const uint32_t seed = a21p_u32(o, w + 4);
    (void)a21p_c_001CFAE0(o, packet, 0, self + 0xD0, phase, seed, F_ONE, F_0_1);
    (void)a21p_c_001CFBE0(o, handle, 0, A21P_GROUP_82B0E0, packet, 0);
    const uint32_t next = A21P_ADD(a21p_u32(o, w), F_1_120);
    a21p_w32(o, w, next);
    if (!A21P_LE(next, F_1_5)) a21p_w8(o, self + 4, 3);
}

int em_area21_groups_00825B00(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_825B00(&o, a0, sp);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x824FE0 (decomp func_overlay_AREA21_00824FA0, byte-identical C) — the
 * node [43] / [44]'s deferred spawn starts (001B6660 from 0x8262D0; the
 * 001EFD20 effect node 0014EC00 creates, behaviour named only in spawn
 * data). A child of the node at +0x24 (par): it watches n of par's child
 * objects, obj(k) = the word at par + 0x110 + 4 * tbl[k], each with its
 * position at +0xC0 and matrix at +0x90. Work block W = +0x1F0: v[12]
 * (saved positions, 16 bytes each), ph[12] at +0xC0, the strength f0 at
 * +0xF0, the table pointer tbl at +0xF4, n at +0xF8, the seed at +0xFC.
 * State 0: by +0xD, 6: mode +5 = 0, tbl 0x82B090, n 7; 7: mode 3, tbl
 * 0x82B0B0, n 12 (other kinds keep tbl and n); clears v[] to (0, 0, 0, 1)
 * and ph[], seed = 00122BB8(), f0 = 1.0, state 1, +6 = 0, and on into
 * state 1. State 1: when par's state +4 is 3, state 3 and nothing more.
 * Else by the mode +5:
 *   0, 4: f0 = 0.
 *   1: each k whose saved y is below -40 and whose object y is now above
 *      -40: effect 0x80000047 at (x, 20, z - 10 * (k / 7)) (D_700038A0);
 *      then f0 = 1.0.
 *   2: f0 = 1.0 when +6 differs from +5; f0 -= 1/60, 0 below 0; each k
 *      whose saved y is above 50 and whose object y is now below 50: an
 *      effect at (x, 20, z - 30 * (k / 7)), 0x80000059 for k 0 and
 *      0x80000047 for the others.
 *   3: f0 = 0; each k whose object y is above -10 and below 20: effect
 *      0x80000047 at (x, 20, z) on frames where D_70003B68 % 12 is 0.
 *   10: for k 6 and 9, when the saved y is above 46 and the object y now
 *      below 46: three effects 2 at the object matrix applied to
 *      (15 * i, 0, 0, 1) (001026A0 into D_700038A0) with y 40; then on
 *      frames where D_70003B68 % 12 is 0, one more from (10, 0, 0, 1)
 *      through the matrix of par's child slot 7 (00122BB8() odd) or 19,
 *      y 40, and the returned node's +0x38 word cleared when non-zero.
 *   11 / 12: three effects 2 at k 6 / 9 the same way (y not forced),
 *      then mode 0.
 *   0x63: state 3, f0 = 0.
 * Then fog 0021B9A0(2, 0, 0) and (3, 0, 2000); when f0 is non-zero, for
 * each k: the matrix D_700036A0 = identity (001029C0) rotated by pi
 * (00102BB0) and translated to the object position (00102918), its
 * translation z (D_700036D8) + 20; the random ((seed >> 16) & 0xFFFF) /
 * 65535 + 0.0001 into D_70003A20 (seed = seed * 37 + 11, a local: the
 * stored seed does not change); when the translation y (D_700036D4) is
 * above 10: h = 001CCF70(D_700036D0), the frame packet from
 * 001CFB50(packet, 0, D_700036A0, ph[k], D_70003A20, f0, 0.1, 20) queued
 * by 001CFBE0(h, 5, 0x82B000, packet, 0); ph[k] += 0.015, less 1.0 when
 * above 2.0. Then fog (1, 0, 0), +6 = +5, and v[k] = the object position
 * (00102948) for each k. States 2 / 3: 001AFC10(self). Others: nothing.
 * ---------------------------------------------------------------------- */
#define A21P_TABLE_82B090 0x0082B090u   /* 7 child slots (+0xD 6) */
#define A21P_TABLE_82B0B0 0x0082B0B0u   /* 12 child slots (+0xD 7) */
#define A21P_GROUP_82B000 0x0082B000u   /* the packet group */
#define F_M40 0xC2200000u
#define F_50 0x42480000u
#define F_M10 0xC1200000u
#define F_46 0x42380000u
#define F_40 0x42200000u
#define F_7 0x40E00000u
#define F_30 0x41F00000u
#define F_2000 0x44FA0000u
#define F_0_015 0x3C75C28Fu
#define A21P_824FE0_PACKET 0x90u   /* the packet: sp - 0x60 */

/* obj(k): par's child pointer for table entry k */
static uint32_t a21p_824FE0_obj(A21P *o, uint32_t par, uint32_t w, uint32_t k)
{
    const uint32_t tbl = a21p_u32(o, w + 0xF4);
    return a21p_u32(o, par + 0x110 + 4u * a21p_u32(o, tbl + 4u * k));
}

/* three effects 2 along the object matrix of entry k (mode 10 / 11 / 12):
 * D_700038A0 = (15 * i, 0, 0, 1) through obj(k) + 0x90, y = 40 when
 * `force` */
static void a21p_824FE0_three(A21P *o, uint32_t par, uint32_t w, uint32_t k, int force)
{
    for (int32_t i = 0; i < 3; i++) {
        a21p_w32(o, D_700038A0, A21P_MUL(F_15, A21P_CVT_S_W(i)));
        a21p_w32(o, D_700038A0 + 4, F_ZERO);
        a21p_w32(o, D_700038A0 + 8, F_ZERO);
        a21p_w32(o, D_700038A0 + 12, F_ONE);
        (void)a21p_c_001026A0(o, D_700038A0, a21p_824FE0_obj(o, par, w, k) + 0x90, D_700038A0);
        if (force) a21p_w32(o, D_700038A0 + 4, F_40);
        uint32_t e = 0;
        (void)a21p_c_001EFD20(o, 2, D_700038A0, &e);
    }
}

static void a21p_824FE0(A21P *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0xF0u + A21P_824FE0_PACKET;
    const uint32_t par = a21p_u32(o, self + 0x24);
    const uint32_t w = self + 0x1F0;
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 2 || state == 3) {
        (void)a21p_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        const uint32_t kind = a21p_u8(o, self + 0xD);
        if (kind == 6) {
            a21p_w8(o, self + 5, 0);
            a21p_w32(o, w + 0xF4, A21P_TABLE_82B090);
            a21p_w32(o, w + 0xF8, 7);
        } else if (kind == 7) {
            a21p_w8(o, self + 5, 3);
            a21p_w32(o, w + 0xF4, A21P_TABLE_82B0B0);
            a21p_w32(o, w + 0xF8, 12);
        }
        for (int32_t j = 0; j < a21p_s32(o, w + 0xF8); j++) {
            const uint32_t v = w + 16u * (uint32_t)j;
            a21p_w32(o, v, F_ZERO);
            a21p_w32(o, v + 4, F_ZERO);
            a21p_w32(o, v + 8, F_ZERO);
            a21p_w32(o, v + 12, F_ONE);
            a21p_w32(o, w + 0xC0 + 4u * (uint32_t)j, F_ZERO);
        }
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        a21p_w32(o, w + 0xFC, (uint32_t)r);
        a21p_w32(o, w + 0xF0, F_ONE);
        a21p_w8(o, self + 4, 1);
        a21p_w8(o, self + 6, 0);
    } else if (state != 1) {
        return;
    }
    if (a21p_u8(o, par + 4) == 3) {
        a21p_w8(o, self + 4, 3);
        return;
    }
    const uint32_t mode = a21p_u8(o, self + 5);
    switch (mode) {
    case 0:
    case 4:
        a21p_w32(o, w + 0xF0, F_ZERO);
        break;
    case 1:
        for (int32_t j = 0; j < a21p_s32(o, w + 0xF8); j++) {
            if (!A21P_LT(a21p_u32(o, w + 16u * (uint32_t)j + 4), F_M40)) continue;
            if (A21P_LE(a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC4), F_M40)) continue;
            a21p_w32(o, D_700038A0 + 4, F_20);
            a21p_w32(o, D_700038A0, a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC0));
            const uint32_t z = a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC8);
            a21p_w32(o, D_700038A0 + 8, A21P_SUB(z, A21P_MUL(F_10, A21P_DIV(A21P_CVT_S_W(j), F_7))));
            uint32_t e = 0;
            (void)a21p_c_001EFD20(o, 0x80000047u, D_700038A0, &e);
        }
        a21p_w32(o, w + 0xF0, F_ONE);
        break;
    case 2: {
        if (a21p_u8(o, self + 6) != mode) a21p_w32(o, w + 0xF0, F_ONE);
        const uint32_t f0 = A21P_SUB(a21p_u32(o, w + 0xF0), F_1_60);
        a21p_w32(o, w + 0xF0, f0);
        if (A21P_LT(f0, F_ZERO)) a21p_w32(o, w + 0xF0, F_ZERO);
        for (int32_t j = 0; j < a21p_s32(o, w + 0xF8); j++) {
            if (A21P_LE(a21p_u32(o, w + 16u * (uint32_t)j + 4), F_50)) continue;
            if (!A21P_LT(a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC4), F_50)) continue;
            a21p_w32(o, D_700038A0 + 4, F_20);
            a21p_w32(o, D_700038A0, a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC0));
            const uint32_t z = a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC8);
            a21p_w32(o, D_700038A0 + 8, A21P_SUB(z, A21P_MUL(F_30, A21P_DIV(A21P_CVT_S_W(j), F_7))));
            uint32_t e = 0;
            (void)a21p_c_001EFD20(o, j == 0 ? 0x80000059u : 0x80000047u, D_700038A0, &e);
        }
        break;
    }
    case 3:
        a21p_w32(o, w + 0xF0, F_ZERO);
        for (int32_t k = 0; k < a21p_s32(o, w + 0xF8); k++) {
            const uint32_t y = a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)k) + 0xC4);
            if (A21P_LE(y, F_M10) || !A21P_LT(y, F_20)) continue;
            a21p_w32(o, D_700038A0 + 4, F_20);
            const int32_t frame = a21p_s32(o, D_70003B68);
            a21p_w32(o, D_700038A0, a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)k) + 0xC0));
            a21p_w32(o, D_700038A0 + 8, a21p_u32(o, a21p_824FE0_obj(o, par, w, (uint32_t)k) + 0xC8));
            if (frame % 12 == 0) {
                uint32_t e = 0;
                (void)a21p_c_001EFD20(o, 0x80000047u, D_700038A0, &e);
            }
        }
        break;
    case 10:
        for (uint32_t j = 0; j < 2; j++) {
            const uint32_t k = j == 0 ? 6 : 9;
            if (A21P_LE(a21p_u32(o, w + 16u * k + 4), F_46)) continue;
            if (!A21P_LT(a21p_u32(o, a21p_824FE0_obj(o, par, w, k) + 0xC4), F_46)) continue;
            a21p_824FE0_three(o, par, w, k, 1);
        }
        if (a21p_s32(o, D_70003B68) % 12 == 0) {
            int32_t r = 0;
            (void)a21p_c_00122BB8(o, &r);
            const uint32_t slot = (r & 1) ? 7 : 0x13;
            a21p_w32(o, D_700038A0, F_10);
            a21p_w32(o, D_700038A0 + 4, F_ZERO);
            a21p_w32(o, D_700038A0 + 8, F_ZERO);
            a21p_w32(o, D_700038A0 + 12, F_ONE);
            (void)a21p_c_001026A0(o, D_700038A0, a21p_u32(o, par + 0x110 + slot * 4) + 0x90, D_700038A0);
            a21p_w32(o, D_700038A0 + 4, F_40);
            uint32_t e = 0;
            (void)a21p_c_001EFD20(o, 2, D_700038A0, &e);
            if (e != 0) a21p_w32(o, e + 0x38, 0);
        }
        break;
    case 11:
        a21p_824FE0_three(o, par, w, 6, 0);
        a21p_w8(o, self + 5, 0);
        break;
    case 12:
        a21p_824FE0_three(o, par, w, 9, 0);
        a21p_w8(o, self + 5, 0);
        break;
    case 0x63:
        a21p_w8(o, self + 4, 3);
        a21p_w32(o, w + 0xF0, F_ZERO);
        break;
    default:
        break;
    }
    (void)a21p_c_0021B9A0(o, 2, F_ZERO, F_ZERO);
    (void)a21p_c_0021B9A0(o, 3, F_ZERO, F_2000);
    if (!A21P_EQ(a21p_u32(o, w + 0xF0), F_ZERO)) {
        uint32_t seed = a21p_u32(o, w + 0xFC);
        for (int32_t j = 0; j < a21p_s32(o, w + 0xF8); j++) {
            (void)a21p_c_001029C0(o, D_700036A0);
            (void)a21p_c_00102BB0(o, D_700036A0, D_700036A0, F_PI);
            (void)a21p_c_00102918(o, D_700036A0, D_700036A0, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC0);
            a21p_w32(o, D_700036D0 + 8, A21P_ADD(a21p_u32(o, D_700036D0 + 8), F_20));
            const uint32_t rr = A21P_ADD(A21P_DIV(A21P_CVT_S_W((seed >> 16) & 0xFFFFu), F_65535), F_0_0001);
            seed = seed * 37u + 11u;
            const uint32_t ty = a21p_u32(o, D_700036D0 + 4);
            a21p_w32(o, D_70003A20, rr);
            if (!A21P_LE(ty, F_10)) {
                int32_t hh = 0;
                (void)a21p_c_001CCF70(o, D_700036D0, &hh);
                (void)a21p_c_001CFB50(o, packet, 0, D_700036A0, a21p_u32(o, w + 0xC0 + 4u * (uint32_t)j),
                                      a21p_u32(o, D_70003A20), a21p_u32(o, w + 0xF0), F_0_1, F_20);
                (void)a21p_c_001CFBE0(o, hh, 5, A21P_GROUP_82B000, packet, 0);
            }
            a21p_8243B0_advance(o, w + 0xC0 + 4u * (uint32_t)j, F_0_015);
        }
    }
    (void)a21p_c_0021B9A0(o, 1, F_ZERO, F_ZERO);
    a21p_w8(o, self + 6, a21p_u8(o, self + 5));
    for (int32_t j = 0; j < a21p_s32(o, w + 0xF8); j++)
        (void)a21p_c_00102948(o, w + 16u * (uint32_t)j, a21p_824FE0_obj(o, par, w, (uint32_t)j) + 0xC0);
}

int em_area21_groups_00824FE0(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_824FE0(&o, a0, sp);
    return a21p_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823B40 (decomp func_overlay_AREA21_00823B00, NEARMISS 99.33%: only
 * register choices around the 001281C0 call differ) — no static
 * reference; the behaviour of the effect node 0x824FE0's 001EFD20 starts
 * (g77 row 12708). Work block +0x1F0: [0] a phase, [1] a seed fraction,
 * [2] a seed. State 0: the +0xD0 matrix from 001029C0 / 00102C58 (+0xC0) /
 * 00102918 (+0xB0), phase 0, [1] = 00122BB8() / 2^31, [2] = 00122BB8(),
 * state 1, and on into state 1, by +0xD:
 *  0: h = 001CCF70(+0x100); the packet in the frame (sp - 0x70) from
 *     001CFB50(packet, 0, +0xD0, phase, [1], 1.0, 0.1, 5.0), queued by
 *     001CFBE0(h, 1, group 0x82A660, packet, 0); phase += 0.01, state 3
 *     unless it is at most 2.0.
 *  1: ten particles from the seed [2] (read once; each random is bits
 *     16..31 / 65535 + 0.0001, then seed = seed * 37 + 11; the seed is not
 *     stored back). Spread (5, 0.2, 5, 1) at 0x700038A0, the size rows
 *     (2, 2.5, 3.5, 1) / (0, 0, 0, 1) at 0x700036A0. Per particle: life =
 *     phase - 0.5 * r0, -1 outside 0..1, else less its 001281C0 (float to
 *     unsigned) part; roll 0x70003A24 = pi * (360 * r1 - 180) / 180; a =
 *     2pi * r2 - pi, 0x70003A20 = life, b = 0.5 * (pi * r3); 0x700038B0 =
 *     (5 * 0011DE90(a), 0.2 * 0011DE90(b), 5 * 0011E2A8(a), 1) scaled by
 *     life and moved through the +0xD0 matrix (VU0 inline: the y term gets
 *     0 * life^2 added); the size 0x700038C0 = the first row (00102948) up
 *     to life 0.5, after that the rows blended by (life - 0.5) / 0.5 and
 *     clamped to 0..255 (VU0 inline; the blend pair lives at sp - 0x10:
 *     [0] the weight, [1] 255.0, [2..3] never written but read with the
 *     quadword). A particle with life below 0 stops there; else the draw
 *     matrix 0x700036E0 = identity, roll, translate by 0x700038B0, its rows
 *     scaled by the size (00102900); h = 001CA7B0(0x70003710, 10.0); when
 *     h >= 0 the colour +0x80..+0x8C times (1 - life) at 0x700038D0 goes
 *     to 001C7900(0x700036E0, 0x700038D0, 0x3F5, 0) and 001CA940(h,
 *     001C6120(D_0028A56C, 6)). Then phase += 0.015, state 3 unless it is
 *     at most 1.5.
 *  2: as 0 with (1.0, 1e-6, 3.0): group 0x82A6F0 and phase += 0.025 when
 *     +0x38 is 1, else group 0x82A780 and += 0.015; state 3 unless the
 *     phase is at most 1.2.
 *  Other kinds: nothing more. States 2 / 3: 001AFC10. Others: nothing.
 * The VU0 registers (vf1..vf3, vf28..vf31, ACC) are this function's
 * scratch: each sequence loads what it reads, so they are locals here;
 * the module keeps no VU state between calls.
 * ---------------------------------------------------------------------- */
#define A21P_GROUP_82A660 0x0082A660u   /* 0x823B40's packet groups (+0xD 0 / 2) */
#define A21P_GROUP_82A6F0 0x0082A6F0u
#define A21P_GROUP_82A780 0x0082A780u
#define D_0028A56C 0x0028A56Cu         /* 001C6120's first argument */
#define D_700036E0 0x700036E0u         /* the particle draw matrix (four rows) */
#define D_700038B0 0x700038B0u         /* the particle position */
#define D_700038C0 0x700038C0u         /* the particle size */
#define D_700038D0 0x700038D0u         /* the particle colour */
#define F_M1 0xBF800000u
#define F_0_2 0x3E4CCCCDu
#define F_2_5 0x40200000u
#define F_3 0x40400000u
#define F_3_5 0x40600000u
#define F_0_025 0x3CCCCCCDu
#define F_180 0x43340000u
#define F_255 0x437F0000u
#define F_360 0x43B40000u

static const uint32_t A21P_VF0[4] = {F_ZERO, F_ZERO, F_ZERO, F_ONE};

/* One VU0 macro instruction; every form here is a measured one, so a
 * refusal is a fail-stop fault at 0x823B40, never a guess. */
static void a21p_823B40_vu(A21P *o, em_vu_op op, unsigned dest, int bc, const uint32_t fs[4],
                           const uint32_t ft[4], const uint32_t acc[4], uint32_t dst[4])
{
    if (a21p_failed(o)) return;
    if (em_vu_vec_bits(op, dest, bc, fs, ft, 0, acc, dst) != EM_EE_FLOAT_OK)
        (void)a21p_latch(o, 0x00823B40u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
}

static void a21p_823B40_lq(A21P *o, uint32_t a, uint32_t v[4])
{
    for (unsigned k = 0; k < 4; k++) v[k] = a21p_u32(o, a + 4 * k);
}

static void a21p_823B40_sq(A21P *o, uint32_t a, const uint32_t v[4])
{
    for (unsigned k = 0; k < 4; k++) a21p_w32(o, a + 4 * k, v[k]);
}

/* the random of a seed: bits 16..31 / 65535 + 0.0001 */
static uint32_t a21p_823B40_rand(uint32_t seed)
{
    return A21P_ADD(A21P_DIV(A21P_CVT_S_W((seed >> 16) & 0xFFFFu), F_65535), F_0_0001);
}

/* +0xD 0 and 2: one packet for a group */
static void a21p_823B40_packet(A21P *o, uint32_t self, uint32_t packet, uint32_t group, uint32_t k1,
                               uint32_t k2, uint32_t step, uint32_t bound)
{
    const uint32_t w = self + 0x1F0;
    int32_t handle = 0;
    (void)a21p_c_001CCF70(o, self + 0x100, &handle);
    const uint32_t phase = a21p_u32(o, w);
    const uint32_t seed = a21p_u32(o, w + 4);
    (void)a21p_c_001CFB50(o, packet, 0, self + 0xD0, phase, seed, F_ONE, k1, k2);
    (void)a21p_c_001CFBE0(o, handle, 1, group, packet, 0);
    const uint32_t next = A21P_ADD(a21p_u32(o, w), step);
    a21p_w32(o, w, next);
    if (!A21P_LE(next, bound)) a21p_w8(o, self + 4, 3);
}

static void a21p_823B40_particles(A21P *o, uint32_t self, uint32_t pair)
{
    const uint32_t w = self + 0x1F0;
    a21p_w32(o, D_700038A0, F_5);
    a21p_w32(o, D_700038A0 + 4, F_0_2);
    a21p_w32(o, D_700038A0 + 8, F_5);
    a21p_w32(o, D_700038A0 + 12, F_ONE);
    a21p_w32(o, D_700036A0, F_TWO);
    a21p_w32(o, D_700036A0 + 4, F_2_5);
    a21p_w32(o, D_700036A0 + 8, F_3_5);
    a21p_w32(o, D_700036A0 + 12, F_ONE);
    a21p_w32(o, D_700036A0 + 16, F_ZERO);
    a21p_w32(o, D_700036A0 + 20, F_ZERO);
    a21p_w32(o, D_700036A0 + 24, F_ZERO);
    a21p_w32(o, D_700036A0 + 28, F_ONE);
    uint32_t seed = a21p_u32(o, w + 8);
    for (int i = 0; i < 10 && !a21p_failed(o); i++) {
        const uint32_t r0 = a21p_823B40_rand(seed);
        seed = seed * 37u + 11u;
        uint32_t life = A21P_SUB(a21p_u32(o, w), A21P_MUL(F_0_5, r0));
        if (A21P_LT(life, F_ZERO) || A21P_LT(F_ONE, life)) {
            life = F_M1;
        } else {
            int32_t whole = 0;
            (void)a21p_c_001281C0(o, life, &whole);
            life = A21P_SUB(life, A21P_CVT_S_W(whole));
        }
        const uint32_t r1 = a21p_823B40_rand(seed);
        seed = seed * 37u + 11u;
        const uint32_t roll = A21P_DIV(A21P_MUL(F_PI, A21P_SUB(A21P_MUL(F_360, r1), F_180)), F_180);
        const uint32_t r2 = a21p_823B40_rand(seed);
        seed = seed * 37u + 11u;
        const uint32_t a = A21P_SUB(A21P_MUL(F_2PI, r2), F_PI);
        a21p_w32(o, D_70003A20, life);
        a21p_w32(o, D_70003A24, roll);
        const uint32_t r3 = a21p_823B40_rand(seed);
        seed = seed * 37u + 11u;
        const uint32_t b = A21P_MUL(F_0_5, A21P_MUL(F_PI, r3));
        uint32_t c = 0;
        (void)a21p_c_0011DE90(o, a, &c);
        a21p_w32(o, D_700038B0, A21P_MUL(a21p_u32(o, D_700038A0), c));
        (void)a21p_c_0011DE90(o, b, &c);
        a21p_w32(o, D_700038B0 + 4, A21P_MUL(a21p_u32(o, D_700038A0 + 4), c));
        (void)a21p_c_0011E2A8(o, a, &c);
        const uint32_t spread_z = a21p_u32(o, D_700038A0 + 8);
        const uint32_t s = a21p_u32(o, D_70003A20);
        a21p_w32(o, D_700038B0 + 8, A21P_MUL(spread_z, c));
        a21p_w32(o, D_700038B0 + 12, F_ONE);
        {
            /* the position: m * (v.xyz * s, v.w), then y += 0 * s * s */
            uint32_t v1[4], m28[4], m29[4], m30[4], m31[4], acc[4];
            uint32_t v2[4] = {s, F_ZERO, F_ZERO, F_ZERO}, v3[4] = {F_ZERO, F_ZERO, F_ZERO, F_ZERO};
            a21p_823B40_lq(o, D_700038B0, v1);
            a21p_823B40_lq(o, self + 0xD0, m28);
            a21p_823B40_lq(o, self + 0xE0, m29);
            a21p_823B40_lq(o, self + 0xF0, m30);
            a21p_823B40_lq(o, self + 0x100, m31);
            a21p_823B40_vu(o, EM_VU_MULBC, 14, 0, v1, v2, NULL, v1);
            a21p_823B40_vu(o, EM_VU_MULBC, 8, 0, v2, v2, NULL, v2);
            a21p_823B40_vu(o, EM_VU_MULBC, 8, 0, v3, v2, NULL, v3);
            a21p_823B40_vu(o, EM_VU_MULABC, 15, 0, m28, v1, NULL, acc);
            a21p_823B40_vu(o, EM_VU_MADDABC, 15, 1, m29, v1, acc, acc);
            a21p_823B40_vu(o, EM_VU_MADDABC, 15, 2, m30, v1, acc, acc);
            a21p_823B40_vu(o, EM_VU_MADDBC, 15, 3, m31, A21P_VF0, acc, v1);
            a21p_823B40_vu(o, EM_VU_ADDBC, 4, 0, v1, v3, NULL, v1);
            a21p_823B40_sq(o, D_700038B0, v1);
        }
        const uint32_t now = a21p_u32(o, D_70003A20);
        if (A21P_LT(F_0_5, now)) {
            a21p_w32(o, pair, A21P_DIV(A21P_SUB(now, F_0_5), F_0_5));
            a21p_w32(o, pair + 4, F_255);
            /* the size: clamp(row0 + (row1 - row0) * pair.x, 0, pair.y) */
            uint32_t v1[4], m28[4], m29[4], k[4];
            a21p_823B40_lq(o, D_700036A0, m28);
            a21p_823B40_lq(o, D_700036A0 + 16, m29);
            a21p_823B40_lq(o, pair, k);
            a21p_823B40_vu(o, EM_VU_SUB, 15, EM_VU_NO_BC, m29, m28, NULL, v1);
            a21p_823B40_vu(o, EM_VU_MULBC, 15, 0, v1, k, NULL, v1);
            a21p_823B40_vu(o, EM_VU_ADD, 15, EM_VU_NO_BC, v1, m28, NULL, v1);
            for (unsigned n = 0; n < 4; n++) v1[n] = em_vu_max_bits(v1[n], A21P_VF0[0]);
            for (unsigned n = 0; n < 4; n++) v1[n] = em_vu_min_bits(v1[n], k[1]);
            a21p_823B40_sq(o, D_700038C0, v1);
        } else {
            (void)a21p_c_00102948(o, D_700038C0, D_700036A0);
        }
        if (A21P_LT(a21p_u32(o, D_70003A20), F_ZERO)) continue;
        (void)a21p_c_001029C0(o, D_700036E0);
        (void)a21p_c_00102BB0(o, D_700036E0, D_700036E0, a21p_u32(o, D_70003A24));
        (void)a21p_c_00102918(o, D_700036E0, D_700036E0, D_700038B0);
        (void)a21p_c_00102900(o, D_700036E0, D_700036E0, a21p_u32(o, D_700038C0));
        (void)a21p_c_00102900(o, D_700036E0 + 0x10, D_700036E0 + 0x10, a21p_u32(o, D_700038C0 + 4));
        (void)a21p_c_00102900(o, D_700036E0 + 0x20, D_700036E0 + 0x20, a21p_u32(o, D_700038C0 + 8));
        int32_t h = 0;
        (void)a21p_c_001CA7B0(o, D_700036E0 + 0x30, F_10, &h);
        if (h < 0) continue;
        const uint32_t q = A21P_SUB(F_ONE, a21p_u32(o, D_70003A20));
        for (unsigned n = 0; n < 4; n++)
            a21p_w32(o, D_700038D0 + 4 * n, A21P_MUL(a21p_u32(o, self + 0x80 + 4 * n), q));
        (void)a21p_c_001C7900(o, D_700036E0, D_700038D0, 0x3F5, 0);
        int32_t v = 0;
        (void)a21p_c_001C6120(o, a21p_s32(o, D_0028A56C), 6, &v);
        (void)a21p_c_001CA940(o, h, v);
    }
    const uint32_t next = A21P_ADD(a21p_u32(o, w), F_0_015);
    a21p_w32(o, w, next);
    if (!A21P_LE(next, F_1_5)) a21p_w8(o, self + 4, 3);
}

static void a21p_823B40(A21P *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0x70u;
    const uint32_t w = self + 0x1F0;
    const uint32_t state = a21p_u8(o, self + 4);

    if (state == 3 || state == 2) {
        (void)a21p_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a21p_c_001029C0(o, self + 0xD0);
        (void)a21p_c_00102C58(o, self + 0xD0, self + 0xD0, self + 0xC0);
        (void)a21p_c_00102918(o, self + 0xD0, self + 0xD0, self + 0xB0);
        a21p_w32(o, w, F_ZERO);
        int32_t r = 0;
        (void)a21p_c_00122BB8(o, &r);
        a21p_w32(o, w + 4, A21P_DIV(A21P_CVT_S_W((uint32_t)r), F_2_31));
        (void)a21p_c_00122BB8(o, &r);
        a21p_w32(o, w + 8, (uint32_t)r);
        a21p_w8(o, self + 4, 1);
    } else if (state != 1) {
        return;
    }
    switch (a21p_u8(o, self + 0xD)) {
    case 0:
        a21p_823B40_packet(o, self, packet, A21P_GROUP_82A660, F_0_1, F_5, F_0_01, F_TWO);
        break;
    case 1:
        a21p_823B40_particles(o, self, sp - 0x10u);
        break;
    case 2:
        if (a21p_s32(o, self + 0x38) == 1)
            a21p_823B40_packet(o, self, packet, A21P_GROUP_82A6F0, F_1E_6, F_3, F_0_025, F_1_2);
        else
            a21p_823B40_packet(o, self, packet, A21P_GROUP_82A780, F_1E_6, F_3, F_0_015, F_1_2);
        break;
    default:
        break;
    }
}

int em_area21_groups_00823B40(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault)
{
    A21P o;
    if (a21p_begin(&o, h, fault)) return -1;
    a21p_823B40(&o, a0, sp);
    return a21p_end(&o);
}
