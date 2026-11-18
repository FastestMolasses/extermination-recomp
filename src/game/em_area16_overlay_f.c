/* AREA16's own overlay owners (em_area16_overlay.h), group f: the five
 * effect behaviours 0x8237A0, 0x8239B0, 0x823D20, 0x823F70 and 0x824110.
 * No AREA16 word names them: an effect node gets its behaviour from its
 * effect id, 001EF9D0 reading the +0x0C word of AREA16's effect table
 * (D_00259C74[0x10], 0x30 bytes a record): ids 0..4 are these five in
 * order. The spawners: 0x823580's 001EFEB0(0, ..) (id 0), [10] 0x825800's
 * 001EFD20(1, ..) and (3, ..), [11] 0x8261A0's 001EFD20(2, +0xB0), [13]
 * 0x8263D0's 001EFD20(4, +0xB0), whose +0x2D0 keeps the node and drives
 * its +5 (docs/AREA16_PORT.md "Effects").
 * Each function is one original function; the comment above it says what
 * the code does, and the order of the memory accesses between calls is the
 * original's (the test compares it one for one). Ground truth: the decomp's
 * C under src/overlays/AREA16/ (0x8237A0 is NEARMISS: its access order is
 * the original's); evidence: tools/test_area16_overlay_reference.py
 * (tools/area16_overlay_cases_f.py). */
#include "em_area16_overlay_internal.h"

#define D_700036A0 0x700036A0u   /* scratchpad matrix */
#define D_700036D0 0x700036D0u   /* scratchpad position */
#define D_70003A20 0x70003A20u   /* scratchpad fraction */

/* overlay data (runtime addresses): the effects' draw tables and points */
#define A16F_TABLE_0A 0x00828DD0u
#define A16F_TABLE_0B 0x00828E60u
#define A16F_TABLE_1A 0x00828EF0u
#define A16F_TABLE_1B 0x00828F80u
#define A16F_TABLE_2 0x00829010u
#define A16F_POINTS_2 0x008290A0u   /* fifteen quadword points */
#define A16F_ORIGIN_2 0x008290E0u
#define A16F_TABLE_3A 0x00829190u
#define A16F_TABLE_3B 0x00829220u
#define A16F_TABLE_4 0x008292B0u

/* 001CD520's GIF tag in 0x8237A0 */
#define A16F_GIFTAG 0x20045BA5154222DCull

/* float bit patterns */
#define F_0 0x00000000u
#define F_1 0x3F800000u
#define F_2 0x40000000u
#define F_5 0x40A00000u
#define F_7 0x40E00000u
#define F_80 0x42A00000u
#define F_160 0x43200000u
#define F_300 0x43960000u
#define F_1_1 0x3F8CCCCDu
#define F_1_2 0x3F99999Au
#define F_1_5 0x3FC00000u
#define F_3_5 0x40600000u
#define F_0_4 0x3ECCCCCDu
#define F_0_2 0x3E4CCCCDu
#define F_0_1 0x3DCCCCCDu
#define F_0_05 0x3D4CCCCDu
#define F_0_02 0x3CA3D70Au
#define F_0_015 0x3C75C28Fu
#define F_0_01 0x3C23D70Au
#define F_0_007 0x3BE56042u
#define F_0_0001 0x38D1B717u
#define F_1_30 0x3D088889u     /* 0.033333335 */
#define F_1_60 0x3C888889u     /* 0.016666668 */
#define F_65535 0x477FFF00u
#define F_2P31 0x4F000000u     /* 2147483648 */
#define F_304 0x43980000u
#define F_210 0x43520000u
#define F_456 0x43E40000u
#define F_178 0x43320000u
#define F_314_8 0x439D6666u
#define F_176_8 0x4330CCCDu
#define F_428_1 0x43D60CCDu
#define F_309_1 0x439A8CCDu
#define F_184_6 0x4338999Au
#define F_428_6 0x43D64CCDu

/* 00122BB8 / 2^31: a fraction of the random. */
static uint32_t a16f_rand_fraction(A16 *o)
{
    int32_t r = 0;
    (void)a16_c_00122BB8(o, &r);
    return A16_DIV(A16_CVT_S_W(r), F_2P31);
}

/* The seeded random the effects share: the high half of the seed as a
 * fraction of 65535, plus 0.0001. */
static uint32_t a16f_seeded(uint32_t seed)
{
    const uint32_t r = A16_DIV(A16_CVT_S_W((int32_t)((seed >> 16) & 0xFFFFu)), F_65535);
    return A16_ADD(r, F_0_0001);
}

static uint32_t a16f_next(uint32_t seed) { return seed * 37u + 11u; }

/* ----------------------------------------------------------------------
 * 0x8237A0 (decomp func_overlay_AREA16_00823760, NEARMISS) — effect id 0;
 * frame 0x50.
 * State 0: +0x1F0 = 0, +0x1F4 / +0x1F8 = two 00122BB8 / 2^31, state 1,
 *  then state 1 at once.
 * State 1: t = 1 - +0x1F0 / 1.1, kept at D_70003A20; the colour is three
 *  001281C0 bytes (80t, 80t, 160t) -> 001CD520(0, 2, self + 0x100, the
 *  tag, colour, 7, 7, 5); 001D04B0(self + 0xD0, 2, 0x828DD0, +0x1F0,
 *  +0x1F4) and (self + 0xD0, 1, 0x828E60, +0x1F0, +0x1F8); +0x1F0 += 0.02,
 *  state 3 past 1.1.
 * States 2 and 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_8237A0(A16 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0u;
    switch (a16_u8(o, self + 4)) {
    case 0:
        a16_w32(o, blk, F_0);
        a16_w32(o, blk + 4, a16f_rand_fraction(o));
        a16_w32(o, blk + 8, a16f_rand_fraction(o));
        a16_w8(o, self + 4, 1);
        /* fall through */
    case 1: {
        const uint32_t t = A16_SUB(F_1, A16_DIV(a16_u32(o, blk), F_1_1));
        a16_w32(o, D_70003A20, t);
        const uint32_t a = A16_MUL(F_80, t);
        int32_t r = 0;
        uint32_t c;
        (void)a16_c_001281C0(o, a, &r);
        c = (uint32_t)r;
        r = 0;
        (void)a16_c_001281C0(o, a, &r);
        c |= (uint32_t)r << 8;
        r = 0;
        (void)a16_c_001281C0(o, A16_MUL(F_160, t), &r);
        c |= (uint32_t)r << 16;
        (void)a16_c_001CD520(o, 0, 2, self + 0x100u, A16F_GIFTAG, c, F_7, F_7, F_5);
        uint32_t f12 = a16_u32(o, blk);
        uint32_t f13 = a16_u32(o, blk + 4);
        (void)a16_c_001D04B0(o, self + 0xD0u, 2, A16F_TABLE_0A, f12, f13);
        f12 = a16_u32(o, blk);
        f13 = a16_u32(o, blk + 8);
        (void)a16_c_001D04B0(o, self + 0xD0u, 1, A16F_TABLE_0B, f12, f13);
        const uint32_t x = A16_ADD(a16_u32(o, blk), F_0_02);
        a16_w32(o, blk, x);
        if (!A16_LE(x, F_1_1)) a16_w8(o, self + 4, 3);
        break;
    }
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------------
 * 0x8239B0 (decomp func_overlay_AREA16_00823970) — effect id 1; frame
 * 0xA0, the packet at sp - 0xA0 + 0x40.
 * State 0: the self + 0xD0 matrix (identity, 00102C58 by +0xC0, 00102918
 *  by +0xB0), +0x1F0 = 1, +0x1F4 = +0x1F8 = 0, +0x1FC = 00122BB8, state 1,
 *  +5 = 0, then state 1 at once.
 * State 1 by +5: 0 fades +0x1F4 in by 1/30 (past 1: 1 and +5 = 1); 1
 *  advances +0x1F8 by 0.01 and past 3.5 fades +0x1F4 out by 1/60 (below 0:
 *  state 3). Then two packets from the seed +0x1FC: matrix D_700036A0 =
 *  identity, position (304, 210, 456, 1), h = 001CCF70(position),
 *  001CFAE0(pkt, 0, matrix, +0x1F0, seeded, +0x1F4, 0.1), 001CFBE0(h, 1,
 *  0x828EF0, pkt, 1); again at (304, 178, 456, 1) with the next seed and
 *  0x828F80. +0x1F0 += 0.007, past 2: -= 1.
 * States 2 and 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_8239B0(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t blk = self + 0x1F0u;
    const uint32_t pkt = sp - 0xA0u + 0x40u;
    uint32_t x;
    switch (a16_u8(o, self + 4)) {
    case 0: {
        (void)a16_c_001029C0(o, self + 0xD0u);
        (void)a16_c_00102C58(o, self + 0xD0u, self + 0xD0u, self + 0xC0u);
        (void)a16_c_00102918(o, self + 0xD0u, self + 0xD0u, self + 0xB0u);
        a16_w32(o, blk, F_1);
        a16_w32(o, blk + 4, F_0);
        a16_w32(o, blk + 8, F_0);
        int32_t r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, blk + 0xC, (uint32_t)r);
        a16_w8(o, self + 4, 1);
        a16_w8(o, self + 5, 0);
    }
        /* fall through */
    case 1: {
        switch (a16_u8(o, self + 5)) {
        case 0:
            x = A16_ADD(a16_u32(o, blk + 4), F_1_30);
            a16_w32(o, blk + 4, x);
            if (!A16_LE(x, F_1)) {
                a16_w32(o, blk + 4, F_1);
                a16_w8(o, self + 5, a16_u8(o, self + 5) + 1u);
            }
            break;
        case 1:
            x = A16_ADD(a16_u32(o, blk + 8), F_0_01);
            a16_w32(o, blk + 8, x);
            if (!A16_LE(x, F_3_5)) {
                x = A16_SUB(a16_u32(o, blk + 4), F_1_60);
                a16_w32(o, blk + 4, x);
                if (A16_LT(x, F_0)) a16_w8(o, self + 4, 3);
            }
            break;
        default:
            break;
        }
        uint32_t seed = a16_u32(o, blk + 0xC);
        int32_t h = 0;
        (void)a16_c_001029C0(o, D_700036A0);
        a16_w32(o, D_700036D0, F_304);
        a16_w32(o, D_700036D0 + 4, F_210);
        a16_w32(o, D_700036D0 + 8, F_456);
        a16_w32(o, D_700036D0 + 0xC, F_1);
        (void)a16_c_001CCF70(o, D_700036D0, &h);
        uint32_t f12 = a16_u32(o, blk);
        uint32_t r = a16f_seeded(seed);
        seed = a16f_next(seed);
        uint32_t f14 = a16_u32(o, blk + 4);
        (void)a16_c_001CFAE0(o, pkt, 0, D_700036A0, f12, r, f14, F_0_1);
        (void)a16_c_001CFBE0(o, h, 1, A16F_TABLE_1A, pkt, 1);
        h = 0;
        (void)a16_c_001029C0(o, D_700036A0);
        a16_w32(o, D_700036D0, F_304);
        a16_w32(o, D_700036D0 + 4, F_178);
        a16_w32(o, D_700036D0 + 8, F_456);
        a16_w32(o, D_700036D0 + 0xC, F_1);
        (void)a16_c_001CCF70(o, D_700036D0, &h);
        f12 = a16_u32(o, blk);
        r = a16f_seeded(seed);
        f14 = a16_u32(o, blk + 4);
        (void)a16_c_001CFAE0(o, pkt, 0, D_700036A0, f12, r, f14, F_0_1);
        (void)a16_c_001CFBE0(o, h, 1, A16F_TABLE_1B, pkt, 1);
        x = A16_ADD(a16_u32(o, blk), F_0_007);
        a16_w32(o, blk, x);
        if (!A16_LE(x, F_2)) a16_w32(o, blk, A16_SUB(a16_u32(o, blk), F_1));
        break;
    }
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------------
 * 0x823D20 (decomp func_overlay_AREA16_00823CE0) — effect id 2; frame
 * 0x70.
 * State 0: fifteen +0x1F0 floats = 0.4 + 0.2 * (00122BB8 / 2^31), +0x22C =
 *  00122BB8, state 1, position +0xB0 = 0x8290E0 (00102948), 001FBD50(self,
 *  0x928, 0, 300), then state 1 at once.
 * State 1: from the seed +0x22C, for each of the fifteen: the seeded
 *  random into D_70003A20; a value below 1.5 draws (matrix D_700036A0 =
 *  identity translated by 0x8290A0[i], 001D04B0(matrix, 1, 0x829010,
 *  value, D_70003A20)) and advances by 0.015; state 3 when all fifteen are
 *  at 1.5 or more.
 * States 2 and 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_823D20(A16 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0u;
    switch (a16_u8(o, self + 4)) {
    case 0: {
        for (uint32_t i = 0; i < 15u; ++i) {
            const uint32_t f = a16f_rand_fraction(o);
            a16_w32(o, blk + 4u * i, A16_ADD(F_0_4, A16_MUL(F_0_2, f)));
        }
        int32_t r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, blk + 0x3C, (uint32_t)r);
        a16_w8(o, self + 4, 1);
        (void)a16_c_00102948(o, self + 0xB0u, A16F_ORIGIN_2);
        (void)a16_c_001FBD50(o, self, 0x928, 0, F_300);
    }
        /* fall through */
    case 1: {
        uint32_t seed = a16_u32(o, blk + 0x3C);
        int32_t done = 0;
        for (uint32_t i = 0; i < 15u; ++i) {
            const uint32_t at = blk + 4u * i;
            const uint32_t r = a16f_seeded(seed);
            seed = a16f_next(seed);
            a16_w32(o, D_70003A20, r);
            if (A16_LT(a16_u32(o, at), F_1_5)) {
                (void)a16_c_001029C0(o, D_700036A0);
                (void)a16_c_00102918(o, D_700036A0, D_700036A0, A16F_POINTS_2 + 0x10u * i);
                const uint32_t f12 = a16_u32(o, at);
                const uint32_t f13 = a16_u32(o, D_70003A20);
                (void)a16_c_001D04B0(o, D_700036A0, 1, A16F_TABLE_2, f12, f13);
                a16_w32(o, at, A16_ADD(a16_u32(o, at), F_0_015));
            } else {
                ++done;
            }
        }
        if (done == 15) a16_w8(o, self + 4, 3);
        break;
    }
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------------
 * 0x823F70 (decomp func_overlay_AREA16_00823F30) — effect id 3; frame
 * 0x40.
 * State 0: the self + 0xD0 matrix (identity, 00102C58 by +0xC0, 00102918
 *  by +0xB0), +0x1F0 = 0, +0x1F4 = 00122BB8, state 1, then state 1 at
 *  once.
 * State 1: from the seed +0x1F4, 001D04B0(self + 0xD0, 1, 0x829190,
 *  +0x1F0, seeded) and (.., 0x829220, +0x1F0, the next seeded); +0x1F0 +=
 *  0.02, state 3 past 1.2.
 * States 2 and 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_823F70(A16 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0u;
    switch (a16_u8(o, self + 4)) {
    case 0: {
        (void)a16_c_001029C0(o, self + 0xD0u);
        (void)a16_c_00102C58(o, self + 0xD0u, self + 0xD0u, self + 0xC0u);
        (void)a16_c_00102918(o, self + 0xD0u, self + 0xD0u, self + 0xB0u);
        a16_w32(o, blk, F_0);
        int32_t r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, blk + 4, (uint32_t)r);
        a16_w8(o, self + 4, 1);
    }
        /* fall through */
    case 1: {
        uint32_t seed = a16_u32(o, blk + 4);
        uint32_t r = a16f_seeded(seed);
        seed = a16f_next(seed);
        uint32_t f12 = a16_u32(o, blk);
        (void)a16_c_001D04B0(o, self + 0xD0u, 1, A16F_TABLE_3A, f12, r);
        r = a16f_seeded(seed);
        f12 = a16_u32(o, blk);
        (void)a16_c_001D04B0(o, self + 0xD0u, 1, A16F_TABLE_3B, f12, r);
        const uint32_t x = A16_ADD(a16_u32(o, blk), F_0_02);
        a16_w32(o, blk, x);
        if (!A16_LE(x, F_1_2)) a16_w8(o, self + 4, 3);
        break;
    }
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------------
 * 0x824110 (decomp func_overlay_AREA16_008240D0) — effect id 4 ([13]
 * 0x8263D0 keeps the node at its +0x2D0 and sets its +5); frame 0xA0, the
 * packet at sp - 0xA0 + 0x40.
 * State 0: +0x1F0 = +0x1F4 = 0, +0x1F8 = 00122BB8 / 2^31, state 1, +5 = 0,
 *  the self + 0xD0 identity, then state 1 at once.
 * State 1 by +5: 1 fades +0x1F4 in by 0.2 (to at most 1), +0x1F0 = 1 when
 *  it was 0, position +0x100 = (314.8, 176.8, 428.1, 1); 2 fades in by 1/30
 *  at (309.1, 184.6, 428.6, 1); 3 fades out by 1/30 (below 0: +0x1F4 =
 *  +0x1F0 = 0, +5 = 0); 0x63 fades out by 0.1 (below 0: state 3); others
 *  nothing. While +0x1F4 != 0: h = 001CCF70(self + 0x100),
 *  001CFAE0(pkt, 0, self + 0xD0, +0x1F0, +0x1F8, +0x1F4, 0.05),
 *  001CFBE0(h, 1, 0x8292B0, pkt, 0), +0x1F0 += 0.015 (past 2: -= 1).
 * States 2 and 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16f_place(A16 *o, uint32_t self, uint32_t x, uint32_t y, uint32_t z)
{
    a16_w32(o, self + 0x100, x);
    a16_w32(o, self + 0x104, y);
    a16_w32(o, self + 0x108, z);
    a16_w32(o, self + 0x10C, F_1);
}

static void a16_824110(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t blk = self + 0x1F0u;
    const uint32_t pkt = sp - 0xA0u + 0x40u;
    uint32_t x;
    switch (a16_u8(o, self + 4)) {
    case 0:
        a16_w32(o, blk, F_0);
        a16_w32(o, blk + 4, F_0);
        a16_w32(o, blk + 8, a16f_rand_fraction(o));
        a16_w8(o, self + 4, 1);
        a16_w8(o, self + 5, 0);
        (void)a16_c_001029C0(o, self + 0xD0u);
        /* fall through */
    case 1:
        switch (a16_u8(o, self + 5)) {
        case 1:
            x = A16_ADD(a16_u32(o, blk + 4), F_0_2);
            a16_w32(o, blk + 4, x);
            if (!A16_LE(x, F_1)) a16_w32(o, blk + 4, F_1);
            if (A16_EQ(a16_u32(o, blk), F_0)) a16_w32(o, blk, F_1);
            a16f_place(o, self, F_314_8, F_176_8, F_428_1);
            break;
        case 2:
            x = A16_ADD(a16_u32(o, blk + 4), F_1_30);
            a16_w32(o, blk + 4, x);
            if (!A16_LE(x, F_1)) a16_w32(o, blk + 4, F_1);
            a16f_place(o, self, F_309_1, F_184_6, F_428_6);
            break;
        case 3:
            x = A16_SUB(a16_u32(o, blk + 4), F_1_30);
            a16_w32(o, blk + 4, x);
            if (A16_LT(x, F_0)) {
                a16_w32(o, blk + 4, F_0);
                a16_w32(o, blk, F_0);
                a16_w8(o, self + 5, 0);
            }
            break;
        case 0x63:
            x = A16_SUB(a16_u32(o, blk + 4), F_0_1);
            a16_w32(o, blk + 4, x);
            if (A16_LT(x, F_0)) a16_w8(o, self + 4, 3);
            break;
        default:
            break;
        }
        if (!A16_EQ(a16_u32(o, blk + 4), F_0)) {
            int32_t h = 0;
            (void)a16_c_001CCF70(o, self + 0x100u, &h);
            const uint32_t f12 = a16_u32(o, blk);
            const uint32_t f13 = a16_u32(o, blk + 8);
            const uint32_t f14 = a16_u32(o, blk + 4);
            (void)a16_c_001CFAE0(o, pkt, 0, self + 0xD0u, f12, f13, f14, F_0_05);
            (void)a16_c_001CFBE0(o, h, 1, A16F_TABLE_4, pkt, 0);
            x = A16_ADD(a16_u32(o, blk), F_0_015);
            a16_w32(o, blk, x);
            if (!A16_LE(x, F_2)) a16_w32(o, blk, A16_SUB(a16_u32(o, blk), F_1));
        }
        break;
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------- */

int em_area16_overlay_008237A0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_8237A0(&o, a0);
    (void)sp;
    return a16_end(&o);
}

int em_area16_overlay_008239B0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_8239B0(&o, a0, sp);
    return a16_end(&o);
}

int em_area16_overlay_00823D20(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_823D20(&o, a0);
    (void)sp;
    return a16_end(&o);
}

int em_area16_overlay_00823F70(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_823F70(&o, a0);
    (void)sp;
    return a16_end(&o);
}

int em_area16_overlay_00824110(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_824110(&o, a0, sp);
    return a16_end(&o);
}
