/* AREA00 overlay owners (see em_area00_overlay.h, docs/AREA00_OVERLAY_PORT.md).
 *
 * Each function below is a translation of the original AREA00 overlay code
 * at the named runtime address. All thirteen follow the decomp's
 * byte-identical C (src/overlays/AREA00/<name>.c, link name = runtime -
 * 0x40). Calls, their arguments and the memory accesses between each two
 * calls follow the original: the test compares memory at every call entry
 * and after the last store, and the memory accesses between calls one for
 * one, in order, by address and size (docs/AREA00_OVERLAY_PORT.md section
 * 3). Where the original loads the operands of one expression in a set
 * order, the translation loads them in separate statements in that order,
 * because C leaves the order of evaluation of arguments and operands open.
 * Float arithmetic is the EE model (em_ee_float.h) on bit patterns; the
 * operand order of each add/sub/mul/div/compare is the original's.
 */
#include "em_area00_overlay_internal.h"

/* Original globals the functions name. */
#define D_00246F20 0x00246F20u /* a boot script (0x8263C0 starts it) */
#define D_00246FB4 0x00246FB4u /* word stored before that script starts */
#define D_00275B40 0x00275B40u /* current bone table (actor + 0x110 while a behaviour runs) */
#define D_002758A8 0x002758A8u
#define D_002758B0 0x002758B0u
#define D_002758B8 0x002758B8u
#define D_008102B0 0x008102B0u /* player actor */
#define D_00810354 0x00810354u /* float */
#define D_008104E6 0x008104E6u
#define D_00810701 0x00810701u /* area sub-state */
#define D_00810702 0x00810702u /* the player's spawn entry */
#define D_0081075A 0x0081075Au /* AREA00 flag 2 */
#define D_0081075B 0x0081075Bu /* AREA00 flag 3 */
#define D_0081075D 0x0081075Du /* AREA00 flag 5 */
#define D_0081075E 0x0081075Eu /* AREA00 flag 6 */
#define D_008107DB 0x008107DBu /* AREA00 counter 3 */
#define D_008107DC 0x008107DCu /* AREA00 counter 4 (ferry bits) */

/* Scratchpad. */
#define S_70003000 0x70003000u /* 4x4 matrix */
#define S_70003400 0x70003400u /* 4x4 matrix */
#define S_70003600 0x70003600u /* three colour words (0x823820) */
#define S_70003604 0x70003604u
#define S_70003608 0x70003608u
#define S_700036A0 0x700036A0u /* 4x4 matrix */
#define S_700036D0 0x700036D0u /* vector */
#define S_700038A0 0x700038A0u /* vector */
#define S_700038A4 0x700038A4u
#define S_700038A8 0x700038A8u
#define S_700038AC 0x700038ACu
#define S_700038B0 0x700038B0u /* four words */
#define S_700038B4 0x700038B4u
#define S_700038B8 0x700038B8u
#define S_700038BC 0x700038BCu
#define S_70003B84 0x70003B84u /* halfword frame counter */
#define S_70003B8D 0x70003B8Du

/* Overlay data (runtime addresses): script entry records, tables. */
#define A00_SCRIPT_8284E0 0x008284E0u
#define A00_SCRIPT_8286E0 0x008286E0u
#define A00_TABLE_8287E0 0x008287E0u
#define A00_TABLE_828870 0x00828870u
#define A00_SCRIPT_828D60 0x00828D60u
#define A00_SCRIPT_8294E0 0x008294E0u
#define A00_SCRIPT_8299A0 0x008299A0u
#define A00_SCRIPT_8299E0 0x008299E0u
#define A00_SCRIPT_829BE0 0x00829BE0u
#define A00_SCRIPT_82A0E0 0x0082A0E0u
#define A00_DATA_82A520 0x0082A520u
#define A00_SCRIPT_82A540 0x0082A540u
#define A00_DATA_82A700 0x0082A700u
#define A00_MATRIX_82A710 0x0082A710u
#define A00_SCRIPT_82A720 0x0082A720u
#define A00_SCRIPT_82B790 0x0082B790u
#define A00_SCRIPT_82B990 0x0082B990u
#define A00_RECORD_827A30 0x00827A30u
#define A00_POINT_82CCE0 0x0082CCE0u /* three floats 0x8263C0 rewrites every frame */
#define A00_POINTS_82D440 0x0082D440u /* four 16-byte vectors */

/* Float constants (bit patterns the original materialises). */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_2_31 0x4F000000u     /* 2147483648.0 */
#define F_0_3 0x3E99999Au      /* 0.3 */
#define F_255 0x437F0000u
#define F_192 0x43400000u
#define F_3 0x40400000u
#define F_2 0x40000000u
#define F_0_09 0x3DB851ECu
#define F_0_01 0x3C23D70Au
#define F_1_5 0x3FC00000u
#define F_200 0x43480000u
#define F_4096 0x45800000u
#define F_MINUS_0_4 0xBECCCCCDu
#define F_155 0x431B0000u
#define F_MINUS_48 0xC2400000u
#define F_MINUS_1590 0xC4C6C000u
#define F_25 0x41C80000u
#define F_85 0x42AA0000u
#define F_5 0x40A00000u
#define F_MINUS_7_55 0xC0F1999Au
#define F_MINUS_5_69 0xC0B6147Bu
#define F_MINUS_1391_65 0xC4ADF4CDu
#define F_MINUS_16_1 0xC180CCCCu   /* -16.099998 */
#define F_12 0x41400000u
#define F_MINUS_40_4 0xC22199A0u   /* -40.400024 */
#define F_MINUS_11_017 0xC13045A2u
#define F_26_99 0x41D7EB85u
#define F_MINUS_41 0xC2240000u
#define F_MINUS_36_954 0xC213D0E5u

static inline float fb(uint32_t bits) { return em_ee_float(bits); }

static int a00_begin(A00Ovl *o, const EmArea00OvlHooks *h, EmArea00OvlFault *fault)
{
    if (!h || !fault) return -1;
    a00_open(o, h, fault);
    return a00_failed(o) ? -1 : 0;
}

static int a00_end(const A00Ovl *o) { return a00_failed(o) ? -1 : 0; }

/* ======================================================================
 * 0x823580 (overlay_AREA00_func_00823540) — the shaft door [52].
 * +0x04: 0 kickoff (001BBDA0; then +0x00 = 1, or, while D_0081075A != 0xFF,
 * +0x00 = 2, +0x05 = 5 and script 0x8284E0); 1 runs the step +0x05 (0..7,
 * other values run only the tail), then 001BC300; 2/3 free the node
 * (001AFC10); other values: nothing.
 * ====================================================================== */
static void a00_shaft_door(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t block = self + 0x1F0;
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a00_c_001BBDA0(o, self);
        if (a00_u8(o, D_0081075A) != 0xFF) {
            a00_w8(o, self + 0, 2);
            a00_w8(o, self + 5, 5);
            (void)a00_c_001BA1A0(o, block, A00_SCRIPT_8284E0);
        } else {
            a00_w8(o, self + 0, 1);
        }
        return;
    }
    if (state != 1) return;

    uint32_t step = a00_u8(o, self + 5);
    switch (step) {
    case 0:
        if (a00_u8(o, D_0081075E) == 0xFF) {
            a00_w8(o, self + 5, 1);
            r = 0;
            (void)a00_c_0019C6F0(o, 2, 0, &r);
            r = 0;
            (void)a00_c_0019C6F0(o, 0, 1, &r);
        } else if (a00_u8(o, D_0081075D) == 0xFF) {
            a00_w8(o, self + 5, 6);
            r = 0;
            (void)a00_c_0019C6F0(o, 2, 0, &r);
        } else {
            r = 0;
            (void)a00_c_001BBE40(o, self, block, 0, &r);
            if (r != 0) a00_w8(o, self + 5, 2);
        }
        break;
    case 1:
        r = 0;
        (void)a00_c_001BBE40(o, self, block, 0, &r);
        if (r != 0) a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        break;
    case 2:
        r = 0;
        (void)a00_c_001BC0E0(o, self, block, &r);
        if (r != 0) a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        break;
    case 3:
        (void)a00_c_001BC240(o, self, block);
        a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        break;
    case 4:
        r = 0;
        (void)a00_c_001BC290(o, self, block, &r);
        if (r != 0) a00_w8(o, self + 5, 0);
        break;
    case 5: /* the arrival script ended: back to the plain door */
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a00_w8(o, self + 0, 1);
            a00_w8(o, self + 0xB, 0);
            a00_w8(o, self + 5, 0);
            (void)a00_c_001C67E0(o, self, 0, fb(F_ZERO), fb(F_ZERO));
        }
        break;
    case 6: /* D_0081075D set: a Use starts the progression script */
        if (a00_u8(o, self + 0xB) & 4) {
            a00_w8(o, self + 5, step + 1); /* the step value read at the dispatch */
            (void)a00_c_001BA1A0(o, block, A00_SCRIPT_8286E0);
        }
        break;
    case 7: /* at its end: the area change */
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            (void)a00_c_001B0C60(o, 1, 0xFF, 0);
            a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        }
        break;
    default:
        break;
    }
    (void)a00_c_001BC300(o, self);
}

int em_area00_ovl_00823580(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_shaft_door(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * 0x823820 (func_overlay_AREA00_008237E0) — eight puffs in block +0x1F0
 * (+0x00 mode, +0x04 count, +0x08 eight timers, +0x28 eight {a, b, c, d}
 * floats). +0x04: 0 seeds them and falls into 1; 1 draws; 2/3 free.
 * ====================================================================== */
static uint32_t a00_rand_fraction(A00Ovl *o)
{
    int32_t r = 0;
    (void)a00_c_00122BB8(o, &r);
    return em_ee_div_bits(em_ee_cvt_s_w_bits((uint32_t)r), F_2_31);
}

static void a00_puffs(A00Ovl *o, uint32_t self, uint32_t sp)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t fx = self + 0x1F0;
    uint32_t packet = sp - 0xE0 + 0x80; /* the original's 0x60-byte local */
    int32_t r, i;

    if (state == 3 || state == 2) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        for (i = 0; i < 8; i++) {
            uint32_t p = fx + 0x28 + 0x10 * (uint32_t)i;
            r = 0;
            (void)a00_c_00122BB8(o, &r);
            a00_w32(o, fx + 8 + 4 * (uint32_t)i, (uint32_t)(r % 120));
            a00_w32(o, p + 0, F_ZERO);
            a00_w32(o, p + 4, F_ZERO);
            a00_w32(o, p + 8, a00_rand_fraction(o));
            a00_w32(o, p + 12, a00_rand_fraction(o));
        }
        a00_w32(o, fx + 0, 0xFFFFFFFFu);
        a00_w32(o, fx + 4, 0);
        a00_w8(o, self + 0xC, 0);
        a00_w8(o, self + 9, 0);
        a00_w8(o, self + 4, 1);
        a00_w8(o, self + 5, 0);
    } else if (state != 1) {
        return;
    }

    (void)a00_c_001029C0(o, S_700036A0);
    (void)a00_c_00102948(o, S_700036D0, self + 0xB0);
    int32_t handle = 0;
    (void)a00_c_001CCF70(o, S_700036D0, &handle);
    for (i = 0; i < 8; i++) {
        uint32_t timer = fx + 8 + 4 * (uint32_t)i;
        uint32_t p = fx + 0x28 + 0x10 * (uint32_t)i;
        a00_w32(o, timer, a00_u32(o, timer) - 1);
        if (a00_s32(o, timer) >= 0) continue;

        uint32_t a = a00_u32(o, p + 0);
        if (em_ee_c_lt_bits(a, F_0_3)) {
            uint32_t t = em_ee_div_bits(em_ee_sub_bits(F_0_3, a), F_0_3);
            r = 0;
            (void)a00_c_001281C0(o, fb(em_ee_mul_bits(F_255, t)), &r);
            a00_w32(o, S_70003600, (uint32_t)r);
            t = em_ee_mul_bits(F_192, t);
            r = 0;
            (void)a00_c_001281C0(o, fb(t), &r);
            a00_w32(o, S_70003604, (uint32_t)r << 8);
            r = 0;
            (void)a00_c_001281C0(o, fb(t), &r);
            a00_w32(o, S_70003608, (uint32_t)r << 16);
            uint32_t a2 = a00_u32(o, p + 0);
            uint32_t c8 = a00_u32(o, S_70003608);
            uint32_t q = em_ee_div_bits(em_ee_mul_bits(F_3, a2), F_0_3);
            uint32_t c4 = a00_u32(o, S_70003604);
            uint32_t c0 = a00_u32(o, S_70003600);
            uint32_t w = em_ee_add_bits(F_ONE, q);
            r = 0;
            (void)a00_c_001CD520(o, 0, 2, S_700036D0, UINT64_C(0x20045B2599421E98),
                                 c0 | ((c8 | 0x80000000u) | c4), fb(w), fb(w), fb(F_2), &r);
        }
        uint32_t fa = a00_u32(o, p + 0);
        uint32_t fc = a00_u32(o, p + 8);
        (void)a00_c_001CFA60(o, packet, S_700036A0, fb(fa), fb(fc));
        (void)a00_c_001CFBE0(o, handle, 2, A00_TABLE_8287E0, packet, 0);
        uint32_t na = em_ee_add_bits(a00_u32(o, p + 0), F_0_09);
        a00_w32(o, p + 0, na);
        if (!em_ee_c_le_bits(na, F_2)) a00_w32(o, p + 0, F_2);

        uint32_t fbb = a00_u32(o, p + 4);
        uint32_t fd = a00_u32(o, p + 12);
        (void)a00_c_001CFA60(o, packet, S_700036A0, fb(fbb), fb(fd));
        (void)a00_c_001CFBE0(o, handle, 2, A00_TABLE_828870, packet, 0);
        uint32_t nb = em_ee_add_bits(a00_u32(o, p + 4), F_0_01);
        a00_w32(o, p + 4, nb);
        if (!em_ee_c_le_bits(nb, F_1_5)) {
            r = 0;
            (void)a00_c_00122BB8(o, &r);
            a00_w32(o, timer, (uint32_t)(r % 60 + 40));
            a00_w32(o, p + 0, F_ZERO);
            a00_w32(o, p + 4, F_ZERO);
            a00_w32(o, p + 8, a00_rand_fraction(o));
            a00_w32(o, p + 12, a00_rand_fraction(o));
        }
    }
    (void)a00_c_001FC3C0(o, self, fx, 0x41C, fb(F_200), fb(F_4096));
}

int em_area00_ovl_00823820(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_puffs(&o, self, sp);
    return a00_end(&o);
}

/* ======================================================================
 * 0x824EA0 (func_overlay_AREA00_00824E60) — the arrival owner.
 * +0x04 0: +0x05 0 waits (to state 3 when 001BA1C0(self, 2) or D_00810701
 * is set) for 001B10B0(self, 0xF, 0x11) == 0, then seeds block +0x1F0 and
 * the +0xD0 matrix; +0x05 1 starts script 0x828D60 once 00129780(self,
 * block, 3) is non-zero. 1: the scripted walk (counter 0x70003B84). 2:
 * 001C4760(3, 1), then 3. 3: free.
 * ====================================================================== */
static void a00_arrival(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t sub = self + 0x1F0;
    int32_t r;

    if (state == 3) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 2) {
        r = 0;
        (void)a00_c_001C4760(o, 3, 1, &r);
        a00_w8(o, self + 4, a00_u8(o, self + 4) + 1);
        return;
    }
    if (state == 0) {
        uint32_t step = a00_u8(o, self + 5);
        if (step == 1) {
            r = 0;
            (void)a00_c_00129780(o, self, sub, 3, &r);
            if (r != 0) (void)a00_c_001BA1A0(o, sub, A00_SCRIPT_828D60);
            return;
        }
        if (step != 0) return;
        r = 0;
        (void)a00_c_001BA1C0(o, self, 2, &r);
        if (r != 0) {
            a00_w8(o, self + 4, 3);
            return;
        }
        if (a00_u8(o, D_00810701) != 0) {
            a00_w8(o, self + 4, 3);
            return;
        }
        r = 0;
        (void)a00_c_001B10B0(o, self, 0xF, 0x11, &r);
        if (r != 0) return;
        (void)a00_c_001C63E0(o, self, 1);
        a00_w32(o, sub + 0xEC, F_ONE);
        a00_w32(o, sub + 0xD8, F_ZERO);
        a00_w32(o, sub + 0xE4, F_ZERO);
        a00_w32(o, sub + 0xDC, F_ZERO);
        a00_w32(o, sub + 0x60, F_ZERO);
        a00_w32(o, sub + 0x64, F_MINUS_0_4);
        a00_w32(o, sub + 0x68, F_ZERO);
        a00_w32(o, sub + 0x6C, F_ONE);
        a00_w32(o, sub + 0x70, F_ZERO);
        a00_w32(o, sub + 0x74, F_ZERO);
        a00_w32(o, sub + 0x78, F_ONE);
        a00_w32(o, sub + 0x7C, F_ONE);
        a00_w32(o, sub + 0x80, F_ZERO);
        a00_w32(o, sub + 0x84, F_ONE);
        a00_w32(o, sub + 0x88, F_ZERO);
        a00_w32(o, sub + 0x8C, F_ONE);
        (void)a00_c_001029C0(o, self + 0xD0);
        a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        return;
    }
    if (state != 1) return;

    (void)a00_c_001029C0(o, S_70003000);
    uint32_t step = a00_u8(o, self + 5);
    if (step == 2) {
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a00_w8(o, self + 4, a00_u8(o, self + 4) + 1);
            a00_w8(o, self + 5, 0);
        }
        if (a00_u8(o, S_70003B8D) != 0 && a00_u16(o, S_70003B84) == 0x4B0)
            (void)a00_c_001B1E20(o, 6, 0);
        return;
    }
    if (step == 0) {
        a00_w8(o, self + 5, step + 1);
        a00_w16(o, S_70003B84, 0);
    } else if (step != 1) {
        return;
    }
    r = 0;
    (void)a00_c_001C2770(o, self, sub, 0, &r);
    if (r == 0) (void)a00_c_001C3D60(o, self, sub);
    uint32_t dt = a00_u32(o, sub + 0xEC);
    r = 0;
    (void)a00_c_001C64F0(o, self, fb(dt), &r);
    (void)a00_c_00102958(o, S_70003400, S_70003000);
    (void)a00_c_001C69A0(o, self);
    a00_callback(o, self);
    if ((int32_t)a00_u16(o, S_70003B84) >= 0x14A) a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
    r = 0;
    (void)a00_c_001BA1F0(o, self, &r);
    if (r != 0) {
        a00_w8(o, self + 4, a00_u8(o, self + 4) + 1);
        a00_w8(o, self + 5, 0);
    }
}

int em_area00_ovl_00824EA0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_arrival(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * 0x825170 (func_overlay_AREA00_00825130) — door [51].
 * +0x04: 0 kickoff (001BBDA0, script 0x8294E0 unless D_0081075B == 0xFF,
 * +0x00 = 1); 1 runs the step +0x05 (0..6, other values only the tail),
 * then 001BC300; 2/3 free; other values: nothing.
 * ====================================================================== */
static void a00_door51(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t block = self + 0x1F0;
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a00_c_001BBDA0(o, self);
        if (a00_u8(o, D_0081075B) != 0xFF) (void)a00_c_001BA1A0(o, block, A00_SCRIPT_8294E0);
        a00_w8(o, self + 0, 1);
        return;
    }
    if (state != 1) return;

    switch (a00_u8(o, self + 5)) {
    case 0:
        if (a00_u8(o, D_00810702) == 5) { /* the player came in at entry 5 */
            a00_w8(o, D_0081075B, 0xFF);
            r = 0;
            (void)a00_c_001BBE40(o, self, block, 0, &r);
            if (r != 0) {
                a00_w8(o, D_008107DB, 0xFF);
                a00_w8(o, self + 5, 3);
            }
        } else if (a00_u8(o, D_0081075B) != 0xFF) {
            if (a00_u8(o, self + 0xB) & 4) a00_w8(o, self + 5, 6);
        } else {
            r = 0;
            (void)a00_c_001BBE40(o, self, block, 0, &r);
            if (r != 0) a00_w8(o, self + 5, 3);
        }
        break;
    case 1:
        r = 0;
        (void)a00_c_001BC0E0(o, self, block, &r);
        if (r != 0) {
            (void)a00_c_001BA1A0(o, block, A00_SCRIPT_8299A0);
            a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        }
        break;
    case 2:
        r = 0;
        (void)a00_c_001BC0E0(o, self, block, &r);
        if (r != 0) {
            a00_w8(o, self + 0xB, 0);
            a00_w8(o, self + 5, 0);
        }
        break;
    case 3:
        r = 0;
        (void)a00_c_001BC0E0(o, self, block, &r);
        if (r != 0) a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        break;
    case 4:
        (void)a00_c_001BC240(o, self, block);
        a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        break;
    case 5:
        r = 0;
        (void)a00_c_001BC290(o, self, block, &r);
        if (r != 0) a00_w8(o, self + 5, 0);
        break;
    case 6: /* the locked script ended */
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            if (a00_u8(o, D_0081075B) == 0) {
                a00_w8(o, D_0081075B, 1);
                r = 0;
                (void)a00_c_001C4760(o, 4, 1, &r);
            }
            a00_w8(o, self + 5, 0);
            a00_w8(o, self + 0xB, 0);
            (void)a00_c_001BA1A0(o, block, A00_SCRIPT_8294E0);
        }
        break;
    default:
        break;
    }
    (void)a00_c_001BC300(o, self);
}

int em_area00_ovl_00825170(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_door51(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * 0x8253E0 (func_overlay_AREA00_008253A0) — script op09 callback: steps
 * byte +0x05 of the block in a1 (reset when +0x04 is 0), plays sound
 * 0x19A every 16 steps, returns 1 once +0x05 exceeds 48. Other +0x04
 * values return 0 at once.
 * ====================================================================== */
static int32_t a00_op09_8253e0(A00Ovl *o, uint32_t s)
{
    uint32_t phase = a00_u8(o, s + 4);
    if (phase == 0) {
        a00_w8(o, s + 4, phase + 1);
        a00_w8(o, s + 5, 0);
    } else if (phase != 1) {
        return 0;
    }
    a00_w8(o, s + 5, a00_u8(o, s + 5) + 1);
    if ((a00_u8(o, s + 5) & 0xF) == 0) (void)a00_c_001FB9F0(o, 0x19A, 0x1000, 0x1000, 0x1000);
    return a00_u8(o, s + 5) > 0x30 ? 1 : 0;
}

int em_area00_ovl_008253E0(const EmArea00OvlHooks *h, uint32_t a0, uint32_t block, uint32_t a2,
                           int32_t *result, EmArea00OvlFault *fault)
{
    A00Ovl o;
    (void)a0;
    (void)a2;
    if (!result || a00_begin(&o, h, fault)) return -1;
    int32_t value = a00_op09_8253e0(&o, block);
    if (a00_failed(&o)) return -1;
    *result = value;
    return 0;
}

/* ======================================================================
 * 0x825480 (func_overlay_AREA00_00825440) — the cage-room terminal [40].
 * +0x04: 0 set-up once 001B0FD0 returns 0; 1: a Use (+0x0B bit 2) starts
 * script 0x8299E0, and at its end D_008107DC |= 1; then 001B17A0 and the
 * +0x4C callback; 2/3 free.
 * ====================================================================== */
static void a00_terminal(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t block = self + 0x1F0;
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        a00_w8(o, self + 0, 1);
        a00_w8(o, self + 8, 1);
        a00_w32(o, self + 0x30, A00_DATA_82A520);
        (void)a00_c_001C6380(o, self);
        a00_w32(o, S_700038A0, F_ZERO);
        a00_w32(o, S_700038A4, F_ONE);
        a00_w32(o, S_700038A8, F_ZERO);
        a00_w32(o, S_700038AC, F_ONE);
        (void)a00_c_001C5570(o, self, S_700038A0, 9, 0);
        return;
    }
    if (state != 1) return;

    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        if (a00_u8(o, self + 0xB) & 4) {
            a00_w8(o, self + 5, step + 1);
            (void)a00_c_001BA1A0(o, block, A00_SCRIPT_8299E0);
        }
    } else if (step == 1) {
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a00_w8(o, D_008107DC, a00_u8(o, D_008107DC) | 1);
            a00_w8(o, self + 0xB, 0);
            a00_w8(o, self + 5, 0);
        }
    }
    r = 0;
    (void)a00_c_001B17A0(o, self, &r);
    a00_callback(o, self);
}

int em_area00_ovl_00825480(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_terminal(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * 0x825600 (func_overlay_AREA00_008255C0) — the ferry group, by kind byte
 * +0x0D: 2 the ferry, 0x15 a rider that keeps its offset from the node at
 * +0x18, 7 the node that writes 85 + (y - 25) of the node two links up,
 * other kinds follow the node at +0x18. +0x04: 0 set-up once 001B0FD0
 * returns 0; 1 runs; 2/3 free.
 * ====================================================================== */
static void a00_ferry(A00Ovl *o, uint32_t self, uint32_t sp)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t block = self + 0x1F0;
    uint32_t vector = sp - 0x50 + 0x40; /* the original's 16-byte local */
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        uint32_t kind = a00_u8(o, self + 0xD);
        if (kind == 2) {
            (void)a00_c_00102948(o, self + 0xA0, self + 0xB0);
            if (a00_u8(o, D_008107DC) & 2) { /* the ferry was left at the east end */
                a00_w32(o, self + 0xB0, F_155);
                a00_w32(o, self + 0xB4, F_MINUS_48);
                a00_w32(o, self + 0xB8, F_MINUS_1590);
                a00_w32(o, self + 0xBC, F_ONE);
            }
        } else if (kind == 0x15) {
            uint32_t other = a00_u32(o, self + 0x18);
            (void)a00_c_001028D0(o, self + 0xA0, self + 0xB0, other + 0xA0);
        }
        (void)a00_c_001C6380(o, self);
        (void)a00_c_001A2370(o, self, self + 0xD0);
        return;
    }
    if (state != 1) return;

    uint32_t kind = a00_u8(o, self + 0xD);
    if (kind == 2) {
        uint32_t step = a00_u8(o, self + 5);
        if (step == 1) {
            r = 0;
            (void)a00_c_001BA1F0(o, self, &r);
            if (r != 0) a00_w8(o, self + 5, 0);
            (void)a00_c_001A2370(o, self, self + 0xD0);
        } else if (step == 0) {
            uint32_t flags = a00_u8(o, D_008107DC);
            if (flags & 1) {
                a00_w8(o, self + 5, step + 1);
                (void)a00_c_001BA1A0(o, block, (flags & 2) ? A00_SCRIPT_82A0E0 : A00_SCRIPT_829BE0);
            }
        }
        (void)a00_c_001C6380(o, self);
        a00_w8(o, self + 1, 1);
        (void)a00_c_001B1B70(o, self);
        a00_callback(o, self);
        for (uint32_t i = 0; i < 4; i++) {
            (void)a00_c_001026A0(o, vector, self + 0xD0, A00_POINTS_82D440 + 0x10 * i);
            (void)a00_c_001F5940(o, 3, vector, 0);
        }
    } else if (kind == 7) {
        uint32_t link = a00_u32(o, self + 0x18);
        uint32_t other = a00_u32(o, link + 0x18);
        uint32_t d = em_ee_sub_bits(a00_u32(o, other + 0xB4), F_25);
        if (!em_ee_c_eq_bits(F_85, d)) {
            uint32_t bones = a00_u32(o, D_00275B40);
            uint32_t sum = em_ee_add_bits(F_85, d);
            uint32_t target = a00_u32(o, bones + 4);
            a00_w32(o, target + 0x80, sum);
        }
        (void)a00_c_001C6380(o, self);
        a00_w8(o, self + 1, 1);
        (void)a00_c_001B1B70(o, self);
        (void)a00_c_001A2370(o, self, self + 0xD0);
        a00_callback(o, self);
    } else {
        uint32_t other = a00_u32(o, self + 0x18);
        (void)a00_c_001028B8(o, self + 0xB0, other + 0xB0, self + 0xA0);
        a00_w32(o, self + 0xBC, F_ONE);
        (void)a00_c_001C6380(o, self);
        if (a00_u8(o, other + 1) != 0) {
            (void)a00_c_001A2370(o, self, self + 0xD0);
            a00_w8(o, self + 1, 1);
            (void)a00_c_001B1B70(o, self);
        }
        a00_callback(o, self);
    }
}

int em_area00_ovl_00825600(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_ferry(&o, self, sp);
    return a00_end(&o);
}

/* ======================================================================
 * 0x825920 (func_overlay_AREA00_008258E0) — [43] (header +0x02 bit 7) and
 * two group nodes. +0x04: 0 waits on 001B0F60(self, 6 or 7); 1 runs the
 * step +0x05 and, while D_00810702 is 5 or 6, animates; 2: nothing; 3 frees.
 * ====================================================================== */
static void a00_store_effect_point(A00Ovl *o)
{
    a00_w32(o, S_700038A0, F_MINUS_7_55);
    a00_w32(o, S_700038A4, F_MINUS_5_69);
    a00_w32(o, S_700038A8, F_MINUS_1391_65);
    a00_w32(o, S_700038AC, F_ONE);
    (void)a00_c_001EFD20(o, 0, S_700038A0);
}

static void a00_switch43(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t talk = self + 0x1F0;
    int32_t r;

    if (state == 3) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 2) return;
    if (state == 0) {
        if (a00_u8(o, D_0081075D) == 0xFF) {
            r = 0;
            (void)a00_c_001B0F60(o, self, 6, &r);
            if (r != 0) return;
            a00_w8(o, self + 5, 2);
            if ((a00_u8(o, self + 2) & 0x80) && a00_u8(o, D_0081075E) != 0xFF) a00_store_effect_point(o);
        } else if (a00_u8(o, self + 2) & 0x80) {
            r = 0;
            (void)a00_c_001B0F60(o, self, 7, &r);
            if (r != 0) return;
            (void)a00_c_001BA1A0(o, talk, A00_SCRIPT_82A540);
            a00_w32(o, self + 0x30, A00_DATA_82A700);
            a00_w8(o, self + 0, 1);
        } else {
            r = 0;
            (void)a00_c_001B0F60(o, self, 6, &r);
            if (r != 0) return;
            a00_w8(o, self + 5, 2);
        }
        return;
    }
    if (state != 1) return;

    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        a00_w8(o, self + 0, a00_u8(o, D_008104E6) != 0 ? 2 : 1);
        if (a00_u8(o, self + 0xB) & 4) a00_w8(o, self + 5, 1);
    } else if (step == 1) {
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            r = 0;
            (void)a00_c_001C64F0(o, self, fb(F_ONE), &r);
            a00_w8(o, self + 5, 2);
            uint32_t y = a00_u32(o, D_00810354);
            uint32_t x = a00_u32(o, self + 0xB0);
            a00_w32(o, S_700038A0, x);
            a00_w32(o, S_700038A4, y);
            uint32_t z = em_ee_sub_bits(a00_u32(o, self + 0xB8), F_5);
            a00_w32(o, S_700038A8, z);
            a00_w32(o, S_700038AC, F_ONE);
            (void)a00_c_00182F90(o, D_008102B0, S_700038A0);
            a00_store_effect_point(o);
        }
    }
    uint32_t entry = a00_u8(o, D_00810702);
    if (entry != 5 && entry != 6) return;
    (void)a00_c_001C68C0(o, self);
    r = 0;
    (void)a00_c_001B17A0(o, self, &r);
    a00_callback(o, self);
    if (!(a00_u8(o, self + 2) & 0x80)) return;
    (void)a00_c_001026A0(o, S_700038A0, self + 0xD0, A00_MATRIX_82A710);
    if (a00_u8(o, D_0081075D) == 0xFF) return;
    a00_w32(o, S_700038B0, 0);
    a00_w32(o, S_700038B4, 0x80);
    a00_w32(o, S_700038B8, 0);
    a00_w32(o, S_700038BC, 0x80);
    (void)a00_c_001F4E20(o, S_700038A0, S_700038B0, fb(F_2));
}

int em_area00_ovl_00825920(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_switch43(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * 0x825C80 / 0x8261E0 / 0x8262D0 (func_overlay_AREA00_00825C40 / 008261A0 /
 * 00826290) — one code, three data sets: +0x04 0 sets +0x30 to a boot word
 * address, +0x08 = 3, +0x00 = 1, +0x04 = 1; 1: a Use (+0x0B bit 2) starts
 * the script, its end clears +0x0B and +0x05, then 001B17A0; 2: nothing;
 * 3 frees.
 * ====================================================================== */
static void a00_examine(A00Ovl *o, uint32_t self, uint32_t word, uint32_t script)
{
    uint32_t state = a00_u8(o, self + 4);
    int32_t r;

    if (state == 3) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 2) return;
    if (state == 0) {
        a00_w32(o, self + 0x30, word);
        a00_w8(o, self + 8, 3);
        a00_w8(o, self + 0, 1);
        a00_w8(o, self + 4, 1);
        return;
    }
    if (state != 1) return;
    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        if (a00_u8(o, self + 0xB) & 4) {
            (void)a00_c_001BA1A0(o, self + 0x1F0, script);
            a00_w8(o, self + 5, 1);
        }
    } else if (step == 1) {
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a00_w8(o, self + 0xB, 0);
            a00_w8(o, self + 5, 0);
        }
    }
    r = 0;
    (void)a00_c_001B17A0(o, self, &r);
}

int em_area00_ovl_00825C80(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_examine(&o, self, D_002758A8, A00_SCRIPT_82A720);
    return a00_end(&o);
}

int em_area00_ovl_008261E0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_examine(&o, self, D_002758B0, A00_SCRIPT_82B790);
    return a00_end(&o);
}

int em_area00_ovl_008262D0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_examine(&o, self, D_002758B8, A00_SCRIPT_82B990);
    return a00_end(&o);
}

/* ======================================================================
 * 0x8263C0 (func_overlay_AREA00_00826380) — the ferry cab [47]. +0x04: 0
 * set-up once 001B0FD0 returns 0 (spawns 001B6660(0x827A30) offset by its
 * own position); 1: a Use starts boot script 0x246F20, then every frame the
 * point 0x82CCE0 follows the cab, the cab follows the node at +0x1C (y + 1),
 * and two points are emitted (001F5940(9, ...)); 2/3 free.
 * ====================================================================== */
static void a00_cab(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t block = self + 0x1F0;
    int32_t r;

    if (state == 3 || state == 2) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        uint32_t e = 0;
        (void)a00_c_001B6660(o, A00_RECORD_827A30, &e);
        if (e != 0) {
            (void)a00_c_001028D0(o, e + 0xA0, e + 0xB0, self + 0xB0);
            a00_w32(o, e + 0x20, a00_u32(o, self + 0x14));
        }
        a00_w8(o, self + 0, 1);
        a00_w8(o, self + 8, 1);
        a00_w32(o, self + 0x30, A00_POINT_82CCE0);
        (void)a00_c_001C6380(o, self);
        return;
    }
    if (state != 1) return;

    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        if (a00_u8(o, self + 0xB) & 4) {
            a00_w32(o, D_00246FB4, 0x8000000Au);
            (void)a00_c_001BA1A0(o, block, D_00246F20);
            a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
        }
    } else if (step == 1) {
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a00_w8(o, self + 5, 0);
            a00_w8(o, self + 0xB, 0);
        }
    }
    a00_w32(o, A00_POINT_82CCE0 + 0, em_ee_add_bits(F_MINUS_16_1, a00_u32(o, self + 0xB0)));
    a00_w32(o, A00_POINT_82CCE0 + 4, em_ee_add_bits(F_12, a00_u32(o, self + 0xB4)));
    a00_w32(o, A00_POINT_82CCE0 + 8, em_ee_add_bits(F_MINUS_40_4, a00_u32(o, self + 0xB8)));

    uint32_t other = a00_u32(o, self + 0x1C);
    uint32_t x = a00_u32(o, self + 0xB0);
    uint32_t ox = a00_u32(o, other + 0xB0);
    int same = 0;
    if (em_ee_c_eq_bits(x, ox)) {
        uint32_t oy = em_ee_add_bits(F_ONE, a00_u32(o, other + 0xB4));
        same = em_ee_c_eq_bits(a00_u32(o, self + 0xB4), oy);
    }
    if (!same) {
        a00_w32(o, self + 0xB0, ox);
        a00_w32(o, self + 0xB4, em_ee_add_bits(F_ONE, a00_u32(o, other + 0xB4)));
        (void)a00_c_001C6380(o, self);
        (void)a00_c_001A2370(o, self, self + 0xD0);
    }
    (void)a00_c_001B1B70(o, self);
    a00_w8(o, self + 1, 1);
    a00_callback(o, self);

    a00_w32(o, S_700038AC, F_ONE);
    a00_w32(o, S_700038A0, em_ee_add_bits(F_MINUS_11_017, a00_u32(o, self + 0xB0)));
    a00_w32(o, S_700038A4, em_ee_add_bits(F_26_99, a00_u32(o, self + 0xB4)));
    a00_w32(o, S_700038A8, em_ee_add_bits(F_MINUS_41, a00_u32(o, self + 0xB8)));
    (void)a00_c_001F5940(o, 9, S_700038A0, 0);
    a00_w32(o, S_700038A8, em_ee_add_bits(F_MINUS_36_954, a00_u32(o, self + 0xB8)));
    (void)a00_c_001F5940(o, 9, S_700038A0, 0);
}

int em_area00_ovl_008263C0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_cab(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * 0x8266A0 (func_overlay_AREA00_00826660) — [60]. +0x04: 0 calls 001B0FD0
 * and 001C6380; 1: once D_0081075E is 0xFF and +0x0D is 0x19, it switches
 * +0x0D to 0x1A, calls 001AF800, 001CB5B0(+0x09), 001B0FD0, 001C6380 and
 * sets +0x04 = 1; then 001B17A0 and the +0x4C callback; any other value
 * frees the node.
 * ====================================================================== */
static void a00_beam(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        (void)a00_c_001C6380(o, self);
        return;
    }
    if (state != 1) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (a00_u8(o, D_0081075E) == 0xFF && a00_u8(o, self + 0xD) == 0x19) {
        a00_w8(o, self + 0xD, 0x1A);
        (void)a00_c_001AF800(o, self);
        (void)a00_c_001CB5B0(o, (int32_t)a00_u8(o, self + 9));
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        (void)a00_c_001C6380(o, self);
        a00_w8(o, self + 4, 1);
    }
    r = 0;
    (void)a00_c_001B17A0(o, self, &r);
    a00_callback(o, self);
}

int em_area00_ovl_008266A0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_beam(&o, self);
    return a00_end(&o);
}

/* ======================================================================
 * Sub 2's owners (lane P14, docs/AREA14_PORT.md). Each follows the decomp's
 * byte-identical C; tools/test_area00_overlay_reference.py runs the
 * original over the k01 captures (route_k01) and designed states.
 * ====================================================================== */
#define D_0081028C 0x0081028Cu /* float: the player's yaw request (pi / 2) */
#define D_00810350 0x00810350u /* the player record's position */
#define D_008106C0 0x008106C0u /* word: 0x825D70's spawned group record */
#define D_00810784 0x00810784u /* AREA00 story byte */
#define D_00810803 0x00810803u /* AREA00 counter (sub 2's sequence) */
#define D_00810804 0x00810804u /* AREA00 counter (sub 2's [28]) */
#define S_70003B92 0x70003B92u
#define A00_GROUP_827EF0 0x00827EF0u
#define A00_GROUP_827F50 0x00827F50u
#define A00_GROUP_827FB0 0x00827FB0u
#define A00_QUAD_82B740 0x0082B740u   /* four 16-byte points (0x825E80's area) */
#define A00_SCRIPT_82ABC0 0x0082ABC0u
#define A00_SCRIPT_82B080 0x0082B080u
#define A00_POINT_82CD00 0x0082CD00u
#define A00_SCRIPT_82D070 0x0082D070u
#define F_HALF_PI 0x3FC90FDBu      /* 1.5707964 */
#define F_0_02 0x3CA3D70Au
#define F_0_008 0x3C03126Fu
#define F_0_5 0x3F000000u
#define F_0_8 0x3F4CCCCDu
#define F_300 0x43960000u

/* ----------------------------------------------------------------------
 * 0x826BE0 (func_overlay_AREA00_00826BA0) — [29]. +0x04: 0 sets +0x0D =
 * 0x12 when D_00810784 is 0xFF, then 001C6380 unless 001B0FD0 is set; 1 the
 * +0x4C callback unless D_00810804 is 2; 2 / 3 free; others nothing.
 * ---------------------------------------------------------------------- */
static void a00_s2_29(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        if (a00_u8(o, D_00810784) == 0xFF) a00_w8(o, self + 0xD, 0x12);
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        if (r == 0) (void)a00_c_001C6380(o, self);
    } else if (state == 1) {
        if (a00_u8(o, D_00810804) != 2) a00_callback(o, self);
    } else if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(o, self);
    }
}

int em_area00_ovl_00826BE0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_29(&o, self);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8260F0 (func_overlay_AREA00_008260B0) — [31]. +0x04: 0 goes to 3 when
 * 001BA1C0(self, 0x2B) is set, else 001B10B0(self, +0x0D, 0x6B),
 * 001C63E0(self, 0) and +0x04 = 1; 1 animates (001C64F0 at 1.0, 001B17A0,
 * 001C68C0, the callback) unless D_00810803 is 0 or 1; 2 / 3 free.
 * ---------------------------------------------------------------------- */
static void a00_s2_31(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a00_c_001BA1C0(o, self, 0x2B, &r);
        if (r != 0) {
            a00_w8(o, self + 4, 3);
            return;
        }
        r = 0;
        (void)a00_c_001B10B0(o, self, (int32_t)a00_u8(o, self + 0xD), 0x6B, &r);
        (void)a00_c_001C63E0(o, self, 0);
        a00_w8(o, self + 4, 1);
    } else if (state == 1) {
        uint32_t d = a00_u8(o, D_00810803);
        if (d == 0 || d == 1) return;
        r = 0;
        (void)a00_c_001C64F0(o, self, fb(F_ONE), &r);
        r = 0;
        (void)a00_c_001B17A0(o, self, &r);
        (void)a00_c_001C68C0(o, self);
        a00_callback(o, self);
    } else if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(o, self);
    }
}

int em_area00_ovl_008260F0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_31(&o, self);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825E80 (func_overlay_AREA00_00825E40, called by 0x825D70) — copies the
 * quad 0x82B740 into its frame (sp - 0x40; eight doubleword copies), then +0x05: 0 starts script
 * 0x82ABC0 once 001B1EA0(0, D_00810350, quad, 4) is 1; 1 sets +0x2E =
 * 0xFFFF while D_00810803 is 2 and at the script's end sets +0x05 = 0,
 * D_00810803 = 3, D_0081028C = pi / 2, 001B6660(0x827F50), 001FB0B0(0x17).
 * Then 001C6380, +0x01 = 1, 001B1B70 and the callback.
 * ---------------------------------------------------------------------- */
static void a00_s2_30a(A00Ovl *o, uint32_t self, uint32_t sp)
{
    const uint32_t quad = sp - 0x70u + 0x30u;
    int32_t r;

    /* the copy is eight doubleword loads, each stored to the local */
    for (uint32_t k = 0; k < 64; k += 8) {
        uint8_t row[8];
        memcpy(row, a00_at(o, A00_QUAD_82B740 + k, 8, 0), 8);
        memcpy(a00_at(o, quad + k, 8, 1), row, 8);
    }
    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        r = 0;
        (void)a00_c_001B1EA0(o, 0, D_00810350, quad, 4, &r);
        if (r == 1) {
            (void)a00_c_001BA1A0(o, self + 0x1F0, A00_SCRIPT_82ABC0);
            a00_w8(o, self + 5, 1);
        }
    } else if (step == 1) {
        if (a00_u8(o, D_00810803) == 2) a00_w16(o, self + 0x2E, 0xFFFF);
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            a00_w8(o, self + 5, 0);
            a00_w8(o, D_00810803, 3);
            a00_w32(o, D_0081028C, F_HALF_PI);
            uint32_t e = 0;
            (void)a00_c_001B6660(o, A00_GROUP_827F50, &e);
            (void)a00_c_001FB0B0(o, 0x17);
        }
    }
    (void)a00_c_001C6380(o, self);
    a00_w8(o, self + 1, 1);
    (void)a00_c_001B1B70(o, self);
    a00_callback(o, self);
}

int em_area00_ovl_00825E80(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_30a(&o, self, sp);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825FC0 (func_overlay_AREA00_00825F80, called by 0x825D70) — +0x05: 0
 * starts script 0x82B080 when D_00810803 is 0x80 (+0x28 = 0); 1 counts
 * +0x28 (001AEDE0(1, 1) at 0x186) and at the script's end calls
 * 001FB0B0(0) and sets +0x04 = 3. Unless D_00810803 is 0xFF: 001C6380,
 * +0x01 = 1, 001B1B70 and the callback.
 * ---------------------------------------------------------------------- */
static void a00_s2_30b(A00Ovl *o, uint32_t self)
{
    uint32_t step = a00_u8(o, self + 5);
    int32_t r;

    if (step == 0) {
        if (a00_u8(o, D_00810803) == 0x80) {
            a00_w8(o, self + 5, 1);
            (void)a00_c_001BA1A0(o, self + 0x1F0, A00_SCRIPT_82B080);
            a00_w16(o, self + 0x28, 0);
        }
    } else if (step == 1) {
        a00_w16(o, self + 0x28, a00_u16(o, self + 0x28) + 1);
        if ((int16_t)a00_u16(o, self + 0x28) == 0x186) (void)a00_c_001AEDE0(o, 1, 1);
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            (void)a00_c_001FB0B0(o, 0);
            a00_w8(o, self + 4, 3);
        }
    }
    if (a00_u8(o, D_00810803) != 0xFF) {
        (void)a00_c_001C6380(o, self);
        a00_w8(o, self + 1, 1);
        (void)a00_c_001B1B70(o, self);
        a00_callback(o, self);
    }
}

int em_area00_ovl_00825FC0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_30b(&o, self);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825D70 (func_overlay_AREA00_00825D30) — [30]. +0x04: 0 goes to 3 when
 * 001BA1C0(self, 0x2B) is set, else 001B0FD0, 001C6380, D_008106C0 =
 * 001B6660(0x827EF0), +0x04 = 1, +0x00 = 1; 1 runs 0x825E80 while
 * D_00810803 is 0..2, else 0x825FC0; 2 / 3 free.
 * ---------------------------------------------------------------------- */
static void a00_s2_30(A00Ovl *o, uint32_t self, uint32_t sp)
{
    uint32_t state = a00_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a00_c_001BA1C0(o, self, 0x2B, &r);
        if (r != 0) {
            a00_w8(o, self + 4, 3);
            return;
        }
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        (void)a00_c_001C6380(o, self);
        uint32_t e = 0;
        (void)a00_c_001B6660(o, A00_GROUP_827EF0, &e);
        a00_w32(o, D_008106C0, e);
        a00_w8(o, self + 4, 1);
        a00_w8(o, self + 0, 1);
    } else if (state == 1) {
        if (a00_u8(o, D_00810803) <= 2)
            a00_s2_30a(o, self, sp - 0x20u);
        else
            a00_s2_30b(o, self);
    } else if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(o, self);
    }
}

int em_area00_ovl_00825D70(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_30(&o, self, sp);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826790 (func_overlay_AREA00_00826750) — [28], the machine item 0x26
 * opens (EIGHTEENTH_LEVEL_ROUTE.md section 7c). +0x04: 0 once 001B0FD0 is
 * 0: 001C6380, +0x00 = +0x08 = 1, +0x30 = 0x82CD00 (001F6AD0 and +0x04 = 3
 * when D_00810784 is 0xFF), then two points of its matrix (001026A0 of
 * (-11.017, 26.99, -41) and (.., -36.954)) at +0x2E0.. / +0x2D4..; 1: +0x05
 * 0 on a Use (+0x0B bit 2) either (bit 0, item 0x26 used) D_00810804 = 2,
 * 001C47E0(0x26, 1), +0x2EC = 1 and script 0x82D070, or event 0x8000001C
 * and the boot script 0x246F20; then both points through 001F5940(9);
 * +0x05 1 at the script's end: with +0x2EC set +0x2E = 0xFFFF,
 * 001B0C60(0xE, 0, 0) (AREA14 entry 0) and +0x04 = 2, else +0x05 = +0x0B =
 * 0; with +0x2EC == 2 it counts +0x28 to 824 (001F6AD0 and 001B6660(
 * 0x827FB0) with D_00810784 = 0xFF for that call); +0x2EC == 0 emits the
 * points. Then, while 0x70003B92 is 0 or +0x2EC is 0, 001B1B70 and the
 * callback. 2: 001B1B70 and the callback. 3 and any other value: free.
 * ---------------------------------------------------------------------- */
static void a00_s2_28_points(A00Ovl *o, uint32_t self)
{
    a00_w32(o, S_700038AC, F_ONE);
    a00_w32(o, S_700038A0, a00_u32(o, self + 0x2E8));
    a00_w32(o, S_700038A4, a00_u32(o, self + 0x2E4));
    a00_w32(o, S_700038A8, a00_u32(o, self + 0x2E0));
    (void)a00_c_001F5940(o, 9, S_700038A0, 0);
    a00_w32(o, S_700038A0, a00_u32(o, self + 0x2DC));
    a00_w32(o, S_700038A4, a00_u32(o, self + 0x2D8));
    a00_w32(o, S_700038A8, a00_u32(o, self + 0x2D4));
    (void)a00_c_001F5940(o, 9, S_700038A0, 0);
}

static void a00_s2_28(A00Ovl *o, uint32_t self)
{
    uint32_t state = a00_u8(o, self + 4);
    uint32_t talk = self + 0x1F0;
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a00_c_001C6380(o, self);
        a00_w8(o, self + 0, 1);
        a00_w8(o, self + 8, 1);
        a00_w32(o, self + 0x30, A00_POINT_82CD00);
        if (a00_u8(o, D_00810784) == 0xFF) {
            (void)a00_c_001F6AD0(o);
            a00_w8(o, self + 4, 3);
        }
        a00_w32(o, S_700038AC, F_ONE);
        a00_w32(o, S_700038A0, F_MINUS_11_017);
        a00_w32(o, S_700038A4, F_26_99);
        a00_w32(o, S_700038A8, F_MINUS_41);
        (void)a00_c_001026A0(o, S_700038B0, self + 0xD0, S_700038A0);
        a00_w32(o, self + 0x2E8, a00_u32(o, S_700038B0));
        a00_w32(o, self + 0x2E4, a00_u32(o, S_700038B4));
        a00_w32(o, self + 0x2E0, a00_u32(o, S_700038B8));
        a00_w32(o, S_700038A8, F_MINUS_36_954);
        (void)a00_c_001026A0(o, S_700038B0, self + 0xD0, S_700038A0);
        a00_w32(o, self + 0x2DC, a00_u32(o, S_700038B0));
        a00_w32(o, self + 0x2D8, a00_u32(o, S_700038B4));
        a00_w32(o, self + 0x2D4, a00_u32(o, S_700038B8));
        return;
    }
    if (state == 2) {
        (void)a00_c_001B1B70(o, self);
        a00_callback(o, self);
        return;
    }
    if (state != 1) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        uint32_t b = a00_u8(o, self + 0xB);
        if (b & 4) {
            if (b & 1) {
                a00_w8(o, D_00810804, 2);
                (void)a00_c_001C47E0(o, 0x26, 1);
                a00_w32(o, self + 0x2EC, 1);
                a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
                a00_w16(o, self + 0x28, 0);
                (void)a00_c_001BA1A0(o, talk, A00_SCRIPT_82D070);
            } else {
                a00_w32(o, D_00246FB4, 0x8000001Cu);
                (void)a00_c_001BA1A0(o, talk, D_00246F20);
                a00_w32(o, self + 0x2EC, 0);
                a00_w8(o, self + 5, a00_u8(o, self + 5) + 1);
            }
        }
        a00_s2_28_points(o, self);
    } else if (step == 1) {
        r = 0;
        (void)a00_c_001BA1F0(o, self, &r);
        if (r != 0) {
            if (a00_s32(o, self + 0x2EC) != 0) {
                a00_w16(o, self + 0x2E, 0xFFFF);
                (void)a00_c_001B0C60(o, 0xE, 0, 0);
                a00_w8(o, self + 4, 2);
            } else {
                a00_w8(o, self + 5, 0);
                a00_w8(o, self + 0xB, 0);
            }
        }
        uint32_t count;
        if (a00_s32(o, self + 0x2EC) == 2 && (int16_t)(count = a00_u16(o, self + 0x28)) < 1000) {
            a00_w16(o, self + 0x28, count + 1);
            if ((int16_t)a00_u16(o, self + 0x28) == 824) {
                uint32_t saved = a00_u8(o, D_00810784);
                a00_w8(o, D_00810784, 0xFF);
                (void)a00_c_001F6AD0(o);
                uint32_t e = 0;
                (void)a00_c_001B6660(o, A00_GROUP_827FB0, &e);
                a00_w8(o, D_00810784, saved);
            }
        }
        if (a00_s32(o, self + 0x2EC) == 0) a00_s2_28_points(o, self);
    }
    if (a00_u8(o, S_70003B92) == 0 || a00_s32(o, self + 0x2EC) == 0) {
        (void)a00_c_001B1B70(o, self);
        a00_callback(o, self);
    }
}

int em_area00_ovl_00826790(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_28(&o, self);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826CC0 (func_overlay_AREA00_00826C80) — [32]. +0x04: 0 001B0FD0, +0x64
 * = 1.0 when D_00810803 is non-zero else 0.0, 001C6380; 1: +0x04 = 3 when
 * D_00810803 is 0xFF, then +0x05: 0 waits for D_00810803 == 2 (+0x28 = 30,
 * +0x2A = 0); 1 counts +0x28 down, at 0 001EFD20(4, a frame local) and +0x05
 * = 2; 2 counts +0x2A, raises +0x64 by 0.02 (bit 3 of the count) or 0.008
 * while it is <= 1.0 (else +0x05 = 3), sound 0x9F at 300 every tenth count
 * (001FBD50) and moves +0xB0 / +0xB8 by -/+0.5 / +/-0.8 (bit 1); then
 * 001C6380, 001B1B70 unless D_00810803 is 0, and the callback. 2 / 3 free.
 * ---------------------------------------------------------------------- */
static void a00_s2_32(A00Ovl *o, uint32_t self, uint32_t sp)
{
    uint32_t state = a00_u8(o, self + 4);
    int32_t r;

    if (state == 0) {
        r = 0;
        (void)a00_c_001B0FD0(o, self, &r);
        a00_w32(o, self + 0x64, a00_u8(o, D_00810803) != 0 ? F_ONE : F_ZERO);
        (void)a00_c_001C6380(o, self);
        return;
    }
    if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state != 1) return;
    if (a00_u8(o, D_00810803) == 0xFF) a00_w8(o, self + 4, 3);
    uint32_t step = a00_u8(o, self + 5);
    if (step == 0) {
        if (a00_u8(o, D_00810803) == 2) {
            a00_w16(o, self + 0x28, 30);
            a00_w16(o, self + 0x2A, 0);
            a00_w8(o, self + 5, 1);
        }
    } else if (step == 1) {
        if (a00_u16(o, self + 0x28) == 0) {
            (void)a00_c_001EFD20(o, 4, sp - 0x30u + 0x20u);
            a00_w8(o, self + 5, 2);
        }
        a00_w16(o, self + 0x28, a00_u16(o, self + 0x28) - 1);
    } else if (step == 2) {
        a00_w16(o, self + 0x2A, a00_u16(o, self + 0x2A) + 1);
        uint32_t level = a00_u32(o, self + 0x64);
        if (em_ee_c_le_bits(level, F_ONE)) {
            if (a00_u16(o, self + 0x2A) & 8)
                a00_w32(o, self + 0x64, em_ee_add_bits(level, F_0_02));
            else
                a00_w32(o, self + 0x64, em_ee_add_bits(level, F_0_008));
        } else {
            a00_w8(o, self + 5, 3);
        }
        if ((int16_t)a00_u16(o, self + 0x2A) % 10 == 0) (void)a00_c_001FBD50(o, self, 0x9F, 0, fb(F_300));
        if (a00_u16(o, self + 0x2A) & 2) {
            a00_w32(o, self + 0xB0, em_ee_sub_bits(a00_u32(o, self + 0xB0), F_0_5));
            a00_w32(o, self + 0xB8, em_ee_add_bits(a00_u32(o, self + 0xB8), F_0_8));
        } else {
            a00_w32(o, self + 0xB0, em_ee_add_bits(a00_u32(o, self + 0xB0), F_0_5));
            a00_w32(o, self + 0xB8, em_ee_sub_bits(a00_u32(o, self + 0xB8), F_0_8));
        }
    }
    (void)a00_c_001C6380(o, self);
    if (a00_u8(o, D_00810803) != 0) (void)a00_c_001B1B70(o, self);
    a00_callback(o, self);
}

int em_area00_ovl_00826CC0(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_32(&o, self, sp);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8241B0 (func_overlay_AREA00_00824170) — the effect node the sub-2
 * creature 0014D260 spawns (001EFE00); its +0x24 is the creature. Seven of
 * the creature's bone nodes (+0x110 + 4 * D_00828A40[k]) are followed:
 * +0x04 0 keeps their heights at +0x1F0.., sets the two timers +0x230 /
 * +0x234 to -1.5 * (i / 2), the seed +0x270 = 00122BB8(), +0x274 = 0, and
 * falls into 1. 1: +0x04 = 3 once the creature's +0x04 is 3; a change of
 * +0x05 (kept in +0x06) restarts the count; +0x05 1 spawns effect
 * 0x80000015 at every node's x / z with y -65 (then +0x05 = 0), 2 every
 * tenth count, 3 where a node falls below y -54, 0x63 ends (+0x04 = 3); then
 * the count, the heights again and two timer pairs from 0x828A30 (node,
 * mode): a timer at or above 0 places 0x700036A0 at its node (001029C0,
 * 00102918), 001CCF70(0x700036D0), steps 0.004 and wraps past 2.0 by -1;
 * a negative one steps 0.1. The seed's fraction goes to 0x70003A20 each
 * time. 2 / 3 free.
 * ---------------------------------------------------------------------- */
#define A00_NODE_INDEX_828A40 0x00828A40u
#define A00_TIMER_PAIRS_828A30 0x00828A30u
#define S_700036A0_M 0x700036A0u
#define S_700036D0_V 0x700036D0u
#define S_70003A20 0x70003A20u
#define F_MINUS_1_5 0xBFC00000u
#define F_MINUS_65 0xC2820000u
#define F_MINUS_54 0xC2580000u
#define F_65535 0x477FFF00u
#define F_0_0001 0x38D1B717u
#define F_0_004 0x3B83126Fu
#define F_0_1 0x3DCCCCCDu

static uint32_t a00_fx_node(A00Ovl *o, uint32_t parent, uint32_t k)
{
    int32_t index = a00_s32(o, A00_NODE_INDEX_828A40 + 4u * k);
    return a00_u32(o, parent + 0x110u + (uint32_t)index * 4u);
}

/* NODE(parent, k) is read again for every field (index, pointer, field) */
static void a00_fx_spawn_at(A00Ovl *o, uint32_t parent, uint32_t k, int with_w, uint32_t node)
{
    /* step 3 reads x through the node its height test computed */
    a00_w32(o, S_700038A0, a00_u32(o, (node ? node : a00_fx_node(o, parent, k)) + 0xC0));
    a00_w32(o, S_700038A8, a00_u32(o, a00_fx_node(o, parent, k) + 0xC8));
    a00_w32(o, S_700038A4, F_MINUS_65);
    if (with_w) a00_w32(o, S_700038AC, F_ONE);
    uint32_t e = 0;
    (void)a00_c_001F00A0(o, (int32_t)0x80000015u, S_700038A0, S_700038A0, 0, &e);
    if (e != 0) {
        (void)a00_c_001029C0(o, e + 0xD0);
        (void)a00_c_00102948(o, e + 0x100, S_700038A0);
    }
}

static void a00_s2_fx(A00Ovl *o, uint32_t self)
{
    const uint32_t fx = self + 0x1F0;
    uint32_t parent = a00_u32(o, self + 0x24);
    uint32_t state = a00_u8(o, self + 4);
    uint32_t i;

    if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        for (i = 0; i < 2; ++i)
            a00_w32(o, fx + 0x40 + 4 * i,
                    em_ee_mul_bits(F_MINUS_1_5, em_ee_div_bits(em_ee_cvt_s_w_bits(i), F_2)));
        for (i = 0; i < 7; ++i)
            a00_w32(o, fx + 4 * i, a00_u32(o, a00_fx_node(o, parent, i) + 0xC4));
        int32_t seed = 0;
        (void)a00_c_00122BB8(o, &seed);
        a00_w32(o, fx + 0x80, (uint32_t)seed);
        a00_w32(o, fx + 0x84, 0);
        a00_w8(o, self + 4, 1);
        a00_w8(o, self + 5, 0);
        a00_w8(o, self + 6, 0);
    } else if (state != 1) {
        return;
    }
    if (a00_u8(o, parent + 4) == 3) {
        a00_w8(o, self + 4, 3);
        return;
    }
    uint32_t seen = a00_u8(o, self + 6), now = a00_u8(o, self + 5);
    if (seen != now) {
        a00_w8(o, self + 6, now);
        a00_w32(o, fx + 0x84, 0);
    }
    uint32_t step = a00_u8(o, self + 5);
    if (step == 1) {
        for (i = 0; i < 7; ++i) a00_fx_spawn_at(o, parent, i, 1, 0);
        a00_w8(o, self + 5, 0);
    } else if (step == 2) {
        if (a00_s32(o, fx + 0x84) % 10 == 0)
            for (i = 0; i < 7; ++i) a00_fx_spawn_at(o, parent, i, 0, 0);
    } else if (step == 3) {
        for (i = 0; i < 7; ++i) {
            uint32_t node = a00_fx_node(o, parent, i);
            if (em_ee_c_lt_bits(a00_u32(o, node + 0xC4), F_MINUS_54) &&
                !em_ee_c_le_bits(a00_u32(o, fx + 4 * i), F_MINUS_54))
                a00_fx_spawn_at(o, parent, i, 1, node);
        }
    } else if (step == 0x63) {
        a00_w8(o, self + 4, 3);
    }
    a00_w32(o, fx + 0x84, a00_u32(o, fx + 0x84) + 1);
    for (i = 0; i < 7; ++i)
        a00_w32(o, fx + 4 * i, a00_u32(o, a00_fx_node(o, parent, i) + 0xC4));
    uint32_t seed = a00_u32(o, fx + 0x80);
    for (i = 0; i < 2; ++i) {
        uint32_t r = em_ee_cvt_s_w_bits((seed >> 16) & 0xFFFFu);
        r = em_ee_div_bits(r, F_65535);
        r = em_ee_add_bits(r, F_0_0001);
        seed = seed * 37u + 11u;
        /* the fraction is stored before the timer's test; after the calls
         * the step and the wrap test each read the timer */
        a00_w32(o, S_70003A20, r);
        uint32_t t = a00_u32(o, fx + 0x40 + 4 * i);
        if (!em_ee_c_lt_bits(t, F_ZERO)) {
            uint32_t node = a00_u32(o, A00_TIMER_PAIRS_828A30 + 8 * i);
            uint32_t mode = a00_u32(o, A00_TIMER_PAIRS_828A30 + 8 * i + 4);
            (void)a00_c_001029C0(o, S_700036A0_M);
            (void)a00_c_00102918(o, S_700036A0_M, S_700036A0_M, a00_u32(o, parent + 0x110 + node * 4) + 0xC0);
            int32_t r2 = 0;
            (void)a00_c_001CCF70(o, S_700036D0_V, &r2);
            if (mode <= 2)
                a00_w32(o, fx + 0x40 + 4 * i, em_ee_add_bits(a00_u32(o, fx + 0x40 + 4 * i), F_0_004));
            t = a00_u32(o, fx + 0x40 + 4 * i);
            if (!em_ee_c_le_bits(t, F_2)) a00_w32(o, fx + 0x40 + 4 * i, em_ee_sub_bits(t, F_ONE));
        } else {
            a00_w32(o, fx + 0x40 + 4 * i, em_ee_add_bits(t, F_0_1));
        }
    }
}

int em_area00_ovl_008241B0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_fx(&o, self);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823EB0 (func_overlay_AREA00_00823E70) — group 0x82CDD0's lamp nodes,
 * placed by script 0x82D070's op14. The colour 0x828A20 is copied into the
 * frame first; +0x04 1: the matrix +0xD0 copied (00102958), its last row set
 * to (0, 0, 1, 1) and moved through +0xD0, 0x700038A0 = the table point
 * 0x828990 through the copy less that row, normalised; d = -(its dot with
 * D_00810600), stored to 0x70003A20 before and after the clamp at 0; the
 * scale 0x8289B0 = 12 + 12 d (w 0); the packet 001CFA60(1.0, 0.1) /
 * 001CFBE0(001CCF70(+0x100), 1, 0x828990, packet, 1); when 001C6190(self)
 * lies outside 320..440 a 7 x 7 sprite 001CD520 at the row, its colour the
 * copy's r, g, b times (128 - (00122BB8() & 31)) / 128. Other states: none.
 * ---------------------------------------------------------------------- */
#define A00_COLOUR_828A20 0x00828A20u
#define A00_TABLE_828990 0x00828990u
#define A00_SCALE_8289B0 0x008289B0u
#define D_00810600 0x00810600u     /* the light direction the lamps face */
#define F_320 0x43A00000u
#define F_440 0x43DC0000u
#define F_7 0x40E00000u

static void a00_s2_lamp(A00Ovl *o, uint32_t self, uint32_t sp)
{
    const uint32_t frame = sp - 0xF0u, m = frame + 0x40u, pos = frame + 0x70u, packet = frame + 0x80u,
                   col = frame + 0xE0u;
    for (uint32_t k = 0; k < 16; k += 8) {   /* two doubleword copies */
        uint8_t half[8];
        memcpy(half, a00_at(o, A00_COLOUR_828A20 + k, 8, 0), 8);
        memcpy(a00_at(o, col + k, 8, 1), half, 8);
    }
    if (a00_u8(o, self + 4) != 1) return;
    (void)a00_c_00102958(o, m, self + 0xD0);
    a00_w32(o, pos + 0, F_ZERO);
    a00_w32(o, pos + 4, F_ZERO);
    a00_w32(o, pos + 8, F_ONE);
    a00_w32(o, pos + 12, F_ONE);
    (void)a00_c_001026A0(o, pos, self + 0xD0, pos);
    (void)a00_c_001026A0(o, S_700038A0, m, A00_TABLE_828990);
    (void)a00_c_001028D0(o, S_700038A0, S_700038A0, pos);
    (void)a00_c_00102760(o, S_700038A0, S_700038A0);
    uint32_t dot = 0;
    (void)a00_c_00102738(o, S_700038A0, D_00810600, &dot);
    uint32_t d = em_ee_neg_bits(dot);
    a00_w32(o, S_70003A20, d);
    if (em_ee_c_lt_bits(d, F_ZERO)) d = F_ZERO;
    a00_w32(o, S_70003A20, d);
    uint32_t s = em_ee_add_bits(F_12, em_ee_mul_bits(F_12, d));
    a00_w32(o, A00_SCALE_8289B0 + 0, s);
    a00_w32(o, A00_SCALE_8289B0 + 4, s);
    a00_w32(o, A00_SCALE_8289B0 + 8, s);
    a00_w32(o, A00_SCALE_8289B0 + 12, F_ZERO);
    int32_t h = 0;
    (void)a00_c_001CCF70(o, self + 0x100, &h);
    (void)a00_c_001CFA60(o, packet, m, fb(F_ONE), fb(em_ee_mul_bits(em_ee_cvt_s_w_bits(1), F_0_1)));
    (void)a00_c_001CFBE0(o, h, 1, A00_TABLE_828990, packet, 1);
    int32_t ix = 0;
    (void)a00_c_001C6190(o, self, &ix);
    uint32_t x = em_ee_cvt_s_w_bits((uint32_t)ix);
    if (em_ee_c_lt_bits(x, F_320) || em_ee_c_lt_bits(F_440, x)) {
        int32_t rnd = 0;
        (void)a00_c_00122BB8(o, &rnd);
        uint32_t k = 0x80u - ((uint32_t)rnd & 0x1Fu);
        uint32_t b = a00_u32(o, col + 8) * k;
        uint32_t g = a00_u32(o, col + 4) * k;
        uint32_t r = a00_u32(o, col + 0) * k;
        uint32_t c = (b >> 7) << 16;
        c |= (g >> 7) << 8;
        c |= r >> 7;
        int32_t res = 0;
        (void)a00_c_001CD520(o, 0, 2, pos, UINT64_C(0x20045B0599421EF0), c, fb(F_7), fb(F_7), fb(F_ZERO), &res);
    }
}

int em_area00_ovl_00823EB0(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    a00_s2_lamp(&o, self, sp);
    return a00_end(&o);
}

/* 0x824130 (func_overlay_AREA00_008240F0) — group 0x82CDD0's emitters:
 * +0x04 1 calls 001EFEB0(0x80000050, +0xD0) and 001EFD20(0x80000015,
 * +0x100); other states nothing. */
int em_area00_ovl_00824130(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    if (a00_u8(&o, self + 4) == 1) {
        (void)a00_c_001EFEB0(&o, (int32_t)0x80000050u, self + 0xD0);
        (void)a00_c_001EFD20(&o, (int32_t)0x80000015u, self + 0x100);
    }
    return a00_end(&o);
}

/* 0x826CB0 (func_overlay_AREA00_00826C70) — script 0x82D070's op09
 * callback: word a0 + 0x2EC = 2, returns 1 (a0 is [28], 0x826790). */
int em_area00_ovl_00826CB0(const EmArea00OvlHooks *h, uint32_t a0, uint32_t block, uint32_t a2,
                           int32_t *result, EmArea00OvlFault *fault)
{
    A00Ovl o;
    (void)block;
    (void)a2;
    if (!result || a00_begin(&o, h, fault)) return -1;
    a00_w32(&o, a0 + 0x2EC, 2);
    if (a00_failed(&o)) return -1;
    *result = 1;
    return 0;
}

/* ----------------------------------------------------------------------
 * Script 0x82ABC0's op14 nodes (group 0x82A920 [3..13], 0x270E entries:
 * 001C8140 places a node running 001C7EB0 that follows one bone of the
 * pose; the record's routine is the callback at node +0x2EC, which 001C7EB0
 * calls with the node).
 *
 * 0x823C50 (func_overlay_AREA00_00823C10) — entries [3..7]: +0x04 1 and the
 * world clock D_70003B68 a multiple of 20 (signed remainder 0): a vector
 * (pi/2, 0, 0, 1.0) in its frame (sp - 0x20 + 0x10, the four words stored in
 * that order before the call) and 001EFD90(1, +0x100, that vector). Other
 * states: nothing.
 * ---------------------------------------------------------------------- */
#define S_70003B68 0x70003B68u /* the world clock */

int em_area00_ovl_00823C50(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    if (a00_u8(&o, self + 4) == 1 && (int32_t)a00_u32(&o, S_70003B68) % 20 == 0) {
        const uint32_t v = sp - 0x20u + 0x10u;
        a00_w32(&o, v + 0, F_HALF_PI);
        a00_w32(&o, v + 4, F_ZERO);
        a00_w32(&o, v + 8, F_ZERO);
        a00_w32(&o, v + 12, F_ONE);
        (void)a00_c_001EFD90(&o, 1, self + 0x100, v);
    }
    return a00_end(&o);
}

/* 0x823E10 (func_overlay_AREA00_00823DD0) — entries [8..13]: +0x04 1:
 * e = 001F00A0(0x80000014, +0x100, +0xC0, 0); when e is nonzero,
 * 001029C0(e + 0xD0) and 00102948(e + 0x100, +0x100). Other states:
 * nothing. */
int em_area00_ovl_00823E10(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    if (a00_u8(&o, self + 4) == 1) {
        uint32_t e = 0;
        (void)a00_c_001F00A0(&o, (int32_t)0x80000014u, self + 0x100, self + 0xC0, 0, &e);
        if (!a00_failed(&o) && e != 0) {
            (void)a00_c_001029C0(&o, e + 0xD0);
            (void)a00_c_00102948(&o, e + 0x100, self + 0x100);
        }
    }
    return a00_end(&o);
}

/* 0x823CF0 (func_overlay_AREA00_00823CB0) — the node of effect 1 (0x823C50's
 * 001EFD90). State 0: +0xD0 = identity (001029C0) turned by +0xC0 (00102C58)
 * and moved to +0xB0 (00102918), +0x1F0 = 0, +0x1F4 = 00122BB8() / 2^31,
 * state 1, and on into state 1 the same call. State 1: 001D04B0(+0xD0, 1,
 * 0x828900, +0x1F0, +0x1F4), +0x1F0 += 0.008, state 3 once +0x1F0 is not at
 * most 1.5. States 2 / 3: 001AFC10. */
#define A00_PUFF_TABLE_828900 0x00828900u

int em_area00_ovl_00823CF0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    uint32_t state = a00_u8(&o, self + 4);
    if (state == 0) {
        (void)a00_c_001029C0(&o, self + 0xD0);
        (void)a00_c_00102C58(&o, self + 0xD0, self + 0xD0, self + 0xC0);
        (void)a00_c_00102918(&o, self + 0xD0, self + 0xD0, self + 0xB0);
        a00_w32(&o, self + 0x1F0, F_ZERO);
        a00_w32(&o, self + 0x1F4, a00_rand_fraction(&o));
        a00_w8(&o, self + 4, 1);
        state = 1;
    }
    if (state == 1) {
        (void)a00_c_001D04B0(&o, self + 0xD0, 1, A00_PUFF_TABLE_828900, fb(a00_u32(&o, self + 0x1F0)),
                             fb(a00_u32(&o, self + 0x1F4)));
        const uint32_t t = em_ee_add_bits(a00_u32(&o, self + 0x1F0), F_0_008);
        a00_w32(&o, self + 0x1F0, t);
        if (!em_ee_c_le_bits(t, F_1_5)) a00_w8(&o, self + 4, 3);
    } else if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(&o, self);
    }
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x824BB0 (func_overlay_AREA00_00824B70) — the 21-sprite arc. Block
 * +0x1F0: +0x00 scale, +0x04 a zero word, +0x08 the seed.
 * State 0: scale = 0.2, the zero word (stored before the call), seed =
 * 00122BB8(), state 1, and on into state 1 the same call.
 * State 1: the seed is read once; for i = 0..20, with the position table
 * 0x828B80 stepping 16 bytes: D_70003A20 = 180 - 180 * (i / 21.0) (stored
 * before 001029C0(0x700036A0)); 00102BB0(0x700036A0, 0x700036A0, pi *
 * D_70003A20 (re-read) / 180); 00102918(0x700036A0, 0x700036A0, entry);
 * h = 001CCF70(0x700036D0); r = ((seed >> 16) & 0xFFFF) / 65535.0 + 0.0001,
 * seed = seed * 37 + 11; 001CFB50(packet, 0, 0x700036A0; scale (re-read), r,
 * 1.0, 1e-6, 12.0); 001CFBE0(h, 1, 0x828CD0, packet, 0). Then scale +=
 * 0.012 (stored), and state 3 once it is not at most 2.0.
 * States 2 / 3: 001AFC10. The packet is a local of the 0xD0-byte frame at
 * sp - 0x60; only the callees touch it.
 * ---------------------------------------------------------------------- */
#define A00_ARC_POINTS_828B80 0x00828B80u
#define A00_ARC_TABLE_828CD0 0x00828CD0u
#define F_0_2 0x3E4CCCCDu
#define F_180 0x43340000u
#define F_21 0x41A80000u
#define F_PI 0x40490FDBu
#define F_1E_6 0x358637BDu
#define F_0_012 0x3C449BA6u

int em_area00_ovl_00824BB0(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    const uint32_t fx = self + 0x1F0;
    uint32_t state = a00_u8(&o, self + 4);
    if (state == 2 || state == 3) {
        (void)a00_c_001AFC10(&o, self);
        return a00_end(&o);
    }
    if (state == 0) {
        int32_t seed0 = 0;
        a00_w32(&o, fx + 0, F_0_2);
        a00_w32(&o, fx + 4, 0);
        (void)a00_c_00122BB8(&o, &seed0);
        a00_w32(&o, fx + 8, (uint32_t)seed0);
        a00_w8(&o, self + 4, 1);
    } else if (state != 1) {
        return a00_end(&o);
    }
    const uint32_t packet = sp - 0x60u;
    uint32_t seed = a00_u32(&o, fx + 8);
    uint32_t point = A00_ARC_POINTS_828B80;
    for (uint32_t i = 0; i < 21 && !a00_failed(&o); ++i) {
        const uint32_t q = em_ee_div_bits(em_ee_cvt_s_w_bits(i), F_21);
        a00_w32(&o, S_70003A20, em_ee_sub_bits(F_180, em_ee_mul_bits(F_180, q)));
        (void)a00_c_001029C0(&o, S_700036A0);
        const uint32_t angle = em_ee_div_bits(em_ee_mul_bits(F_PI, a00_u32(&o, S_70003A20)), F_180);
        (void)a00_c_00102BB0(&o, S_700036A0, S_700036A0, fb(angle));
        (void)a00_c_00102918(&o, S_700036A0, S_700036A0, point);
        int32_t handle = 0;
        (void)a00_c_001CCF70(&o, S_700036D0, &handle);
        uint32_t r = em_ee_cvt_s_w_bits((seed >> 16) & 0xFFFFu);
        r = em_ee_add_bits(em_ee_div_bits(r, F_65535), F_0_0001);
        seed = seed * 37u + 11u;
        (void)a00_c_001CFB50(&o, packet, 0, S_700036A0, fb(a00_u32(&o, fx + 0)), fb(r), fb(F_ONE), fb(F_1E_6),
                             fb(F_12));
        (void)a00_c_001CFBE0(&o, handle, 1, A00_ARC_TABLE_828CD0, packet, 0);
        point += 0x10u;
    }
    const uint32_t scale = em_ee_add_bits(a00_u32(&o, fx + 0), F_0_012);
    a00_w32(&o, fx + 0, scale);
    if (!em_ee_c_le_bits(scale, F_2)) a00_w8(&o, self + 4, 3);
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8247C0 (func_overlay_AREA00_00824780): an empty routine; 0x8247D0
 * stores its address at a shot's +0x34.
 * ---------------------------------------------------------------------- */
int em_area00_ovl_008247C0(const EmArea00OvlHooks *h, uint32_t self, EmArea00OvlFault *fault)
{
    A00Ovl o;
    (void)self;
    if (a00_begin(&o, h, fault)) return -1;
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8247D0 (func_overlay_AREA00_00824790) — the shot node. L = word +0x14
 * (read at entry, before the state byte); block B = +0x1F0: +0x00 three
 * scale words, +0x14 seed, +0x18 t, +0x1C t2, +0x20 velocity, +0x30 the
 * last position, +0x40 a matrix copy, +0x70 the second draw's point.
 * State 0: +0xD0 = identity (001029C0) turned by +0xC0 (00102C58) and moved
 * to +0xB0 (00102918); +0x00 = 1, +0x30 = L + 0x1F0, +0x34 = 0x8247C0;
 * velocity = (0, 0, 0.22222222, 1.0) through +0xD0 (001026A0) plus +0xB0
 * (001028D0); 00102948(B + 0x30, +0xB0); 00102958(B + 0x40, +0xD0); seed =
 * 00122BB8(); t = t2 = 0; state 1; on into states 1 / 2 the same call.
 * States 1 / 2: +0x100..+0x108 = +0xB0..+0xB8; D_70003A20 = t; d = t
 * (re-read), 1.0 when not at most 1.0; D_70003A20 = d; scale[0] = 8 d, and
 * scale[1], scale[2] = 8 * D_70003A20 (re-read each); the seed is read once;
 * h = 001CCF70(+0x100); r = ((seed >> 16) & 0xFFFF) / 65535 + 0.0001, seed
 * = seed * 37 + 11; 001CFAE0(packet, 0, +0xD0; t, r, 1.0, 1e-6);
 * 001CFBE0(h, 6, 0x828A60, packet, 0); t += 0.006666667, state 2 once it
 * is not at most 1.2; 0019A570(B + 0x30, +0xB0, 6, 0) nonzero: state 2;
 * 00102948(B + 0x30, +0xB0); +0xB0..+0xB8 += the velocity; 001B17A0 when the
 * state byte (re-read) is 1. Then, while t2 < 1.5: h = 001CCF70(B + 0x70),
 * r from the stepped seed (the seed word is not stored back), 001CFAE0(
 * packet, 0, B + 0x40; t2, r, 1.0, 1e-6), 001CFBE0(h, 1, 0x828AF0, packet,
 * 0), t2 += 0.015; otherwise state 3 when the state byte is 2.
 * State 3: 001AFC10. The packet is the frame's local at sp - 0x60.
 * ---------------------------------------------------------------------- */
#define A00_SHOT_EMPTY_8247C0 0x008247C0u
#define A00_SHOT_TABLE_828A60 0x00828A60u
#define A00_SHOT_TABLE_828AF0 0x00828AF0u
#define F_2_9 0x3E638E39u       /* 0.22222222 */
#define F_8 0x41000000u
#define F_0_0066667 0x3BDA740Eu /* 0.006666667 */
#define F_1_2 0x3F99999Au
#define F_0_015 0x3C75C28Fu

static uint32_t a00_seed_fraction(uint32_t seed)
{
    return em_ee_add_bits(em_ee_div_bits(em_ee_cvt_s_w_bits((seed >> 16) & 0xFFFFu), F_65535), F_0_0001);
}

int em_area00_ovl_008247D0(const EmArea00OvlHooks *h, uint32_t self, uint32_t sp, EmArea00OvlFault *fault)
{
    A00Ovl o;
    if (a00_begin(&o, h, fault)) return -1;
    const uint32_t link = a00_u32(&o, self + 0x14) + 0x1F0u;
    const uint32_t b = self + 0x1F0;
    const uint32_t packet = sp - 0x60u;
    uint32_t state = a00_u8(&o, self + 4);
    if (state == 3) {
        (void)a00_c_001AFC10(&o, self);
        return a00_end(&o);
    }
    if (state == 0) {
        int32_t seed0 = 0;
        (void)a00_c_001029C0(&o, self + 0xD0);
        (void)a00_c_00102C58(&o, self + 0xD0, self + 0xD0, self + 0xC0);
        (void)a00_c_00102918(&o, self + 0xD0, self + 0xD0, self + 0xB0);
        a00_w8(&o, self + 0, 1);
        a00_w32(&o, self + 0x30, link);
        a00_w32(&o, self + 0x34, A00_SHOT_EMPTY_8247C0);
        a00_w32(&o, b + 0x20, F_ZERO);
        a00_w32(&o, b + 0x24, F_ZERO);
        a00_w32(&o, b + 0x28, F_2_9);
        a00_w32(&o, b + 0x2C, F_ONE);
        (void)a00_c_001026A0(&o, b + 0x20, self + 0xD0, b + 0x20);
        (void)a00_c_001028D0(&o, b + 0x20, b + 0x20, self + 0xB0);
        (void)a00_c_00102948(&o, b + 0x30, self + 0xB0);
        (void)a00_c_00102958(&o, b + 0x40, self + 0xD0);
        (void)a00_c_00122BB8(&o, &seed0);
        a00_w32(&o, b + 0x14, (uint32_t)seed0);
        a00_w32(&o, b + 0x18, F_ZERO);
        a00_w32(&o, b + 0x1C, F_ZERO);
        a00_w8(&o, self + 4, 1);
    } else if (state != 1 && state != 2) {
        return a00_end(&o);
    }
    a00_w32(&o, self + 0x100, a00_u32(&o, self + 0xB0));
    a00_w32(&o, self + 0x104, a00_u32(&o, self + 0xB4));
    a00_w32(&o, self + 0x108, a00_u32(&o, self + 0xB8));
    a00_w32(&o, S_70003A20, a00_u32(&o, b + 0x18));
    uint32_t d = a00_u32(&o, b + 0x18);
    if (!em_ee_c_le_bits(d, F_ONE)) d = F_ONE;
    a00_w32(&o, S_70003A20, d);
    a00_w32(&o, b + 0x00, em_ee_mul_bits(F_8, d));
    a00_w32(&o, b + 0x04, em_ee_mul_bits(F_8, a00_u32(&o, S_70003A20)));
    a00_w32(&o, b + 0x08, em_ee_mul_bits(F_8, a00_u32(&o, S_70003A20)));
    uint32_t seed = a00_u32(&o, b + 0x14);
    int32_t handle = 0;
    (void)a00_c_001CCF70(&o, self + 0x100, &handle);
    uint32_t r = a00_seed_fraction(seed);
    seed = seed * 37u + 11u;
    (void)a00_c_001CFAE0(&o, packet, 0, self + 0xD0, fb(a00_u32(&o, b + 0x18)), fb(r), fb(F_ONE), fb(F_1E_6));
    (void)a00_c_001CFBE0(&o, handle, 6, A00_SHOT_TABLE_828A60, packet, 0);
    const uint32_t t = em_ee_add_bits(a00_u32(&o, b + 0x18), F_0_0066667);
    a00_w32(&o, b + 0x18, t);
    if (!em_ee_c_le_bits(t, F_1_2)) a00_w8(&o, self + 4, 2);
    int32_t hit = 0;
    (void)a00_c_0019A570(&o, b + 0x30, self + 0xB0, 6, 0, &hit);
    if (!a00_failed(&o) && hit != 0) a00_w8(&o, self + 4, 2);
    (void)a00_c_00102948(&o, b + 0x30, self + 0xB0);
    for (uint32_t k = 0; k < 3; ++k) {
        const uint32_t v = a00_u32(&o, b + 0x20 + 4 * k);
        a00_w32(&o, self + 0xB0 + 4 * k, em_ee_add_bits(a00_u32(&o, self + 0xB0 + 4 * k), v));
    }
    if (a00_u8(&o, self + 4) == 1) {
        int32_t ignored = 0;
        (void)a00_c_001B17A0(&o, self, &ignored);
    }
    if (em_ee_c_lt_bits(a00_u32(&o, b + 0x1C), F_1_5)) {
        int32_t h2 = 0;
        (void)a00_c_001CCF70(&o, b + 0x70, &h2);
        const uint32_t r2 = a00_seed_fraction(seed);
        (void)a00_c_001CFAE0(&o, packet, 0, b + 0x40, fb(a00_u32(&o, b + 0x1C)), fb(r2), fb(F_ONE), fb(F_1E_6));
        (void)a00_c_001CFBE0(&o, h2, 1, A00_SHOT_TABLE_828AF0, packet, 0);
        a00_w32(&o, b + 0x1C, em_ee_add_bits(a00_u32(&o, b + 0x1C), F_0_015));
    } else if (a00_u8(&o, self + 4) == 2) {
        a00_w8(&o, self + 4, 3);
    }
    return a00_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8260B0 (func_overlay_AREA00_00826070): 001EFD20(0x8000005D, 0x82B780),
 * then 001EFD20(0x80000004, 0xAC2D40); returns 1.
 * ---------------------------------------------------------------------- */
#define A00_POINT_82B780 0x0082B780u
#define A00_POINT_AC2D40 0x00AC2D40u

int em_area00_ovl_008260B0(const EmArea00OvlHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                           EmArea00OvlFault *fault)
{
    A00Ovl o;
    (void)a0;
    (void)a1;
    (void)a2;
    if (!result || a00_begin(&o, h, fault)) return -1;
    (void)a00_c_001EFD20(&o, (int32_t)0x8000005Du, A00_POINT_82B780);
    (void)a00_c_001EFD20(&o, (int32_t)0x80000004u, A00_POINT_AC2D40);
    if (a00_failed(&o)) return a00_end(&o);
    *result = 1;
    return a00_end(&o);
}
