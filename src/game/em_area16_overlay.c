/* AREA16's own overlay owners (em_area16_overlay.h), group a: 0x823580,
 * 0x8261A0, 0x826C50, 0x827030. Each function is one original function; the
 * comment above it says what the code does. The order of the memory
 * accesses between calls is the original's (the test compares it one for
 * one). Ground truth: the decomp's byte-identical C under
 * src/overlays/AREA16/; evidence: tools/test_area16_overlay_reference.py. */
#include "em_area16_overlay_internal.h"

/* globals */
#define D_0024D7C0 0x0024D7C0u   /* per area, per sub: the 0x28-byte spawn records */
#define D_0028A59C 0x0028A59Cu
#define D_00810700 0x00810700u   /* the area */
#define D_00810701 0x00810701u   /* the sub */
#define D_0081080B 0x0081080Bu   /* AREA16's event bits */
#define D_008102C8 0x008102C8u   /* a pointer to a byte (1 = the swap) */

/* The halfword +6 of record self[0x9A] + 1 of the current area's and sub's
 * 0x28-byte table (D_0024D7C0[area][sub]). */
static uint32_t a16_spawn_id(A16 *o, uint32_t self)
{
    const uint32_t area = a16_u8(o, D_00810700);
    const uint32_t rec = a16_u8(o, self + 0x9A);
    const uint32_t sub = a16_u8(o, D_00810701);
    const uint32_t per_area = a16_u32(o, D_0024D7C0 + 4u * area);
    const uint32_t table = a16_u32(o, per_area + 4u * sub);
    return a16_u16(o, table + 0x28u * (rec + 1u) + 6u);
}

/* ----------------------------------------------------------------------
 * 0x8261A0 (decomp func_overlay_AREA16_00826160) — sub 0 [11]. State 0:
 * with bit 1 of D_0081080B: +0xD = 0x10, +0xE = the spawn id, and when
 * 001B0FD0 is 0: 001C6380, state 2; without it, when 001B0FD0 is 0:
 * 001C6380, +0 = 1, state 1. State 1: when +0x36 is set and the +0x18
 * parent's state is not 0xC9: with *D_008102C8 == 1 the swap
 * (001CA6E0(self, 001C6120(D_0028A59C, 0x10)), 001EFD20(2, +0xB0), +0xE =
 * the spawn id, 001C62C0, 001C6380, D_0081080B |= 2, state 2), else +0x36 =
 * 0; then 001B17A0 and the +0x4C method. State 2: 001B17A0 and the method.
 * State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_8261A0(A16 *o, uint32_t self)
{
    int32_t r = 0;
    switch (a16_u8(o, self + 4)) {
    case 0:
        if (a16_u8(o, D_0081080B) & 2u) {
            a16_w8(o, self + 0xD, 0x10);
            a16_w16(o, self + 0xE, a16_spawn_id(o, self));
            r = 0;
            if (a16_c_001B0FD0(o, self, &r) < 0) return;
            if (r == 0) {
                (void)a16_c_001C6380(o, self);
                a16_w8(o, self + 4, 2);
            }
        } else {
            r = 0;
            if (a16_c_001B0FD0(o, self, &r) < 0) return;
            if (r == 0) {
                (void)a16_c_001C6380(o, self);
                a16_w8(o, self, 1);
                a16_w8(o, self + 4, 1);
            }
        }
        break;
    case 1:
        if (a16_s16(o, self + 0x36) != 0 && a16_u8(o, a16_u32(o, self + 0x18) + 4) != 0xC9u) {
            if (a16_u8(o, a16_u32(o, D_008102C8)) == 1u) {
                r = 0;
                if (a16_c_001C6120(o, a16_s32(o, D_0028A59C), 0x10, &r) < 0) return;
                (void)a16_c_001CA6E0(o, self, r);
                uint32_t ignored = 0;
                (void)a16_c_001EFD20(o, 2, self + 0xB0, &ignored);
                a16_w16(o, self + 0xE, a16_spawn_id(o, self));
                (void)a16_c_001C62C0(o, self);
                (void)a16_c_001C6380(o, self);
                a16_w8(o, D_0081080B, a16_u8(o, D_0081080B) | 2u);
                a16_w8(o, self + 4, 2);
            } else {
                a16_w16(o, self + 0x36, 0);
            }
        }
        r = 0;
        (void)a16_c_001B17A0(o, self, &r);
        (void)a16_method(o, self);
        break;
    case 2:
        r = 0;
        (void)a16_c_001B17A0(o, self, &r);
        (void)a16_method(o, self);
        break;
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_008261A0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_8261A0(&o, a0);
    return a16_end(&o);
}

/* scratchpad blocks */
#define D_700031F0 0x700031F0u   /* set to 1 while 0x827030 moves the player */
#define D_700038A0 0x700038A0u   /* a scratch vector */
#define D_700038B0 0x700038B0u   /* a scratch colour */
#define D_00275B40 0x00275B40u   /* a pointer to the area's object table */
#define D_00810350 0x00810350u   /* the player's position (x, y, z, w) */

#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_MINUS_ONE 0xBF800000u
#define F_0_2 0x3E4CCCCDu
#define F_SIXTH 0x3E2AAAABu
#define F_MINUS_SIXTH 0xBE2AAAABu
#define F_1_35 0x3FACCCCDu
#define F_2_5 0x40200000u
#define F_MINUS_2_5 0xC0200000u
#define F_4 0x40800000u
#define F_5 0x40A00000u
#define F_7 0x40E00000u
#define F_8 0x41000000u
#define F_12 0x41400000u
#define F_15 0x41700000u
#define F_30 0x41F00000u
#define F_300 0x43960000u

/* ----------------------------------------------------------------------
 * 0x823580 (decomp overlay_AREA16_func_00823540) — group 0x829750 [4..8].
 * State 1: +0x94 picks the size and the count (0xA: 4.0 / 6, 0xB: 5.0 / 6,
 * 0xC: 7.0 / 8, 0xD: 8.0 / 8, 0xE: 12.0 / 10; the rate is 0.2); two
 * 001EFEB0(0x8000003B, +0xD0) spawns each get +5 = 0, +0x1F0 = the count,
 * +0x1F4 = the size, +0x1F8 = the rate; the frame vector (0, 0, size, 1)
 * (sp - 0x50) goes through the +0xD0 matrix (001026A0); the frame matrix
 * (sp - 0x40) is 001029C0's identity, its rows 00103230(row, the +0xD0 /
 * +0xE0 / +0xF0 row, -1.0), its last row the vector (00102948); then
 * 001EFEB0(0, the frame matrix) and 001EFEB0(0, +0xD0). Another +0x94 has
 * the original use its caller's registers: fail-stop. Other states:
 * nothing.
 * ---------------------------------------------------------------------- */
static void a16_823580(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t v = sp - 0x50u, m = sp - 0x40u;
    uint32_t size, count, spawned = 0;
    if (a16_u8(o, self + 4) != 1) return;
    switch (a16_s16(o, self + 0x94)) {
    case 0xA: size = F_4; count = 6; break;
    case 0xB: size = F_5; count = 6; break;
    case 0xC: size = F_7; count = 8; break;
    case 0xD: size = F_8; count = 8; break;
    case 0xE: size = F_12; count = 10; break;
    default:
        if (!a16_failed(o)) (void)a16_latch(o, 0x00823580u, EM_AREA16_OVERLAY_FAULT_CALLER_REGISTER);
        return;
    }
    for (int i = 0; i < 2; i++) {
        spawned = 0;
        if (a16_c_001EFEB0(o, (int32_t)0x8000003B, self + 0xD0, &spawned) < 0) return;
        if (spawned != 0) {
            a16_w8(o, spawned + 5, 0);
            a16_w32(o, spawned + 0x1F0, count);
            a16_w32(o, spawned + 0x1F4, size);
            a16_w32(o, spawned + 0x1F8, F_0_2);
        }
    }
    a16_w32(o, v, F_ZERO);
    a16_w32(o, v + 4, F_ZERO);
    a16_w32(o, v + 8, size);
    a16_w32(o, v + 12, F_ONE);
    (void)a16_c_001026A0(o, v, self + 0xD0, v);
    (void)a16_c_001029C0(o, m);
    (void)a16_c_00103230(o, m, self + 0xD0, F_MINUS_ONE);
    (void)a16_c_00103230(o, m + 0x10, self + 0xE0, F_MINUS_ONE);
    (void)a16_c_00103230(o, m + 0x20, self + 0xF0, F_MINUS_ONE);
    (void)a16_c_00102948(o, m + 0x30, v);
    uint32_t ignored = 0;
    (void)a16_c_001EFEB0(o, 0, m, &ignored);
    (void)a16_c_001EFEB0(o, 0, self + 0xD0, &ignored);
}

int em_area16_overlay_00823580(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_823580(&o, a0, sp);
    return a16_end(&o);
}

/* The +0x18 parent link followed n times from node. */
static uint32_t a16_up(A16 *o, uint32_t node, int n)
{
    while (n-- > 0) node = a16_u32(o, node + 0x18);
    return node;
}

/* 0x826C50's marker: D_700038A0 = (+0x2EC, +0x2E8, +0x2E4, 1.0), D_700038B0
 * = (0, 0x80, 0, 0x80), 001F4BF0(D_700038A0, D_700038B0). */
static void a16_826C50_marker(A16 *o, uint32_t self)
{
    a16_w32(o, D_700038A0, a16_u32(o, self + 0x2EC));
    a16_w32(o, D_700038A0 + 4, a16_u32(o, self + 0x2E8));
    a16_w32(o, D_700038A0 + 8, a16_u32(o, self + 0x2E4));
    a16_w32(o, D_700038A0 + 12, F_ONE);
    a16_w32(o, D_700038B0, 0);
    a16_w32(o, D_700038B0 + 4, 0x80);
    a16_w32(o, D_700038B0 + 8, 0);
    a16_w32(o, D_700038B0 + 12, 0x80);
    (void)a16_c_001F4BF0(o, D_700038A0, D_700038B0);
}

/* ----------------------------------------------------------------------
 * 0x826C50 (decomp func_overlay_AREA16_00826C10) — sub 1 [44], [45], [46].
 * State 0, when 001B0FD0 is 0: state 1, +0 = +8 = 1, +0x30 by +3 (0:
 * 0x82C240, 1: 0x82C260, 2: 0x82C280, else kept), +0x28 = 0, 001C6380,
 * D_700038A0 = (0, 15, 1.35, 1) through the +0xD0 matrix (001026A0), whose
 * x, y, z go to +0x2EC, +0x2E8, +0x2E4. State 1: 001B17A0; with bit 2 of
 * +0xB: script by +3 (0x82BC40 / 0x82BE40 / 0x82C040, else none) on +0x1F0,
 * state 4, 001BA1F0, +0x28 = 120; the +0x4C method and the marker. State
 * 4: +0x28 counts down; at 0 two links up the +0x18 chain by +3 (0: 1 and
 * 2 up, 1: 2 and 3, 2: 3 and 4) get +0x2A = 1, +0x28 = 0 and
 * 001FBD50(self, 0x19A, 0, 300.0); when 001BA1F0 is nonzero: state 1, +0xB
 * = 0; the method and the marker. State 3 and any other: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_826C50(A16 *o, uint32_t self)
{
    int32_t r = 0;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0 || r != 0) return;
        a16_w8(o, self + 4, 1);
        a16_w8(o, self, 1);
        a16_w8(o, self + 8, 1);
        switch (a16_u8(o, self + 3)) {
        case 0: a16_w32(o, self + 0x30, 0x0082C240u); break;
        case 1: a16_w32(o, self + 0x30, 0x0082C260u); break;
        case 2: a16_w32(o, self + 0x30, 0x0082C280u); break;
        }
        a16_w16(o, self + 0x28, 0);
        (void)a16_c_001C6380(o, self);
        a16_w32(o, D_700038A0, F_ZERO);
        a16_w32(o, D_700038A0 + 4, F_15);
        a16_w32(o, D_700038A0 + 8, F_1_35);
        a16_w32(o, D_700038A0 + 12, F_ONE);
        (void)a16_c_001026A0(o, D_700038A0, self + 0xD0, D_700038A0);
        a16_w32(o, self + 0x2EC, a16_u32(o, D_700038A0));
        a16_w32(o, self + 0x2E8, a16_u32(o, D_700038A0 + 4));
        a16_w32(o, self + 0x2E4, a16_u32(o, D_700038A0 + 8));
        break;
    case 1:
        r = 0;
        (void)a16_c_001B17A0(o, self, &r);
        if (a16_u8(o, self + 0xB) & 4u) {
            switch (a16_u8(o, self + 3)) {
            case 0: (void)a16_c_001BA1A0(o, self + 0x1F0, 0x0082BC40u); break;
            case 1: (void)a16_c_001BA1A0(o, self + 0x1F0, 0x0082BE40u); break;
            case 2: (void)a16_c_001BA1A0(o, self + 0x1F0, 0x0082C040u); break;
            }
            a16_w8(o, self + 4, 4);
            r = 0;
            (void)a16_c_001BA1F0(o, self, &r);
            a16_w16(o, self + 0x28, 120);
        }
        (void)a16_method(o, self);
        a16_826C50_marker(o, self);
        break;
    case 4: {
        const int32_t count = a16_s16(o, self + 0x28);
        if (count != 0) {
            a16_w16(o, self + 0x28, (uint32_t)(count - 1));
            if (a16_s16(o, self + 0x28) <= 0) {
                const uint32_t k = a16_u8(o, self + 3);
                if (k <= 2) {
                    a16_w16(o, a16_up(o, self, (int)k + 1) + 0x2A, 1);
                    a16_w16(o, a16_up(o, self, (int)k + 2) + 0x2A, 1);
                }
                a16_w16(o, self + 0x28, 0);
                (void)a16_c_001FBD50(o, self, 0x19A, 0, F_300);
            }
        }
        r = 0;
        (void)a16_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a16_w8(o, self + 4, 1);
            a16_w8(o, self + 0xB, 0);
        }
        (void)a16_method(o, self);
        a16_826C50_marker(o, self);
        break;
    }
    default:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00826C50(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_826C50(&o, a0);
    return a16_end(&o);
}

/* The object D_00275B40[i] (the table pointer re-read each time). */
static uint32_t a16_object(A16 *o, int i)
{
    return a16_u32(o, a16_u32(o, D_00275B40) + 4u * (uint32_t)i);
}

/* lo < v < hi (EE compares) */
static int a16_between(uint32_t v, uint32_t lo, uint32_t hi) { return A16_LT(lo, v) && A16_LT(v, hi); }

/* ----------------------------------------------------------------------
 * 0x827030 (decomp func_overlay_AREA16_00826FF0) — sub 1 [42], [43].
 * State 0, when 001B0FD0 is 0: state 1, +0 = 1, +0x28 = +0x2A = 0, the
 * direction +0x1F0 = 1, the step +0x1F4 = 1/6. State 1: a nonzero +0x2A
 * flips the direction (positive: -1 and -1/6, else 1 and 1/6) and clears
 * itself; +0x28 += the direction's low halfword, wrapping above 30 to 0 and
 * below 0 to 30; t = +0x28 / 30; objects D_00275B40[1..26] get +0x84 = 5.0,
 * [4..22] +0x80 = -2.5, then [1] +0x84 = 5 + 5t, [3] +0x80 = -(2.5t), [0x17]
 * +0x80 = -2.5 + 2.5t; 001C6380, 001B17A0, the +0x4C method; when the
 * player is inside the box for +3 (nonzero: 149.9 < y < 200.4, 180 < x <
 * 201, 124 < z < 239.7; zero: 99.9 < y < 152, 205 < x < 226, 122 < z <
 * 236): D_700031F0 = 1 and the player's z moves by +0x1F4 (nonzero +3: +,
 * zero: -). State 3 and any other: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_827030(A16 *o, uint32_t self)
{
    int32_t r = 0;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0 || r != 0) return;
        a16_w8(o, self + 4, 1);
        a16_w8(o, self, 1);
        a16_w16(o, self + 0x28, 0);
        a16_w16(o, self + 0x2A, 0);
        a16_w32(o, self + 0x1F0, 1);
        a16_w32(o, self + 0x1F4, F_SIXTH);
        break;
    case 1: {
        if (a16_s16(o, self + 0x2A) != 0) {
            if (a16_s32(o, self + 0x1F0) > 0) {
                a16_w32(o, self + 0x1F0, (uint32_t)-1);
                a16_w32(o, self + 0x1F4, F_MINUS_SIXTH);
            } else {
                a16_w32(o, self + 0x1F0, 1);
                a16_w32(o, self + 0x1F4, F_SIXTH);
            }
            a16_w16(o, self + 0x2A, 0);
        }
        const int32_t dir = a16_s16(o, self + 0x1F0);
        a16_w16(o, self + 0x28, (uint32_t)(a16_s16(o, self + 0x28) + dir));
        if (a16_s16(o, self + 0x28) > 30) a16_w16(o, self + 0x28, 0);
        if (a16_s16(o, self + 0x28) < 0) a16_w16(o, self + 0x28, 30);
        const uint32_t t = A16_DIV(A16_CVT_S_W(a16_s16(o, self + 0x28)), F_30);
        const uint32_t a = A16_NEG(A16_MUL(F_2_5, t));
        const uint32_t b = A16_MUL(F_5, t);
        for (int i = 1; i < 27; i++) a16_w32(o, a16_object(o, i) + 0x84, F_5);
        for (int i = 4; i < 23; i++) a16_w32(o, a16_object(o, i) + 0x80, F_MINUS_2_5);
        a16_w32(o, a16_object(o, 1) + 0x84, A16_ADD(F_5, b));
        a16_w32(o, a16_object(o, 3) + 0x80, a);
        a16_w32(o, a16_object(o, 0x17) + 0x80, A16_SUB(F_MINUS_2_5, a));
        (void)a16_c_001C6380(o, self);
        r = 0;
        (void)a16_c_001B17A0(o, self, &r);
        (void)a16_method(o, self);
        uint32_t z;
        if (a16_u8(o, self + 3) != 0) {
            if (a16_between(a16_u32(o, D_00810350 + 4), 0x4315E666u, 0x43486666u) &&
                a16_between(a16_u32(o, D_00810350), 0x43340000u, 0x43490000u) &&
                a16_between(z = a16_u32(o, D_00810350 + 8), 0x42F80000u, 0x436FB333u)) {
                const uint32_t step = a16_u32(o, self + 0x1F4);
                a16_w32(o, D_700031F0, 1);
                a16_w32(o, D_00810350 + 8, A16_ADD(z, step));
            }
        } else {
            if (a16_between(a16_u32(o, D_00810350 + 4), 0x42C7CCCDu, 0x43180000u) &&
                a16_between(a16_u32(o, D_00810350), 0x434D0000u, 0x43620000u) &&
                a16_between(z = a16_u32(o, D_00810350 + 8), 0x42F40000u, 0x436C0000u)) {
                const uint32_t step = a16_u32(o, self + 0x1F4);
                a16_w32(o, D_700031F0, 1);
                a16_w32(o, D_00810350 + 8, A16_SUB(z, step));
            }
        }
        break;
    }
    default:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00827030(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_827030(&o, a0);
    return a16_end(&o);
}
