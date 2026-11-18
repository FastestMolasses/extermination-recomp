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

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_TWO 0x40000000u
#define F_5 0x40A00000u
#define F_10 0x41200000u
#define F_15 0x41700000u
#define F_20 0x41A00000u
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
