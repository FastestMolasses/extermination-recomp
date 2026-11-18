/* AREA21's doors and small sub-0 owners (em_area21_doors.h). Each function
 * is one original function; the comment above it says what the code does.
 * The order of the memory accesses between calls is the original's (the
 * test compares it one for one). */
#include "em_area21_doors_internal.h"

/* overlay data (runtime addresses) */
#define A21D_SCRIPT_82CB00 0x0082CB00u  /* the doors' step-1 script (001BA1A0) */
#define A21D_TABLE_82B170 0x0082B170u   /* the rounds' +0x30 */
#define A21D_CODE_825C60 0x00825C60u    /* the rounds' +0x34 (an empty function) */

/* globals */
#define D_0024D7C0 0x0024D7C0u          /* per area, per room: the placement records */
#define D_00275B40 0x00275B40u          /* the running actor's bone array */
#define D_0028A59C 0x0028A59Cu
#define D_00810701 0x00810701u          /* the room index of the placement records */
#define D_00810374 0x00810374u
#define D_00810700 0x00810700u          /* the area index of the lock bytes */
#define D_0081080E 0x0081080Eu          /* counter 0x36 */
#define D_00810841 0x00810841u          /* the lock bytes, one per area */
#define D_700031B0 0x700031B0u          /* the hit record: point */
#define D_700031D0 0x700031D0u          /* the hit record: surface pointer */
#define D_700031D4 0x700031D4u          /* the hit record: object hit */
#define D_700038A0 0x700038A0u
#define D_700038B0 0x700038B0u

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_0_3 0x3E99999Au
#define F_5 0x40A00000u
#define F_7 0x40E00000u
#define F_10 0x41200000u
#define F_32 0x42000000u
#define F_96 0x42C00000u
#define F_180 0x43340000u
#define F_PI 0x40490FDBu
#define F_0_01 0x3C23D70Au
#define F_0_005 0x3BA3D70Au
#define F_M0_005 0xBBA3D70Au
#define F_2_M31 0x30000000u        /* 4.656613e-10, 2^-31 */
#define F_PI_3 0x3F860A92u         /* 1.0471976 */
#define F_M_PI_6 0xBF060A92u       /* -0.5235988 */
#define F_PI_30 0x3DD67750u        /* 0.10471976 */
#define F_PI_60 0x3D567750u        /* 0.05235988 */
#define F_0_523 0x3F05E354u
#define F_0_05 0x3D4CCCCDu
#define F_0_06 0x3D75C28Fu
#define F_0_075 0x3D99999Au
#define F_0_2 0x3E4CCCCDu
#define F_0_4 0x3ECCCCCDu
#define F_12 0x41400000u
#define F_25 0x41C80000u
#define F_M25 0xC1C80000u
#define F_150 0x43160000u
#define F_600 0x44160000u
#define F_800 0x44480000u
#define F_4096 0x45800000u

/* The bit of a lock byte for a shift count (the EE shifts by its low five
 * bits). */
static inline uint32_t a21d_bit(uint32_t n) { return 1u << (n & 31u); }

/* ----------------------------------------------------------------------
 * The door steps shared by 0x826BB0 and 0x826E60 (state 1, after the
 * dispatch on +5; a jump table in the original). Step 0: +0 = 1 and
 * 001BBE40(self, +0x1F0, 0) (step 3 when it returns non-zero) while the
 * lock bit (+0x34, a half) of D_00810841[D_00810700] is set, else +0 = 2
 * and 001BBE40(.., 1) (step 1). Step 1: at 001BC0E0, script 0x82CB00 and
 * step 2. Step 2: at 001BC0E0, +0xB = 0 and step 0. Step 3: at 001BC0E0,
 * step 4. Step 4: 001BC240, step 5. Step 5: step 0 at 001BC290. Then
 * 001BC300(self) (also for steps 6 and above).
 * ---------------------------------------------------------------------- */
static void a21d_door_steps(A21D *o, uint32_t self)
{
    const uint32_t talk = self + 0x1F0;
    const uint32_t step = a21d_u8(o, self + 5);
    int32_t r;

    switch (step) {
    case 0: {
        const uint32_t area = a21d_u8(o, D_00810700);
        const uint32_t lock = (uint32_t)a21d_s16(o, self + 0x34);
        const uint32_t bits = a21d_u8(o, D_00810841 + area);
        if (bits & a21d_bit(lock)) {
            a21d_w8(o, self, 1);
            r = 0;
            (void)a21d_c_001BBE40(o, self, talk, 0, &r);
            if (r != 0) a21d_w8(o, self + 5, 3);
        } else {
            a21d_w8(o, self, 2);
            r = 0;
            (void)a21d_c_001BBE40(o, self, talk, 1, &r);
            if (r != 0) a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
        }
        break;
    }
    case 1:
        r = 0;
        (void)a21d_c_001BC0E0(o, self, talk, &r);
        if (r != 0) {
            (void)a21d_c_001BA1A0(o, talk, A21D_SCRIPT_82CB00);
            a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
        }
        break;
    case 2:
        r = 0;
        (void)a21d_c_001BC0E0(o, self, talk, &r);
        if (r != 0) {
            a21d_w8(o, self + 0xB, 0);
            a21d_w8(o, self + 5, 0);
        }
        break;
    case 3:
        r = 0;
        (void)a21d_c_001BC0E0(o, self, talk, &r);
        if (r != 0) a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
        break;
    case 4:
        (void)a21d_c_001BC240(o, self, talk);
        a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
        break;
    case 5:
        r = 0;
        (void)a21d_c_001BC290(o, self, talk, &r);
        if (r != 0) a21d_w8(o, self + 5, 0);
        break;
    default:
        break;
    }
    (void)a21d_c_001BC300(o, self);
}

/* ----------------------------------------------------------------------
 * 0x826BB0 (decomp func_overlay_AREA21_00826B70) — door [30]. State 0:
 * 001BBDA0(self). State 1: while D_0081080E is 3, a timer instead of the
 * door: +7 0 sets +7 = 1 and +0x28 = 0; +7 1 counts +0x28 (a half), sets
 * the lock bit +0x34 (a byte here) of D_00810841[D_00810700] at 1492 and
 * clears it at 2000 with +7 = 2; +7 2 waits. Otherwise the door steps.
 * States 2 / 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a21d_826BB0(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a21d_c_001BBDA0(o, self);
        return;
    }
    if (state != 1) return;
    if (a21d_u8(o, D_0081080E) == 3) {
        const uint32_t phase = a21d_u8(o, self + 7);
        if (phase == 1) {
            a21d_w16(o, self + 0x28, (uint32_t)(a21d_s16(o, self + 0x28) + 1));
            if (a21d_s16(o, self + 0x28) == 1492) {
                const uint32_t area = a21d_u8(o, D_00810700);
                const uint32_t lock = a21d_u8(o, self + 0x34);
                const uint32_t at = D_00810841 + area;
                a21d_w8(o, at, a21d_u8(o, at) | a21d_bit(lock));
            }
            if (a21d_s16(o, self + 0x28) == 2000) {
                const uint32_t lock = a21d_u8(o, self + 0x34);
                const uint32_t area = a21d_u8(o, D_00810700);
                const uint32_t at = D_00810841 + area;
                a21d_w8(o, at, a21d_u8(o, at) & ~a21d_bit(lock));
                a21d_w8(o, self + 7, 2);
            }
        } else if (phase == 0) {
            a21d_w8(o, self + 7, 1);
            a21d_w16(o, self + 0x28, 0);
        }
        return;
    }
    a21d_door_steps(o, self);
}

int em_area21_doors_00826BB0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_826BB0(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826E60 (decomp func_overlay_AREA21_00826E20) — doors [27] / [33].
 * State 0: 001BBDA0(self). State 1: the door steps. States 2 / 3:
 * 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a21d_826E60(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
    } else if (state == 1) {
        a21d_door_steps(o, self);
    } else if (state == 0) {
        (void)a21d_c_001BBDA0(o, self);
    }
}

int em_area21_doors_00826E60(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_826E60(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x82A1D0 (decomp func_overlay_AREA21_0082A190) — [54] / [55]. State 0:
 * 001B0FD0, 001C6380, +0x1F0 = +0x1F4 = 0. State 1: with +3 == 0x4C,
 * +0xC4 += +0x1F0 and, once 0011DF78(+0xC4 - +0x1F4) is below 0.01, a new
 * target +0x1F4 = -pi/6 + pi/3 * (00122BB8() * 2^-31) and the step +0x1F0
 * = +0.005 towards it (-0.005 when it is not above +0xC4); otherwise +0xC4
 * copies the +0x18 object's. Then 001C6380, 001A2370(self, +0xD0) and the
 * +0x4C method when 001B17A0 is non-zero. State 3: 001AFC10. Others:
 * nothing.
 * ---------------------------------------------------------------------- */
static void a21d_82A1D0(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);
    int32_t r;

    if (state == 3) {
        (void)a21d_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a21d_c_001B0FD0(o, self, &r);
        (void)a21d_c_001C6380(o, self);
        a21d_w32(o, self + 0x1F0, F_ZERO);
        a21d_w32(o, self + 0x1F4, F_ZERO);
        return;
    }
    if (state != 1) return;
    if (a21d_u8(o, self + 3) == 0x4C) {
        const uint32_t step = a21d_u32(o, self + 0x1F0);
        a21d_w32(o, self + 0xC4, A21D_ADD(a21d_u32(o, self + 0xC4), step));
        const uint32_t target = a21d_u32(o, self + 0x1F4);
        const uint32_t angle = a21d_u32(o, self + 0xC4);
        uint32_t gap = 0;
        (void)a21d_c_0011DF78(o, A21D_SUB(angle, target), &gap);
        if (a21d_failed(o)) return;
        if (A21D_LT(gap, F_0_01)) {
            r = 0;
            (void)a21d_c_00122BB8(o, &r);
            if (a21d_failed(o)) return;
            uint32_t t = A21D_MUL(F_2_M31, A21D_CVT_S_W(r));
            t = A21D_MUL(F_PI_3, t);
            t = A21D_ADD(F_M_PI_6, t);
            a21d_w32(o, self + 0x1F4, t);
            if (A21D_LE(t, a21d_u32(o, self + 0xC4)))
                a21d_w32(o, self + 0x1F0, F_M0_005);
            else
                a21d_w32(o, self + 0x1F0, F_0_005);
        }
    } else {
        const uint32_t other = a21d_u32(o, self + 0x18);
        a21d_w32(o, self + 0xC4, a21d_u32(o, other + 0xC4));
    }
    (void)a21d_c_001C6380(o, self);
    (void)a21d_c_001A2370(o, self, self + 0xD0);
    r = 0;
    (void)a21d_c_001B17A0(o, self, &r);
    if (a21d_failed(o)) return;
    if (r != 0) (void)a21d_method(o, self);
}

int em_area21_doors_0082A1D0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_82A1D0(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825C70 (decomp func_overlay_AREA21_00825C30) — the rounds (a moving
 * object, no static reference). State 0: +0x70 = +0xC0, +0xC0 normalised
 * (00102760), the matrix +0xD0 from it (001CD390), the velocity +0x200 =
 * 7 * +0xC0 (00103230), +0x1F0 = +0xB0, +0x30 = 0x82B170, +0x34 = 0x825C60,
 * +0xC = +9 = 0, +0 = 1, state 1, life +0x20 = 180; and on into state 1:
 * +0x100 = +0xB0 (three words), the draw 001CEEE0(+0xD0, (0, 0, 0, 1),
 * (96, 96, 32, 1), 0x18, 5, 0.3, 0.3) through D_700038A0 / D_700038B0,
 * then the hit test r = 0019A570(+0x1F0, +0xB0, 7, 0x20). A hit: the
 * point D_700038A0 = the hit point (w 1); r 1 with an object +0 1 whose +2
 * low five bits are 2: 001B41F0(object, point, +0x70, the surface's +0x1C,
 * 0, 10); r 2 with a live object: its +0x36 = 5, its +0x70 = our +0x70; the
 * direction D_700038B0 = (0, 001B1470(pi + D_00810374), 0, 1) for r 1, else
 * the surface's +0x24..+0x2C with both w 0; effect 0x80000003; +0 = 2,
 * state 3. Below y 10: effect 0x80000026 at (x, 10, z, 1) up (0, 1, 0, 1);
 * +0 = 2, state 3. Else the life counts down (state 3 at zero), +0x1F0 =
 * +0xB0 and +0xB0 += the velocity. States 2 / 3: 001AFC10. Others:
 * nothing.
 * ---------------------------------------------------------------------- */
static void a21d_825C70(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);
    const uint32_t w = self + 0x1F0;
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
        return;
    }
    if (state != 1 && state != 0) return;
    if (state == 0) {
        (void)a21d_c_00102948(o, self + 0x70, self + 0xC0);
        (void)a21d_c_00102760(o, self + 0xC0, self + 0xC0);
        (void)a21d_c_001CD390(o, self + 0xD0, self + 0xC0);
        (void)a21d_c_00103230(o, w + 0x10, self + 0xC0, F_7);
        (void)a21d_c_00102948(o, w, self + 0xB0);
        a21d_w32(o, self + 0x30, A21D_TABLE_82B170);
        a21d_w32(o, self + 0x34, A21D_CODE_825C60);
        a21d_w8(o, self + 0xC, 0);
        a21d_w8(o, self + 9, 0);
        a21d_w8(o, self, 1);
        a21d_w8(o, self + 4, 1);
        a21d_w32(o, self + 0x20, F_180);
    }
    a21d_w32(o, self + 0x100, a21d_u32(o, self + 0xB0));
    a21d_w32(o, self + 0x104, a21d_u32(o, self + 0xB4));
    a21d_w32(o, self + 0x108, a21d_u32(o, self + 0xB8));
    a21d_w32(o, D_700038A0, F_ZERO);
    a21d_w32(o, D_700038A0 + 4, F_ZERO);
    a21d_w32(o, D_700038A0 + 8, F_ZERO);
    a21d_w32(o, D_700038A0 + 0xC, F_ONE);
    a21d_w32(o, D_700038B0, F_96);
    a21d_w32(o, D_700038B0 + 4, F_96);
    a21d_w32(o, D_700038B0 + 8, F_32);
    a21d_w32(o, D_700038B0 + 0xC, F_ONE);
    (void)a21d_c_001CEEE0(o, self + 0xD0, D_700038A0, D_700038B0, 0x18, F_5, F_0_3, F_0_3);
    r = 0;
    (void)a21d_c_0019A570(o, w, self + 0xB0, 7, 0x20, &r);
    if (a21d_failed(o)) return;
    if (r != 0) {
        (void)a21d_c_00102948(o, D_700038A0, D_700031B0);
        a21d_w32(o, D_700038A0 + 0xC, F_ONE);
        if (r == 1) {
            const uint32_t hit = a21d_u32(o, D_700031D4);
            if (hit != 0 && a21d_u8(o, hit) == 1 && (a21d_u8(o, hit + 2) & 0x1Fu) == 2) {
                const uint32_t surface = a21d_u32(o, D_700031D0);
                (void)a21d_c_001B41F0(o, hit, D_700038A0, self + 0x70, a21d_s32(o, surface + 0x1C), 0, 10);
            }
        } else if (r == 2) {
            const uint32_t hit = a21d_u32(o, D_700031D4);
            if (hit != 0 && a21d_u8(o, hit) != 0) {
                a21d_w16(o, hit + 0x36, 5);
                (void)a21d_c_00102948(o, hit + 0x70, self + 0x70);
            }
        }
        if (r == 1) {
            a21d_w32(o, D_700038B0, F_ZERO);
            uint32_t up = 0;
            (void)a21d_c_001B1470(o, A21D_ADD(F_PI, a21d_u32(o, D_00810374)), &up);
            a21d_w32(o, D_700038B0 + 4, up);
            a21d_w32(o, D_700038B0 + 8, F_ZERO);
            a21d_w32(o, D_700038B0 + 0xC, F_ONE);
        } else {
            const uint32_t surface = a21d_u32(o, D_700031D0);
            a21d_w32(o, D_700038B0, a21d_u32(o, surface + 0x24));
            a21d_w32(o, D_700038B0 + 4, a21d_u32(o, surface + 0x28));
            a21d_w32(o, D_700038B0 + 8, a21d_u32(o, surface + 0x2C));
            a21d_w32(o, D_700038A0 + 0xC, F_ZERO);
            a21d_w32(o, D_700038B0 + 0xC, F_ZERO);
        }
        (void)a21d_c_001EFD90(o, 0x80000003u, D_700038A0, D_700038B0);
        a21d_w8(o, self, 2);
        a21d_w8(o, self + 4, 3);
    } else if (A21D_LT(a21d_u32(o, self + 0xB4), F_10)) {
        a21d_w32(o, D_700038A0, a21d_u32(o, self + 0xB0));
        a21d_w32(o, D_700038A0 + 4, F_10);
        a21d_w32(o, D_700038A0 + 8, a21d_u32(o, self + 0xB8));
        a21d_w32(o, D_700038A0 + 0xC, F_ONE);
        a21d_w32(o, D_700038B0, F_ZERO);
        a21d_w32(o, D_700038B0 + 4, F_ONE);
        a21d_w32(o, D_700038B0 + 8, F_ZERO);
        a21d_w32(o, D_700038B0 + 0xC, F_ONE);
        (void)a21d_c_001EFD90(o, 0x80000026u, D_700038A0, D_700038B0);
        a21d_w8(o, self, 2);
        a21d_w8(o, self + 4, 3);
    } else {
        const uint32_t life = A21D_SUB(a21d_u32(o, self + 0x20), F_ONE);
        a21d_w32(o, self + 0x20, life);
        if (A21D_EQ(life, F_ZERO)) a21d_w8(o, self + 4, 3);
        (void)a21d_c_00102948(o, w, self + 0xB0);
        for (uint32_t k = 0; k < 12; k += 4) {
            const uint32_t v = a21d_u32(o, w + 0x10 + k);
            a21d_w32(o, self + 0xB0 + k, A21D_ADD(a21d_u32(o, self + 0xB0 + k), v));
        }
    }
}

int em_area21_doors_00825C70(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_825C70(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825C60 (decomp func_overlay_AREA21_00825C20) — the rounds' +0x34 code
 * pointer: returns at once.
 * ---------------------------------------------------------------------- */
int em_area21_doors_00825C60(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    (void)a0;
    if (a21d_begin(&o, h, fault)) return -1;
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8272C0 (decomp func_overlay_AREA21_00827280) — [47]. State 0:
 * 001B0FD0, 001C6380. State 1: the +0x74 of the object at
 * *(D_00275B40 + 4) = 001B1470(its +0x74 - pi/30); the one at +8 gets
 * 001B1470(the first one's new +0x74 - pi/60); 001C6380, then the +0x4C
 * method when 001B17A0 is non-zero. States 2 / 3: 001AFC10. Others:
 * nothing.
 * ---------------------------------------------------------------------- */
static void a21d_8272C0(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);
    int32_t r;
    uint32_t v;

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a21d_c_001B0FD0(o, self, &r);
        (void)a21d_c_001C6380(o, self);
        return;
    }
    if (state != 1) return;
    v = 0;
    (void)a21d_c_001B1470(o, A21D_SUB(a21d_u32(o, a21d_u32(o, a21d_u32(o, D_00275B40) + 4) + 0x74), F_PI_30), &v);
    if (a21d_failed(o)) return;
    a21d_w32(o, a21d_u32(o, a21d_u32(o, D_00275B40) + 4) + 0x74, v);
    v = 0;
    (void)a21d_c_001B1470(o, A21D_SUB(a21d_u32(o, a21d_u32(o, a21d_u32(o, D_00275B40) + 4) + 0x74), F_PI_60), &v);
    if (a21d_failed(o)) return;
    a21d_w32(o, a21d_u32(o, a21d_u32(o, D_00275B40) + 8) + 0x74, v);
    (void)a21d_c_001C6380(o, self);
    r = 0;
    (void)a21d_c_001B17A0(o, self, &r);
    if (a21d_failed(o)) return;
    if (r != 0) (void)a21d_method(o, self);
}

int em_area21_doors_008272C0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_8272C0(&o, a0);
    return a21d_end(&o);
}

/* One step of the 8-step wobble of 0x8273C0 / 0x827640: +0xB0 from +0x1F0
 * and +0xB4 from +0x1F4, each the saved value, the saved value plus a
 * constant (add > 0) or minus it (add < 0). */
typedef struct {
    int8_t add_x;
    uint32_t x;
    int8_t add_y;
    uint32_t y;
} A21DWobble;

static uint32_t a21d_offset(int8_t add, uint32_t c, uint32_t v)
{
    return add > 0 ? A21D_ADD(c, v) : add < 0 ? A21D_SUB(v, c) : v;
}

/* State 1's start: +0x28 counts (a half) and its low three bits pick the
 * step (a jump table in the original). */
static void a21d_wobble(A21D *o, uint32_t self, const A21DWobble *steps)
{
    a21d_w16(o, self + 0x28, (uint32_t)(a21d_s16(o, self + 0x28) + 1));
    const A21DWobble *m = &steps[(uint32_t)a21d_s16(o, self + 0x28) & 7u];
    a21d_w32(o, self + 0xB0, a21d_offset(m->add_x, m->x, a21d_u32(o, self + 0x1F0)));
    a21d_w32(o, self + 0xB4, a21d_offset(m->add_y, m->y, a21d_u32(o, self + 0x1F4)));
}

/* ----------------------------------------------------------------------
 * 0x8273C0 (decomp func_overlay_AREA21_00827380, NEARMISS: its .s is the
 * original) — [49] / [50]. State 0: 001B0FD0, 001C6380, +0x1F0 / +0x1F4
 * = +0xB0 / +0xB4, +0x1F8 = -1. State 1: the wobble (0.3 / 0.2 / 0.4),
 * 001FC3C0(self, +0x1F8, 0x450, 150, 4096), the +0x78 of the object at
 * *(D_00275B40 + 4) = 001B1470(its +0x78 - 0.523), 001C6380 and the +0x4C
 * method. States 2 / 3: 001AFC10. Others: nothing.
 * ---------------------------------------------------------------------- */
static const A21DWobble a21d_wobble_8273C0[8] = {
    {0, 0, 0, 0},           {-1, F_0_3, 0, 0},      {0, 0, -1, F_0_2},      {0, 0, 1, F_0_2},
    {1, F_0_3, 0, 0},       {-1, F_0_2, 1, F_0_4},  {1, F_0_3, -1, F_0_2},  {1, F_0_2, 1, F_0_2},
};

static void a21d_8273C0(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);
    int32_t r;
    uint32_t v;

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a21d_c_001B0FD0(o, self, &r);
        (void)a21d_c_001C6380(o, self);
        a21d_w32(o, self + 0x1F0, a21d_u32(o, self + 0xB0));
        a21d_w32(o, self + 0x1F4, a21d_u32(o, self + 0xB4));
        a21d_w32(o, self + 0x1F8, 0xFFFFFFFFu);
        return;
    }
    if (state != 1) return;
    a21d_wobble(o, self, a21d_wobble_8273C0);
    (void)a21d_c_001FC3C0(o, self, self + 0x1F8, 0x450, F_150, F_4096);
    v = 0;
    (void)a21d_c_001B1470(o, A21D_SUB(a21d_u32(o, a21d_u32(o, a21d_u32(o, D_00275B40) + 4) + 0x78), F_0_523), &v);
    if (a21d_failed(o)) return;
    a21d_w32(o, a21d_u32(o, a21d_u32(o, D_00275B40) + 4) + 0x78, v);
    (void)a21d_c_001C6380(o, self);
    (void)a21d_method(o, self);
}

int em_area21_doors_008273C0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_8273C0(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x827640 (decomp func_overlay_AREA21_00827600) — [51]. State 0:
 * 001B0FD0, 001C6380, +0x1F0 / +0x1F4 = +0xB0 / +0xB4. State 1: the
 * wobble (0.075 / 0.05 / 0.06), 001C6380, 001B17A0 (its result unused)
 * and the +0x4C method. States 2 / 3: 001AFC10. Others: nothing.
 * ---------------------------------------------------------------------- */
static const A21DWobble a21d_wobble_827640[8] = {
    {0, 0, 0, 0},              {-1, F_0_075, 0, 0},        {0, 0, -1, F_0_05},        {0, 0, 1, F_0_075},
    {1, F_0_075, 0, 0},        {-1, F_0_05, 1, F_0_06},    {1, F_0_075, -1, F_0_05},  {1, F_0_05, 1, F_0_05},
};

static void a21d_827640(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a21d_c_001B0FD0(o, self, &r);
        (void)a21d_c_001C6380(o, self);
        a21d_w32(o, self + 0x1F0, a21d_u32(o, self + 0xB0));
        a21d_w32(o, self + 0x1F4, a21d_u32(o, self + 0xB4));
        return;
    }
    if (state != 1) return;
    a21d_wobble(o, self, a21d_wobble_827640);
    (void)a21d_c_001C6380(o, self);
    r = 0;
    (void)a21d_c_001B17A0(o, self, &r);
    (void)a21d_method(o, self);
}

int em_area21_doors_00827640(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_827640(&o, a0);
    return a21d_end(&o);
}

/* The start shared by 0x827880 / 0x827A30 / 0x827E70. State 0:
 * 001B0FD0 and, when it returns 0, 001C6380 and +5 = 0. Returns 1 when
 * the state was handled here (0, 2, 3 or another than 1). */
static int a21d_timed_start(A21D *o, uint32_t self, uint32_t state)
{
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a21d_c_001AFC10(o, self);
        return 1;
    }
    if (state == 0) {
        r = 0;
        (void)a21d_c_001B0FD0(o, self, &r);
        if (a21d_failed(o) || r != 0) return 1;
        (void)a21d_c_001C6380(o, self);
        a21d_w8(o, self + 5, 0);
        return 1;
    }
    return state != 1;
}

/* Sub-steps 0 and 1 of the same three: +5 0 waits for D_0081080E ==
 * 0x10 (+0x28 = 0, +5 1); +5 1 counts +0x28 (a half) and moves on once it
 * is not below `frames`. */
static void a21d_timed_count(A21D *o, uint32_t self, uint32_t step, int32_t frames)
{
    if (step == 0) {
        if (a21d_u8(o, D_0081080E) == 0x10) {
            a21d_w16(o, self + 0x28, 0);
            a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
        }
    } else {
        a21d_w16(o, self + 0x28, (uint32_t)(a21d_s16(o, self + 0x28) + 1));
        if (a21d_s16(o, self + 0x28) >= frames) a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
    }
}

/* D_700038A0 = (x, y, z, 1). */
static void a21d_point(A21D *o, uint32_t x, uint32_t y, uint32_t z)
{
    a21d_w32(o, D_700038A0, x);
    a21d_w32(o, D_700038A0 + 4, y);
    a21d_w32(o, D_700038A0 + 8, z);
    a21d_w32(o, D_700038A0 + 0xC, F_ONE);
}

/* The three effects 0x8000002F at (-25 / 0 / 25, y, 0) from +0xB0
 * (001028B8 adds +0xB0 in place). */
static void a21d_three_effects(A21D *o, uint32_t self, uint32_t y)
{
    static const uint32_t xs[3] = {F_M25, F_ZERO, F_25};
    for (int i = 0; i < 3; i++) {
        a21d_point(o, xs[i], y, F_ZERO);
        (void)a21d_c_001028B8(o, D_700038A0, D_700038A0, self + 0xB0);
        (void)a21d_c_001EFD20(o, 0x8000002Fu, D_700038A0);
    }
}

/* ----------------------------------------------------------------------
 * 0x827880 (decomp func_overlay_AREA21_00827840) — [56]. The shared
 * start. State 1: +5 0 / 1 (a 255-frame count after D_0081080E == 0x10),
 * each followed by 001B1B70 and the +0x4C method; +5 2: effect 0x8000004C
 * at +0xB0 with (0, pi, 0, 1), effect 0x80000041 at +0xB0, state 3,
 * 001F02C0(+0xB0, 0x44D, 600). Other sub-steps: nothing.
 * ---------------------------------------------------------------------- */
static void a21d_827880(A21D *o, uint32_t self)
{
    if (a21d_timed_start(o, self, a21d_u8(o, self + 4))) return;
    const uint32_t step = a21d_u8(o, self + 5);
    if (step == 0 || step == 1) {
        a21d_timed_count(o, self, step, 0xFF);
        (void)a21d_c_001B1B70(o, self);
        (void)a21d_method(o, self);
    } else if (step == 2) {
        a21d_point(o, F_ZERO, F_PI, F_ZERO);
        (void)a21d_c_001EFD90(o, 0x8000004Cu, self + 0xB0, D_700038A0);
        (void)a21d_c_001EFD20(o, 0x80000041u, self + 0xB0);
        a21d_w8(o, self + 4, 3);
        (void)a21d_c_001F02C0(o, self + 0xB0, 0x44D, F_600);
    }
}

int em_area21_doors_00827880(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_827880(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x827A30 (decomp func_overlay_AREA21_008279F0) — [57]. As 0x827880 with
 * a 65-frame count; +5 2: the three effects at y 12, effect 0x8000005E at
 * +0xB0 with (0, pi, 0, 1), state 3, 001F02C0(+0xB0, 0x44B, 800).
 * ---------------------------------------------------------------------- */
static void a21d_827A30(A21D *o, uint32_t self)
{
    if (a21d_timed_start(o, self, a21d_u8(o, self + 4))) return;
    const uint32_t step = a21d_u8(o, self + 5);
    if (step == 0 || step == 1) {
        a21d_timed_count(o, self, step, 0x41);
        (void)a21d_c_001B1B70(o, self);
        (void)a21d_method(o, self);
    } else if (step == 2) {
        a21d_three_effects(o, self, F_12);
        a21d_point(o, F_ZERO, F_PI, F_ZERO);
        (void)a21d_c_001EFD90(o, 0x8000005Eu, self + 0xB0, D_700038A0);
        a21d_w8(o, self + 4, 3);
        (void)a21d_c_001F02C0(o, self + 0xB0, 0x44B, F_800);
    }
}

int em_area21_doors_00827A30(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_827A30(&o, a0);
    return a21d_end(&o);
}

/* The placement record of 0x827E70's node: record +0x9A (0x28 bytes each)
 * of D_0024D7C0[D_00810700][D_00810701], read afresh for every field. */
static uint32_t a21d_record(A21D *o, uint32_t self)
{
    const uint32_t area = a21d_u8(o, D_00810700);
    const uint32_t slot = a21d_u8(o, self + 0x9A);
    const uint32_t room = a21d_u8(o, D_00810701);
    const uint32_t rooms = a21d_u32(o, D_0024D7C0 + area * 4);
    return slot * 0x28 + a21d_u32(o, rooms + room * 4);
}

/* ----------------------------------------------------------------------
 * 0x827E70 (decomp func_overlay_AREA21_00827E30) — [59] / [60]. The
 * shared start. State 1: +5 0 / 1 (a 220-frame count after D_0081080E ==
 * 0x10); +5 2: the three effects at y 0, effect 0x8000005E at +0xB0 with
 * (0, pi, 0, 1), +5 3, 001F02C0(+0xB0, 0x449, 800); +5 3: +0xE (a half),
 * +0xC0..C8 and +0xB0..B8 from the placement record (+0x2E, +0x40..48,
 * +0x34..3C), 001CA6E0(self, 001C6120(D_0028A59C, 9)), 001C62C0,
 * 001C6380, +5 4; every sub-step then 001B1B70 and the +0x4C method.
 * ---------------------------------------------------------------------- */
static void a21d_827E70(A21D *o, uint32_t self)
{
    static const uint8_t from[6] = {0x40, 0x44, 0x48, 0x34, 0x38, 0x3C};
    static const uint8_t to[6] = {0xC0, 0xC4, 0xC8, 0xB0, 0xB4, 0xB8};
    int32_t r;

    if (a21d_timed_start(o, self, a21d_u8(o, self + 4))) return;
    const uint32_t step = a21d_u8(o, self + 5);
    if (step == 0 || step == 1) {
        a21d_timed_count(o, self, step, 0xDC);
    } else if (step == 2) {
        a21d_three_effects(o, self, F_ZERO);
        a21d_point(o, F_ZERO, F_PI, F_ZERO);
        (void)a21d_c_001EFD90(o, 0x8000005Eu, self + 0xB0, D_700038A0);
        a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
        (void)a21d_c_001F02C0(o, self + 0xB0, 0x449, F_800);
    } else if (step == 3) {
        a21d_w16(o, self + 0xE, a21d_u16(o, a21d_record(o, self) + 0x2E));
        for (int k = 0; k < 6; k++) a21d_w32(o, self + to[k], a21d_u32(o, a21d_record(o, self) + from[k]));
        r = 0;
        (void)a21d_c_001C6120(o, a21d_s32(o, D_0028A59C), 9, &r);
        (void)a21d_c_001CA6E0(o, self, r);
        (void)a21d_c_001C62C0(o, self);
        (void)a21d_c_001C6380(o, self);
        a21d_w8(o, self + 5, a21d_u8(o, self + 5) + 1);
    }
    (void)a21d_c_001B1B70(o, self);
    (void)a21d_method(o, self);
}

int em_area21_doors_00827E70(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_827E70(&o, a0);
    return a21d_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x828350 (decomp func_overlay_AREA21_00828310) — the deferred group
 * 0x82A380's member. State 0: 0015AC00(self, +0x1F0). State 1:
 * 0015AE20(self, +0x1F0) and, while D_0081080E is 0x10, +0x28 counts (a
 * half) and state 3 once it is above 250. Other states: 001B1190(+0x9A),
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21d_828350(A21D *o, uint32_t self)
{
    const uint32_t state = a21d_u8(o, self + 4);

    if (state == 0) {
        (void)a21d_c_0015AC00(o, self, self + 0x1F0);
    } else if (state == 1) {
        (void)a21d_c_0015AE20(o, self, self + 0x1F0);
        if (a21d_u8(o, D_0081080E) == 0x10) {
            a21d_w16(o, self + 0x28, (uint32_t)(a21d_s16(o, self + 0x28) + 1));
            if (a21d_s16(o, self + 0x28) >= 0xFB) a21d_w8(o, self + 4, 3);
        }
    } else {
        (void)a21d_c_001B1190(o, (int32_t)a21d_u8(o, self + 0x9A));
        (void)a21d_c_001AFC10(o, self);
    }
}

int em_area21_doors_00828350(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault)
{
    A21D o;
    if (a21d_begin(&o, h, fault)) return -1;
    a21d_828350(&o, a0);
    return a21d_end(&o);
}
