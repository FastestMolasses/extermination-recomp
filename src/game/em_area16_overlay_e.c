/* AREA16's own overlay owners (em_area16_overlay.h), group e: 0x8263D0
 * (sub 0's placement owner [13]) and 0x825310 (sub 1's placement owner
 * [10]). Each function is one original function; the comment above it says
 * what the code does. The order of the memory accesses between calls is the
 * original's (the test compares it one for one). Ground truth: the decomp's
 * C under src/overlays/AREA16/ (0x825310 is a NEARMISS: its access order is
 * the original's, read from its .s); evidence:
 * tools/test_area16_overlay_reference.py. */
#include "em_area16_overlay_internal.h"

/* globals */
#define D_00275B40 0x00275B40u   /* object table; [2] +0x80 is the slid value */
#define D_00810809 0x00810809u
#define D_0081080B 0x0081080Bu   /* AREA16's event bits */
#define D_700036A0 0x700036A0u   /* scratchpad matrix */
#define D_700036D0 0x700036D0u   /* scratchpad position */
#define D_700038A0 0x700038A0u   /* scratchpad vector for 001F5940 */
#define D_70003A20 0x70003A20u   /* scratchpad size fraction */
#define D_70003B92 0x70003B92u

/* overlay data: these D_overlay_AREA16_ names are already runtime addresses (the oracle shows it) */
#define A16E_SCRIPT_A 0x0082A6C0u
#define A16E_SCRIPT_B 0x0082A840u
#define A16E_SCRIPT_C 0x0082AA80u
#define A16E_MODEL_13 0x0082AD00u
#define A16E_TABLE_13 0x0082AD20u
#define A16E_TABLE_10 0x0082A130u
#define A16E_SIZE_A 0x0082A140u
#define A16E_SIZE_B 0x0082A148u

/* float bit patterns */
#define F_0 0x00000000u
#define F_1 0x3F800000u
#define F_2 0x40000000u
#define F_4 0x40800000u
#define F_10 0x41200000u
#define F_2P31 0x4F000000u     /* 2147483648 */
#define F_65535 0x477FFF00u
#define F_M1_5 0xBFC00000u     /* -1.5 */
#define F_0_075 0x3D99999Au
#define F_0_02 0x3CA3D70Au
#define F_0_01 0x3C23D70Au
#define F_0_0025 0x3B23D70Au
#define F_0_0001 0x38D1B717u
#define F_0_1 0x3DCCCCCDu
#define F_0_25 0x3E800000u
#define F_0_3 0x3E99999Au
#define F_M1_2 0xBF99999Au
#define F_300 0x43960000u
#define F_115 0x42E60000u
#define F_318 0x439F0000u
#define F_176_3 0x43304CCDu
#define F_486 0x43F30000u
#define F_PI 0x40490FDBu
#define F_MPI 0xC0490FDBu
#define F_M20DEG 0xBEB2B8C3u   /* -0.34906587 */
#define F_MHALFPI 0xBFC90FDBu  /* -1.5707964 */

/* The object D_00275B40[2]'s +0x80 word. */
static uint32_t a16e_slid(A16 *o)
{
    return a16_u32(o, a16_u32(o, D_00275B40) + 8u) + 0x80u;
}

/* ----------------------------------------------------------------------
 * 0x8263D0 (decomp func_overlay_AREA16_00826390) — sub 0's placement
 * owner [13]; frame 0x90, the packet at sp - 0x90 + 0x30.
 * State 0: when 001B0FD0 is 0: 001C6380, +0 = +8 = 1, +0x30 = 0x82AD00,
 *  +0x2EC = 0, +0x2E8 / +0x2E4 / +0x2E0 = three 00122BB8 / 2^31; then with
 *  D_0081080B >= 4: state 2, +8 = +0 = 0, +0x2D0 = 0; else +0x2D0 =
 *  001EFD20(4, +0xB0) and +0x2D4 = bit 0 of D_0081080B, the D_00275B40[2]
 *  object's +0x80 = -1.5 (bit 0) or 0.
 * State 1: with bit 2 of D_0081080B: +0 = 2 and, when +0x2D0 is set, its
 *  +5 = 0x63 and +0x2D0 = 0. Sub-state +5: 0 waits for bit 2 of +0xB, then
 *  +0x28 = 0, script 001BA1A0(+0x1F0, 0x82A6C0) with +0x2D4 = 0 (bit 0),
 *  0x82AA80 (bit 1) or 0x82A840 with +0x2D4 = 1; +0x2DC = 0, +5 = 1.
 *  1: +0x28 counts to 120 (at 120 001FBD50(self, 0x19B, 0, 300)); when
 *  001BA1F0 is set +5 = +0xB = 0, else +++0x2DC and for 121..139: at 139 the
 *  +0x80 value = -1.5 (+0x2D4) or 0 and bit 0 of D_0081080B toggles; else
 *  the value -= 0.075 (+0x2D4) or += 0.075.
 *  Then bit 0 of D_0081080B: +0x2EC += 0.02 (above 1 -> 1), +0x2D0's +5 =
 *  1 (bit 1) or 2; else +0x2EC -= 0.02 (below 0 -> 0), its +5 = 3. While
 *  +0x2EC != 0: matrix (identity, 00102B08 by -20 deg, 00102BB0 by -pi/2),
 *  position (318, 176.3, 486), h = 001CCF70(position), packet 001CFAE0(pkt,
 *  0, matrix, +0x2E8, +0x2E4, +0x2EC, 0.1), 001CFBE0(h, 1, 0x82AD20, pkt,
 *  1). +0x2E8 += 0.02 (above 2: -= 1). 001C6380, 001B17A0, the +0x4C
 *  method; with +0 == 1: 001F5940(7, (+0xB0 + 0.25, +0xB4 - 1.2, +0xB8 + 1,
 *  1), 0).
 * State 2: 001B17A0, the method, then the +0x2D0 retire as above.
 * State 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_8263D0(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t blk = self + 0x1F0u;
    const uint32_t pkt = sp - 0x90u + 0x30u;
    int32_t r = 0;
    uint32_t v, x;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r != 0) break;
        (void)a16_c_001C6380(o, self);
        a16_w8(o, self, 1);
        a16_w8(o, self + 8, 1);
        a16_w32(o, self + 0x30, A16E_MODEL_13);
        a16_w32(o, self + 0x2EC, F_0);
        r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, self + 0x2E8, A16_DIV(A16_CVT_S_W(r), F_2P31));
        r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, self + 0x2E4, A16_DIV(A16_CVT_S_W(r), F_2P31));
        r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, self + 0x2E0, A16_DIV(A16_CVT_S_W(r), F_2P31));
        if (a16_u8(o, D_0081080B) >= 4u) {
            a16_w8(o, self + 4, 2);
            a16_w8(o, self + 8, 0);
            a16_w8(o, self, 0);
            a16_w32(o, self + 0x2D0, 0);
        } else {
            uint32_t made = 0;
            (void)a16_c_001EFD20(o, 4, self + 0xB0u, &made);
            a16_w32(o, self + 0x2D0, made);
            if (a16_u8(o, D_0081080B) & 1u) {
                a16_w32(o, self + 0x2D4, 1);
                a16_w32(o, a16e_slid(o), F_M1_5);
            } else {
                a16_w32(o, self + 0x2D4, 0);
                a16_w32(o, a16e_slid(o), F_0);
            }
        }
        break;
    case 1:
        if (a16_u8(o, D_0081080B) & 4u) {
            a16_w8(o, self, 2);
            const uint32_t eff = a16_u32(o, self + 0x2D0);
            if (eff != 0) {
                a16_w8(o, eff + 5, 0x63);
                a16_w32(o, self + 0x2D0, 0);
            }
        }
        switch (a16_u8(o, self + 5)) {
        case 0:
            if (a16_u8(o, self + 0xB) & 4u) {
                a16_w16(o, self + 0x28, 0);
                v = a16_u8(o, D_0081080B);
                if (v & 1u) {
                    (void)a16_c_001BA1A0(o, blk, A16E_SCRIPT_A);
                    a16_w32(o, self + 0x2D4, 0);
                } else {
                    (void)a16_c_001BA1A0(o, blk, (v & 2u) ? A16E_SCRIPT_C : A16E_SCRIPT_B);
                    a16_w32(o, self + 0x2D4, 1);
                }
                a16_w32(o, self + 0x2DC, 0);
                a16_w8(o, self + 5, 1);
            }
            break;
        case 1: {
            const int32_t t = a16_s16(o, self + 0x28);
            if (t < 120) {
                a16_w16(o, self + 0x28, (uint32_t)(t + 1));
                if (a16_s16(o, self + 0x28) == 120)
                    (void)a16_c_001FBD50(o, self, 0x19B, 0, F_300);
            }
            r = 0;
            if (a16_c_001BA1F0(o, self, &r) < 0) return;
            if (r != 0) {
                a16_w8(o, self + 5, 0);
                a16_w8(o, self + 0xB, 0);
                break;
            }
            a16_w32(o, self + 0x2DC, a16_u32(o, self + 0x2DC) + 1u);
            const int32_t n = a16_s32(o, self + 0x2DC);
            if (n < 121 || n >= 140) break;
            if (n == 139) {
                if (a16_u32(o, self + 0x2D4) != 0)
                    a16_w32(o, a16e_slid(o), F_M1_5);
                else
                    a16_w32(o, a16e_slid(o), F_0);
                v = a16_u8(o, D_0081080B);
                a16_w8(o, D_0081080B, (v & 1u) ? (v & 0xFEu) : (v | 1u));
            } else {
                const int down = a16_u32(o, self + 0x2D4) != 0;
                const uint32_t at = a16e_slid(o);
                x = a16_u32(o, at);
                a16_w32(o, at, down ? A16_SUB(x, F_0_075) : A16_ADD(x, F_0_075));
            }
            break;
        }
        default:
            break;
        }
        if (a16_u8(o, D_0081080B) & 1u) {
            x = A16_ADD(a16_u32(o, self + 0x2EC), F_0_02);
            a16_w32(o, self + 0x2EC, x);
            if (!A16_LE(x, F_1)) a16_w32(o, self + 0x2EC, F_1);
            const uint32_t eff = a16_u32(o, self + 0x2D0);
            if (eff != 0) a16_w8(o, eff + 5, (a16_u8(o, D_0081080B) & 2u) ? 1u : 2u);
        } else {
            x = A16_SUB(a16_u32(o, self + 0x2EC), F_0_02);
            a16_w32(o, self + 0x2EC, x);
            if (A16_LT(x, F_0)) a16_w32(o, self + 0x2EC, F_0);
            const uint32_t eff = a16_u32(o, self + 0x2D0);
            if (eff != 0) a16_w8(o, eff + 5, 3);
        }
        if (!A16_EQ(F_0, a16_u32(o, self + 0x2EC))) {
            int32_t hnd = 0;
            (void)a16_c_001029C0(o, D_700036A0);
            (void)a16_c_00102B08(o, D_700036A0, D_700036A0, F_M20DEG);
            (void)a16_c_00102BB0(o, D_700036A0, D_700036A0, F_MHALFPI);
            a16_w32(o, D_700036D0, F_318);
            a16_w32(o, D_700036D0 + 4, F_176_3);
            a16_w32(o, D_700036D0 + 8, F_486);
            (void)a16_c_001CCF70(o, D_700036D0, &hnd);
            const uint32_t f12 = a16_u32(o, self + 0x2E8);
            const uint32_t f13 = a16_u32(o, self + 0x2E4);
            const uint32_t f14 = a16_u32(o, self + 0x2EC);
            (void)a16_c_001CFAE0(o, pkt, 0, D_700036A0, f12, f13, f14, F_0_1);
            (void)a16_c_001CFBE0(o, hnd, 1, A16E_TABLE_13, pkt, 1);
        }
        x = A16_ADD(a16_u32(o, self + 0x2E8), F_0_02);
        a16_w32(o, self + 0x2E8, x);
        if (!A16_LE(x, F_2)) a16_w32(o, self + 0x2E8, A16_SUB(a16_u32(o, self + 0x2E8), F_1));
        (void)a16_c_001C6380(o, self);
        r = 0;
        (void)a16_c_001B17A0(o, self, &r);
        (void)a16_method(o, self);
        if (a16_u8(o, self) == 1u) {
            a16_w32(o, D_700038A0 + 0xC, F_1);
            a16_w32(o, D_700038A0, A16_ADD(F_0_25, a16_u32(o, self + 0xB0)));
            a16_w32(o, D_700038A0 + 4, A16_ADD(F_M1_2, a16_u32(o, self + 0xB4)));
            a16_w32(o, D_700038A0 + 8, A16_ADD(F_1, a16_u32(o, self + 0xB8)));
            (void)a16_c_001F5940(o, 7, D_700038A0, 0);
        }
        break;
    case 2: {
        r = 0;
        (void)a16_c_001B17A0(o, self, &r);
        (void)a16_method(o, self);
        const uint32_t eff = a16_u32(o, self + 0x2D0);
        if (eff != 0) {
            a16_w8(o, eff + 5, 0x63);
            a16_w32(o, self + 0x2D0, 0);
        }
        break;
    }
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    default:
        break;
    }
}

int em_area16_overlay_008263D0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_8263D0(&o, a0, sp);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825310 (decomp func_overlay_AREA16_008252D0, NEARMISS: order from the
 * original) — sub 1's placement owner [10]; frame 0xA0, the packet at
 * sp - 0xA0 + 0x40. The block at +0x1F0: four phase values [0..3], +0x200
 * / +0x204 the x / z, +0x208 the random seed.
 * State 0: when 001B0FD0 is 0: 001C6380, +0x2E8 = +0xB4, +0x2EC = 0, +0x208
 *  = 00122BB8, +0x200 = +0xB0, +0x204 = +0xB8, the four phases = 00122BB8 /
 *  2^31.
 * State 1: unless D_00810809 == 2 with D_70003B92 set: +0x2EC += 0.01
 *  (above pi -> -pi), +0xB4 = +0x2E8 + 10 * 0011E2A8(+0x2EC), 001C6380, and
 *  the +0x4C method when 001B17A0 is set. Then the matrix is reset
 *  (001029C0), position (+0x200, 115, +0x204), and for i = 0..3: size
 *  D_70003A20 = (i + 1) / 4, r = ((seed >> 16) & 0xFFFF) / 65535, seed =
 *  seed * 37 + 11, 0x82A140 = 0x82A148 = 10 * size, 001CFAE0(pkt, 0,
 *  matrix, phase i, r + 0.0001, 1, 0.3), 001CFBE0(0, 1, 0x82A130, pkt, 1),
 *  phase i += 0.0025 * size (above 2: -= 1).
 * State 3 and any other: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_825310(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t blk = self + 0x1F0u;
    const uint32_t pkt = sp - 0xA0u + 0x40u;
    int32_t r = 0;
    uint32_t x;
    int i;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r != 0) break;
        (void)a16_c_001C6380(o, self);
        a16_w32(o, self + 0x2E8, a16_u32(o, self + 0xB4));
        a16_w32(o, self + 0x2EC, F_0);
        r = 0;
        (void)a16_c_00122BB8(o, &r);
        a16_w32(o, blk + 0x18, (uint32_t)r);
        a16_w32(o, blk + 0x10, a16_u32(o, self + 0xB0));
        a16_w32(o, blk + 0x14, a16_u32(o, self + 0xB8));
        for (i = 0; i < 4; i++) {
            r = 0;
            if (a16_c_00122BB8(o, &r) < 0) return;
            a16_w32(o, blk + 4u * (uint32_t)i, A16_DIV(A16_CVT_S_W(r), F_2P31));
        }
        break;
    case 1: {
        int run = 1;
        if (a16_u8(o, D_00810809) == 2u && a16_u8(o, D_70003B92) != 0) run = 0;
        if (run) {
            uint32_t s = 0;
            x = A16_ADD(a16_u32(o, self + 0x2EC), F_0_01);
            a16_w32(o, self + 0x2EC, x);
            if (!A16_LE(x, F_PI)) a16_w32(o, self + 0x2EC, F_MPI);
            (void)a16_c_0011E2A8(o, a16_u32(o, self + 0x2EC), &s);
            s = A16_MUL(F_10, s);
            a16_w32(o, self + 0xB4, A16_ADD(a16_u32(o, self + 0x2E8), s));
            (void)a16_c_001C6380(o, self);
            r = 0;
            if (a16_c_001B17A0(o, self, &r) < 0) return;
            if (r != 0) (void)a16_method(o, self);
        }
        uint32_t seed = a16_u32(o, blk + 0x18);
        (void)a16_c_001029C0(o, D_700036A0);
        a16_w32(o, D_700036D0, a16_u32(o, blk + 0x10));
        a16_w32(o, D_700036D0 + 8, a16_u32(o, blk + 0x14));
        a16_w32(o, D_700036D0 + 4, F_115);
        for (i = 0; i < 4; i++) {
            const uint32_t phase = blk + 4u * (uint32_t)i;
            const uint32_t size = A16_DIV(A16_CVT_S_W(i + 1), F_4);
            const uint32_t hi = (seed >> 16) & 0xFFFFu;
            seed = seed * 37u + 11u;
            a16_w32(o, D_70003A20, size);
            const uint32_t sz = a16_u32(o, D_70003A20);
            uint32_t f13 = A16_DIV(A16_CVT_S_W(hi), F_65535);
            const uint32_t big = A16_MUL(F_10, sz);
            a16_w32(o, A16E_SIZE_A, big);
            a16_w32(o, A16E_SIZE_B, big);
            const uint32_t f12 = a16_u32(o, phase);
            f13 = A16_ADD(f13, F_0_0001);
            if (a16_c_001CFAE0(o, pkt, 0, D_700036A0, f12, f13, F_1, F_0_3) < 0) return;
            if (a16_c_001CFBE0(o, 0, 1, A16E_TABLE_10, pkt, 1) < 0) return;
            const uint32_t sz2 = a16_u32(o, D_70003A20);
            x = A16_ADD(a16_u32(o, phase), A16_MUL(F_0_0025, sz2));
            a16_w32(o, phase, x);
            if (!A16_LE(x, F_2)) a16_w32(o, phase, A16_SUB(a16_u32(o, phase), F_1));
        }
        break;
    }
    default: /* 3 and any other state */
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00825310(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825310(&o, a0, sp);
    return a16_end(&o);
}
