/* AREA21's own placement owners and script callbacks (em_area21_owners.h).
 * Each function is one original function; the comment above it says what
 * the code does. The order of the memory accesses between calls is the
 * original's (the test compares it one for one). */
#include "em_area21_owners_internal.h"

/* overlay data (runtime addresses) */
#define A21W_GROUP_82A540 0x0082A540u   /* 0x826310's group at its script end */
#define A21W_GROUP_82A5A0 0x0082A5A0u   /* 0x8269A0's group */
#define A21W_GROUP_82A600 0x0082A600u   /* [53]'s group after flag 0x36 */
#define A21W_SCRIPT_82B240 0x0082B240u  /* [42]'s script */
#define A21W_SCRIPT_82B5E0 0x0082B5E0u  /* 0x826310's script */
#define A21W_SCRIPT_82B8A0 0x0082B8A0u  /* 0x8264D0's Use script */
#define A21W_TABLE_82BA60 0x0082BA60u   /* 0x8264D0's +0x30 */
#define A21W_SCRIPT_82BEC0 0x0082BEC0u  /* 0x826840's script */
#define A21W_SCRIPT_82C5C0 0x0082C5C0u  /* 0x8268F0's script */
#define A21W_TABLE_82CA00 0x0082CA00u   /* 0x8269F0's 32 x offsets */
#define A21W_TABLE_82CA80 0x0082CA80u   /* 0x8269F0's 32 y offsets */
#define A21W_SCRIPT_82CBD0 0x0082CBD0u  /* [53]'s ending script */

/* globals */
#define D_008102B0 0x008102B0u
#define D_008104D0 0x008104D0u
#define D_008104D8 0x008104D8u
#define D_008105D0 0x008105D0u
#define D_008105E0 0x008105E0u
#define D_00810650 0x00810650u
#define D_008106B8 0x008106B8u
#define D_008106C0 0x008106C0u
#define D_00810702 0x00810702u
#define D_0081078C 0x0081078Cu
#define D_0081078D 0x0081078Du
#define D_0081078E 0x0081078Eu
#define D_0081078F 0x0081078Fu
#define D_0081080C 0x0081080Cu
#define D_0081080D 0x0081080Du
#define D_0081080E 0x0081080Eu
#define D_0081080F 0x0081080Fu
#define D_00810856 0x00810856u
#define D_00810858 0x00810858u
#define D_0081085C 0x0081085Cu
#define D_70003600 0x70003600u
#define D_70003610 0x70003610u
#define D_70003B93 0x70003B93u

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_10 0x41200000u
#define F_20 0x41A00000u
#define F_100 0x42C80000u
#define F_1_308997 0x3FA78D37u

/* ----------------------------------------------------------------------
 * 0x8261A0 (decomp func_overlay_AREA21_00826160) — [42]. State 0: state 3
 * with flag 0x34 set, else state 1 and +0 = 1. State 1: step 0 starts
 * script 0x82B240 (step 1) once D_0081078C is set; step 1 at the script's
 * end: 001FAE70(0), +0x2E = 0xFFFF, state 3, D_0081080C = 0xFF; 001B17A0
 * in every step. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21w_8261A0(A21W *o, uint32_t self)
{
    const uint32_t state = a21w_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a21w_c_001BA1C0(o, self, 0x34, &r);
        if (r != 0) {
            a21w_w8(o, self + 4, 3);
        } else {
            a21w_w8(o, self + 4, 1);
            a21w_w8(o, self, 1);
        }
    } else if (state == 1) {
        const uint32_t step = a21w_u8(o, self + 5);
        if (step == 0) {
            if (a21w_u8(o, D_0081078C) != 0) {
                (void)a21w_c_001BA1A0(o, self + 0x1F0, A21W_SCRIPT_82B240);
                a21w_w8(o, self + 5, 1);
            }
        } else if (step == 1) {
            r = 0;
            (void)a21w_c_001BA1F0(o, self, &r);
            if (r != 0) {
                (void)a21w_c_001FAE70(o, 0);
                a21w_w16(o, self + 0x2E, 0xFFFF);
                a21w_w8(o, self + 4, 3);
                a21w_w8(o, D_0081080C, 0xFF);
            }
        }
        (void)a21w_c_001B17A0(o, self);
    } else if (state == 2 || state == 3) {
        (void)a21w_c_001AFC10(o, self);
    }
}

int em_area21_owners_008261A0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_8261A0(&o, a0);
    return a21w_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826310 (decomp func_overlay_AREA21_008262D0) — [43] / [44] with +3 0.
 * State 0: D_00810856 = 0xFF; state 3 with flag 0x35 set, else state 1
 * and +0 = 1. State 1: step 0 ORs 1 into D_0081080D at arrival entry
 * D_00810702 1 and 2 at entry 5, step 1 once D_0081080D is 3; step 1
 * starts script 0x82B5E0 (step 2) at entry 6 or 2; step 2 at the script's
 * end: D_008106C0 = 001B6660(group 0x82A540), +0x2E = 0xFFFF, D_00810856 =
 * 0, 001FB0B0(0xC), state 3. 001B17A0 in every step. States 2 / 3:
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21w_826310(A21W *o, uint32_t self)
{
    const uint32_t state = a21w_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        a21w_w8(o, D_00810856, 0xFF);
        r = 0;
        (void)a21w_c_001BA1C0(o, self, 0x35, &r);
        if (r != 0) {
            a21w_w8(o, self + 4, 3);
        } else {
            a21w_w8(o, self + 4, 1);
            a21w_w8(o, self, 1);
        }
    } else if (state == 1) {
        const uint32_t step = a21w_u8(o, self + 5);
        if (step == 0) {
            if (a21w_u8(o, D_00810702) == 1) a21w_w8(o, D_0081080D, a21w_u8(o, D_0081080D) | 1);
            if (a21w_u8(o, D_00810702) == 5) a21w_w8(o, D_0081080D, a21w_u8(o, D_0081080D) | 2);
            if (a21w_u8(o, D_0081080D) == 3) a21w_w8(o, self + 5, 1);
        } else if (step == 1) {
            const uint32_t entry = a21w_u8(o, D_00810702);
            if (entry == 6 || entry == 2) {
                (void)a21w_c_001BA1A0(o, self + 0x1F0, A21W_SCRIPT_82B5E0);
                a21w_w8(o, self + 5, 2);
            }
        } else if (step == 2) {
            r = 0;
            (void)a21w_c_001BA1F0(o, self, &r);
            if (r != 0) {
                r = 0;
                (void)a21w_c_001B6660(o, A21W_GROUP_82A540, &r);
                a21w_w32(o, D_008106C0, (uint32_t)r);
                a21w_w16(o, self + 0x2E, 0xFFFF);
                a21w_w8(o, D_00810856, 0);
                (void)a21w_c_001FB0B0(o, 0xC);
                a21w_w8(o, self + 4, 3);
            }
        }
        (void)a21w_c_001B17A0(o, self);
    } else if (state == 2 || state == 3) {
        (void)a21w_c_001AFC10(o, self);
    }
}

int em_area21_owners_00826310(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_826310(&o, a0);
    return a21w_end(&o);
}

/* The tail of 0x8264D0's state 1: 001B17A0, 001C68C0, the method. */
static void a21w_tail(A21W *o, uint32_t self)
{
    (void)a21w_c_001B17A0(o, self);
    (void)a21w_c_001C68C0(o, self);
    (void)a21w_method(o, self);
}

/* ----------------------------------------------------------------------
 * 0x8264D0 (decomp func_overlay_AREA21_00826490) — [43] / [44] with +3
 * not 0. State 0, once flag 0x34 is set: 001B10B0(self, +0xD, 0x52),
 * 001C63E0(self, 0xA), 001CA6F0(self, 2), 001BA8E0(self, +0xD), +0x30 =
 * 0x82BA60, state 1, +0 = 1. State 1: step 0 goes to step 2 once
 * D_0081078D is set (nothing else that frame); else a Use (+0xB bit 2)
 * starts script 0x82B8A0 (step 1); step 1 at the script's end:
 * 001C67E0(self, 0xA, 20.0, 0.0), +0xB = 0, step 0. Steps 0 / 1 then run
 * 001BA580(self, +0xD) and 001C64F0(self, 1.0); step 2 only the latter;
 * then the tail. States 2 / 3: 001BA540, 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21w_8264D0(A21W *o, uint32_t self)
{
    const uint32_t state = a21w_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a21w_c_001BA1C0(o, self, 0x34, &r);
        if (r != 0) {
            (void)a21w_c_001B10B0(o, self, (int32_t)a21w_u8(o, self + 0xD), 0x52);
            (void)a21w_c_001C63E0(o, self, 0xA);
            (void)a21w_c_001CA6F0(o, self, 2);
            (void)a21w_c_001BA8E0(o, self, (int32_t)a21w_u8(o, self + 0xD));
            a21w_w32(o, self + 0x30, A21W_TABLE_82BA60);
            a21w_w8(o, self + 4, 1);
            a21w_w8(o, self, 1);
        }
    } else if (state == 1) {
        const uint32_t step = a21w_u8(o, self + 5);
        if (step == 0) {
            if (a21w_u8(o, D_0081078D) != 0) {
                a21w_w8(o, self + 5, 2);
                return;
            }
            if (a21w_u8(o, self + 0xB) & 4) {
                (void)a21w_c_001BA1A0(o, self + 0x1F0, A21W_SCRIPT_82B8A0);
                a21w_w8(o, self + 5, 1);
            }
            (void)a21w_c_001BA580(o, self, (int32_t)a21w_u8(o, self + 0xD));
            (void)a21w_c_001C64F0(o, self, F_ONE);
            a21w_tail(o, self);
        } else if (step == 1) {
            r = 0;
            (void)a21w_c_001BA1F0(o, self, &r);
            if (r != 0) {
                (void)a21w_c_001C67E0(o, self, 0xA, F_20, A21W_CVT_S_W(0));
                a21w_w8(o, self + 0xB, 0);
                a21w_w8(o, self + 5, 0);
            }
            (void)a21w_c_001BA580(o, self, (int32_t)a21w_u8(o, self + 0xD));
            (void)a21w_c_001C64F0(o, self, F_ONE);
            a21w_tail(o, self);
        } else if (step == 2) {
            (void)a21w_c_001C64F0(o, self, F_ONE);
            a21w_tail(o, self);
        }
    } else if (state == 2 || state == 3) {
        (void)a21w_c_001BA540(o, self);
        (void)a21w_c_001AFC10(o, self);
    }
}

int em_area21_owners_008264D0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_8264D0(&o, a0);
    return a21w_end(&o);
}

/* 0x8262D0 (decomp func_overlay_AREA21_00826290) — [43] / [44]: 0x826310
 * when +3 is 0, else 0x8264D0. */
int em_area21_owners_008262D0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    if (a21w_u8(&o, a0 + 3) == 0) a21w_826310(&o, a0);
    else a21w_8264D0(&o, a0);
    return a21w_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826840 (decomp func_overlay_AREA21_00826800) — [45]'s first step.
 * Step 0: at D_0081080E == 1, 0019C6F0(0x1C, 0) and script 0x82BEC0 (step
 * 1); step 1 at the script's end: step 0, 001FAE70(0), D_0081080E = 0x11.
 * ---------------------------------------------------------------------- */
static void a21w_826840(A21W *o, uint32_t self)
{
    const uint32_t step = a21w_u8(o, self + 5);
    int32_t r;

    if (step == 0) {
        if (a21w_u8(o, D_0081080E) == 1) {
            (void)a21w_c_0019C6F0(o, 0x1C, 0);
            (void)a21w_c_001BA1A0(o, self + 0x1F0, A21W_SCRIPT_82BEC0);
            a21w_w8(o, self + 5, 1);
        }
    } else if (step == 1) {
        r = 0;
        (void)a21w_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a21w_w8(o, self + 5, 0);
            (void)a21w_c_001FAE70(o, 0);
            a21w_w8(o, D_0081080E, 0x11);
        }
    }
}

int em_area21_owners_00826840(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_826840(&o, a0);
    return a21w_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8268F0 (decomp func_overlay_AREA21_008268B0) — [45]'s second step.
 * Step 0: at D_0081080E == 2, script 0x82C5C0, step 1, +0x2E = 0; step 1
 * at the script's end: +0x2E = 0xFFFF, D_0081080E = 0xFF, step 0,
 * 001FB0B0(0), 001FAE70(0).
 * ---------------------------------------------------------------------- */
static void a21w_8268F0(A21W *o, uint32_t self)
{
    const uint32_t step = a21w_u8(o, self + 5);
    int32_t r;

    if (step == 0) {
        if (a21w_u8(o, D_0081080E) == 2) {
            (void)a21w_c_001BA1A0(o, self + 0x1F0, A21W_SCRIPT_82C5C0);
            a21w_w8(o, self + 5, 1);
            a21w_w16(o, self + 0x2E, 0);
        }
    } else if (step == 1) {
        r = 0;
        (void)a21w_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a21w_w16(o, self + 0x2E, 0xFFFF);
            a21w_w8(o, D_0081080E, 0xFF);
            a21w_w8(o, self + 5, 0);
            (void)a21w_c_001FB0B0(o, 0);
            (void)a21w_c_001FAE70(o, 0);
        }
    }
}

int em_area21_owners_008268F0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_8268F0(&o, a0);
    return a21w_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8266F0 (decomp func_overlay_AREA21_008266B0) — [45]. State 0: +5 =
 * 0xFF with flag 0x36 set; then, unless 001B0FD0(self): 001C6380, state 1,
 * +0 = 1. State 1: by D_0081080E, 1 / 4 / 0x10 / 0x11 run 0x826840 and
 * 2 / 3 / 0xFF run 0x8268F0; then 001B17A0 and the method. States 2 / 3:
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21w_8266F0(A21W *o, uint32_t self)
{
    const uint32_t state = a21w_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a21w_c_001BA1C0(o, self, 0x36, &r);
        if (r != 0) a21w_w8(o, self + 5, 0xFF);
        r = 0;
        (void)a21w_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a21w_c_001C6380(o, self);
        a21w_w8(o, self + 4, 1);
        a21w_w8(o, self, 1);
    } else if (state == 1) {
        const uint32_t v = a21w_u8(o, D_0081080E);
        if (v == 1 || v == 4 || v == 0x10 || v == 0x11) a21w_826840(o, self);
        else if (v == 2 || v == 3 || v == 0xFF) a21w_8268F0(o, self);
        (void)a21w_c_001B17A0(o, self);
        (void)a21w_method(o, self);
    } else if (state == 2 || state == 3) {
        (void)a21w_c_001AFC10(o, self);
    }
}

int em_area21_owners_008266F0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_8266F0(&o, a0);
    return a21w_end(&o);
}

/* 0x8269A0 (decomp func_overlay_AREA21_00826960): script 0x82BEC0's op09
 * callback: D_008106C0 = 001B6660(group 0x82A5A0), D_0081080E = 0x10,
 * 001FABB0(), 001FA790(0, 0x11); returns 1. */
int em_area21_owners_008269A0(const EmArea21OwnersHooks *h, int32_t *result, EmArea21OwnersFault *fault)
{
    A21W o;
    int32_t r = 0;
    if (!result || a21w_begin(&o, h, fault)) return -1;
    (void)a21w_c_001B6660(&o, A21W_GROUP_82A5A0, &r);
    a21w_w32(&o, D_008106C0, (uint32_t)r);
    a21w_w8(&o, D_0081080E, 0x10);
    (void)a21w_c_001FABB0(&o);
    (void)a21w_c_001FA790(&o, 0, 0x11);
    if (a21w_end(&o)) return -1;
    *result = 1;
    return 0;
}

/* ----------------------------------------------------------------------
 * 0x8269F0 (decomp func_overlay_AREA21_008269B0) — script 0x82BEC0's op09
 * callback (self, record, work): the shake. Record +4 0: work +0x20 / +0x30
 * = copies of D_008105D0 / D_008105E0, record +4 = 1, work +0x10 = 0, and
 * on as +4 1: once work +0x10 is not below work +0xC the two vectors are
 * restored and it returns 1; else work +0x10 += 1 and, when 001281C0 of it
 * is odd, the scratchpad vector 0x70003600 = (table 0x82CA00[n & 31] / 10,
 * table 0x82CA80[m & 31] / 10, 0, 1) (n, m: 001281C0 of the count again),
 * 001026A0(0x70003610, D_00810650, it), and D_008105D0 / D_008105E0 = the
 * saved vectors plus 0x70003610 (001028B8). Returns 0 otherwise.
 * ---------------------------------------------------------------------- */
static int32_t a21w_8269F0(A21W *o, uint32_t rec, uint32_t work)
{
    const uint32_t step = a21w_u8(o, rec + 4);
    int32_t n;

    if (step == 0) {
        (void)a21w_c_00102948(o, work + 0x20, D_008105D0);
        (void)a21w_c_00102948(o, work + 0x30, D_008105E0);
        a21w_w8(o, rec + 4, a21w_u8(o, rec + 4) + 1);
        a21w_w32(o, work + 0x10, F_ZERO);
    } else if (step != 1) {
        return 0;
    }
    const uint32_t count = a21w_u32(o, work + 0x10);
    if (!A21W_LT(count, a21w_u32(o, work + 0xC))) {
        (void)a21w_c_00102948(o, D_008105D0, work + 0x20);
        (void)a21w_c_00102948(o, D_008105E0, work + 0x30);
        return 1;
    }
    const uint32_t t = A21W_ADD(count, F_ONE);
    a21w_w32(o, work + 0x10, t);
    n = 0;
    (void)a21w_c_001281C0(o, t, &n);
    if (n & 1) {
        n = 0;
        (void)a21w_c_001281C0(o, t, &n);
        a21w_w32(o, D_70003600, A21W_DIV(a21w_u32(o, A21W_TABLE_82CA00 + 4u * ((uint32_t)n & 0x1F)), F_10));
        n = 0;
        (void)a21w_c_001281C0(o, a21w_u32(o, work + 0x10), &n);
        a21w_w32(o, D_70003600 + 4, A21W_DIV(a21w_u32(o, A21W_TABLE_82CA80 + 4u * ((uint32_t)n & 0x1F)), F_10));
        a21w_w32(o, D_70003600 + 8, F_ZERO);
        a21w_w32(o, D_70003600 + 0xC, F_ONE);
        (void)a21w_c_001026A0(o, D_70003610, D_00810650, D_70003600);
        (void)a21w_c_001028B8(o, D_008105D0, work + 0x20, D_70003610);
        (void)a21w_c_001028B8(o, D_008105E0, work + 0x30, D_70003610);
    }
    return 0;
}

int em_area21_owners_008269F0(const EmArea21OwnersHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                              EmArea21OwnersFault *fault)
{
    A21W o;
    (void)a0;
    if (!result || a21w_begin(&o, h, fault)) return -1;
    const int32_t v = a21w_8269F0(&o, a1, a2);
    if (a21w_end(&o)) return -1;
    *result = v;
    return 0;
}

/* ----------------------------------------------------------------------
 * 0x827040 (decomp func_overlay_AREA21_00827000) — [53], the ending
 * request. State 0, unless 001B0FD0(self): 001C6380, state 1, +0 = 1.
 * State 1: step 0, once flag 0x36 is set: D_008106C0 = 001B6660(group
 * 0x82A600), step 1; step 1, when D_0081080F is set: D_0081078F = 1, script
 * 0x82CBD0, step 2, 001D2830(0x24, 1); step 2 at the script's end (result
 * r): 001AEDE0(4, 0), D_70003B93 = 2 when r is 3 else 1, 001FBC50(),
 * D_00810858 = D_008104D0 = 100, D_0081085C = D_008104D8 = 0, +0x2E =
 * 0xFFFF, state 3, D_0081080F = 0xFF, D_008106B8 = 1. Then, while
 * D_0081080F is set, +0xC0 = 1.308997 and 001C6380; then 001B1B70 and the
 * method. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21w_827040(A21W *o, uint32_t self)
{
    const uint32_t state = a21w_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a21w_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a21w_c_001C6380(o, self);
        a21w_w8(o, self + 4, 1);
        a21w_w8(o, self, 1);
    } else if (state == 1) {
        const uint32_t step = a21w_u8(o, self + 5);
        if (step == 0) {
            r = 0;
            (void)a21w_c_001BA1C0(o, self, 0x36, &r);
            if (r != 0) {
                r = 0;
                (void)a21w_c_001B6660(o, A21W_GROUP_82A600, &r);
                a21w_w32(o, D_008106C0, (uint32_t)r);
                a21w_w8(o, self + 5, 1);
            }
        } else if (step == 1) {
            if (a21w_u8(o, D_0081080F) != 0) {
                a21w_w8(o, D_0081078F, 1);
                (void)a21w_c_001BA1A0(o, self + 0x1F0, A21W_SCRIPT_82CBD0);
                a21w_w8(o, self + 5, 2);
                (void)a21w_c_001D2830(o, 0x24, 1);
            }
        } else if (step == 2) {
            r = 0;
            (void)a21w_c_001BA1F0(o, self, &r);
            if (r != 0) {
                const int32_t end = r;
                (void)a21w_c_001AEDE0(o, 4, 0);
                a21w_w8(o, D_70003B93, end == 3 ? 2 : 1);
                (void)a21w_c_001FBC50(o);
                a21w_w32(o, D_00810858, F_100);
                a21w_w32(o, D_008104D0, F_100);
                a21w_w32(o, D_0081085C, F_ZERO);
                a21w_w32(o, D_008104D8, F_ZERO);
                a21w_w16(o, self + 0x2E, 0xFFFF);
                a21w_w8(o, self + 4, 3);
                a21w_w8(o, D_0081080F, 0xFF);
                a21w_w8(o, D_008106B8, 1);
            }
        }
        if (a21w_u8(o, D_0081080F) != 0) {
            a21w_w32(o, self + 0xC0, F_1_308997);
            (void)a21w_c_001C6380(o, self);
        }
        (void)a21w_c_001B1B70(o, self);
        (void)a21w_method(o, self);
    } else if (state == 2 || state == 3) {
        (void)a21w_c_001AFC10(o, self);
    }
}

int em_area21_owners_00827040(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_827040(&o, a0);
    return a21w_end(&o);
}

/* 0x827250 (decomp func_overlay_AREA21_00827210): script 0x82CBD0's op09
 * callback: D_00810858 = D_008104D0 = 100, D_0081085C = D_008104D8 = 0,
 * 0015C700 / 0015C750 / 001B81D0 on D_008102B0, D_00810856 = 0xFF;
 * returns 1. */
int em_area21_owners_00827250(const EmArea21OwnersHooks *h, int32_t *result, EmArea21OwnersFault *fault)
{
    A21W o;
    if (!result || a21w_begin(&o, h, fault)) return -1;
    a21w_w32(&o, D_00810858, F_100);
    a21w_w32(&o, D_008104D0, F_100);
    a21w_w32(&o, D_0081085C, F_ZERO);
    a21w_w32(&o, D_008104D8, F_ZERO);
    (void)a21w_c_0015C700(&o, D_008102B0);
    (void)a21w_c_0015C750(&o, D_008102B0);
    (void)a21w_c_001B81D0(&o, D_008102B0);
    a21w_w8(&o, D_00810856, 0xFF);
    if (a21w_end(&o)) return -1;
    *result = 1;
    return 0;
}

/* ----------------------------------------------------------------------
 * 0x827CD0 (decomp func_overlay_AREA21_00827C90) — [58]. State 0, unless
 * 001B0FD0(self): 001C6380, +5 = 0 (the state stays 0). State 1: +5 = 2
 * once D_0081078E is 0xFF; step 0: at D_0081080E == 3, +0x28 = 0 and step
 * 1; step 1: +0x28 += 1, step 2 once it is at least 830; step 2:
 * 0019C6F0(n, 1) for n = 0x10, 0x13, 0x14, 0x16 .. 0x1B, 001B1B70 and the
 * method. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a21w_827CD0(A21W *o, uint32_t self)
{
    static const int32_t ids[] = {0x10, 0x13, 0x14, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B};
    const uint32_t state = a21w_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a21w_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a21w_c_001C6380(o, self);
        a21w_w8(o, self + 5, 0);
    } else if (state == 1) {
        if (a21w_u8(o, D_0081078E) == 0xFF) a21w_w8(o, self + 5, 2);
        const uint32_t step = a21w_u8(o, self + 5);
        if (step == 0) {
            if (a21w_u8(o, D_0081080E) == 3) {
                a21w_w16(o, self + 0x28, 0);
                a21w_w8(o, self + 5, a21w_u8(o, self + 5) + 1);
            }
        } else if (step == 1) {
            a21w_w16(o, self + 0x28, (uint32_t)(a21w_s16(o, self + 0x28) + 1));
            if (a21w_s16(o, self + 0x28) >= 0x33E) a21w_w8(o, self + 5, a21w_u8(o, self + 5) + 1);
        } else if (step == 2) {
            for (unsigned k = 0; k < sizeof ids / sizeof ids[0]; ++k) (void)a21w_c_0019C6F0(o, ids[k], 1);
            (void)a21w_c_001B1B70(o, self);
            (void)a21w_method(o, self);
        }
    } else if (state == 2 || state == 3) {
        (void)a21w_c_001AFC10(o, self);
    }
}

int em_area21_owners_00827CD0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault)
{
    A21W o;
    if (a21w_begin(&o, h, fault)) return -1;
    a21w_827CD0(&o, a0);
    return a21w_end(&o);
}
