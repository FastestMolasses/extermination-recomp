/* AREA20's own overlay owners (em_area20_overlay.h). Each function is one
 * original function; the comment above it says what the code does. The
 * order of the memory accesses between calls is the original's (the test
 * compares it one for one). */
#include "em_area20_overlay_internal.h"

/* overlay data (runtime addresses) */
#define A20_PACKET_826680 0x00826680u  /* 0x823660's 001D04B0 table */
#define A20_PACKET_826710 0x00826710u  /* 0x823840's table for +0x94 == 11 */
#define A20_PACKET_8267A0 0x008267A0u  /* 0x823840's table for +0x94 == 10 / 12 */
#define A20_SCRIPT_826830 0x00826830u  /* [32]'s arrival script */
#define A20_SCRIPT_8270B0 0x008270B0u  /* [32]'s script before the AREA21 request */
#define A20_QUAD_8273B0 0x008273B0u    /* [32]'s 64-byte quad record (001B1EA0 kind 4) */
#define A20_SPEED_827E70 0x00827E70u   /* 0x823D30's four speeds */
#define A20_LIFT_827E80 0x00827E80u    /* 0x823D30's four lifts */

/* globals */
#define D_PLAYER_POS 0x00810350u
#define D_0081080C 0x0081080Cu         /* counter 0x34 */
#define D_700036A0 0x700036A0u
#define D_700038A0 0x700038A0u
#define D_700038B0 0x700038B0u
#define D_70003B68 0x70003B68u
#define D_70003B8A 0x70003B8Au
#define D_70003B92 0x70003B92u

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_HALF 0x3F000000u
#define F_TWO 0x40000000u
#define F_4 0x40800000u
#define F_5 0x40A00000u
#define F_7 0x40E00000u
#define F_M4 0xC0800000u
#define F_M10 0xC1200000u
#define F_180 0x43340000u
#define F_256 0x43800000u
#define F_2P31 0x4F000000u
#define F_0_02 0x3CA3D70Au
#define F_0_03 0x3CF5C28Fu
#define F_0_06 0x3D75C28Fu
#define F_0_2 0x3E4CCCCDu
#define F_1_2 0x3F99999Au
#define F_PI 0x40490FDBu
#define F_TWO_PI 0x40C90FDBu
#define F_HALF_PI 0x3FC90FDBu
#define F_DEG 0x3C8EFA35u              /* 0.017453292 */
#define F_HALF_DEG 0x3C0EFA35u         /* 0.008726646 */

/* ----------------------------------------------------------------------
 * 0x823580 (decomp overlay_AREA20_func_00823540). State 1: 001EFD20(0,
 * +0x100) and 001EFEB0(0x80000053, +0xD0) every frame; other states do
 * nothing.
 * ---------------------------------------------------------------------- */
int em_area20_overlay_00823580(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    uint32_t unused = 0;
    if (a20_begin(&o, h, fault)) return -1;
    if (a20_u8(&o, a0 + 4) == 1) {
        (void)a20_c_001EFD20(&o, 0, a0 + 0x100);
        (void)a20_c_001EFEB0(&o, (int32_t)0x80000053u, a0 + 0xD0, &unused);
    }
    return a20_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823600 (decomp func_overlay_AREA20_008235C0). State 1: 001EFD20(0,
 * +0x100) every frame.
 * ---------------------------------------------------------------------- */
int em_area20_overlay_00823600(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    if (a20_u8(&o, a0 + 4) == 1) (void)a20_c_001EFD20(&o, 0, a0 + 0x100);
    return a20_end(&o);
}

/* +0x1F4 = rand() / 2^31 (the word converted to a float first) */
static void a20_rand_fraction(A20 *o, uint32_t self)
{
    int32_t r = 0;
    (void)a20_c_00122BB8(o, &r);
    a20_w32(o, self + 0x1F4, A20_DIV(A20_CVT_S_W(r), F_2P31));
}

/* ----------------------------------------------------------------------
 * 0x823660 (decomp func_overlay_AREA20_00823620). State 0: +0xD0 = the
 * identity turned by +0xC0 (00102C58) and moved to +0xB0 (00102918),
 * +0x1F0 = 0, +0x1F4 = rand fraction, state 1. State 1: 001D04B0(+0xD0, 1,
 * 0x826680, +0x1F0, +0x1F4), +0x1F0 += 0.02, state 3 once it is not below
 * 2.0. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
int em_area20_overlay_00823660(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    const uint32_t state = a20_u8(&o, a0 + 4);
    if (state == 0) {
        (void)a20_c_001029C0(&o, a0 + 0xD0);
        (void)a20_c_00102C58(&o, a0 + 0xD0, a0 + 0xD0, a0 + 0xC0);
        (void)a20_c_00102918(&o, a0 + 0xD0, a0 + 0xD0, a0 + 0xB0);
        a20_w32(&o, a0 + 0x1F0, F_ZERO);
        a20_rand_fraction(&o, a0);
        a20_w8(&o, a0 + 4, 1);
    } else if (state == 1) {
        (void)a20_c_001D04B0(&o, a0 + 0xD0, 1, A20_PACKET_826680, a20_u32(&o, a0 + 0x1F0), a20_u32(&o, a0 + 0x1F4));
        const uint32_t t = A20_ADD(a20_u32(&o, a0 + 0x1F0), F_0_02);
        a20_w32(&o, a0 + 0x1F0, t);
        if (!A20_LT(t, F_TWO)) a20_w8(&o, a0 + 4, 3);
    } else if (state == 2 || state == 3) {
        (void)a20_c_001AFC10(&o, a0);
    }
    return a20_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823780 (decomp func_overlay_AREA20_00823740). State 1: v =
 * 001C6190(self); k = 25 while 150 < v < 450, else 5; when D_70003B68 is a
 * multiple of k, 001EFEB0(1, +0xD0) and, when that gives an object, its
 * +0x94 halfword = own +0x94.
 * ---------------------------------------------------------------------- */
int em_area20_overlay_00823780(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    if (a20_u8(&o, a0 + 4) == 1) {
        int32_t v = 0;
        uint32_t made = 0;
        (void)a20_c_001C6190(&o, a0, &v);
        const int32_t k = v > 150 && v < 450 ? 25 : 5;
        if (!a20_failed(&o) && a20_s32(&o, D_70003B68) % k == 0) {
            (void)a20_c_001EFEB0(&o, 1, a0 + 0xD0, &made);
            if (!a20_failed(&o) && made != 0) a20_w16(&o, made + 0x94, a20_u16(&o, a0 + 0x94));
        }
    }
    return a20_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823840 (decomp func_overlay_AREA20_00823800). State 0: +0x1F0 = 0,
 * +0x1F4 = rand fraction, state 1. State 1: 001D04B0(+0xD0, 1, table,
 * +0x1F0, +0x1F4) with table 0x826710 when the halfword +0x94 is 11,
 * 0x8267A0 when it is 10 or 12 (no call otherwise); +0x1F0 += 0.03, state 3
 * once it is not below 1.2. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
int em_area20_overlay_00823840(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    const uint32_t state = a20_u8(&o, a0 + 4);
    if (state == 0) {
        a20_w32(&o, a0 + 0x1F0, F_ZERO);
        a20_rand_fraction(&o, a0);
        a20_w8(&o, a0 + 4, 1);
    } else if (state == 1) {
        const int32_t kind = a20_s16(&o, a0 + 0x94);
        if (kind == 11)
            (void)a20_c_001D04B0(&o, a0 + 0xD0, 1, A20_PACKET_826710, a20_u32(&o, a0 + 0x1F0),
                                 a20_u32(&o, a0 + 0x1F4));
        else if (kind == 10 || kind == 12)
            (void)a20_c_001D04B0(&o, a0 + 0xD0, 1, A20_PACKET_8267A0, a20_u32(&o, a0 + 0x1F0),
                                 a20_u32(&o, a0 + 0x1F4));
        const uint32_t t = A20_ADD(a20_u32(&o, a0 + 0x1F0), F_0_03);
        a20_w32(&o, a0 + 0x1F0, t);
        if (!A20_LT(t, F_1_2)) a20_w8(&o, a0 + 4, 3);
    } else if (state == 2 || state == 3) {
        (void)a20_c_001AFC10(&o, a0);
    }
    return a20_end(&o);
}

/* The tail 0x823B00 and 0x823BE0 share: 001C68C0, 001B17A0 (and, in
 * 0x823BE0, +1 = 1), then the node's +0x4C method. */
static void a20_tail(A20 *o, uint32_t self, int mark)
{
    (void)a20_c_001C68C0(o, self);
    (void)a20_c_001B17A0(o, self);
    if (mark) a20_w8(o, self + 1, 1);
    (void)a20_method(o, self);
}

/* ----------------------------------------------------------------------
 * 0x823B00 (decomp func_overlay_AREA20_00823AC0) — [32] while counter 0x34
 * is 0. Step +5 0: 001FABB0(), script 0x826830 on the +0x1F0 work, step 1.
 * Step 1: 001BA580(self, +0xD); at the script's end (001BA1F0)
 * 001FB0B0(0), D_0081080C = 1, step 0. Then (every step) 001C64F0(self,
 * 1.0), 001C68C0, 001B17A0 and the method.
 * ---------------------------------------------------------------------- */
static void a20_823B00(A20 *o, uint32_t self)
{
    int32_t r = 0;
    const uint32_t step = a20_u8(o, self + 5);
    if (step == 0) {
        (void)a20_c_001FABB0(o);
        (void)a20_c_001BA1A0(o, self + 0x1F0, A20_SCRIPT_826830);
        a20_w8(o, self + 5, 1);
    } else if (step == 1) {
        (void)a20_c_001BA580(o, self, (int32_t)a20_u8(o, self + 0xD));
        (void)a20_c_001BA1F0(o, self, &r);
        if (!a20_failed(o) && r != 0) {
            (void)a20_c_001FB0B0(o, 0);
            a20_w8(o, D_0081080C, 1);
            a20_w8(o, self + 5, 0);
        }
    }
    (void)a20_c_001C64F0(o, self, F_ONE, &r);
    a20_tail(o, self, 0);
}

/* ----------------------------------------------------------------------
 * 0x823BE0 (decomp func_overlay_AREA20_00823BA0) — [32] once counter 0x34
 * is set. Every call first copies the quad record 0x8273B0 (64 bytes) into
 * its frame (sp - 0x40). Step +5 0: when 001B1EA0(0, the player's
 * position, the copy, 4) gives 1, D_0081080C = 2, script 0x8270B0, step 1;
 * then 001C64F0(self, 1.0). Step 1: 001BA580(self, +0xD); at the script's
 * end 001B0C60(0x15, 0, 3) (AREA21 entry 3), +0x2E = 0xFFFF, state 3; the
 * work's +0xE halfword = 001C64F0(self, 0.5). Then 001C68C0, 001B17A0, +1
 * = 1 and the method.
 * ---------------------------------------------------------------------- */
static void a20_823BE0(A20 *o, uint32_t self, uint32_t sp)
{
    const uint32_t area = sp - 0x40u;
    uint64_t q[8];
    int32_t r = 0;

    /* each quadword move is two doubleword accesses */
    for (unsigned k = 0; k < 8; ++k) q[k] = a20_u64(o, A20_QUAD_8273B0 + 8 * k);
    for (unsigned k = 0; k < 8; ++k) a20_w64(o, area + 8 * k, q[k]);

    const uint32_t step = a20_u8(o, self + 5);
    if (step == 0) {
        (void)a20_c_001B1EA0(o, 0, D_PLAYER_POS, area, 4, &r);
        if (!a20_failed(o) && r == 1) {
            a20_w8(o, D_0081080C, 2);
            (void)a20_c_001BA1A0(o, self + 0x1F0, A20_SCRIPT_8270B0);
            a20_w8(o, self + 5, 1);
        }
        (void)a20_c_001C64F0(o, self, F_ONE, &r);
    } else if (step == 1) {
        (void)a20_c_001BA580(o, self, (int32_t)a20_u8(o, self + 0xD));
        (void)a20_c_001BA1F0(o, self, &r);
        if (!a20_failed(o) && r != 0) {
            (void)a20_c_001B0C60(o, 0x15, 0, 3);
            a20_w16(o, self + 0x2E, 0xFFFF);
            a20_w8(o, self + 4, 3);
        }
        r = 0;
        (void)a20_c_001C64F0(o, self, F_HALF, &r);
        if (!a20_failed(o)) a20_w16(o, self + 0x1F0 + 0xE, (uint32_t)r);
    }
    a20_tail(o, self, 1);
}

/* ----------------------------------------------------------------------
 * 0x8239B0 (decomp func_overlay_AREA20_00823970) — [32]. State 0: state 3
 * with flag 0x34 set; else 001B10B0(self, +0xD, 0x5B), 001C63E0(self, 3),
 * 001CA6F0(self, 2), 001BA8E0(self, +0xD), state 1, +0 = 1. State 1:
 * 0x823B00 while D_0081080C is 0, else 0x823BE0; then 001F9100(+0xB0,
 * D_700038A0 = a copy of +0xB0, D_700038B0 = (0, 1, 0, 1), 5.0). States 2 /
 * 3: 001BA540, 001AFC10.
 * ---------------------------------------------------------------------- */
static void a20_8239B0(A20 *o, uint32_t self, uint32_t sp)
{
    int32_t r = 0;
    const uint32_t state = a20_u8(o, self + 4);
    if (state == 0) {
        (void)a20_c_001BA1C0(o, self, 0x34, &r);
        if (a20_failed(o)) return;
        if (r != 0) {
            a20_w8(o, self + 4, 3);
            return;
        }
        (void)a20_c_001B10B0(o, self, (int32_t)a20_u8(o, self + 0xD), 0x5B);
        (void)a20_c_001C63E0(o, self, 3);
        (void)a20_c_001CA6F0(o, self, 2);
        (void)a20_c_001BA8E0(o, self, (int32_t)a20_u8(o, self + 0xD));
        a20_w8(o, self + 4, 1);
        a20_w8(o, self, 1);
    } else if (state == 1) {
        if (a20_u8(o, D_0081080C) == 0) a20_823B00(o, self);
        else a20_823BE0(o, self, sp - 0x20u);
        (void)a20_c_00102948(o, D_700038A0, self + 0xB0);
        a20_w32(o, D_700038B0 + 0, F_ZERO);
        a20_w32(o, D_700038B0 + 4, F_ONE);
        a20_w32(o, D_700038B0 + 8, F_ZERO);
        a20_w32(o, D_700038B0 + 0xC, F_ONE);
        (void)a20_c_001F9100(o, self + 0xB0, D_700038A0, D_700038B0, F_5);
    } else if (state == 2 || state == 3) {
        (void)a20_c_001BA540(o, self);
        (void)a20_c_001AFC10(o, self);
    }
}

int em_area20_overlay_008239B0(const EmArea20OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    a20_8239B0(&o, a0, sp);
    return a20_end(&o);
}

int em_area20_overlay_00823B00(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    a20_823B00(&o, a0);
    return a20_end(&o);
}

int em_area20_overlay_00823BE0(const EmArea20OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    a20_823BE0(&o, a0, sp);
    return a20_end(&o);
}

/* D_700038A0 = (x, y, z, 1.0) */
static void a20_point(A20 *o, uint32_t x, uint32_t y, uint32_t z)
{
    a20_w32(o, D_700038A0 + 0, x);
    a20_w32(o, D_700038A0 + 4, y);
    a20_w32(o, D_700038A0 + 8, z);
    a20_w32(o, D_700038A0 + 0xC, F_ONE);
}

/* D_700038A0 = (+0xB0, lift + +0xB4, +0xB8, 1.0), each field read just
 * before its store */
static void a20_raised(A20 *o, uint32_t self, uint32_t lift)
{
    a20_w32(o, D_700038A0 + 0, a20_u32(o, self + 0xB0));
    a20_w32(o, D_700038A0 + 4, A20_ADD(lift, a20_u32(o, self + 0xB4)));
    a20_w32(o, D_700038A0 + 8, a20_u32(o, self + 0xB8));
    a20_w32(o, D_700038A0 + 0xC, F_ONE);
}

/* ----------------------------------------------------------------------
 * 0x823D30 (decomp func_overlay_AREA20_00823CF0) — the breakables [19] ..
 * [31]; the work block is +0x1F0.
 * State 0: unless 001B0FD0(self), +0x34 = 1, +0 = 1, the spawn position
 * and rotation copied to +0x200 / +0x210, 001C6380.
 * State 1: when the shot halfword +0x36 is set: +0 = 2, state 2, and when
 * 0019A570 finds a hit between +0xB0 raised and lowered by 4 in y, effect
 * 4 (001F0460) on D_700036A0 = the identity turned by +0xC0, turned by
 * pi/2 (00102B08), moved to +0xB0, its y raised by 0.2. Then, unless
 * D_70003B92 is set while counter 0x34 is 2, 001B1B70 and the method.
 * State 2, step +5 0: effects 0x80000013 and, for kinds (+3) 0x18 / 0x2A,
 * 0x8000001C with sound 0x1A1, else 0x8000002E with sound 0x19F, at +0xB0
 * raised by 7; +0x28 = 2, step 1; the heading +0x264 (kind 10: 2pi * (rand
 * & 0xF0) / 256; kind 12: +0xC4 + pi * (rand & 0x1F) / 180; wrapped by
 * 001B1470); +0x38 = the speed table 0x827E70 [rand bits 8..9], +0x268 =
 * the lift table 0x827E80 [another rand's bits 8..9]. Step 1: when +0x28
 * counts down to 0, step 2 and +0xC4 = the heading; kinds 0x18 / 0x2A end
 * there (state 3, step 0). Step 2, the fall: +0xC0 turns by 1 degree while
 * the lift is negative, else by half a degree; +0xB0 / +0xB8 move by +0x38
 * along the heading's sine / cosine; when +0x38 is not 0 and 0019AD00
 * finds a wall 7 ahead (at y + 7), +0x38 = 0; the lift falls by 0.06 to
 * no lower than -4 and is added to +0xB4; while it is negative 0019AB20
 * looks down (0, -10, 0) from +0xB0: on landing effects 0x80000013 and
 * 0x8000002E at +0xB0 raised by 4, sound 0x1A0, +0x36 = 0, state 3, step
 * 0; otherwise 001B0D80 every 64th frame (D_70003B68 + the halfword
 * D_70003B8A). Every state-2 step then runs 001C6380, 001A2370(self,
 * +0xD0), 001B17A0 and the method. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a20_break_start(A20 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0;
    int32_t rnd = 0;
    uint32_t heading = 0;
    a20_raised(o, self, F_7);
    (void)a20_c_001EFD20(o, (int32_t)0x80000013u, D_700038A0);
    uint32_t kind = a20_u8(o, self + 3);
    if (kind == 0x18 || kind == 0x2A) {
        (void)a20_c_001EFD20(o, (int32_t)0x8000001Cu, D_700038A0);
        (void)a20_c_001FC580(o, self, 0x1A1);
    } else {
        (void)a20_c_001EFD20(o, (int32_t)0x8000002Eu, D_700038A0);
        (void)a20_c_001FC580(o, self, 0x19F);
    }
    a20_w16(o, self + 0x28, 2);
    a20_w8(o, self + 5, a20_u8(o, self + 5) + 1);
    kind = a20_u8(o, self + 3);
    if (kind == 10) {
        (void)a20_c_00122BB8(o, &rnd);
        const uint32_t x = A20_DIV(A20_MUL(F_TWO_PI, A20_CVT_S_W(rnd & 0xF0)), F_256);
        (void)a20_c_001B1470(o, x, &heading);
        a20_w32(o, blk + 0x74, heading);
    } else if (kind == 12) {
        (void)a20_c_00122BB8(o, &rnd);
        const uint32_t a = A20_CVT_S_W(rnd & 0x1F);
        const uint32_t x = A20_ADD(a20_u32(o, self + 0xC4), A20_DIV(A20_MUL(F_PI, a), F_180));
        (void)a20_c_001B1470(o, x, &heading);
        a20_w32(o, blk + 0x74, heading);
    }
    (void)a20_c_00122BB8(o, &rnd);
    a20_w32(o, self + 0x38, a20_u32(o, A20_SPEED_827E70 + 4 * (((uint32_t)rnd & 0x300) >> 8)));
    (void)a20_c_00122BB8(o, &rnd);
    a20_w32(o, blk + 0x78, a20_u32(o, A20_LIFT_827E80 + 4 * (((uint32_t)rnd & 0x300) >> 8)));
}

static void a20_break_fall(A20 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0;
    uint32_t v = 0, s = 0, c = 0;
    int32_t r = 0;
    const uint32_t turn = A20_LT(a20_u32(o, blk + 0x78), F_ZERO) ? F_DEG : F_HALF_DEG;
    (void)a20_c_001B1470(o, A20_ADD(turn, a20_u32(o, self + 0xC0)), &v);
    a20_w32(o, self + 0xC0, v);
    uint32_t speed, at;
    (void)a20_c_0011E2A8(o, a20_u32(o, blk + 0x74), &s);
    speed = a20_u32(o, self + 0x38);
    at = a20_u32(o, self + 0xB0);
    a20_w32(o, self + 0xB0, A20_ADD(at, A20_MUL(speed, s)));
    (void)a20_c_0011DE90(o, a20_u32(o, blk + 0x74), &c);
    speed = a20_u32(o, self + 0x38);
    at = a20_u32(o, self + 0xB8);
    a20_w32(o, self + 0xB8, A20_ADD(at, A20_MUL(speed, c)));
    (void)a20_c_0011E2A8(o, a20_u32(o, blk + 0x74), &s);
    a20_w32(o, D_700038A0 + 0, A20_ADD(a20_u32(o, self + 0xB0), A20_MUL(F_7, s)));
    a20_w32(o, D_700038A0 + 4, A20_ADD(F_7, a20_u32(o, self + 0xB4)));
    (void)a20_c_0011DE90(o, a20_u32(o, blk + 0x74), &c);
    a20_w32(o, D_700038A0 + 8, A20_ADD(a20_u32(o, self + 0xB8), A20_MUL(F_7, c)));
    if (!A20_EQ(a20_u32(o, self + 0x38), F_ZERO)) {
        r = 0;
        (void)a20_c_0019AD00(o, self, D_700038A0, (int32_t)0x80000007u, &r);
        if (!a20_failed(o) && r != 0) a20_w32(o, self + 0x38, F_ZERO);
    }
    uint32_t lift = A20_SUB(a20_u32(o, blk + 0x78), F_0_06);
    a20_w32(o, blk + 0x78, lift);
    if (A20_LT(lift, F_M4)) a20_w32(o, blk + 0x78, F_M4);
    lift = a20_u32(o, blk + 0x78);
    a20_w32(o, self + 0xB4, A20_ADD(a20_u32(o, self + 0xB4), lift));
    if (!A20_LT(a20_u32(o, blk + 0x78), F_ZERO)) return;
    a20_point(o, F_ZERO, F_M10, F_ZERO);
    r = 0;
    (void)a20_c_0019AB20(o, self, self + 0xB0, D_700038A0, (int32_t)0x80000007u, &r);
    if (a20_failed(o)) return;
    if (r != 0) {
        a20_raised(o, self, F_4);
        (void)a20_c_001EFD20(o, (int32_t)0x80000013u, D_700038A0);
        (void)a20_c_001EFD20(o, (int32_t)0x8000002Eu, D_700038A0);
        (void)a20_c_001FC580(o, self, 0x1A0);
        a20_w16(o, self + 0x36, 0);
        a20_w8(o, self + 4, 3);
        a20_w8(o, self + 5, 0);
    } else {
        const uint32_t extra = (uint32_t)a20_s16(o, D_70003B8A);
        if (((a20_u32(o, D_70003B68) + extra) & 0x3F) == 0) (void)a20_c_001B0D80(o, self);
    }
}

static void a20_823D30(A20 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0;
    int32_t r = 0;
    const uint32_t state = a20_u8(o, self + 4);
    if (state == 0) {
        (void)a20_c_001B0FD0(o, self, &r);
        if (a20_failed(o) || r != 0) return;
        a20_w16(o, self + 0x34, 1);
        a20_w8(o, self, 1);
        (void)a20_c_00102948(o, blk + 0x10, self + 0xB0);
        (void)a20_c_00102948(o, blk + 0x20, self + 0xC0);
        (void)a20_c_001C6380(o, self);
    } else if (state == 1) {
        if (a20_s16(o, self + 0x36) != 0) {
            a20_w8(o, self, 2);
            a20_w8(o, self + 4, 2);
            (void)a20_c_00102948(o, D_700038A0, self + 0xB0);
            (void)a20_c_00102948(o, D_700038B0, D_700038A0);
            const uint32_t up = a20_u32(o, D_700038A0 + 4);
            const uint32_t down = a20_u32(o, D_700038B0 + 4);
            a20_w32(o, D_700038A0 + 4, A20_ADD(up, F_4));
            a20_w32(o, D_700038B0 + 4, A20_SUB(down, F_4));
            r = 0;
            (void)a20_c_0019A570(o, D_700038A0, D_700038B0, 4, 0, &r);
            if (!a20_failed(o) && r != 0) {
                (void)a20_c_001029C0(o, D_700036A0);
                (void)a20_c_00102C58(o, D_700036A0, D_700036A0, self + 0xC0);
                (void)a20_c_00102B08(o, D_700036A0, D_700036A0, F_HALF_PI);
                (void)a20_c_00102918(o, D_700036A0, D_700036A0, self + 0xB0);
                a20_w32(o, D_700036A0 + 0x34, A20_ADD(a20_u32(o, D_700036A0 + 0x34), F_0_2));
                (void)a20_c_001F0460(o, 4, D_700036A0);
            }
        }
        if (a20_u8(o, D_70003B92) == 0 || a20_u8(o, D_0081080C) != 2) {
            (void)a20_c_001B1B70(o, self);
            (void)a20_method(o, self);
        }
    } else if (state == 2) {
        const uint32_t step = a20_u8(o, self + 5);
        if (step == 0) {
            a20_break_start(o, self);
        } else if (step == 1) {
            const uint32_t n = (a20_u16(o, self + 0x28) - 1u) & 0xFFFFu;
            a20_w16(o, self + 0x28, n);
            if (n == 0) {
                a20_w8(o, self + 5, a20_u8(o, self + 5) + 1);
                a20_w32(o, self + 0xC4, a20_u32(o, blk + 0x74));
                const uint32_t kind = a20_u8(o, self + 3);
                if (kind == 0x18 || kind == 0x2A) {
                    a20_w8(o, self + 4, 3);
                    a20_w8(o, self + 5, 0);
                }
            }
        } else if (step == 2) {
            a20_break_fall(o, self);
        }
        (void)a20_c_001C6380(o, self);
        (void)a20_c_001A2370(o, self, self + 0xD0);
        (void)a20_c_001B17A0(o, self);
        (void)a20_method(o, self);
    } else if (state == 3) {
        (void)a20_c_001AFC10(o, self);
    }
}

int em_area20_overlay_00823D30(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault)
{
    A20 o;
    if (a20_begin(&o, h, fault)) return -1;
    a20_823D30(&o, a0);
    return a20_end(&o);
}
