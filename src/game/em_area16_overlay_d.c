/* AREA16's own overlay owners (em_area16_overlay.h), group d: 0x8273F0,
 * 0x828200, 0x827B70, 0x827CE0, 0x827FA0. Each function is one original
 * function; the comment above it says what the code does. The order of the
 * memory accesses between calls is the original's (the test compares it one
 * for one). Ground truth: the decomp's byte-identical C under
 * src/overlays/AREA16/; evidence: tools/test_area16_overlay_reference.py. */
#include "em_area16_overlay_internal.h"

/* globals */
#define D_00275B40 0x00275B40u   /* pointer table; entry 2 is the object whose +0x80 value moves */
#define D_0024A850 0x0024A850u   /* per area: halfword base row of the spawn table */
#define D_0024D820 0x0024D820u   /* per area: pointer to the spawn-record rows */
#define D_00810700 0x00810700u   /* the area */
#define D_00810701 0x00810701u   /* the sub */
#define D_00810789 0x00810789u
#define D_00810809 0x00810809u
#define D_0028A59C 0x0028A59Cu
#define D_700036A0 0x700036A0u   /* scratchpad matrices and vectors */
#define D_700036D0 0x700036D0u
#define D_700036E0 0x700036E0u
#define D_70003710 0x70003710u
#define D_70003720 0x70003720u
#define D_700038A0 0x700038A0u
#define D_700038B0 0x700038B0u
#define D_700038C0 0x700038C0u
#define D_700038D0 0x700038D0u
#define CHILD_BEHAVIOUR 0x00828200u /* the class 8 child's behaviour (this file) */

/* Copy a word (the original copies floats through a register, bit exact). */
static void a16_copy_word(A16 *o, uint32_t dst, uint32_t src)
{
    a16_w32(o, dst, a16_u32(o, src));
}

/* The class 8 child 0x8273F0 and 0x827B70 spawn (child `c`, parent `self`):
 * +3 = 0, +0x2E = 0, +0xD = kind (kind 0: read from the parent's +3, 6 or
 * 0x25 when it is 0x31), +0xE = 0xFF00, +0x56 = +0x54 = 0, +0xB0 = the
 * parent's, +0xB4 = the parent's +0xB4 - the parent's +0x2E0, +0xB8 and
 * +0xC0..+0xC8 the parent's, +0x10 = the behaviour 0x828200. */
static void a16_spawn_child(A16 *o, uint32_t self, uint32_t c, uint32_t kind)
{
    a16_w8(o, c + 3, 0);
    a16_w16(o, c + 0x2E, 0);
    if (kind == 0) kind = a16_u8(o, self + 3) != 0x31u ? 6u : 0x25u;
    a16_w8(o, c + 0xD, kind);
    a16_w16(o, c + 0xE, 0xFF00);
    a16_w16(o, c + 0x56, 0);
    a16_w16(o, c + 0x54, 0);
    a16_copy_word(o, c + 0xB0, self + 0xB0);
    {
        const uint32_t lower = a16_u32(o, self + 0x2E0);
        const uint32_t y = a16_u32(o, self + 0xB4);
        a16_w32(o, c + 0xB4, A16_SUB(y, lower));
    }
    a16_copy_word(o, c + 0xB8, self + 0xB8);
    a16_copy_word(o, c + 0xC0, self + 0xC0);
    a16_copy_word(o, c + 0xC4, self + 0xC4);
    a16_copy_word(o, c + 0xC8, self + 0xC8);
    a16_w32(o, c + 0x10, CHILD_BEHAVIOUR);
}

/* The +0x80 value's object: D_00275B40[2]. */
static uint32_t a16_lift_obj(A16 *o)
{
    const uint32_t t = a16_u32(o, D_00275B40);
    return a16_u32(o, t + 8);
}

/* One of the three effect vectors written to D_700038A0 (x, y, z, 1.0). */
static void a16_effect_vec(A16 *o, uint32_t x, uint32_t z)
{
    a16_w32(o, D_700038A0, x);
    a16_w32(o, D_700038A0 + 4, 0x40666666u); /* 3.6 */
    a16_w32(o, D_700038A0 + 8, z);
    a16_w32(o, D_700038A0 + 12, 0x3F800000u);
}

/* ----------------------------------------------------------------------
 * 0x8273F0 (decomp func_overlay_AREA16_008273B0) — sub 0 [9], sub 1 x15:
 * the lift-like platform. State 0: +0x2E0 = 35.5 (25.5 when +3 is 0x31),
 * +0x2DC = +0x2E0 / 200.0, +0x2D8 = 0; when 001B0FD0 is 0: +0 = 1, +0x2E8 =
 * the +0x80 value of D_00275B40[2]; with D_00810809 above 1: +0x2EC = 0 and
 * +0x2D4 = 0; else with bit 0 of +0xE: +0x2EC = 0; else +0x2EC = 1 and the
 * +0x80 value = +0x2E8 - +0x2E0. Then 001C6380, 001A2370(self,
 * D_00275B40[2] + 0x90) and, with +0x2EC, a class 8 child (behaviour
 * 0x828200, +0xD 6, or 0x25 for +3 == 0x31) kept in +0x2D8.
 * State 1, sub-state 0: when +0x2E4 is set it counts down; at 0 or below, or
 * with D_00810789 == 0xFF: +0x2E4 = 0, sub-state 1, +0x28 = 0 and, for +0x56
 * == 0, sound 0x452 (not for 0x31) and three class 0x80000020 effects around
 * self + 0xD0 at offsets (0, 3.6, 8), (8, 3.6, 0) and (-8, 3.6, 0).
 * Sub-state 1: +0x28 counts; below 200 (and D_00810789 != 0xFF) the +0x80
 * value (and the child's +0xB4) moves by +0x2DC (the child by +0x2DC too) per
 * frame, up with +0x2EC and down without, then 001C6380 and 001A2370 as in
 * state 0; otherwise +0x2EC toggles and the end values are set (with +0x2EC
 * set: the +0x80 value = +0x2E8 - +0x2E0, the child's +0xB4 = +0xB4 - +0x2E0;
 * clear: +0x2D4 = +0x52 * 12 + 0xF0, the +0x80 value = +0x2E8, the child's
 * +0xB4 = +0xB4), sub-state 0, 001C6380 and 001A2370.
 * Then (any sub-state) when +0x2D4 is set it counts down only while
 * D_00810809 == 0xFF; at 0 with +0x2EC clear and a child: the child's state =
 * 3, +0x2D8 = 0, the position is copied to D_700038B0, sound 0x453 and a class
 * 0x80000077 effect 20.0 (15.0 for 0x31) above it. Then 001B1B70 and the
 * +0x4C method. State 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_8273F0(A16 *o, uint32_t self)
{
    int32_t r = 0;
    uint32_t u = 0;
    const uint32_t state = a16_u8(o, self + 4);
    if (state == 3) {
        (void)a16_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        if (a16_u8(o, self + 3) != 0x31u) a16_w32(o, self + 0x2E0, 0x420E0000u); /* 35.5 */
        else a16_w32(o, self + 0x2E0, 0x41CC0000u);                              /* 25.5 */
        const uint32_t step = A16_DIV(a16_u32(o, self + 0x2E0), 0x43480000u);      /* / 200.0 */
        a16_w32(o, self + 0x2DC, step);
        a16_w32(o, self + 0x2D8, 0);
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r != 0) return;
        a16_w8(o, self, 1);
        a16_w32(o, self + 0x2E8, a16_u32(o, a16_lift_obj(o) + 0x80));
        if (a16_u8(o, D_00810809) > 1u) {
            a16_w32(o, self + 0x2EC, 0);
            a16_w32(o, self + 0x2D4, 0);
        } else if (a16_u16(o, self + 0xE) & 1u) {
            a16_w32(o, self + 0x2EC, 0);
        } else {
            a16_w32(o, self + 0x2EC, 1);
            const uint32_t t = a16_u32(o, D_00275B40);
            const uint32_t hi = a16_u32(o, self + 0x2E8);
            const uint32_t lo = a16_u32(o, self + 0x2E0);
            const uint32_t obj = a16_u32(o, t + 8);
            a16_w32(o, obj + 0x80, A16_SUB(hi, lo));
        }
        if (a16_c_001C6380(o, self) < 0) return;
        if (a16_c_001A2370(o, self, a16_lift_obj(o) + 0x90) < 0) return;
        if (a16_u32(o, self + 0x2EC) == 0) return;
        u = 0;
        if (a16_c_001AFA90(o, 8, &u) < 0) return;
        if (u == 0) return;
        a16_w32(o, self + 0x2D8, u);
        a16_spawn_child(o, self, u, 0);
        return;
    }
    if (state != 1) return;

    const uint32_t sub = a16_u8(o, self + 5);
    if (sub == 0) {
        if (a16_u32(o, self + 0x2E4) != 0) {
            a16_w32(o, self + 0x2E4, a16_u32(o, self + 0x2E4) - 1u);
            const int32_t left = (int32_t)a16_u32(o, self + 0x2E4);
            if (left <= 0 || a16_u8(o, D_00810789) == 0xFFu) {
                a16_w32(o, self + 0x2E4, 0);
                a16_w8(o, self + 5, 1);
                a16_w16(o, self + 0x28, 0);
                if (a16_s16(o, self + 0x56) == 0) {
                    if (a16_u8(o, self + 3) != 0x31u)
                        if (a16_c_001FBD50(o, self, 0x452, 0, 0x43960000u) < 0) return; /* 300.0 */
                    (void)a16_c_00102958(o, D_700036A0, self + 0xD0);
                    a16_effect_vec(o, 0, 0x41000000u);                                  /* (0, 3.6, 8) */
                    (void)a16_c_001026A0(o, D_700038B0, D_700036A0, D_700038A0);
                    a16_effect_vec(o, 0x41000000u, 0);                                  /* (8, 3.6, 0) */
                    (void)a16_c_001026A0(o, D_700038C0, D_700036A0, D_700038A0);
                    a16_effect_vec(o, 0xC1000000u, 0);                                  /* (-8, 3.6, 0) */
                    (void)a16_c_001026A0(o, D_700038D0, D_700036A0, D_700038A0);
                    (void)a16_c_001029C0(o, D_70003720);
                    (void)a16_c_00102B08(o, D_70003720, D_70003720, 0xBF9C61ABu);      /* -1.2217306 */
                    (void)a16_c_001026D0(o, D_700036A0, D_700036A0, D_70003720);
                    (void)a16_c_00102948(o, D_700036D0, D_700038B0);
                    (void)a16_c_001EFEB0(o, 0x80000020, D_700036A0, &u);
                    (void)a16_c_001029C0(o, D_70003720);
                    (void)a16_c_00102BB0(o, D_70003720, D_70003720, 0x3FC90FDBu);      /* pi / 2 */
                    (void)a16_c_001026D0(o, D_700036E0, D_70003720, D_700036A0);
                    (void)a16_c_00102948(o, D_70003710, D_700038C0);
                    (void)a16_c_001EFEB0(o, 0x80000020, D_700036E0, &u);
                    (void)a16_c_001029C0(o, D_70003720);
                    (void)a16_c_00102BB0(o, D_70003720, D_70003720, 0xBFC90FDBu);      /* -pi / 2 */
                    (void)a16_c_001026D0(o, D_700036E0, D_70003720, D_700036A0);
                    (void)a16_c_00102948(o, D_70003710, D_700038D0);
                    (void)a16_c_001EFEB0(o, 0x80000020, D_700036E0, &u);
                }
            }
        }
    } else if (sub == 1) {
        a16_w16(o, self + 0x28, (uint32_t)a16_s16(o, self + 0x28) + 1u);
        if (a16_s16(o, self + 0x28) < 200 && a16_u8(o, D_00810789) != 0xFFu) {
            const int up = a16_u32(o, self + 0x2EC) != 0;
            const uint32_t step = a16_u32(o, self + 0x2DC);
            const uint32_t obj = a16_lift_obj(o);
            const uint32_t v = a16_u32(o, obj + 0x80);
            a16_w32(o, obj + 0x80, up ? A16_ADD(v, step) : A16_SUB(v, step));
            const uint32_t c = a16_u32(o, self + 0x2D8);
            if (c != 0) {
                const uint32_t s = a16_u32(o, self + 0x2DC);
                const uint32_t y = a16_u32(o, c + 0xB4);
                a16_w32(o, c + 0xB4, up ? A16_ADD(y, s) : A16_SUB(y, s));
            }
        } else {
            a16_w32(o, self + 0x2EC, a16_u32(o, self + 0x2EC) == 0 ? 1u : 0u);
            if (a16_u32(o, self + 0x2EC) != 0) {
                const uint32_t hi = a16_u32(o, self + 0x2E8);
                const uint32_t lo = a16_u32(o, self + 0x2E0);
                const uint32_t obj = a16_lift_obj(o);
                a16_w32(o, obj + 0x80, A16_SUB(hi, lo));
                const uint32_t c = a16_u32(o, self + 0x2D8);
                if (c != 0) {
                    const uint32_t lo2 = a16_u32(o, self + 0x2E0);
                    const uint32_t y = a16_u32(o, self + 0xB4);
                    a16_w32(o, c + 0xB4, A16_SUB(y, lo2));
                }
            } else {
                a16_w32(o, self + 0x2D4, a16_u16(o, self + 0x52) * 12u + 0xF0u);
                const uint32_t t = a16_u32(o, D_00275B40);
                const uint32_t hi = a16_u32(o, self + 0x2E8);
                const uint32_t obj = a16_u32(o, t + 8);
                a16_w32(o, obj + 0x80, hi);
                const uint32_t c = a16_u32(o, self + 0x2D8);
                if (c != 0) a16_w32(o, c + 0xB4, a16_u32(o, self + 0xB4));
            }
            a16_w8(o, self + 5, 0);
        }
        if (a16_c_001C6380(o, self) < 0) return;
        if (a16_c_001A2370(o, self, a16_lift_obj(o) + 0x90) < 0) return;
    }

    if (a16_u32(o, self + 0x2D4) != 0) {
        if (a16_u8(o, D_00810809) == 0xFFu) a16_w32(o, self + 0x2D4, a16_u32(o, self + 0x2D4) - 1u);
        if (a16_u32(o, self + 0x2D4) == 0 && a16_u32(o, self + 0x2EC) == 0) {
            const uint32_t c = a16_u32(o, self + 0x2D8);
            if (c != 0) {
                a16_w8(o, c + 4, 3);
                a16_w32(o, self + 0x2D8, 0);
                (void)a16_c_00102948(o, D_700038B0, self + 0xB0);
                if (a16_c_001FBD50(o, self, 0x453, 0, 0x43960000u) < 0) return;       /* 300.0 */
                const uint32_t add = a16_u8(o, self + 3) != 0x31u ? 0x41A00000u : 0x41700000u; /* 20.0 / 15.0 */
                a16_w32(o, D_700038B0 + 4, A16_ADD(a16_u32(o, D_700038B0 + 4), add));
                (void)a16_c_001EFD20(o, (int32_t)0x80000077, D_700038B0, &u);
            }
        }
    }
    if (a16_c_001B1B70(o, self) < 0) return;
    (void)a16_method(o, self);
}

int em_area16_overlay_008273F0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_8273F0(&o, a0);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x827B70 (decomp func_overlay_AREA16_00827B30) — sub 1 owner. State 0:
 * when 001B0FD0 is 0: 001C6380 and a class 8 child (+0xD 0x14, behaviour
 * 0x828200) at the position lowered by +0x2E0. State 1: +0x28 counts; above
 * 200 the random value 00122BB8's high half doubled and shifted (non-zero when
 * it is 0x4000 or more, or negative) plays sound 0x451
 * (500.0), then +0x28 = 0; then 001B1B70 and the +0x4C method. States 2 and
 * 3: 001AFC10. Other states: nothing.
 * ---------------------------------------------------------------------- */
static void a16_827B70(A16 *o, uint32_t self)
{
    int32_t r = 0;
    uint32_t u = 0;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r != 0) return;
        if (a16_c_001C6380(o, self) < 0) return;
        u = 0;
        if (a16_c_001AFA90(o, 8, &u) < 0) return;
        if (u == 0) return;
        a16_spawn_child(o, self, u, 0x14);
        break;
    case 1:
        a16_w16(o, self + 0x28, (uint32_t)a16_s16(o, self + 0x28) + 1u);
        if (a16_s16(o, self + 0x28) > 200) {
            int32_t rnd = 0;
            if (a16_c_00122BB8(o, &rnd) < 0) return;
            int32_t t = a16_sra((uint32_t)rnd, 16);
            t = (int32_t)((uint32_t)t << 1);
            t = a16_sra((uint32_t)t, 15);
            if (t != 0)
                if (a16_c_001FBD50(o, self, 0x451, 0, 0x43FA0000u) < 0) return; /* 500.0 */
            a16_w16(o, self + 0x28, 0);
        }
        if (a16_c_001B1B70(o, self) < 0) return;
        (void)a16_method(o, self);
        break;
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00827B70(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_827B70(&o, a0);
    return a16_end(&o);
}

/* The follower's anchor (shared by 0x827CE0 and 0x827FA0 state 0): walk the
 * +0x18 chain +0x54 times from self into +0x1F8, +0x1F0 = -(10.0 + the
 * anchor's +0x2E0), position (+0xB0) = the anchor's +0x118 object's +0xC0
 * vector, +0xB4 += +0x1F0. */
static void a16_follow_anchor(A16 *o, uint32_t self)
{
    uint32_t p = self;
    const int32_t n = a16_s16(o, self + 0x54);
    for (int32_t i = 0; i < n; i++) {
        p = a16_u32(o, p + 0x18);
        if (a16_failed(o)) return;
    }
    a16_w32(o, self + 0x1F8, p);
    const uint32_t a = a16_u32(o, self + 0x1F8);
    const uint32_t lift = a16_u32(o, a + 0x2E0);
    a16_w32(o, self + 0x1F0, A16_NEG(A16_ADD(0x41200000u, lift)));
    const uint32_t a2 = a16_u32(o, self + 0x1F8);
    const uint32_t obj = a16_u32(o, a2 + 0x118);
    (void)a16_c_00102948(o, self + 0xB0, obj + 0xC0);
    const uint32_t off = a16_u32(o, self + 0x1F0);
    const uint32_t y = a16_u32(o, self + 0xB4);
    a16_w32(o, self + 0xB4, A16_ADD(y, off));
}

/* State 1's follow step and spawn (shared by 0x827CE0 and 0x827FA0): +0xB4 =
 * the anchor's +0x118 object's +0xC4 + +0x1F0; when the anchor's +0x2EC and
 * +0x2D4 are both clear: with +0x56 >= 0 the record (D_0024D820[area] row
 * D_0024A850[area] (1 when 0) + +0x56, entry +0xE of 44 bytes) spawns unless
 * 001B11E0(record id) says it exists (copying the record into the new
 * object and the position/velocity words), and the state becomes 3 (also with
 * +0x56 < 0). Returns 1 when the spawn block ran (state set to 3). */
static void a16_follow_spawn(A16 *o, uint32_t self, int *tail)
{
    int32_t r = 0;
    uint32_t u = 0;
    *tail = 0;
    {
        const uint32_t a = a16_u32(o, self + 0x1F8);
        const uint32_t off = a16_u32(o, self + 0x1F0);
        const uint32_t obj = a16_u32(o, a + 0x118);
        const uint32_t y = a16_u32(o, obj + 0xC4);
        a16_w32(o, self + 0xB4, A16_ADD(y, off));
    }
    const uint32_t p = a16_u32(o, self + 0x1F8);
    if (a16_u32(o, p + 0x2EC) != 0) return;
    if (a16_u32(o, p + 0x2D4) != 0) return;
    *tail = 1;
    const int32_t idx = a16_s16(o, self + 0x56);
    if (idx >= 0) {
        const uint32_t d = a16_u8(o, D_00810700);
        uint32_t k = (uint32_t)a16_s16(o, D_0024A850 + 2u * d);
        if (k == 0) k = 1;
        k += (uint32_t)idx;
        const uint32_t col = a16_u8(o, self + 0xE);
        const uint32_t row = a16_u32(o, D_0024D820 + 4u * d);
        const uint32_t e = a16_u32(o, row + 4u * k) + col * 44u;
        r = 0;
        if (a16_c_001B11E0(o, (int32_t)a16_u8(o, e + 2), &r) < 0) return;
        if (r == 0) {
            u = 0;
            if (a16_c_001AFA90(o, (int32_t)a16_u8(o, e + 4), &u) < 0) return;
            if (u != 0) {
                a16_w8(o, u + 0x9A, a16_u8(o, e + 2));
                a16_w8(o, u + 3, a16_u8(o, e + 6));
                a16_w16(o, u + 0x2E, ((uint32_t)a16_s16(o, e + 6) >> 8) & 0xFFu);
                a16_w8(o, u + 0xD, a16_u8(o, e + 8));
                if (((uint32_t)a16_s16(o, e + 4) & 0xFFFFFF1Fu) == 2u) {
                    a16_w8(o, u + 0x9D, a16_u8(o, D_00810701));
                    a16_w8(o, u + 0x9E, a16_u8(o, e + 0xA));
                } else {
                    a16_w16(o, u + 0xE, a16_u16(o, e + 0xA));
                }
                a16_w16(o, u + 0x54, a16_u16(o, e + 0xC));
                a16_w16(o, u + 0x56, a16_u16(o, e + 0xE));
                a16_copy_word(o, u + 0xB0, self + 0xB0);
                a16_copy_word(o, u + 0xB4, self + 0xB4);
                a16_copy_word(o, u + 0xB8, self + 0xB8);
                a16_copy_word(o, u + 0xC0, self + 0xC0);
                a16_copy_word(o, u + 0xC4, self + 0xC4);
                a16_copy_word(o, u + 0xC8, self + 0xC8);
                a16_w32(o, u + 0x10, a16_u32(o, e + 0x28));
            }
        }
    }
    a16_w8(o, self + 4, 3);
}

/* ----------------------------------------------------------------------
 * 0x827CE0 (decomp func_overlay_AREA16_00827CA0) — sub 1 owner x2: a model
 * follower. State 0: the anchor walk (a16_follow_anchor), +0x68 = +0x60 =
 * 0.8 and, when 001B10B0(self, 0x74, 0x71) is 0: 001C63E0(self, 5), state 1.
 * State 1: follow and spawn (a16_follow_spawn), then 001C68C0 and, when
 * 001B17A0 is non-zero, the +0x4C method. State 3 and other states:
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_827CE0(A16 *o, uint32_t self)
{
    int32_t r = 0;
    int tail = 0;
    const uint32_t state = a16_u8(o, self + 4);
    if (state == 3) {
        (void)a16_c_001AFC10(o, self);
        return;
    }
    if (state == 1) {
        a16_follow_spawn(o, self, &tail);
        if (a16_failed(o)) return;
        if (a16_c_001C68C0(o, self) < 0) return;
        r = 0;
        if (a16_c_001B17A0(o, self, &r) < 0) return;
        if (r == 0) return;
        (void)a16_method(o, self);
        return;
    }
    if (state == 0) {
        a16_follow_anchor(o, self);
        if (a16_failed(o)) return;
        a16_w32(o, self + 0x68, 0x3F4CCCCDu);
        a16_w32(o, self + 0x60, 0x3F4CCCCDu);
        r = 0;
        if (a16_c_001B10B0(o, self, 0x74, 0x71, &r) < 0) return;
        if (r != 0) return;
        if (a16_c_001C63E0(o, self, 5) < 0) return;
        a16_w8(o, self + 4, 1);
        return;
    }
    (void)a16_c_001AFC10(o, self);
}

int em_area16_overlay_00827CE0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_827CE0(&o, a0);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x827FA0 (decomp func_overlay_AREA16_00827F60) — sub 1 owner x5: the same
 * follower without the model setup and the animation. State 0: the anchor
 * walk, state 1. State 1: follow and spawn, state 3 when the spawn block ran.
 * State 3 and other states: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_827FA0(A16 *o, uint32_t self)
{
    int tail = 0;
    const uint32_t state = a16_u8(o, self + 4);
    if (state == 0) {
        a16_follow_anchor(o, self);
        if (a16_failed(o)) return;
        a16_w8(o, self + 4, 1);
    } else if (state == 1) {
        a16_follow_spawn(o, self, &tail);
    } else {
        (void)a16_c_001AFC10(o, self);
    }
}

int em_area16_overlay_00827FA0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_827FA0(&o, a0);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x828200 (decomp func_overlay_AREA16_008281C0) — the class 8 child
 * behaviour. State 0: state 3 when the area and sub are 0x10 and 0x00; else
 * when 001B0FD0 is 0: 001CA5E0(self, +0x44, 1) and 001C6380. State 1:
 * 001C6380; the position (+0xB0) is saved to D_700038A0 and the matrix
 * (+0xD0) to D_700036A0; by +0xD (6, 0x25: 4 parts; 0x14: 16 parts) each
 * part sets +0xB0 = +0xD0 matrix applied to a table point (0x82C2B0, 0x82C300
 * or 0x82C380, 16 bytes each), the model from 001C6120(D_0028A59C, the id
 * from 0x82C2A0, 0x82C2F0 or 0x82C340) with 001CA5E0, resets the +0x110
 * matrix, 001C6380, 001B17A0 and the +0x4C method, then restores the
 * position and the matrix. Another +0xD has the original take its part
 * count from its caller's register: fail-stop. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_828200(A16 *o, uint32_t self)
{
    int32_t r = 0;
    switch (a16_u8(o, self + 4)) {
    case 0: {
        const uint32_t area = a16_u8(o, D_00810700);
        const uint32_t sub = a16_u8(o, D_00810701);
        if (((area << 8) + sub) == 0x1000u) {
            a16_w8(o, self + 4, 3);
            break;
        }
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r == 0) {
            if (a16_c_001CA5E0(o, self, (int32_t)a16_u32(o, self + 0x44), 1) < 0) return;
            (void)a16_c_001C6380(o, self);
        }
        break;
    }
    case 1: {
        uint32_t ids = 0, pts = 0;
        int32_t n = 0;
        if (a16_c_001C6380(o, self) < 0) return;
        (void)a16_c_00102948(o, D_700038A0, self + 0xB0);
        (void)a16_c_00102958(o, D_700036A0, self + 0xD0);
        const uint32_t kind = a16_u8(o, self + 0xD);
        if (kind == 0x14) {
            ids = 0x0082C340u;
            n = 16;
            pts = 0x0082C380u;
        } else if (kind == 0x25) {
            ids = 0x0082C2F0u;
            n = 4;
            pts = 0x0082C300u;
        } else if (kind == 6) {
            ids = 0x0082C2A0u;
            n = 4;
            pts = 0x0082C2B0u;
        } else {
            /* the original's part count here is its caller's register */
            if (!a16_failed(o)) (void)a16_latch(o, 0x00828200u, EM_AREA16_OVERLAY_FAULT_CALLER_REGISTER);
            return;
        }
        for (int32_t i = 0; i < n; i++) {
            if (a16_failed(o)) return;
            (void)a16_c_001026A0(o, self + 0xB0, self + 0xD0, pts);
            const int32_t id = a16_s32(o, ids);
            const int32_t model_base = a16_s32(o, D_0028A59C);
            r = 0;
            if (a16_c_001C6120(o, model_base, id, &r) < 0) return;
            (void)a16_c_001CA5E0(o, self, r, 1);
            (void)a16_c_001029C0(o, a16_u32(o, self + 0x110));
            (void)a16_c_001C6380(o, self);
            r = 0;
            (void)a16_c_001B17A0(o, self, &r);
            (void)a16_method(o, self);
            (void)a16_c_00102948(o, self + 0xB0, D_700038A0);
            (void)a16_c_00102958(o, self + 0xD0, D_700036A0);
            ids += 4;
            pts += 16;
        }
        break;
    }
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00828200(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_828200(&o, a0);
    return a16_end(&o);
}
