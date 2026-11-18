/* AREA16's own overlay owners (em_area16_overlay.h), group b: the sub-1
 * placement owners 0x824440 and 0x824690, the group owner 0x824E10, the
 * child behaviour 0x825260, the script op09 callback 0x825630 and the
 * helpers 0x824540, 0x825660, 0x825670, 0x8256E0, 0x825730, 0x825780. Each
 * function is one original function; the comment above it says what the
 * code does. The order of the memory accesses between calls is the
 * original's (the test compares it one for one). Ground truth: the decomp's
 * C under src/overlays/AREA16/ (byte-identical, but 0x824690 is NEARMISS:
 * its original instructions settle the order); evidence:
 * tools/test_area16_overlay_reference.py. */
#include "em_area16_overlay_internal.h"

/* overlay data (the addresses the original code holds) */
#define A16_AREA_829600 0x00829600u    /* 0x824540's six-point area (0x60 bytes), copied to its frame */
#define A16_SCRIPT_829340 0x00829340u  /* 0x824540's script */
#define A16_SCRIPT_829970 0x00829970u  /* 0x824690's script (op09 callback 0x825630) */
#define A16_POINTS_82C480 0x0082C480u  /* 0x825670's points, 16 bytes each */
#define A16_TABLE_82C4A0 0x0082C4A0u   /* 0x8256E0's 001D7FA0 table */
#define A16_CHILD_825260 0x00825260u   /* the behaviour 0x824690 gives its class 9 spawn */
#define A16_BOOT_1C4820 0x001C4820u    /* the behaviour 0x824690 gives its class 4 spawn */

/* globals */
#define D_0024D7C0 0x0024D7C0u   /* per area, per sub: the 0x28-byte spawn records */
#define D_00810350 0x00810350u   /* the player's position (x, y, z, w) */
#define D_00810354 0x00810354u   /* its y */
#define D_008104C4 0x008104C4u   /* the object the player talks to */
#define D_00810700 0x00810700u   /* the area */
#define D_00810701 0x00810701u   /* the sub */
#define D_00810789 0x00810789u
#define D_00810809 0x00810809u
#define D_70003B88 0x70003B88u   /* scratchpad loop counter (halfword) */
#define D_70003B92 0x70003B92u
#define D_700038A0 0x700038A0u   /* scratchpad vector (four floats) */
#define D_700038B0 0x700038B0u   /* scratchpad colour (four words) */

/* float constants (bit patterns) */
#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_QUARTER 0x3E800000u
#define F_8 0x41000000u
#define F_195 0x43430000u

/* The halfword at +off of the 0x28-byte record `rec` of the current area's
 * and sub's table (D_0024D7C0[area][sub]); reads area, self+0x9A, sub, the
 * per-area word, the per-sub word, the halfword. */
static uint32_t a16b_record_half(A16 *o, uint32_t self, uint32_t off)
{
    const uint32_t area = a16_u8(o, D_00810700);
    const uint32_t rec = a16_u8(o, self + 0x9A);
    const uint32_t sub = a16_u8(o, D_00810701);
    const uint32_t per_area = a16_u32(o, D_0024D7C0 + 4u * area);
    const uint32_t table = a16_u32(o, per_area + 4u * sub);
    return a16_u16(o, table + 0x28u * rec + off);
}

/* D_700038A0 = (x, y, z, w). */
static void a16b_vec(A16 *o, uint32_t x, uint32_t y, uint32_t z, uint32_t w)
{
    a16_w32(o, D_700038A0, x);
    a16_w32(o, D_700038A0 + 4, y);
    a16_w32(o, D_700038A0 + 8, z);
    a16_w32(o, D_700038A0 + 12, w);
}

/* The marker colour D_700038B0 = (0x80, 0x80, 0x66, 0x80), then
 * 001F4E20(D_700038A0, D_700038B0, 8.0). */
static void a16b_marker_draw(A16 *o)
{
    a16_w32(o, D_700038B0, 0x80);
    a16_w32(o, D_700038B0 + 4, 0x80);
    a16_w32(o, D_700038B0 + 8, 0x66);
    a16_w32(o, D_700038B0 + 12, 0x80);
    (void)a16_c_001F4E20(o, D_700038A0, D_700038B0, F_8);
}

/* ----------------------------------------------------------------------
 * 0x825660 (decomp func_overlay_AREA16_00825620): +0x2E0 = -1.
 * ---------------------------------------------------------------------- */
static void a16_825660(A16 *o, uint32_t self) { a16_w32(o, self + 0x2E0, 0xFFFFFFFFu); }

int em_area16_overlay_00825660(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825660(&o, a0);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825670 (decomp func_overlay_AREA16_00825630, first function):
 * 001026A0(out, M + 0x90, 0x82C480 + 16 * idx), M the +0x12C object when
 * idx is 1, else the +0x110 object.
 * ---------------------------------------------------------------------- */
static void a16_825670(A16 *o, uint32_t self, uint32_t out, int32_t idx)
{
    const uint32_t m = a16_u32(o, self + (idx == 1 ? 0x12Cu : 0x110u));
    (void)a16_c_001026A0(o, out, m + 0x90u, A16_POINTS_82C480 + 16u * (uint32_t)idx);
}

int em_area16_overlay_00825670(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, int32_t a2, uint32_t sp,
                               EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825670(&o, a0, a1, a2);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8256E0 (decomp func_overlay_AREA16_00825630, second function; link
 * 008256A0): 0x825670(self, v, idx) into a frame vector (sp - 0x10), then
 * +0x2E0 = 001D7FA0(v, 0x82C4A0, 2, 1.0, 0.0).
 * ---------------------------------------------------------------------- */
static void a16_8256E0(A16 *o, uint32_t self, int32_t idx, uint32_t sp)
{
    const uint32_t v = sp - 0x10u;
    int32_t r = 0;
    a16_825670(o, self, v, idx);
    if (a16_c_001D7FA0(o, v, A16_TABLE_82C4A0, 2, F_ONE, F_ZERO, &r) < 0) return;
    a16_w32(o, self + 0x2E0, (uint32_t)r);
}

int em_area16_overlay_008256E0(const EmArea16OverlayHooks *h, uint32_t a0, int32_t a1, uint32_t sp,
                               EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_8256E0(&o, a0, a1, sp);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825730 (decomp func_overlay_AREA16_008256F0, first function): when the
 * +0x2E0 handle is not -1, 001D80B0(handle) and +0x2E0 = -1.
 * ---------------------------------------------------------------------- */
static void a16_825730(A16 *o, uint32_t self)
{
    const uint32_t handle = a16_u32(o, self + 0x2E0);
    if (handle == 0xFFFFFFFFu) return;
    if (a16_c_001D80B0(o, (int32_t)handle) < 0) return;
    a16_w32(o, self + 0x2E0, 0xFFFFFFFFu);
}

int em_area16_overlay_00825730(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825730(&o, a0);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825780 (decomp func_overlay_AREA16_008256F0, second function; link
 * 00825740): when the +0x2E0 handle is not -1: p = 001D8060(handle); p 0
 * sets +0x2E0 = -1, else 0x825670(self, p + 0x10, idx).
 * ---------------------------------------------------------------------- */
static void a16_825780(A16 *o, uint32_t self, int32_t idx)
{
    const uint32_t handle = a16_u32(o, self + 0x2E0);
    uint32_t p = 0;
    if (handle == 0xFFFFFFFFu) return;
    if (a16_c_001D8060(o, (int32_t)handle, &p) < 0) return;
    if (p == 0)
        a16_w32(o, self + 0x2E0, 0xFFFFFFFFu);
    else
        a16_825670(o, self, p + 0x10u, idx);
}

int em_area16_overlay_00825780(const EmArea16OverlayHooks *h, uint32_t a0, int32_t a1, uint32_t sp,
                               EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825780(&o, a0, a1);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x824540 (decomp func_overlay_AREA16_00824500) — every call first copies
 * the six-point area 0x829600 (0x60 bytes) into its frame (sp - 0x60).
 * Sub-state +5 0: when the player's y is above 195 and 001B1EA0(0, the
 * player, the copy, 6): script 0x829340 on self + 0x1F0, +5 = 1. +5 1: at
 * the script's end (001BA1F0) D_00810809 = 1, +0x2E = 0xFFFF,
 * 001C4760(0x1D, 1), +5 = 0. Then 001B1B70 and the +0x4C method when
 * D_00810809 is 0 or D_70003B92 is set.
 * ---------------------------------------------------------------------- */
static void a16_824540(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t area = sp - 0x60u;
    uint64_t q[8];
    int32_t r;

    /* each quadword move is two doubleword accesses */
    for (unsigned k = 0; k < 8; ++k) q[k] = a16_u64(o, A16_AREA_829600 + 8 * k);
    for (unsigned k = 0; k < 8; ++k) a16_w64(o, area + 8 * k, q[k]);
    for (unsigned k = 0; k < 4; ++k) q[k] = a16_u64(o, A16_AREA_829600 + 0x40 + 8 * k);
    for (unsigned k = 0; k < 4; ++k) a16_w64(o, area + 0x40 + 8 * k, q[k]);

    const uint32_t step = a16_u8(o, self + 5);
    if (step == 0) {
        if (!A16_LE(a16_u32(o, D_00810354), F_195)) {
            r = 0;
            if (a16_c_001B1EA0(o, 0, D_00810350, area, 6, &r) < 0) return;
            if (r != 0) {
                (void)a16_c_001BA1A0(o, self + 0x1F0, A16_SCRIPT_829340);
                a16_w8(o, self + 5, 1);
            }
        }
    } else if (step == 1) {
        r = 0;
        if (a16_c_001BA1F0(o, self, &r) < 0) return;
        if (r != 0) {
            a16_w8(o, D_00810809, 1);
            a16_w16(o, self + 0x2E, 0xFFFF);
            (void)a16_c_001C4760(o, 0x1D, 1);
            a16_w8(o, self + 5, 0);
        }
    }
    if (a16_u8(o, D_00810809) == 0 || a16_u8(o, D_70003B92) != 0) {
        (void)a16_c_001B1B70(o, self);
        (void)a16_method(o, self);
    }
}

int em_area16_overlay_00824540(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_824540(&o, a0, sp);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x824440 (decomp func_overlay_AREA16_00824400) — sub 1 placement. State
 * 0: state 3 when 001BA1C0(self, 0x31); else when 001B0FD0 is 0:
 * 001C6380, 0x825660, state 1, +0 = 1. State 1 by D_00810809: 0 runs
 * 0x824540 (its frame below this one's 0x20 bytes), 0xFF state 3, others
 * nothing. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_824440(A16 *o, uint32_t self, uint32_t sp)
{
    int32_t r;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001BA1C0(o, self, 0x31, &r) < 0) return;
        if (r != 0) {
            a16_w8(o, self + 4, 3);
            return;
        }
        r = 0;
        if (a16_c_001B0FD0(o, self, &r) < 0) return;
        if (r == 0) {
            (void)a16_c_001C6380(o, self);
            a16_825660(o, self);
            a16_w8(o, self + 4, 1);
            a16_w8(o, self, 1);
        }
        break;
    case 1:
        switch (a16_u8(o, D_00810809)) {
        case 0:
            a16_824540(o, self, sp - 0x20u);
            break;
        case 0xFF:
            a16_w8(o, self + 4, 3);
            break;
        default:
            break;
        }
        break;
    case 2:
    case 3:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00824440(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_824440(&o, a0, sp);
    return a16_end(&o);
}

/* The colour of an attachment: when the object at self+slot is set, its
 * +0xA0 = rgb, and (each through a fresh read of self+slot) +0xA4 = rgb,
 * +0xA8 = rgb, +0xAC = alpha. */
static void a16b_colour(A16 *o, uint32_t self, uint32_t slot, uint32_t rgb, uint32_t alpha)
{
    const uint32_t c = a16_u32(o, self + slot);
    if (c == 0) return;
    a16_w32(o, c + 0xA0, rgb);
    a16_w32(o, a16_u32(o, self + slot) + 0xA4, rgb);
    a16_w32(o, a16_u32(o, self + slot) + 0xA8, rgb);
    a16_w32(o, a16_u32(o, self + slot) + 0xAC, alpha);
}

/* A spawned object's common fields: +0x9A = 0, +3 = 0, +0x2E = 0, +0xD =
 * cls, +0xE = 0xFFFF, +0x54 = +0x56 = 0. */
static void a16b_spawn_head(A16 *o, uint32_t n, uint32_t cls)
{
    a16_w8(o, n + 0x9A, 0);
    a16_w8(o, n + 3, 0);
    a16_w16(o, n + 0x2E, 0);
    a16_w8(o, n + 0xD, cls);
    a16_w16(o, n + 0xE, 0xFFFF);
    a16_w16(o, n + 0x54, 0);
    a16_w16(o, n + 0x56, 0);
}

/* ----------------------------------------------------------------------
 * 0x824690 (decomp func_overlay_AREA16_00824650, NEARMISS: the order is the
 * original's) — sub 1 placement.
 * State 0: +0 = +8 = 1; D_00810809 = 0xFF when D_00810789 is 0xFF. By
 * D_00810809: 2 sets +0xD = 0x2C and +0xE = record [+0x9A + 1]'s id
 * (halfword +6), then when 001B0FD0 is 0 001C6380 and state 1; 0xFF when
 * 001B0FD0 is 0 001C6380 and state 2; others when 001B0FD0 is 0 001C6380
 * and state 1. Then D_700038A0 = (1, 1, 1, 0.25), +0x2EC =
 * 001C5570(self, D_700038A0, 0x31, 0), +0x2E4 = the same with 0x33; in
 * state 1 D_700038A0 = (0, 1, 0, 0.25), +0x2E8 = the same with 0x32 and
 * 0x8256E0(self, 0); otherwise +0x2E8 = 0 and 0x825730.
 * State 1, +5 0: when D_008104C4 is self and D_00810809 < 2: D_00810809 =
 * D_00810789 = 1, script 0x829970 on self + 0x1F0, the colours of the
 * +0x2EC, +0x2E8, +0x2E4 attachments to (0, 0, 0, 0), 0x825730, +5 += 1,
 * and the fifteen +0x1C chain members after self get +0x2E4 = 0x2D0 and
 * +0x52 = their index (D_70003B88 counts), then the tail. Otherwise
 * D_700038A0 = (345.0, 113.28, 258.50, 1) and the marker draw. Then (any
 * +5) when +5 is 1, the script has ended (001BA1F0) and D_00810809 is 1 or
 * 2: 001FAE70(0), +0x2EC and +0x2E4 colours (1, 1, 1, 0), the +0x2E8
 * attachment's state 3 and +0x2E8 = 0, a class 9 spawn (001AFA90) at
 * (371.64, 101.7, 250.02) turned -1.536 with behaviour 0x825260, a class 4
 * spawn at (371.41, 101.3, 256.84) turned (1.573, 0.305, -0.035) with
 * behaviour 001C4820, +0x2E = 0xFFFF, +0xD = 0x30, +0xE = record
 * [+0x9A]'s id, +5 = 0, 001C4760(0x1E, 1), D_00810809 = D_00810789 = 0xFF,
 * 001AF800, 001CB5B0(+9), and when 001B0FD0 is 0 001C6380 and state 2.
 * The tail: 001B1B70 and the +0x4C method unless D_00810789 is 1 and
 * D_70003B92 is set.
 * State 2: 001B1B70, the method, the marker at (345.0, 113.28, 258.50).
 * State 3 / others: 0x825730, 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_824690_tail(A16 *o, uint32_t self)
{
    const uint32_t d = a16_u8(o, D_00810789);
    if (d != 1 || a16_u8(o, D_70003B92) == 0 || d == 0xFF) {
        (void)a16_c_001B1B70(o, self);
        (void)a16_method(o, self);
    }
}

static void a16_824690_marker(A16 *o)
{
    a16b_vec(o, 0x43AC7FF6u, 0x42E28D29u, 0x43814024u, F_ONE);
    a16b_marker_draw(o);
}

static void a16_824690(A16 *o, uint32_t self, uint32_t sp)
{
    int32_t r;
    uint32_t n;
    switch (a16_u8(o, self + 4)) {
    case 0: {
        a16_w8(o, self, 1);
        a16_w8(o, self + 8, 1);
        if (a16_u8(o, D_00810789) == 0xFF) a16_w8(o, D_00810809, 0xFF);
        const uint32_t mode = a16_u8(o, D_00810809);
        if (mode == 0xFF) {
            r = 0;
            if (a16_c_001B0FD0(o, self, &r) < 0) return;
            if (r == 0) {
                (void)a16_c_001C6380(o, self);
                a16_w8(o, self + 4, 2);
            }
        } else {
            if (mode == 2) {
                a16_w8(o, self + 0xD, 0x2C);
                a16_w16(o, self + 0xE, a16b_record_half(o, self, 0x2E));
            }
            r = 0;
            if (a16_c_001B0FD0(o, self, &r) < 0) return;
            if (r == 0) {
                (void)a16_c_001C6380(o, self);
                a16_w8(o, self + 4, 1);
            }
        }
        a16b_vec(o, F_ONE, F_ONE, F_ONE, F_QUARTER);
        n = 0;
        if (a16_c_001C5570(o, self, D_700038A0, 0x31, 0, &n) < 0) return;
        a16_w32(o, self + 0x2EC, n);
        n = 0;
        if (a16_c_001C5570(o, self, D_700038A0, 0x33, 0, &n) < 0) return;
        a16_w32(o, self + 0x2E4, n);
        if (a16_u8(o, self + 4) == 1) {
            a16b_vec(o, F_ZERO, F_ONE, F_ZERO, F_QUARTER);
            n = 0;
            if (a16_c_001C5570(o, self, D_700038A0, 0x32, 0, &n) < 0) return;
            a16_w32(o, self + 0x2E8, n);
            a16_8256E0(o, self, 0, sp - 0x20u);
        } else {
            a16_w32(o, self + 0x2E8, 0);
            a16_825730(o, self);
        }
        break;
    }
    case 1: {
        const uint32_t step = a16_u8(o, self + 5);
        if (step == 0) {
            if (a16_u32(o, D_008104C4) == self && a16_u8(o, D_00810809) < 2) {
                a16_w8(o, D_00810809, 1);
                a16_w8(o, D_00810789, 1);
                (void)a16_c_001BA1A0(o, self + 0x1F0, A16_SCRIPT_829970);
                a16b_colour(o, self, 0x2EC, F_ZERO, F_ZERO);
                a16b_colour(o, self, 0x2E8, F_ZERO, F_ZERO);
                a16b_colour(o, self, 0x2E4, F_ZERO, F_ZERO);
                a16_825730(o, self);
                a16_w8(o, self + 5, a16_u8(o, self + 5) + 1);
                uint32_t m = a16_u32(o, self + 0x1C);
                a16_w16(o, D_70003B88, 0);
                while (!a16_failed(o) && a16_s16(o, D_70003B88) < 15) {
                    a16_w32(o, m + 0x2E4, 0x2D0);
                    a16_w16(o, m + 0x52, a16_u16(o, D_70003B88));
                    const int32_t k = a16_s16(o, D_70003B88);
                    m = a16_u32(o, m + 0x1C);
                    a16_w16(o, D_70003B88, (uint32_t)(k + 1));
                }
                a16_824690_tail(o, self);
                break;
            }
            a16_824690_marker(o);
        }
        if (a16_u8(o, self + 5) == 1) {
            r = 0;
            if (a16_c_001BA1F0(o, self, &r) < 0) return;
            const uint32_t mode = r != 0 ? a16_u8(o, D_00810809) : 0;
            if (r != 0 && (mode == 1 || mode == 2)) {
                (void)a16_c_001FAE70(o, 0);
                a16b_colour(o, self, 0x2EC, F_ONE, F_ZERO);
                const uint32_t c = a16_u32(o, self + 0x2E8);
                if (c != 0) {
                    a16_w8(o, c + 4, 3);
                    a16_w32(o, self + 0x2E8, 0);
                }
                a16b_colour(o, self, 0x2E4, F_ONE, F_ZERO);
                n = 0;
                if (a16_c_001AFA90(o, 9, &n) < 0) return;
                if (n != 0) {
                    a16b_spawn_head(o, n, 0x68);
                    a16_w32(o, n + 0xB0, 0x43B9D247u);
                    a16_w32(o, n + 0xB4, 0x42CB6666u);
                    a16_w32(o, n + 0xB8, 0x437A0595u);
                    a16_w32(o, n + 0xC0, F_ZERO);
                    a16_w32(o, n + 0xC4, 0xBFC4980Au);
                    a16_w32(o, n + 0xC8, F_ZERO);
                    a16_w32(o, n + 0x10, A16_CHILD_825260);
                }
                n = 0;
                if (a16_c_001AFA90(o, 4, &n) < 0) return;
                if (n != 0) {
                    a16b_spawn_head(o, n, 0xE);
                    a16_w32(o, n + 0xB0, 0x43B9B47Bu);
                    a16_w32(o, n + 0xB4, 0x42CA999Au);
                    a16_w32(o, n + 0xB8, 0x43806B85u);
                    a16_w32(o, n + 0xC0, 0x3FC9490Bu);
                    a16_w32(o, n + 0xC4, 0x3E9C61ABu);
                    a16_w32(o, n + 0xC8, 0xBD0EFA35u);
                    a16_w32(o, n + 0x10, A16_BOOT_1C4820);
                }
                a16_w16(o, self + 0x2E, 0xFFFF);
                a16_w8(o, self + 0xD, 0x30);
                a16_w16(o, self + 0xE, a16b_record_half(o, self, 6));
                a16_w8(o, self + 5, 0);
                (void)a16_c_001C4760(o, 0x1E, 1);
                a16_w8(o, D_00810809, 0xFF);
                a16_w8(o, D_00810789, 0xFF);
                (void)a16_c_001AF800(o, self);
                (void)a16_c_001CB5B0(o, (int32_t)a16_u8(o, self + 9));
                r = 0;
                if (a16_c_001B0FD0(o, self, &r) < 0) return;
                if (r == 0) {
                    (void)a16_c_001C6380(o, self);
                    a16_w8(o, self + 4, 2);
                }
            }
        }
        a16_824690_tail(o, self);
        break;
    }
    case 2:
        (void)a16_c_001B1B70(o, self);
        (void)a16_method(o, self);
        a16_824690_marker(o);
        break;
    default:
        a16_825730(o, self);
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00824690(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_824690(&o, a0, sp);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825260 (decomp func_overlay_AREA16_00825220) — the child 0x824690
 * spawns. State 0: when 001B10B0(self, 0x68, 0x69) is 0: 001C63E0(self, 0),
 * state 1. State 1: 001C68C0, then the +0x4C method when 001B17A0. State 3
 * / others: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_825260(A16 *o, uint32_t self)
{
    int32_t r;
    switch (a16_u8(o, self + 4)) {
    case 0:
        r = 0;
        if (a16_c_001B10B0(o, self, 0x68, 0x69, &r) < 0) return;
        if (r == 0) {
            (void)a16_c_001C63E0(o, self, 0);
            a16_w8(o, self + 4, 1);
        }
        break;
    case 1:
        (void)a16_c_001C68C0(o, self);
        r = 0;
        if (a16_c_001B17A0(o, self, &r) < 0) return;
        if (r != 0) (void)a16_method(o, self);
        break;
    default:
        (void)a16_c_001AFC10(o, self);
        break;
    }
}

int em_area16_overlay_00825260(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    (void)sp;
    if (a16_begin(&o, h, fault)) return -1;
    a16_825260(&o, a0);
    return a16_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825630 (decomp func_overlay_AREA16_008255F0) — script 0x829970's op09
 * callback (self, record, work): +0x2E = 0xFFFF when work word +8 is set,
 * else 0; returns 1.
 * ---------------------------------------------------------------------- */
static int32_t a16_825630(A16 *o, uint32_t self, uint32_t work)
{
    a16_w16(o, self + 0x2E, a16_u32(o, work + 8) != 0 ? 0xFFFFu : 0u);
    return 1;
}

int em_area16_overlay_00825630(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea16OverlayFault *fault)
{
    A16 o;
    (void)a1;
    if (!result || a16_begin(&o, h, fault)) return -1;
    const int32_t v = a16_825630(&o, a0, a2);
    if (a16_end(&o)) return -1;
    *result = v;
    return 0;
}

/* The +0x2EC / +0x2E8 / +0x2E4 attachment at self+slot follows self: when
 * it is set and its state is 1, 00102958(its +0x12C object + 0x90, self's
 * +0x12C object + 0x90). */
static void a16b_follow(A16 *o, uint32_t self, uint32_t c)
{
    if (c == 0 || a16_u8(o, c + 4) != 1) return;
    const uint32_t mc = a16_u32(o, c + 0x12C);
    const uint32_t ms = a16_u32(o, self + 0x12C);
    (void)a16_c_00102958(o, mc + 0x90u, ms + 0x90u);
}

/* ----------------------------------------------------------------------
 * 0x824E10 (decomp func_overlay_AREA16_00824DD0) — group owner; +0x20 the
 * record a, +0x24 the switch owner b.
 * State 0: unless 001BAD40(self, a): state 1, D_700038A0 = (1, 1, 1,
 * 0.25), +0x2EC = 001C5570(self, D_700038A0, 0xB, 0), +0x2E4 = the same
 * with 0xD, D_700038A0 = (0, 1, 0, 0.25), +0x2E8 = the same with 0xC,
 * 0x8256E0(self, 1), and on into state 1.
 * State 1: +0x28 += 1. When b's +0x2E has bit (self +0x2E & 31): state 2.
 * Else, a's +4 0x270D / 0x270C returns; by a's +0xA: 5 001C5C90 and
 * return; 4 001C68C0 and the method; 3 nothing; 0 001BA580(self, a's +4),
 * 001C64F0(self, a's +0xC), 001C68C0, +1 = 1, the method; others
 * 0x825780(self, 1) and the same as 0 after 001BA580. Then the +0x2EC
 * attachment follows (00102958); the +0x2E8 one, while +0x28 < 0x2121, is
 * retired at 0x2120 (its state 3, +0x2E8 = 0, 0x825730) or follows; the
 * +0x2E4 one follows; D_700038A0 = (-22.9961, 30.6261, 10.9414, 1) is
 * moved by the +0x12C object's matrix (001026A0) and the marker drawn.
 * State 2: state 3; when a's +4 is not 0x270D and a's +0xA is 0:
 * 001BA540. State 3: the three attachments' state 3, 001AFC10.
 * ---------------------------------------------------------------------- */
static void a16_824E10(A16 *o, uint32_t self, uint32_t sp)
{
    const uint32_t st = a16_u8(o, self + 4);
    const uint32_t a = a16_u32(o, self + 0x20);
    const uint32_t b = a16_u32(o, self + 0x24);
    int32_t r;
    uint32_t n;

    switch (st) {
    case 0:
        r = 0;
        if (a16_c_001BAD40(o, self, a, &r) < 0) return;
        if (r != 0) return;
        a16_w8(o, self + 4, 1);
        a16b_vec(o, F_ONE, F_ONE, F_ONE, F_QUARTER);
        n = 0;
        if (a16_c_001C5570(o, self, D_700038A0, 0xB, 0, &n) < 0) return;
        a16_w32(o, self + 0x2EC, n);
        n = 0;
        if (a16_c_001C5570(o, self, D_700038A0, 0xD, 0, &n) < 0) return;
        a16_w32(o, self + 0x2E4, n);
        a16b_vec(o, F_ZERO, F_ONE, F_ZERO, F_QUARTER);
        n = 0;
        if (a16_c_001C5570(o, self, D_700038A0, 0xC, 0, &n) < 0) return;
        a16_w32(o, self + 0x2E8, n);
        a16_8256E0(o, self, 1, sp - 0x40u);
        /* fall through */
    case 1: {
        a16_w16(o, self + 0x28, (uint32_t)(a16_s16(o, self + 0x28) + 1));
        const uint32_t bit = a16_u16(o, self + 0x2E);
        const uint32_t mask = a16_u16(o, b + 0x2E);
        if (mask & (1u << (bit & 31u))) {
            a16_w8(o, self + 4, 2);
        } else {
            const int32_t kind = a16_s16(o, a + 4);
            if (kind == 0x270D || kind == 0x270C) return;
            const int32_t mode = a16_s16(o, a + 0xA);
            if (mode == 5) {
                (void)a16_c_001C5C90(o, self);
                return;
            }
            if (mode == 4) {
                (void)a16_c_001C68C0(o, self);
                (void)a16_method(o, self);
            } else if (mode != 3) {
                if (mode == 0)
                    (void)a16_c_001BA580(o, self, kind);
                else
                    a16_825780(o, self, 1);
                (void)a16_c_001C64F0(o, self, a16_u32(o, a + 0xC));
                (void)a16_c_001C68C0(o, self);
                a16_w8(o, self + 1, 1);
                (void)a16_method(o, self);
            }
        }
        a16b_follow(o, self, a16_u32(o, self + 0x2EC));
        const uint32_t c2 = a16_u32(o, self + 0x2E8);
        if (c2 != 0) {
            const int32_t t = a16_s16(o, self + 0x28);
            if (t < 0x2121) {
                if (t == 0x2120) {
                    a16_w8(o, c2 + 4, 3);
                    a16_w32(o, self + 0x2E8, 0);
                    a16_825730(o, self);
                } else {
                    a16b_follow(o, self, c2);
                }
            }
        }
        a16b_follow(o, self, a16_u32(o, self + 0x2E4));
        a16b_vec(o, 0xC1B7F803u, 0x41F50241u, 0x412F0FF9u, F_ONE);
        (void)a16_c_001026A0(o, D_700038A0, a16_u32(o, self + 0x12C) + 0x90u, D_700038A0);
        a16b_marker_draw(o);
        break;
    }
    case 2:
        a16_w8(o, self + 4, st + 1);
        if (a16_s16(o, a + 4) != 0x270D && a16_s16(o, a + 0xA) == 0) (void)a16_c_001BA540(o, self);
        break;
    case 3: {
        uint32_t c = a16_u32(o, self + 0x2EC);
        if (c != 0) a16_w8(o, c + 4, 3);
        c = a16_u32(o, self + 0x2E8);
        if (c != 0) a16_w8(o, c + 4, 3);
        c = a16_u32(o, self + 0x2E4);
        if (c != 0) a16_w8(o, c + 4, 3);
        (void)a16_c_001AFC10(o, self);
        break;
    }
    }
}

int em_area16_overlay_00824E10(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault)
{
    A16 o;
    if (a16_begin(&o, h, fault)) return -1;
    a16_824E10(&o, a0, sp);
    return a16_end(&o);
}
