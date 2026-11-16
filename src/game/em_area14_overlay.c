/* AREA14's own overlay owners (em_area14_overlay.h). Each function is one
 * original function; the comment above it says what the code does. The
 * order of the memory accesses between calls is the original's (the test
 * compares it one for one). */
#include "em_area14_overlay_internal.h"

/* overlay data (runtime addresses) */
#define A14_GROUP_8265A0 0x008265A0u   /* the group [5] starts (001B6660) */
#define A14_PACKET_8266E0 0x008266E0u  /* 0x823580's packet header (001CFBE0) */
#define A14_SCRIPT_826800 0x00826800u  /* [5]'s arrival script */
#define A14_SCRIPT_826C10 0x00826C10u  /* [0]'s script */
#define A14_AREA_826F10 0x00826F10u    /* [0]'s 80-byte area, copied to its frame */
#define A14_SCRIPT_826FC0 0x00826FC0u  /* [2]'s first script (the Major's handover) */
#define A14_SCRIPT_827340 0x00827340u  /* [2]'s Use script */
#define A14_TABLE_827480 0x00827480u   /* [2]'s +0x30 */
#define A14_SCRIPT_827490 0x00827490u  /* [4]'s first script */
#define A14_SCRIPT_827590 0x00827590u  /* [4]'s second script */
#define A14_SCRIPT_828120 0x00828120u  /* [6]'s Use script */
#define A14_TABLE_8283A0 0x008283A0u   /* [6]'s +0x30 */
#define A14_AREA_828400 0x00828400u    /* [2]'s area (001B1EA0 kind 4) */

/* globals */
#define D_PLAYER 0x008102B0u
#define D_PLAYER_POS 0x00810350u        /* the player's position (x, y, z, w) */
#define D_PLAYER_Y 0x00810354u
#define D_PLAYER_ROOT 0x00810360u       /* 0x825C40's copy source and its y at +4 */
#define D_008105D0 0x008105D0u
#define D_008105E0 0x008105E0u
#define D_00810702 0x00810702u
#define D_00810784 0x00810784u
#define D_00810804 0x00810804u
#define D_00810815 0x00810815u
#define D_00810816 0x00810816u
#define D_00810817 0x00810817u
#define D_00810C8C 0x00810C8Cu
#define D_0028A5E8 0x0028A5E8u
#define D_0028A5EC 0x0028A5ECu
#define D_0028A610 0x0028A610u
#define D_70003B92 0x70003B92u
#define D_700038A0 0x700038A0u

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_HALF 0x3F000000u
#define F_TWO 0x40000000u
#define F_0_8 0x3F4CCCCDu
#define F_0_6 0x3F19999Au
#define F_0_001 0x3A83126Fu
#define F_0_01 0x3C23D70Au
#define F_6 0x40C00000u
#define F_15 0x41700000u
#define F_20 0x41A00000u
#define F_80 0x42A00000u
#define F_180 0x43340000u
#define F_435 0x43D98000u
#define F_675 0x4428C000u
#define F_810 0x444A8000u
#define F_816 0x444C0000u
#define F_900 0x44610000u
#define F_2_31 0x4F000000u        /* 2147483648.0 */
#define F_221_6 0x435D999Au
#define F_695_1 0x442DC666u
#define F_1181 0x4493A000u
#define F_PI 0x40490FDBu
#define F_HALF_PI 0x3FC90FDBu


/* ----------------------------------------------------------------------
 * 0x823580 (decomp overlay_AREA14_func_00823540) — [35] / [36]. State 0:
 * the matrix at +0xD0 from 001029C0 / 00102C58 (+0xC0) / 00102918 (+0xB0),
 * +0x1F4 = 0.8, +0x1F0 = 00122BB8() / 2^31, state 1, and on into state 1:
 * 0021B9A0(2, 0, 0) and (3, 0, 700), the packet in the frame (sp - 0x60)
 * from 001CFB50(packet, 0, +0xD0, +0x1F4, +0x1F0, 1.0, 0.6, 15.0) queued by
 * 001CFBE0(001CCF70(+0x100), 5, 0x8266E0, packet, 0); +0x1F4 += 0.001, less
 * 1.0 once it is not below 2.0; 0021B9A0(1, 0, 0). States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a14_823580(A14 *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0x60u;
    uint32_t state = a14_u8(o, self + 4);
    int32_t r = 0;

    if (state == 2 || state == 3) {
        (void)a14_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a14_c_001029C0(o, self + 0xD0);
        (void)a14_c_00102C58(o, self + 0xD0, self + 0xD0, self + 0xC0);
        (void)a14_c_00102918(o, self + 0xD0, self + 0xD0, self + 0xB0);
        a14_w32(o, self + 0x1F4, F_0_8);
        r = 0;
        (void)a14_c_00122BB8(o, &r);
        a14_w32(o, self + 0x1F0, A14_DIV(A14_CVT_S_W(r), F_2_31));
        a14_w8(o, self + 4, 1);
    } else if (state != 1) {
        return;
    }
    (void)a14_c_0021B9A0(o, 2, F_ZERO, F_ZERO);
    (void)a14_c_0021B9A0(o, 3, F_ZERO, A14_CVT_S_W(700));
    r = 0;
    (void)a14_c_001CCF70(o, self + 0x100, &r);
    const int32_t handle = r;
    (void)a14_c_001CFB50(o, packet, 0, self + 0xD0, a14_u32(o, self + 0x1F4), a14_u32(o, self + 0x1F0), F_ONE,
                         F_0_6, F_15);
    (void)a14_c_001CFBE0(o, handle, 5, A14_PACKET_8266E0, packet, 0);
    const uint32_t v = A14_ADD(a14_u32(o, self + 0x1F4), F_0_001);
    a14_w32(o, self + 0x1F4, v);
    if (!A14_LT(v, F_TWO)) a14_w32(o, self + 0x1F4, A14_SUB(a14_u32(o, self + 0x1F4), F_ONE));
    (void)a14_c_0021B9A0(o, 1, F_ZERO, F_ZERO);
}

int em_area14_overlay_00823580(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea14OverlayFault *fault)
{
    A14 o;
    if (a14_begin(&o, h, fault)) return -1;
    a14_823580(&o, a0, sp);
    return a14_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823760 (decomp func_overlay_AREA14_00823720) — [5], the arrival.
 * State 0: +0 = 1; with flag 0x2C set: 001F6B00, 001B6660(group 0x8265A0),
 * state 2; else script 0x826800, state 1. State 1: +0x28 counts to 400,
 * where it runs the same two calls; at the script's end +0x2E = 0xFFFF,
 * D_00810784 = D_00810804 = 0xFF, the two calls unless +0x28 reached 400,
 * state 2. State 2: state 3. Any other state: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a14_823760(A14 *o, uint32_t self)
{
    uint32_t state = a14_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        a14_w8(o, self, 1);
        r = 0;
        (void)a14_c_001BA1C0(o, self, 0x2C, &r);
        if (r != 0) {
            (void)a14_c_001F6B00(o);
            (void)a14_c_001B6660(o, A14_GROUP_8265A0);
            a14_w8(o, self + 4, 2);
        } else {
            (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_826800);
            a14_w8(o, self + 4, 1);
        }
    } else if (state == 1) {
        const int32_t n = a14_s16(o, self + 0x28);
        if (n < 400) {
            a14_w16(o, self + 0x28, (uint32_t)(n + 1));
            if (a14_s16(o, self + 0x28) == 400) {
                (void)a14_c_001F6B00(o);
                (void)a14_c_001B6660(o, A14_GROUP_8265A0);
            }
        }
        r = 0;
        (void)a14_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a14_w16(o, self + 0x2E, 0xFFFF);
            a14_w8(o, D_00810784, 0xFF);
            a14_w8(o, D_00810804, 0xFF);
            if (a14_s16(o, self + 0x28) != 400) {
                (void)a14_c_001F6B00(o);
                (void)a14_c_001B6660(o, A14_GROUP_8265A0);
            }
            a14_w8(o, self + 4, 2);
        }
    } else if (state == 2) {
        a14_w8(o, self + 4, 3);
    } else {
        (void)a14_c_001AFC10(o, self);
    }
}

int em_area14_overlay_00823760(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault)
{
    A14 o;
    if (a14_begin(&o, h, fault)) return -1;
    a14_823760(&o, a0);
    return a14_end(&o);
}

/* 0x8238C0 (decomp func_overlay_AREA14_00823880): script 0x826800's op09
 * callback: 001FAE70(0), returns 1. */
int em_area14_overlay_008238C0(const EmArea14OverlayHooks *h, int32_t *result, EmArea14OverlayFault *fault)
{
    A14 o;
    if (!result || a14_begin(&o, h, fault)) return -1;
    (void)a14_c_001FAE70(&o, 0);
    if (a14_end(&o)) return -1;
    *result = 1;
    return 0;
}

/* The tail of [0]'s and [2]'s state 1: 001B17A0, 001C68C0, the method. */
static void a14_tail(A14 *o, uint32_t self)
{
    (void)a14_c_001B17A0(o, self);
    (void)a14_c_001C68C0(o, self);
    (void)a14_method(o, self);
}

/* ----------------------------------------------------------------------
 * 0x8238E0 (decomp func_overlay_AREA14_008238A0) — [0]. Every call first
 * copies the 80-byte area 0x826F10 into its frame (sp - 0x50, five
 * quadwords). State 0: state 3 with flag 0x3D set; else 001B10B0(self,
 * +0xD, 0x5F), 001C63E0(self, 1), 001CA6F0(self, 2), +0x58 = D_0028A610,
 * state 1, +0 = 1. State 1, step +5 0: script 0x826C10 (+5 1) when
 * 001B1EA0(0, the player, the copy, 5) and the player's y is below 435;
 * step 1: at the script's end +0x2E = 0xFFFF, 001C4760(0x16, 1),
 * D_00810815 = 0xFF, 001FAE70(0), +5 2. The tail (001B17A0, 001C68C0, the
 * method) runs in every step, but in step 1 not while D_70003B92 is set.
 * States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a14_8238E0(A14 *o, uint32_t self, uint32_t sp)
{
    const uint32_t area = sp - 0x50u;
    uint64_t q[8];
    int32_t r;

    /* each quadword move is two doubleword accesses */
    for (unsigned k = 0; k < 8; ++k) q[k] = a14_u64(o, A14_AREA_826F10 + 8 * k);
    for (unsigned k = 0; k < 8; ++k) a14_w64(o, area + 8 * k, q[k]);
    q[0] = a14_u64(o, A14_AREA_826F10 + 0x40);
    q[1] = a14_u64(o, A14_AREA_826F10 + 0x48);
    a14_w64(o, area + 0x40, q[0]);
    a14_w64(o, area + 0x48, q[1]);

    uint32_t state = a14_u8(o, self + 4);
    if (state == 2 || state == 3) {
        (void)a14_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a14_c_001BA1C0(o, self, 0x3D, &r);
        if (r != 0) {
            a14_w8(o, self + 4, 3);
            return;
        }
        (void)a14_c_001B10B0(o, self, (int32_t)a14_u8(o, self + 0xD), 0x5F);
        (void)a14_c_001C63E0(o, self, 1);
        (void)a14_c_001CA6F0(o, self, 2);
        a14_w32(o, self + 0x58, a14_u32(o, D_0028A610));
        a14_w8(o, self + 4, 1);
        a14_w8(o, self, 1);
        return;
    }
    if (state != 1) return;
    uint32_t step = a14_u8(o, self + 5);
    if (step == 0) {
        r = 0;
        (void)a14_c_001B1EA0(o, 0, D_PLAYER_POS, area, 5, &r);
        if (r != 0 && A14_LT(a14_u32(o, D_PLAYER_Y), F_435)) {
            (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_826C10);
            a14_w8(o, self + 5, 1);
        }
        a14_tail(o, self);
    } else if (step == 1) {
        r = 0;
        (void)a14_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a14_w16(o, self + 0x2E, 0xFFFF);
            (void)a14_c_001C4760(o, 0x16, 1);
            a14_w8(o, D_00810815, 0xFF);
            (void)a14_c_001FAE70(o, 0);
            a14_w8(o, self + 5, 2);
        }
        if (a14_u8(o, D_70003B92) == 0) a14_tail(o, self);
    } else if (step == 2) {
        a14_tail(o, self);
    }
}

int em_area14_overlay_008238E0(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea14OverlayFault *fault)
{
    A14 o;
    if (a14_begin(&o, h, fault)) return -1;
    a14_8238E0(&o, a0, sp);
    return a14_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823B30 (decomp func_overlay_AREA14_00823AF0) — [2], the Major. State
 * 0: 001B10B0(self, +0xD, 0x56), 001C63E0(self, 0), 001CA6F0(self, 2),
 * 001BA8E0(self, +0xD), +0 = 1, +0x30 = 0x827480, +0x58 = D_0028A5EC,
 * state 1. State 1: state 3 with flag 0x3F set. While D_00810816 is 0: step
 * 0, with the player's y not below 675: script 0x826FC0 (step 1) when
 * 001B1EA0(0, the player, 0x828400, 4), then 001BA580(self, +0xD) and
 * 001C64F0(self, 1.0); step 1: at the script's end D_00810816 = 1, +0x2E =
 * 0xFFFF, +0x40 = D_0028A5E8, 001C67E0(self, 0, 0, 0), step 0,
 * 001C47A0(0xF, 1), 001C4760(0x17, 1), 001FAE70(0); then 001BA580 and
 * +0x1FE = 001C64F0(self, 0.5). Once D_00810816 is set: step 0 starts script
 * 0x827340 on a Use (+0xB bit 2), step 1 at its end 001C67E0(self, 0, 20,
 * 0), +0xB = 0, step 0; then 001BA580 and 001C64F0(self, 1.0). Then
 * 001B17A0, +1 = 1, 001C68C0, the method. States 2 / 3: 001BA540,
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a14_823B30(A14 *o, uint32_t self)
{
    uint32_t state = a14_u8(o, self + 4);
    int32_t r;

    if (state == 2 || state == 3) {
        (void)a14_c_001BA540(o, self);
        (void)a14_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a14_c_001B10B0(o, self, (int32_t)a14_u8(o, self + 0xD), 0x56);
        (void)a14_c_001C63E0(o, self, 0);
        (void)a14_c_001CA6F0(o, self, 2);
        (void)a14_c_001BA8E0(o, self, (int32_t)a14_u8(o, self + 0xD));
        a14_w8(o, self, 1);
        a14_w32(o, self + 0x30, A14_TABLE_827480);
        a14_w32(o, self + 0x58, a14_u32(o, D_0028A5EC));
        a14_w8(o, self + 4, 1);
        return;
    }
    if (state != 1) return;
    r = 0;
    (void)a14_c_001BA1C0(o, self, 0x3F, &r);
    if (r != 0) {
        a14_w8(o, self + 4, 3);
        return;
    }
    if (a14_u8(o, D_00810816) == 0) {
        uint32_t step = a14_u8(o, self + 5);
        if (step == 0) {
            if (!A14_LT(a14_u32(o, D_PLAYER_Y), F_675)) {
                r = 0;
                (void)a14_c_001B1EA0(o, 0, D_PLAYER_POS, A14_AREA_828400, 4, &r);
                if (r != 0) {
                    (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_826FC0);
                    a14_w8(o, self + 5, 1);
                }
                (void)a14_c_001BA580(o, self, (int32_t)a14_u8(o, self + 0xD));
                r = 0;
                (void)a14_c_001C64F0(o, self, F_ONE, &r);
            }
        } else if (step == 1) {
            r = 0;
            (void)a14_c_001BA1F0(o, self, &r);
            if (r != 0) {
                a14_w8(o, D_00810816, 1);
                a14_w16(o, self + 0x2E, 0xFFFF);
                a14_w32(o, self + 0x40, a14_u32(o, D_0028A5E8));
                (void)a14_c_001C67E0(o, self, 0, F_ZERO, F_ZERO);
                a14_w8(o, self + 5, 0);
                (void)a14_c_001C47A0(o, 0xF, 1);
                (void)a14_c_001C4760(o, 0x17, 1);
                (void)a14_c_001FAE70(o, 0);
            }
            (void)a14_c_001BA580(o, self, (int32_t)a14_u8(o, self + 0xD));
            r = 0;
            (void)a14_c_001C64F0(o, self, F_HALF, &r);
            a14_w16(o, self + 0x1FE, (uint32_t)r);
        }
    } else {
        uint32_t step = a14_u8(o, self + 5);
        if (step == 0) {
            if (a14_u8(o, self + 0xB) & 4) {
                (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_827340);
                a14_w8(o, self + 5, 1);
            }
        } else if (step == 1) {
            r = 0;
            (void)a14_c_001BA1F0(o, self, &r);
            if (r != 0) {
                (void)a14_c_001C67E0(o, self, 0, F_20, F_ZERO);
                a14_w8(o, self + 0xB, 0);
                a14_w8(o, self + 5, 0);
            }
        }
        (void)a14_c_001BA580(o, self, (int32_t)a14_u8(o, self + 0xD));
        r = 0;
        (void)a14_c_001C64F0(o, self, F_ONE, &r);
    }
    (void)a14_c_001B17A0(o, self);
    a14_w8(o, self + 1, 1);
    (void)a14_c_001C68C0(o, self);
    (void)a14_method(o, self);
}

int em_area14_overlay_00823B30(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault)
{
    A14 o;
    if (a14_begin(&o, h, fault)) return -1;
    a14_823B30(&o, a0);
    return a14_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823E30 (decomp func_overlay_AREA14_00823DF0, NEARMISS: the .s is the
 * original) — [4]. State 0: state 3 with flag 0x3F set or D_00810C8C 0,
 * else state 1, +0 = 1. State 1, step 0: when D_00810702 is 1, +0x28 = 360,
 * script 0x827490, step 1. Step 1: while +0x28 > 0, 001BA1F0(self) (result
 * not used), 00183160(2, 0), +0x28 - 1; at 0, 00183160(0, 0), script
 * 0x827590, step 2. Step 2: at the script's end state 3, 001C4760(0x18, 1),
 * D_00810817 = 0xFF. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a14_823E30(A14 *o, uint32_t self)
{
    uint32_t state = a14_u8(o, self + 4);
    int32_t r;

    if (state == 2 || state == 3) {
        (void)a14_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a14_c_001BA1C0(o, self, 0x3F, &r);
        if (r != 0 || a14_u8(o, D_00810C8C) == 0) {
            a14_w8(o, self + 4, 3);
            return;
        }
        a14_w8(o, self + 4, 1);
        a14_w8(o, self, 1);
        return;
    }
    if (state != 1) return;
    uint32_t step = a14_u8(o, self + 5);
    if (step == 0) {
        if (a14_u8(o, D_00810702) == 1) {
            a14_w16(o, self + 0x28, 360);
            (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_827490);
            a14_w8(o, self + 5, 1);
        }
    } else if (step == 1) {
        if (a14_s16(o, self + 0x28) > 0) {
            r = 0;
            (void)a14_c_001BA1F0(o, self, &r);
            (void)a14_c_00183160(o, 2, F_ZERO);
            a14_w16(o, self + 0x28, (uint32_t)(a14_s16(o, self + 0x28) - 1));
        } else {
            (void)a14_c_00183160(o, 0, F_ZERO);
            (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_827590);
            a14_w8(o, self + 5, 2);
        }
    } else if (step == 2) {
        r = 0;
        (void)a14_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a14_w8(o, self + 4, 3);
            (void)a14_c_001C4760(o, 0x18, 1);
            a14_w8(o, D_00810817, 0xFF);
        }
    }
}

int em_area14_overlay_00823E30(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault)
{
    A14 o;
    if (a14_begin(&o, h, fault)) return -1;
    a14_823E30(&o, a0);
    return a14_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825C40 (decomp func_overlay_AREA14_00825C00) — script 0x828120's op09
 * callback (self, the op09 record, the script's work block). Record +4 0:
 * self +0x2E = 1, work +0x10 = 80, +4 1. +4 1: work +0x10 less 1; at 0:
 * D_008105E0 = the player's root (00102948), its y + 6, D_008105D0 = record
 * +0x10 (00102948), work +0x10 = 0, +4 2, +5 0, self +0x38 = 0,
 * 001FB9F0(0x451, 0x1000, 0x1000, 0x1000). +4 2: step +5 0 calls
 * 001AEDE0(4, 0) and 001B0C60(0x11, 0, 0) once the root's y is above 810
 * (+5 1); step 1 returns 1 once it is above 900. Otherwise self +0x38 grows
 * by 0.01 to at most 2.0 and is added to self +0xB4; D_700038A0 = (0, +0x38,
 * 0, 1) and 00183010(the player, D_700038A0); while D_008105E0's y is below
 * 816 the copy and + 6 again; while work +0x10 is below 180 it counts up
 * and D_008105D0 = record +0x10 + d * t with d = 001028D0(work + 0x20 less
 * record + 0x10) and t = (1 + 0011E2A8(pi * (n / 180) - pi / 2)) / 2.
 * Returns 0 otherwise.
 * ---------------------------------------------------------------------- */
static int32_t a14_825C40(A14 *o, uint32_t self, uint32_t rec, uint32_t work)
{
    uint32_t step = a14_u8(o, rec + 4);
    if (step == 0) {
        a14_w16(o, self + 0x2E, 1);
        a14_w32(o, work + 0x10, F_80);
        a14_w8(o, rec + 4, a14_u8(o, rec + 4) + 1);
        return 0;
    }
    if (step == 1) {
        uint32_t n = A14_SUB(a14_u32(o, work + 0x10), F_ONE);
        a14_w32(o, work + 0x10, n);
        if (A14_EQ(n, F_ZERO)) {
            (void)a14_c_00102948(o, D_008105E0, D_PLAYER_ROOT);
            a14_w32(o, D_008105E0 + 4, A14_ADD(a14_u32(o, D_008105E0 + 4), F_6));
            (void)a14_c_00102948(o, rec + 0x10, D_008105D0);
            a14_w32(o, work + 0x10, F_ZERO);
            a14_w8(o, rec + 4, a14_u8(o, rec + 4) + 1);
            a14_w8(o, rec + 5, 0);
            a14_w32(o, self + 0x38, F_ZERO);
            (void)a14_c_001FB9F0(o, 0x451, 0x1000, 0x1000, 0x1000);
        }
        return 0;
    }
    if (step != 2) return 0;
    uint32_t sub = a14_u8(o, rec + 5);
    if (sub == 0) {
        if (!A14_LE(a14_u32(o, D_PLAYER_ROOT + 4), F_810)) {
            (void)a14_c_001AEDE0(o, 4, 0);
            (void)a14_c_001B0C60(o, 0x11, 0, 0);
            a14_w8(o, rec + 5, a14_u8(o, rec + 5) + 1);
        }
    } else if (sub == 1) {
        if (!A14_LE(a14_u32(o, D_PLAYER_ROOT + 4), F_900)) return 1;
    }
    uint32_t v = A14_ADD(a14_u32(o, self + 0x38), F_0_01);
    a14_w32(o, self + 0x38, v);
    if (!A14_LE(v, F_TWO)) a14_w32(o, self + 0x38, F_TWO);
    const uint32_t speed = a14_u32(o, self + 0x38);
    a14_w32(o, self + 0xB4, A14_ADD(a14_u32(o, self + 0xB4), speed));
    a14_w32(o, D_700038A0, F_ZERO);
    a14_w32(o, D_700038A0 + 4, a14_u32(o, self + 0x38));
    a14_w32(o, D_700038A0 + 8, F_ZERO);
    a14_w32(o, D_700038A0 + 12, F_ONE);
    (void)a14_c_00183010(o, D_PLAYER, D_700038A0);
    if (A14_LT(a14_u32(o, D_008105E0 + 4), F_816)) {
        (void)a14_c_00102948(o, D_008105E0, D_PLAYER_ROOT);
        a14_w32(o, D_008105E0 + 4, A14_ADD(a14_u32(o, D_008105E0 + 4), F_6));
    }
    const uint32_t n0 = a14_u32(o, work + 0x10);
    if (A14_LT(n0, F_180)) {
        a14_w32(o, work + 0x10, A14_ADD(n0, F_ONE));
        const uint32_t n = a14_u32(o, work + 0x10);
        uint32_t s = 0;
        (void)a14_c_0011E2A8(o, A14_SUB(A14_MUL(F_PI, A14_DIV(n, F_180)), F_HALF_PI), &s);
        const uint32_t t = A14_DIV(A14_ADD(F_ONE, s), F_TWO);
        (void)a14_c_001028D0(o, D_008105D0, work + 0x20, rec + 0x10);
        for (unsigned k = 0; k < 3; ++k) {
            const uint32_t d = a14_u32(o, D_008105D0 + 4 * k);
            a14_w32(o, D_008105D0 + 4 * k, A14_ADD(a14_u32(o, rec + 0x10 + 4 * k), A14_MUL(d, t)));
        }
    }
    return 0;
}

int em_area14_overlay_00825C40(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea14OverlayFault *fault)
{
    A14 o;
    if (!result || a14_begin(&o, h, fault)) return -1;
    const int32_t v = a14_825C40(&o, a0, a1, a2);
    if (a14_end(&o)) return -1;
    *result = v;
    return 0;
}

/* ----------------------------------------------------------------------
 * 0x825FD0 (decomp func_overlay_AREA14_00825F90) — [6]. State 0: nothing
 * while 001B0FD0(self) is set; else 001C6380, +8 = 1, +0x30 = 0x8283A0,
 * +0x2E = 0. State 1, step 0: D_700038A0 = (221.6, 695.1, 1181, 1); with
 * D_00810C8C set +0 = 1 and 00158590(self, 1, -2), else +0 = 2 and
 * 00158590(self, 0, -2); a Use (+0xB bit 2) starts script 0x828120 (step
 * + 1). Step 1: step + 1 at the script's end. Then 001B17A0, 001C6380, the
 * method. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a14_825FD0(A14 *o, uint32_t self)
{
    uint32_t state = a14_u8(o, self + 4);
    int32_t r;

    if (state == 2 || state == 3) {
        (void)a14_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a14_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a14_c_001C6380(o, self);
        a14_w8(o, self + 8, 1);
        a14_w32(o, self + 0x30, A14_TABLE_8283A0);
        a14_w16(o, self + 0x2E, 0);
        return;
    }
    if (state != 1) return;
    uint32_t step = a14_u8(o, self + 5);
    if (step == 0) {
        a14_w32(o, D_700038A0, F_221_6);
        a14_w32(o, D_700038A0 + 4, F_695_1);
        a14_w32(o, D_700038A0 + 8, F_1181);
        a14_w32(o, D_700038A0 + 12, F_ONE);
        if (a14_u8(o, D_00810C8C) != 0) {
            a14_w8(o, self, 1);
            (void)a14_c_00158590(o, self, 1, -2);
        } else {
            a14_w8(o, self, 2);
            (void)a14_c_00158590(o, self, 0, -2);
        }
        if (a14_u8(o, self + 0xB) & 4) {
            (void)a14_c_001BA1A0(o, self + 0x1F0, A14_SCRIPT_828120);
            a14_w8(o, self + 5, a14_u8(o, self + 5) + 1);
        }
    } else if (step == 1) {
        r = 0;
        (void)a14_c_001BA1F0(o, self, &r);
        if (r != 0) a14_w8(o, self + 5, a14_u8(o, self + 5) + 1);
    }
    (void)a14_c_001B17A0(o, self);
    (void)a14_c_001C6380(o, self);
    (void)a14_method(o, self);
}

int em_area14_overlay_00825FD0(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault)
{
    A14 o;
    if (a14_begin(&o, h, fault)) return -1;
    a14_825FD0(&o, a0);
    return a14_end(&o);
}

/* ----------------------------------------------------------------------
 * 00191350(record): 1 when 001B1EA0(0, record + 0xA0, the area D_0024A4F0,
 * 4) is non-zero, else 0.
 * ---------------------------------------------------------------------- */
#define D_0024A4F0 0x0024A4F0u

static int32_t a14_00191350(A14 *o, uint32_t record)
{
    int32_t r = 0;
    (void)a14_c_001B1EA0(o, 0, record + 0xA0, D_0024A4F0, 4, &r);
    return r != 0 ? 1 : 0;
}

int em_area14_overlay_00191350(const EmArea14OverlayHooks *h, uint32_t a0, int32_t *result,
                               EmArea14OverlayFault *fault)
{
    A14 o;
    if (!result || a14_begin(&o, h, fault)) return -1;
    const int32_t v = a14_00191350(&o, a0);
    if (a14_end(&o)) return -1;
    *result = v;
    return 0;
}

/* ----------------------------------------------------------------------
 * 00230230(camera, player): AREA14's camera arm. Nothing (0) unless
 * 00191350(player). The pitch: v = 0011DF78(555.4 - player +0xA4) at most
 * 123.4, w = 123.4 - v at most 90, camera +0x94 = -w. Then by the player's
 * +0x230: 20: camera +0x44 = 001B12B0(001B1470(pi + player +0x218), +0x44,
 * 1 degree), the eye +0x10 / +0x18 at -35 from D_008105E0 / D_008105E8 by
 * sin / cos of it, then the follow height 00191D40(camera, 60 + (+0x8C + (11
 * + (+0x5C + player +0xA4))), 0.8); 21: that height alone; 10 / 25: with
 * player +0x38 non-zero, +0x44 chases 001B1470(player +0xC4 +- 35 degrees)
 * at 0.3 degrees (the sign of +0x38), else +0x44 = 00191120(player +0xC4,
 * +0x44, 2 degrees, 45 degrees); the eye at +0xC + +0x94 from D_008105E0 /
 * D_008105E8 by sin / cos of +0x44, 00192010(camera, +0x8C + (+0x5C +
 * player +0xB4), 35, 30), and the eye's x at most 447 (then its z at least
 * 1340). Each handled state ends with 0018D7B0(camera, 4), 0018C6A0(camera
 * + 0x10, D_008105D0, 0.8) and 0018C4B0(D_008105D0, camera +0x14, 0.8) and
 * returns 1; any other state returns 0. (The original's 10 / 25 branch
 * also holds arms for state 21, which cannot be reached there.)
 * ---------------------------------------------------------------------- */
#define D_008105E8 0x008105E8u
#define F_555_4 0x440AD99Au       /* 555.4 */
#define F_123_4 0x42F6CCCCu       /* 123.399994 */
#define F_90 0x42B40000u
#define F_M35 0xC20C0000u
#define F_35 0x420C0000u
#define F_30 0x41F00000u
#define F_60 0x42700000u
#define F_11 0x41300000u
#define F_447 0x43DF8000u
#define F_1340 0x44A78000u
#define F_DEG1 0x3C8EFA35u        /* 0.017453292 */
#define F_DEG035 0x3BAB92A7u      /* 0.005235988 */
#define F_DEG2 0x3D0EFA35u        /* 0.034906585 */
#define F_DEG35 0x3F1C61ABu       /* 0.6108653 */
#define F_DEG45 0x3F490FDBu       /* 0.7853982 */

static uint32_t a14_f1(A14 *o, int (*call)(A14 *, uint32_t, uint32_t *), uint32_t x)
{
    uint32_t out = 0;
    (void)call(o, x, &out);
    return out;
}

/* the end of every handled state */
static void a14_cam_tail(A14 *o, uint32_t cam)
{
    (void)a14_c_0018D7B0(o, cam, 4);
    (void)a14_c_0018C6A0(o, cam + 0x10, D_008105D0, F_0_8);
    (void)a14_c_0018C4B0(o, D_008105D0, a14_u32(o, cam + 0x14), F_0_8);
}

/* the follow height of states 20 and 21 */
static void a14_cam_height(A14 *o, uint32_t cam, uint32_t player)
{
    const uint32_t height = a14_u32(o, player + 0xA4);
    const uint32_t y = A14_ADD(a14_u32(o, cam + 0x5C), height);
    const uint32_t want = A14_ADD(F_60, A14_ADD(a14_u32(o, cam + 0x8C), A14_ADD(F_11, y)));
    (void)a14_c_00191D40(o, cam, want, F_0_8);
}

/* the eye at the camera's radius by sin / cos of +0x44 (states 10 / 25);
 * the sine's argument is `angle` when given (the 00191120 arm keeps the
 * angle it stored), else +0x44 read back */
static void a14_cam_eye(A14 *o, uint32_t cam, const uint32_t *angle)
{
    uint32_t s = a14_f1(o, a14_c_0011E2A8, angle ? *angle : a14_u32(o, cam + 0x44));
    uint32_t radius = a14_u32(o, cam + 0xC);
    radius = A14_ADD(radius, a14_u32(o, cam + 0x94));
    a14_w32(o, cam + 0x10, A14_ADD(a14_u32(o, D_008105E0), A14_MUL(radius, s)));
    s = a14_f1(o, a14_c_0011DE90, a14_u32(o, cam + 0x44));
    radius = a14_u32(o, cam + 0xC);
    radius = A14_ADD(radius, a14_u32(o, cam + 0x94));
    a14_w32(o, cam + 0x18, A14_ADD(a14_u32(o, D_008105E8), A14_MUL(radius, s)));
}

static int32_t a14_00230230(A14 *o, uint32_t cam, uint32_t player)
{
    if (a14_00191350(o, player) == 0) return 0;
    uint32_t v = a14_f1(o, a14_c_0011DF78, A14_SUB(F_555_4, a14_u32(o, player + 0xA4)));
    if (A14_LT(F_123_4, v)) v = F_123_4;
    uint32_t w = A14_SUB(F_123_4, v);
    if (A14_LT(F_90, w)) w = F_90;
    a14_w32(o, cam + 0x94, A14_NEG(w));
    const int32_t st = a14_s32(o, player + 0x230);
    if (st == 20) {
        const uint32_t goal = a14_f1(o, a14_c_001B1470, A14_ADD(F_PI, a14_u32(o, player + 0x218)));
        uint32_t r = 0;
        (void)a14_c_001B12B0(o, goal, a14_u32(o, cam + 0x44), F_DEG1, &r);
        a14_w32(o, cam + 0x44, r);
        const uint32_t s = a14_f1(o, a14_c_0011E2A8, r);
        a14_w32(o, cam + 0x10, A14_ADD(a14_u32(o, D_008105E0), A14_MUL(F_M35, s)));
        const uint32_t c = a14_f1(o, a14_c_0011DE90, a14_u32(o, cam + 0x44));
        a14_w32(o, cam + 0x18, A14_ADD(a14_u32(o, D_008105E8), A14_MUL(F_M35, c)));
        a14_cam_height(o, cam, player);
        a14_cam_tail(o, cam);
        return 1;
    }
    if (st == 21) {
        a14_cam_height(o, cam, player);
        a14_cam_tail(o, cam);
        return 1;
    }
    if (st != 10 && st != 25) return 0;
    const uint32_t f = a14_u32(o, player + 0x38);
    if (!A14_EQ(f, F_ZERO)) {
        const uint32_t yaw = a14_u32(o, player + 0xC4);
        const uint32_t goal = a14_f1(o, a14_c_001B1470, A14_LT(f, F_ZERO) ? A14_SUB(yaw, F_DEG35)
                                                                            : A14_ADD(F_DEG35, yaw));
        uint32_t r = 0;
        (void)a14_c_001B12B0(o, goal, a14_u32(o, cam + 0x44), F_DEG035, &r);
        a14_w32(o, cam + 0x44, r);
        a14_cam_eye(o, cam, NULL);
    } else {
        uint32_t r = 0;
        const uint32_t current = a14_u32(o, cam + 0x44);
        (void)a14_c_00191120(o, a14_u32(o, player + 0xC4), current, F_DEG2, F_DEG45, &r);
        a14_w32(o, cam + 0x44, r);
        a14_cam_eye(o, cam, &r);
    }
    const uint32_t b4 = a14_u32(o, player + 0xB4);
    const uint32_t y5c = A14_ADD(a14_u32(o, cam + 0x5C), b4);
    const uint32_t y = A14_ADD(a14_u32(o, cam + 0x8C), y5c);
    (void)a14_c_00192010(o, cam, y, F_35, F_30);
    if (!A14_LT(a14_u32(o, cam + 0x10), F_447)) {
        a14_w32(o, cam + 0x10, F_447);
        if (A14_LT(a14_u32(o, cam + 0x18), F_1340)) a14_w32(o, cam + 0x18, F_1340);
    }
    a14_cam_tail(o, cam);
    return 1;
}

int em_area14_overlay_00230230(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t a1, int32_t *result,
                               EmArea14OverlayFault *fault)
{
    A14 o;
    if (!result || a14_begin(&o, h, fault)) return -1;
    const int32_t v = a14_00230230(&o, a0, a1);
    if (a14_end(&o)) return -1;
    *result = v;
    return 0;
}
