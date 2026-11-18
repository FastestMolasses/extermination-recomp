/* AREA17's own overlay owners, second part (em_area17_overlay.h): [33]
 * 0x8245A0, the exchange [34] 0x824700, the part 0x8251B0, the breakables
 * [5]..[8] 0x825560 and [36] 0x825EA0. Each function is one original
 * function; the order of the memory accesses between calls is the
 * original's (test_area17_overlay_reference compares it one for one). */
#include "em_area17_overlay_internal.h"

#define A17_GROUP_826560 0x00826560u   /* [34]'s group after the exchange */
#define A17_AREA_827530 0x00827530u    /* [33]'s 80-byte area */
#define A17_SCRIPT_8275E0 0x008275E0u  /* [34]'s scripts */
#define A17_SCRIPT_827A40 0x00827A40u
#define A17_SCRIPT_827E00 0x00827E00u
#define A17_SCRIPT_828050 0x00828050u
#define A17_TABLE_828390 0x00828390u   /* [34]'s +0x30 */
#define A17_AREA_8283A0 0x008283A0u    /* [34]'s 64-byte area */
#define A17_SPEED_828E10 0x00828E10u   /* 0x825560's speed table (4 floats) */
#define A17_RISE_828E20 0x00828E20u    /* its rise table (4 floats) */
#define A17_TABLE_828E30 0x00828E30u   /* [36]'s +0x30 */

#define D_PLAYER_POS 0x00810350u
#define D_00810700 0x00810700u
#define D_00810785 0x00810785u
#define D_00810786 0x00810786u
#define D_00810787 0x00810787u
#define D_00810806 0x00810806u
#define D_00810807 0x00810807u
#define D_0028A5E8 0x0028A5E8u
#define D_0028A5EC 0x0028A5ECu
#define D_700036A0 0x700036A0u
#define D_700038A0 0x700038A0u
#define D_700038B0 0x700038B0u
#define D_70003A20 0x70003A20u
#define D_70003A24 0x70003A24u
#define D_70003B68 0x70003B68u
#define D_70003B8A 0x70003B8Au

#define K(x) a17_bits(x)

/* Copy `quads` quadwords of an overlay area into the frame: the original
 * loads up to four quadwords, stores them, then moves the fifth (each
 * quadword move is two doubleword accesses). */
static void a17_copy_area(A17 *o, uint32_t dst, uint32_t src, unsigned quads)
{
    uint64_t q[8];
    for (unsigned k = 0; k < 8; ++k) q[k] = a17_u64(o, src + 8 * k);
    for (unsigned k = 0; k < 8; ++k) a17_w64(o, dst + 8 * k, q[k]);
    if (quads > 4) {
        q[0] = a17_u64(o, src + 0x40);
        q[1] = a17_u64(o, src + 0x48);
        a17_w64(o, dst + 0x40, q[0]);
        a17_w64(o, dst + 0x48, q[1]);
    }
}

static void a17_method_if_drawn(A17 *o, uint32_t self)
{
    int32_t r = 0;
    (void)a17_c_001B17A0(o, self, &r);
    if (r != 0) (void)a17_method(o, self);
}

/* ----------------------------------------------------------------------
 * 0x8245A0 (decomp func_overlay_AREA17_00824560) — [33]. Every call first
 * copies the 80-byte area 0x827530 into its frame (sp - 0x50). State 0:
 * after 001B0FD0(self) is 0: 001C6380, +0 = 1. State 1: step 0 waits for
 * flag 0x2D (then step 1 and a return without the method); step 1: once
 * 001B1EA0(0, the player, the copy, 5) is 1: 001FB0B0(0), D_00810786 = 1,
 * step 2. Then a17_method_if_drawn. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_8245A0(A17 *o, uint32_t self, uint32_t sp)
{
    const uint32_t area = sp - 0x50u;
    int32_t r = 0;
    a17_copy_area(o, area, A17_AREA_827530, 5);
    const uint32_t state = a17_u8(o, self + 4);
    if (state == 2 || state == 3) {
        (void)a17_c_001AFC10(o, self);
    } else if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r == 0) {
            (void)a17_c_001C6380(o, self);
            a17_w8(o, self, 1);
        }
    } else if (state == 1) {
        const uint32_t step = a17_u8(o, self + 5);
        if (step == 0) {
            (void)a17_c_001BA1C0(o, self, 0x2D, &r);
            if (r != 0) {
                a17_w8(o, self + 5, 1);
                return;
            }
        } else if (step == 1) {
            (void)a17_c_001B1EA0(o, 0, D_PLAYER_POS, area, 5, &r);
            if (r == 1) {
                (void)a17_c_001FB0B0(o, 0);
                a17_w8(o, D_00810786, 1);
                a17_w8(o, self + 5, 2);
            }
        }
        a17_method_if_drawn(o, self);
    }
}

int em_area17_overlay_008245A0(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8245A0(&o, a0, sp);
    return a17_end(&o);
}

/* [34]'s turn toward the talk: 001BA580(self, +0xD) and 001C64F0(self,
 * step); with `keep` the result's halfword goes to the talk block's +0xE. */
static void a17_face(A17 *o, uint32_t self, uint32_t step, int keep)
{
    int32_t r = 0;
    (void)a17_c_001BA580(o, self, (int32_t)a17_u8(o, self + 0xD));
    (void)a17_c_001C64F0(o, self, step, &r);
    if (keep) a17_w16(o, self + 0x1F0 + 0xE, (uint32_t)r);
}

/* [34]'s pose after a script: +0x40 = D_0028A5E8 and 001C67E0(self, pose,
 * 0, 0); `between` runs between the load and the call (the original
 * stores +0x40 in the call's delay). */
static void a17_pose(A17 *o, uint32_t self, int32_t pose)
{
    const uint32_t v = a17_u32(o, D_0028A5E8);
    a17_w32(o, self + 0x40, v);
    (void)a17_c_001C67E0(o, self, pose, 0, 0);
}

/* ----------------------------------------------------------------------
 * 0x824700 (decomp func_overlay_AREA17_008246C0) — [34], the exchange
 * (counters 0x2E / 0x2F). Every call first copies the 64-byte area
 * 0x8283A0 into its frame (sp - 0x40). State 0: state 3 with flag 0x2F;
 * else 001B10B0(self, +0xD, 0x56), 001C63E0(self, 9), 001CA6F0(self, 2),
 * 001BA8E0(self, +0xD), +0x58 = D_0028A5EC, +0x30 = 0x828390, state 1, +0
 * = 1. State 1 (nothing while D_00810785 is 0): with D_00810786 == 1, step
 * 0 script 0x8275E0 (step 1) and a17_face(1.0); step 1 at its end step 0,
 * +0x2E = 0xFFFF, a17_pose(0xA), D_00810806 = 0xFF, 001FAE70(0),
 * 0019C6F0(0xF, 0); then a17_face(0.5, kept). Else by D_00810807: 0: step
 * 0 starts script 0x827A40 (step 1, +0x2E = 0) when 001B1EA0(0, the
 * player, the copy, 4) is 1, a17_face(1.0); step 1 at its end
 * 001AEE10(4, 0), 001C47E0(0x28, 1), +0x2E = 0xFFFF, step 0, a17_pose(4),
 * 001FAE70(0), D_00810807 = 1, a17_face(0.5, kept). 1: step 0 starts
 * script 0x827E00 on a talk (+0xB bit 2); step 1 at its end +0xC4 =
 * 1.466076, 001C67E0(self, 4, 0, 0), +0xB = 0, step 0; a17_face(1.0). 2 /
 * 3: step 0 D_00810787 = 1, script 0x828050, +0x2E = 0, step 1,
 * a17_face(1.0); step 1 at its end 001C47A0(0x2B, 1), 001C4760(0x19, 1),
 * 001B6660(group 0x826560), D_00810807 = 0xFF, 001FAE70(0), +0x2E =
 * 0xFFFF, state 3; a17_face(0.5, kept) and the marker 001F9100(D_700038A0,
 * D_700038A0, D_700038B0, 5.0) at the +0x114 object's +0xC0 plus (2,
 * -13.7, -3), D_700038B0 = (0, 1, 0, 1). The tail: 001C68C0, 001B17A0
 * (result unused), +1 = 1, the method. States 2 / 3: 001BA540, 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_824700(A17 *o, uint32_t self, uint32_t sp)
{
    const uint32_t area = sp - 0x40u;
    const uint32_t talk = self + 0x1F0;
    int32_t r = 0;
    a17_copy_area(o, area, A17_AREA_8283A0, 4);
    const uint32_t state = a17_u8(o, self + 4);
    if (state == 2 || state == 3) {
        (void)a17_c_001BA540(o, self);
        (void)a17_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a17_c_001BA1C0(o, self, 0x2F, &r);
        if (r != 0) {
            a17_w8(o, self + 4, 3);
            return;
        }
        (void)a17_c_001B10B0(o, self, (int32_t)a17_u8(o, self + 0xD), 0x56);
        (void)a17_c_001C63E0(o, self, 9);
        (void)a17_c_001CA6F0(o, self, 2);
        (void)a17_c_001BA8E0(o, self, (int32_t)a17_u8(o, self + 0xD));
        a17_w32(o, self + 0x58, a17_u32(o, D_0028A5EC));
        a17_w32(o, self + 0x30, A17_TABLE_828390);
        a17_w8(o, self + 4, 1);
        a17_w8(o, self, 1);
        return;
    }
    if (state != 1) return;
    if (a17_u8(o, D_00810785) == 0) return;
    if (a17_u8(o, D_00810786) == 1) {
        const uint32_t step = a17_u8(o, self + 5);
        if (step == 0) {
            (void)a17_c_001BA1A0(o, talk, A17_SCRIPT_8275E0);
            a17_w8(o, self + 5, 1);
            a17_face(o, self, K(1.0f), 0);
        } else if (step == 1) {
            (void)a17_c_001BA1F0(o, self, &r);
            if (r != 0) {
                a17_w8(o, self + 5, 0);
                a17_w16(o, self + 0x2E, 0xFFFF);
                a17_pose(o, self, 0xA);
                a17_w8(o, D_00810806, 0xFF);
                (void)a17_c_001FAE70(o, 0);
                (void)a17_c_0019C6F0(o, 0xF, 0);
            }
            a17_face(o, self, K(0.5f), 1);
        }
    } else {
        const uint32_t phase = a17_u8(o, D_00810807);
        const uint32_t step = (phase <= 3) ? a17_u8(o, self + 5) : 0xFFu;
        if (phase == 0) {
            if (step == 0) {
                (void)a17_c_001B1EA0(o, 0, D_PLAYER_POS, area, 4, &r);
                if (r == 1) {
                    (void)a17_c_001BA1A0(o, talk, A17_SCRIPT_827A40);
                    a17_w8(o, self + 5, 1);
                    a17_w16(o, self + 0x2E, 0);
                }
                a17_face(o, self, K(1.0f), 0);
            } else if (step == 1) {
                (void)a17_c_001BA1F0(o, self, &r);
                if (r != 0) {
                    (void)a17_c_001AEE10(o, 4, 0);
                    (void)a17_c_001C47E0(o, 0x28, 1);
                    a17_w16(o, self + 0x2E, 0xFFFF);
                    a17_w8(o, self + 5, 0);
                    a17_pose(o, self, 4);
                    (void)a17_c_001FAE70(o, 0);
                    a17_w8(o, D_00810807, 1);
                }
                a17_face(o, self, K(0.5f), 1);
            }
        } else if (phase == 1) {
            if (step == 0) {
                if (a17_u8(o, self + 0xB) & 4) {
                    (void)a17_c_001BA1A0(o, talk, A17_SCRIPT_827E00);
                    a17_w8(o, self + 5, 1);
                }
            } else if (step == 1) {
                (void)a17_c_001BA1F0(o, self, &r);
                if (r != 0) {
                    a17_w32(o, self + 0xC4, K(1.466076f));
                    (void)a17_c_001C67E0(o, self, 4, 0, 0);
                    a17_w8(o, self + 0xB, 0);
                    a17_w8(o, self + 5, 0);
                }
            }
            a17_face(o, self, K(1.0f), 0);
        } else if (phase == 2 || phase == 3) {
            if (step == 0) {
                a17_w8(o, D_00810787, 1);
                (void)a17_c_001BA1A0(o, talk, A17_SCRIPT_828050);
                a17_w16(o, self + 0x2E, 0);
                a17_w8(o, self + 5, 1);
                a17_face(o, self, K(1.0f), 0);
            } else if (step == 1) {
                (void)a17_c_001BA1F0(o, self, &r);
                if (r != 0) {
                    uint32_t g = 0;
                    (void)a17_c_001C47A0(o, 0x2B, 1);
                    (void)a17_c_001C4760(o, 0x19, 1);
                    (void)a17_c_001B6660(o, A17_GROUP_826560, &g);
                    a17_w8(o, D_00810807, 0xFF);
                    (void)a17_c_001FAE70(o, 0);
                    a17_w16(o, self + 0x2E, 0xFFFF);
                    a17_w8(o, self + 4, 3);
                }
                a17_face(o, self, K(0.5f), 1);
                (void)a17_c_00102948(o, D_700038A0, a17_u32(o, self + 0x114) + 0xC0);
                a17_w32(o, D_700038B0, 0);
                const uint32_t x = A17_ADD(a17_u32(o, D_700038A0), K(2.0f));
                a17_w32(o, D_700038B0 + 4, K(1.0f));
                a17_w32(o, D_700038A0, x);
                a17_w32(o, D_700038B0 + 8, 0);
                a17_w32(o, D_700038B0 + 12, K(1.0f));
                a17_w32(o, D_700038A0 + 4, A17_SUB(a17_u32(o, D_700038A0 + 4), K(13.7f)));
                a17_w32(o, D_700038A0 + 8, A17_SUB(a17_u32(o, D_700038A0 + 8), K(3.0f)));
                a17_w32(o, D_700038A0 + 12, K(1.0f));
                (void)a17_c_001F9100(o, D_700038A0, D_700038A0, D_700038B0, K(5.0f));
            }
        }
    }
    (void)a17_c_001C68C0(o, self);
    (void)a17_c_001B17A0(o, self, &r);
    a17_w8(o, self + 1, 1);
    (void)a17_method(o, self);
}

int em_area17_overlay_00824700(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_824700(&o, a0, sp);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8251B0 (decomp func_overlay_AREA17_00825170) — group 0x827F40 [4], a
 * part riding its owner (+0x20; +0x24 the group record). State 0: state 1
 * once 001BAD40(self, owner) is 0, and on into state 1. State 1: the
 * halfword +0x28 counts; the group's halfword +0x2E with the bit (1 <<
 * +0x2E) gives state 2; else nothing more while the owner's +4 is 0x270D
 * or 0x270C, and by the owner's +0xA: 5 001C5C90(self) and return; 4
 * 001C68C0 and the method; 3 nothing; 0 001BA580(self, owner +4) then as
 * default; default 001C64F0(self, owner +0xC), 001C68C0, +1 = 1, the
 * method. Then for the counts 521..2282 and 3699..3989 001F4E20 of the
 * +0x138 object's matrix (+0x90) times (-1.2367, 0.2497, -0.4078, 1)
 * (001026A0 into D_700038A0) with D_700038B0 = (0, 0xA0, 0, 0x80), size
 * 1.0. State 2: state 3; 001BA540 unless the owner's +4 is 0x270D or its
 * +0xA is not 0. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_8251B0(A17 *o, uint32_t self)
{
    const uint32_t st = a17_u8(o, self + 4);
    const uint32_t own = a17_u32(o, self + 0x20);
    const uint32_t grp = a17_u32(o, self + 0x24);
    int32_t r = 0;
    if (st == 3) {
        (void)a17_c_001AFC10(o, self);
        return;
    }
    if (st == 2) {
        a17_w8(o, self + 4, st + 1);
        if (a17_s16(o, own + 4) != 0x270D && a17_s16(o, own + 0xA) == 0) (void)a17_c_001BA540(o, self);
        return;
    }
    if (st == 0) {
        (void)a17_c_001BAD40(o, self, own, &r);
        if (r != 0) return;
        a17_w8(o, self + 4, 1);
    } else if (st != 1) {
        return;
    }
    a17_w16(o, self + 0x28, a17_u16(o, self + 0x28) + 1);
    const uint32_t bit = a17_u16(o, self + 0x2E);
    const uint32_t bits = a17_u16(o, grp + 0x2E);
    if (bits & (1u << (bit & 31))) {
        a17_w8(o, self + 4, 2);
    } else {
        const int32_t kind = a17_s16(o, own + 4);
        if (kind == 0x270D || kind == 0x270C) return;
        const int32_t mode = a17_s16(o, own + 0xA);
        if (mode == 5) {
            (void)a17_c_001C5C90(o, self);
            return;
        }
        if (mode == 4) {
            (void)a17_c_001C68C0(o, self);
            (void)a17_method(o, self);
        } else if (mode != 3) {
            if (mode == 0) (void)a17_c_001BA580(o, self, kind);
            (void)a17_c_001C64F0(o, self, a17_u32(o, own + 0xC), &r);
            (void)a17_c_001C68C0(o, self);
            a17_w8(o, self + 1, 1);
            (void)a17_method(o, self);
        }
    }
    const int32_t n = a17_s16(o, self + 0x28);
    if ((n > 520 && n < 2283) || (n > 3698 && n <= 3989)) {
        a17_w32(o, D_700038A0, K(-1.2367f));
        a17_w32(o, D_700038A0 + 4, K(0.2497f));
        a17_w32(o, D_700038A0 + 8, K(-0.4078f));
        a17_w32(o, D_700038A0 + 12, K(1.0f));
        (void)a17_c_001026A0(o, D_700038A0, a17_u32(o, self + 0x138) + 0x90, D_700038A0);
        a17_w32(o, D_700038B0, 0);
        a17_w32(o, D_700038B0 + 4, 0xA0);
        a17_w32(o, D_700038B0 + 8, 0);
        a17_w32(o, D_700038B0 + 12, 0x80);
        (void)a17_c_001F4E20(o, D_700038A0, D_700038B0, K(1.0f));
    }
}

int em_area17_overlay_008251B0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8251B0(&o, a0);
    return a17_end(&o);
}

/* 0x825560's burst at the object (D_700038A0 = +0xB0 with y + 4, w 1):
 * effects 0x80000013 and 0x8000002E, sound 0x1A0, +0x36 = 0, state 3,
 * step 0. */
static void a17_burst(A17 *o, uint32_t self)
{
    a17_w32(o, D_700038A0, a17_u32(o, self + 0xB0));
    a17_w32(o, D_700038A0 + 4, A17_ADD(K(4.0f), a17_u32(o, self + 0xB4)));
    a17_w32(o, D_700038A0 + 8, a17_u32(o, self + 0xB8));
    a17_w32(o, D_700038A0 + 12, K(1.0f));
    (void)a17_c_001EFD20(o, (int32_t)0x80000013u, D_700038A0);
    (void)a17_c_001EFD20(o, (int32_t)0x8000002Eu, D_700038A0);
    (void)a17_c_001FC580(o, self, 0x1A0);
    a17_w16(o, self + 0x36, 0);
    a17_w8(o, self + 4, 3);
    a17_w8(o, self + 5, 0);
}

/* 0x825560's state 1 */
static void a17_825560_rest(A17 *o, uint32_t self)
{
    if (a17_s16(o, self + 0x36) != 0) {
        a17_w8(o, self, 2);
        a17_w8(o, self + 4, 2);
        (void)a17_c_00102948(o, D_700038A0, self + 0xB0);
        (void)a17_c_00102948(o, D_700038B0, D_700038A0);
        const uint32_t ay = a17_u32(o, D_700038A0 + 4);
        const uint32_t by = a17_u32(o, D_700038B0 + 4);
        a17_w32(o, D_700038A0 + 4, A17_ADD(ay, K(4.0f)));
        a17_w32(o, D_700038B0 + 4, A17_SUB(by, K(4.0f)));
        int32_t hit = 0;
        (void)a17_c_0019A570(o, D_700038A0, D_700038B0, 4, 0, &hit);
        if (hit != 0) {
            (void)a17_c_001029C0(o, D_700036A0);
            (void)a17_c_00102B08(o, D_700036A0, D_700036A0, K(1.5707964f));
            (void)a17_c_00102918(o, D_700036A0, D_700036A0, self + 0xB0);
            a17_w32(o, D_700036A0 + 0x34, A17_ADD(a17_u32(o, D_700036A0 + 0x34), K(0.2f)));
            (void)a17_c_001F0460(o, 4, D_700036A0);
        }
    } else if (a17_u8(o, 0x00810805u) >= 3) {
        a17_w8(o, self, 2);
        a17_w8(o, self + 4, 3);
    }
    (void)a17_method(o, self);
    (void)a17_c_001028D0(o, D_700038A0, D_PLAYER_POS, self + 0xB0);
    a17_w32(o, D_700038A0 + 12, 0);
    uint32_t dot = 0;
    (void)a17_c_00102738(o, D_700038A0, D_700038A0, &dot);
    a17_w32(o, D_70003A20, dot);
    /* the original keeps 0x70003A20 loaded before it squares the bound
     * (decomp NEARMISS note): store 50, load it, load the distance, store
     * 50 * 50, compare the distance with the product */
    a17_w32(o, D_70003A24, K(50.0f));
    const uint32_t k = a17_u32(o, D_70003A24);
    const uint32_t d = a17_u32(o, D_70003A20);
    const uint32_t bound = A17_MUL(k, K(50.0f));
    a17_w32(o, D_70003A24, bound);
    int32_t r = 0;
    if (!A17_LE(d, bound)) {
        (void)a17_c_001B17A0(o, self, &r);
    } else {
        a17_w8(o, self + 1, 1);
        (void)a17_c_001B1D20(o, self);
    }
}

/* 0x825560's state 2 (step +5; the block +0x1F0: +0x74 the heading, +0x78
 * the rise speed). Returns 0 when the step ended the frame early (the
 * shot burst), 1 when the tail runs. */
static int a17_825560_fall(A17 *o, uint32_t self)
{
    const uint32_t blk = self + 0x1F0;
    const uint32_t st5 = a17_u8(o, self + 5);
    int32_t r = 0;
    if (st5 == 0) {
        a17_w32(o, D_700038A0, a17_u32(o, self + 0xB0));
        a17_w32(o, D_700038A0 + 4, A17_ADD(K(7.0f), a17_u32(o, self + 0xB4)));
        a17_w32(o, D_700038A0 + 8, a17_u32(o, self + 0xB8));
        a17_w32(o, D_700038A0 + 12, K(1.0f));
        (void)a17_c_001EFD20(o, (int32_t)0x80000013u, D_700038A0);
        const uint32_t kind = a17_u8(o, self + 3);
        if (kind == 0x18 || kind == 0x2A) {
            (void)a17_c_001EFD20(o, (int32_t)0x8000001Cu, D_700038A0);
            (void)a17_c_001FC580(o, self, 0x1A1);
        } else {
            (void)a17_c_001EFD20(o, (int32_t)0x8000002Eu, D_700038A0);
            (void)a17_c_001FC580(o, self, 0x19F);
        }
        a17_w16(o, self + 0x28, 2);
        a17_w8(o, self + 5, a17_u8(o, self + 5) + 1);
        const uint32_t kind2 = a17_u8(o, self + 3);
        uint32_t h = 0;
        if (kind2 == 10) {
            (void)a17_c_00122BB8(o, &r);
            const uint32_t a = A17_DIV(A17_MUL(K(6.2831855f), A17_CVT_S_W((uint32_t)r & 0xF0u)), K(256.0f));
            (void)a17_c_001B1470(o, a, &h);
            a17_w32(o, blk + 0x74, h);
        } else if (kind2 == 12) {
            (void)a17_c_00122BB8(o, &r);
            const uint32_t a = A17_CVT_S_W((uint32_t)r & 0x1Fu);
            const uint32_t c4 = a17_u32(o, self + 0xC4);
            (void)a17_c_001B1470(o, A17_ADD(c4, A17_DIV(A17_MUL(K(3.1415927f), a), K(180.0f))), &h);
            a17_w32(o, blk + 0x74, h);
        }
        (void)a17_c_00122BB8(o, &r);
        const uint32_t speed = a17_u32(o, A17_SPEED_828E10 + 4 * (((uint32_t)r & 0x300u) >> 8));
        a17_w32(o, self + 0x38, speed);
        (void)a17_c_00122BB8(o, &r);
        a17_w32(o, blk + 0x78, a17_u32(o, A17_RISE_828E20 + 4 * (((uint32_t)r & 0x300u) >> 8)));
        return 1;
    }
    if (st5 == 1) {
        const uint32_t n = (a17_u16(o, self + 0x28) - 1u) & 0xFFFFu;
        a17_w16(o, self + 0x28, n);
        if (n == 0) {
            a17_w32(o, self + 0xC4, a17_u32(o, blk + 0x74));
            const uint32_t kind = a17_u8(o, self + 3);
            if (kind == 0x18 || kind == 0x2A) {
                a17_w8(o, self + 4, 3);
                a17_w8(o, self + 5, 0);
            } else {
                a17_w8(o, self + 5, a17_u8(o, self + 5) + 1);
                a17_w16(o, self + 0x28, 8);
            }
        }
        return 1;
    }
    if (st5 == 2) {
        const int32_t n = a17_s16(o, self + 0x28);
        if (n == 0) {
            a17_w8(o, self + 5, st5 + 1);
            a17_w8(o, self, 1);
            a17_w16(o, self + 0x36, 0);
            a17_w16(o, self + 0x34, 1);
        } else {
            a17_w16(o, self + 0x28, (uint32_t)(n - 1));
        }
    } else if (st5 != 3) {
        return 1;
    }
    /* steps 2 and 3: the fall */
    uint32_t v = 0;
    const uint32_t turn = A17_LT(a17_u32(o, blk + 0x78), 0) ? K(0.017453292f) : K(0.008726646f);
    (void)a17_c_001B1470(o, A17_ADD(turn, a17_u32(o, self + 0xC0)), &v);
    a17_w32(o, self + 0xC0, v);
    uint32_t s = 0;
    (void)a17_c_0011E2A8(o, a17_u32(o, blk + 0x74), &s);
    uint32_t sp = a17_u32(o, self + 0x38);
    a17_w32(o, self + 0xB0, A17_ADD(a17_u32(o, self + 0xB0), A17_MUL(sp, s)));
    (void)a17_c_0011DE90(o, a17_u32(o, blk + 0x74), &s);
    sp = a17_u32(o, self + 0x38);
    a17_w32(o, self + 0xB8, A17_ADD(a17_u32(o, self + 0xB8), A17_MUL(sp, s)));
    (void)a17_c_0011E2A8(o, a17_u32(o, blk + 0x74), &s);
    a17_w32(o, D_700038A0, A17_ADD(a17_u32(o, self + 0xB0), A17_MUL(K(7.0f), s)));
    a17_w32(o, D_700038A0 + 4, A17_ADD(K(7.0f), a17_u32(o, self + 0xB4)));
    (void)a17_c_0011DE90(o, a17_u32(o, blk + 0x74), &s);
    a17_w32(o, D_700038A0 + 8, A17_ADD(a17_u32(o, self + 0xB8), A17_MUL(K(7.0f), s)));
    if (!A17_EQ(a17_u32(o, self + 0x38), 0)) {
        r = 0;
        (void)a17_c_0019AD00(o, self, D_700038A0, (int32_t)0x80000007u, &r);
        if (r != 0) a17_w32(o, self + 0x38, 0);
    }
    uint32_t rise = A17_SUB(a17_u32(o, blk + 0x78), K(0.06f));
    a17_w32(o, blk + 0x78, rise);
    if (A17_LT(rise, K(-4.0f))) a17_w32(o, blk + 0x78, K(-4.0f));
    rise = a17_u32(o, blk + 0x78);
    a17_w32(o, self + 0xB4, A17_ADD(a17_u32(o, self + 0xB4), rise));
    if (a17_u8(o, self + 5) == 3) {
        const uint32_t shot = a17_u16(o, self + 0x36);
        if (shot != 0) {
            if (!(shot & 0x2000u)) {
                a17_burst(o, self);
                a17_w8(o, self, 2);
                return 1;
            }
            a17_w16(o, self + 0x36, 0);
        }
    }
    if (!A17_LT(a17_u32(o, blk + 0x78), 0)) return 1;
    a17_w32(o, D_700038A0, 0);
    a17_w32(o, D_700038A0 + 4, K(-10.0f));
    a17_w32(o, D_700038A0 + 8, 0);
    a17_w32(o, D_700038A0 + 12, K(1.0f));
    r = 0;
    (void)a17_c_0019AB20(o, self, self + 0xB0, D_700038A0, (int32_t)0x80000007u, &r);
    if (r != 0) {
        a17_burst(o, self);
        return 1;
    }
    const uint32_t tick = (uint32_t)a17_s16(o, D_70003B8A) + a17_u32(o, D_70003B68);
    if ((tick & 0x3Fu) == 0) {
        r = 0;
        (void)a17_c_001B0D80(o, self, &r);
        if (r != 0) return 1;
    }
    if (a17_u8(o, D_00810700) != 0x15) return 1;
    if (!A17_LT(a17_u32(o, self + 0xB4), K(5.0f))) return 1;
    a17_w32(o, D_700038A0, a17_u32(o, self + 0xB0));
    a17_w32(o, D_700038A0 + 4, K(10.0f));
    a17_w32(o, D_700038A0 + 8, a17_u32(o, self + 0xB8));
    a17_w32(o, D_700038A0 + 12, K(1.0f));
    (void)a17_c_001EFD20(o, (int32_t)0x8000005Fu, D_700038A0);
    (void)a17_c_001FBD50(o, self, 0xDB, 0, K(800.0f));
    a17_w16(o, self + 0x36, 0);
    a17_w8(o, self + 4, 3);
    a17_w8(o, self + 5, 0);
    return 1;
}

/* ----------------------------------------------------------------------
 * 0x825560 (decomp func_overlay_AREA17_00825520, NEARMISS: the original
 * words run in the oracle) — [5]..[8], the breakables. State 0: after
 * 001B0FD0(self) is 0: +0x34 = 1, +0 = 1, the block's +0x10 / +0x20 =
 * copies of +0xB0 / +0xC0 (00102948), 001C6380. State 1
 * (a17_825560_rest): a shot (+0x36) sets +0 / state 2 and, when
 * 0019A570 hits between +0xB0 +- 4 in y, effect 4 (001F0460) on the
 * matrix D_700036A0 (identity, 00102B08 by pi/2, translated to +0xB0, y +
 * 0.2); else D_00810805 >= 3 sets +0 2 and state 3; the method; with the
 * squared distance (001028D0 / 00102738, w 0) to the player at most 50 *
 * 50 (kept in 0x70003A20 / 0x70003A24) +1 = 1 and 001B1D20, else 001B17A0.
 * State 2 (a17_825560_fall), then 001C6380, 001A2370(self, +0xD0),
 * 001B17A0 and the method. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_825560(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 3) {
        (void)a17_c_001AFC10(o, self);
    } else if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        a17_w16(o, self + 0x34, 1);
        a17_w8(o, self, 1);
        (void)a17_c_00102948(o, self + 0x200, self + 0xB0);
        (void)a17_c_00102948(o, self + 0x210, self + 0xC0);
        (void)a17_c_001C6380(o, self);
    } else if (state == 1) {
        a17_825560_rest(o, self);
    } else if (state == 2) {
        if (!a17_825560_fall(o, self)) return;
        (void)a17_c_001C6380(o, self);
        (void)a17_c_001A2370(o, self, self + 0xD0);
        (void)a17_c_001B17A0(o, self, &r);
        (void)a17_method(o, self);
    }
}

int em_area17_overlay_00825560(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_825560(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825EA0 (decomp func_overlay_AREA17_00825E60) — [36] (counter 0x2F).
 * State 0: after 001B0FD0(self) is 0: 001C6380, +0 = 2 with flag 0x2F
 * else 1, +0x30 = 0x828E30, D_700038A0 = (0, 1, 0, 0.25), +0x2EC =
 * 001C5570(self, D_700038A0, 0x10, 0) and that child's +0xB4 less 0.4.
 * State 1: step 0: a talk (+0xB bit 2) sets +0 = 2, +0xB = 0, D_00810807 =
 * 2; D_00810807 == 3 sets step 1 and the halfword +0x28 = 1. While +0x28
 * is set (or D_00810787 is 0xFF) it counts, and past 139 (or with
 * D_00810787 == 0xFF) the child (if any) gets state 3, +0x2EC = 0, and
 * +0x28 = 0. While D_00810807 != 3: 001C6380, 001B17A0 (result unused),
 * the method. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_825EA0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 2 || state == 3) {
        (void)a17_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a17_c_001C6380(o, self);
        (void)a17_c_001BA1C0(o, self, 0x2F, &r);
        a17_w8(o, self, r != 0 ? 2 : 1);
        a17_w32(o, self + 0x30, A17_TABLE_828E30);
        a17_w32(o, D_700038A0, 0);
        a17_w32(o, D_700038A0 + 4, K(1.0f));
        a17_w32(o, D_700038A0 + 8, 0);
        a17_w32(o, D_700038A0 + 12, K(0.25f));
        uint32_t child = 0;
        (void)a17_c_001C5570(o, self, D_700038A0, 0x10, 0, &child);
        a17_w32(o, self + 0x2EC, child);
        const uint32_t c = a17_u32(o, self + 0x2EC);
        a17_w32(o, c + 0xB4, A17_SUB(a17_u32(o, c + 0xB4), K(0.4f)));
        return;
    }
    if (state != 1) return;
    if (a17_u8(o, self + 5) == 0) {
        if (a17_u8(o, self + 0xB) & 4) {
            a17_w8(o, self, 2);
            a17_w8(o, self + 0xB, 0);
            a17_w8(o, D_00810807, 2);
        }
        if (a17_u8(o, D_00810807) == 3) {
            a17_w8(o, self + 5, 1);
            a17_w16(o, self + 0x28, 1);
        }
    }
    if (a17_s16(o, self + 0x28) != 0 || a17_u8(o, D_00810787) == 0xFF) {
        a17_w16(o, self + 0x28, a17_u16(o, self + 0x28) + 1);
        if (a17_s16(o, self + 0x28) > 139 || a17_u8(o, D_00810787) == 0xFF) {
            const uint32_t child = a17_u32(o, self + 0x2EC);
            if (child != 0) {
                a17_w8(o, child + 4, 3);
                a17_w32(o, self + 0x2EC, 0);
            }
            a17_w16(o, self + 0x28, 0);
        }
    }
    if (a17_u8(o, D_00810807) != 3) {
        (void)a17_c_001C6380(o, self);
        (void)a17_c_001B17A0(o, self, &r);
        (void)a17_method(o, self);
    }
}

int em_area17_overlay_00825EA0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_825EA0(&o, a0);
    return a17_end(&o);
}
