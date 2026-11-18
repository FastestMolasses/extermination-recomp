/* AREA21's group members (em_area21_groups.h). Each function is one
 * original function; the comment above it says what the code does. The
 * order of the memory accesses between calls is the original's (the test
 * compares it one for one). */
#include "em_area21_groups_internal.h"

/* overlay data (runtime addresses) */
#define A21P_GROUP_82AE50 0x0082AE50u   /* 0x824CF0's packet groups (+0xD 3 / 5) */
#define A21P_GROUP_82AEE0 0x0082AEE0u
#define A21P_GROUP_82AF70 0x0082AF70u   /* 0x824CF0's packet group (+0xD 4) */

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
        (void)a21p_c_001EFD20(o, 0x80000047u, pos);
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
        (void)a21p_c_001EFD20(o, 0x80000059u, pos);
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
    if (a21p_s32(o, D_70003B68) % k == 0) (void)a21p_c_001EFD20(o, 0, self + 0x100);
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
    if (a21p_s32(o, D_70003B68) % 30 == 0) (void)a21p_c_001EFD20(o, 1, self + 0x100);
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
 * 0x824CF0 (decomp func_overlay_AREA21_00824CB0) — no static reference
 * (unreached in the original). The work block is +0x1F0: [0] a phase,
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
