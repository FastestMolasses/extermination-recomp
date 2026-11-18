/* AREA17's own overlay owners (em_area17_overlay.h): the effect objects,
 * the arrival [30] and its script's callbacks, the flag-gated models and
 * markers. The exchange [34], the part 0x8251B0, [36] and the breakables
 * [5]..[8] are em_area17_overlay_npc.c's. Each function is one original
 * function; the comment above it says what the code does. The order of the
 * memory accesses between calls is the original's (the test compares it
 * one for one). */
#include "em_area17_overlay_internal.h"

/* overlay data and BSS (runtime addresses) */
#define A17_TABLE_8265C0 0x008265C0u   /* 0x8235E0's first 001D04B0 table */
#define A17_TABLE_826650 0x00826650u   /* its second */
#define A17_TABLE_8266E0 0x008266E0u   /* 0x8236F0's 001D04B0 table */
#define A17_PACKET_826770 0x00826770u  /* 0x823860's packet headers (001CFBE0) */
#define A17_PACKET_826800 0x00826800u
#define A17_GROUP_8263C0 0x008263C0u   /* [30]'s first group (Roger, 00133E30) */
#define A17_GROUP_826420 0x00826420u   /* [30]'s second group */
#define A17_GROUP_826500 0x00826500u   /* 0x824280's group */
#define A17_SCRIPT_826A20 0x00826A20u  /* [30]'s arrival script */
#define A17_SCRIPT_8270A0 0x008270A0u  /* [30]'s second script (0x823F70 / 0x824280) */
#define A17_POINT_827520 0x00827520u   /* 0x823F70's second point; 0x824280's effect point */
#define A17_MID_969A80 0x00969A80u     /* BSS: 0x823F70's midpoint (x, y, z) and radius */
#define A17_POINT_969A90 0x00969A90u   /* BSS: 0x824280's second effect point */

/* globals */
#define D_PLAYER_POS 0x00810350u
#define D_PLAYER_HEADING 0x00810374u
#define D_008101F0 0x008101F0u
#define D_00810200 0x00810200u
#define D_008105D0 0x008105D0u
#define D_008105E0 0x008105E0u
#define D_008106C0 0x008106C0u
#define D_00810787 0x00810787u
#define D_00810805 0x00810805u
#define D_00810806 0x00810806u
#define D_0028A59C 0x0028A59Cu
#define D_700038A0 0x700038A0u
#define D_700038B0 0x700038B0u
#define D_70003B92 0x70003B92u

#define K(x) a17_bits(x)

/* ----------------------------------------------------------------------
 * 0x823580 (decomp overlay_AREA17_func_00823540) — group 0x828440's first
 * member. State 1: 001EFD20(0, +0x100). Other states: nothing.
 * 0x823800 (decomp func_overlay_AREA17_008237C0) — the same with effect 1.
 * ---------------------------------------------------------------------- */
static void a17_effect_node(A17 *o, uint32_t self, int32_t id)
{
    if (a17_u8(o, self + 4) == 1) (void)a17_c_001EFD20(o, id, self + 0x100);
}

int em_area17_overlay_00823580(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_effect_node(&o, a0, 0);
    return a17_end(&o);
}

int em_area17_overlay_00823800(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_effect_node(&o, a0, 1);
    return a17_end(&o);
}

/* State 0 of 0x8235E0 and 0x823860: the matrix at +0xD0 from 001029C0 and
 * 00102918 (+0xB0), +0x1F0 = 00122BB8(), +0x1F4 = 0, state 1. */
static void a17_seed_matrix(A17 *o, uint32_t self)
{
    int32_t r = 0;
    (void)a17_c_001029C0(o, self + 0xD0);
    (void)a17_c_00102918(o, self + 0xD0, self + 0xD0, self + 0xB0);
    (void)a17_c_00122BB8(o, &r);
    a17_w32(o, self + 0x1F0, (uint32_t)r);
    a17_w32(o, self + 0x1F4, 0);
    a17_w8(o, self + 4, 1);
}

/* +0x1F4 += step; state 3 once it is not at most 1.5. */
static void a17_ramp(A17 *o, uint32_t self, uint32_t step)
{
    const uint32_t v = A17_ADD(a17_u32(o, self + 0x1F4), step);
    a17_w32(o, self + 0x1F4, v);
    if (!A17_LE(v, K(1.5f))) a17_w8(o, self + 4, 3);
}

/* ----------------------------------------------------------------------
 * 0x8235E0 (decomp func_overlay_AREA17_008235A0) — an effect object no
 * placement or group names. State 0: a17_seed_matrix. State 1:
 * 001D04B0(+0xD0, 5, table, +0x1F4, (float)+0x1F0) for the tables 0x8265C0
 * and 0x826650; +0x1F4 += 0.012, state 3 past 1.5.
 * ---------------------------------------------------------------------- */
static void a17_8235E0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    if (state == 0) {
        a17_seed_matrix(o, self);
    } else if (state == 1) {
        static const uint32_t tables[2] = {A17_TABLE_8265C0, A17_TABLE_826650};
        for (unsigned k = 0; k < 2; ++k) {
            const uint32_t seed = A17_CVT_S_W(a17_u32(o, self + 0x1F0));
            const uint32_t t = a17_u32(o, self + 0x1F4);
            (void)a17_c_001D04B0(o, self + 0xD0, 5, tables[k], t, seed);
        }
        a17_ramp(o, self, K(0.012f));
    }
}

int em_area17_overlay_008235E0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8235E0(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8236F0 (decomp func_overlay_AREA17_008236B0) — group 0x828440's second
 * member. State 0: +0x1F0 = 00122BB8(), +0x1F4 = 0.8 (the state stays 0).
 * State 1: 001EFD20(1, +0x100) while +0x1F4 equals 0.8; 001D04B0(+0xD0, 5,
 * 0x8266E0, +0x1F4, (float)+0x1F0); +0x1F4 += 0.001, less 1.0 once it is
 * not at most 2.0.
 * ---------------------------------------------------------------------- */
static void a17_8236F0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    if (state == 0) {
        int32_t r = 0;
        (void)a17_c_00122BB8(o, &r);
        a17_w32(o, self + 0x1F0, (uint32_t)r);
        a17_w32(o, self + 0x1F4, K(0.8f));
    } else if (state == 1) {
        if (A17_EQ(a17_u32(o, self + 0x1F4), K(0.8f))) (void)a17_c_001EFD20(o, 1, self + 0x100);
        const uint32_t seed = A17_CVT_S_W(a17_u32(o, self + 0x1F0));
        const uint32_t t = a17_u32(o, self + 0x1F4);
        (void)a17_c_001D04B0(o, self + 0xD0, 5, A17_TABLE_8266E0, t, seed);
        const uint32_t v = A17_ADD(a17_u32(o, self + 0x1F4), K(0.001f));
        a17_w32(o, self + 0x1F4, v);
        if (!A17_LE(v, K(2.0f))) a17_w32(o, self + 0x1F4, A17_SUB(a17_u32(o, self + 0x1F4), K(1.0f)));
    }
}

int em_area17_overlay_008236F0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8236F0(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x823860 (decomp func_overlay_AREA17_00823820) — an effect object no
 * placement or group names. State 0: a17_seed_matrix. State 1: the packet
 * in the frame (sp - 0x60) from 001CFB50(packet, 0, +0xD0, +0x1F4,
 * (float)+0x1F0, 1.0, 1e-6, 100.0) with its word +0x54 negated, queued by
 * 001CFBE0(001CCF70(+0x100), 5, header, packet, 0) for the headers 0x826770
 * and 0x826800; +0x1F4 += 0.01, state 3 past 1.5.
 * ---------------------------------------------------------------------- */
static void a17_823860(A17 *o, uint32_t self, uint32_t sp)
{
    const uint32_t packet = sp - 0x60u;
    const uint32_t state = a17_u8(o, self + 4);
    if (state == 0) {
        a17_seed_matrix(o, self);
    } else if (state == 1) {
        int32_t handle = 0;
        (void)a17_c_001CCF70(o, self + 0x100, &handle);
        const uint32_t seed = A17_CVT_S_W(a17_u32(o, self + 0x1F0));
        const uint32_t t = a17_u32(o, self + 0x1F4);
        (void)a17_c_001CFB50(o, packet, 0, self + 0xD0, t, seed, K(1.0f), K(1e-6f), K(100.0f));
        a17_w32(o, packet + 0x54, A17_NEG(a17_u32(o, packet + 0x54)));
        (void)a17_c_001CFBE0(o, handle, 5, A17_PACKET_826770, packet, 0);
        (void)a17_c_001CFBE0(o, handle, 5, A17_PACKET_826800, packet, 0);
        a17_ramp(o, self, K(0.01f));
    }
}

int em_area17_overlay_00823860(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_823860(&o, a0, sp);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8239D0 (decomp func_overlay_AREA17_00823990) — group 0x826890 [4]..[7].
 * State 1, by the halfword +0x94: 5, 6 and 7 D_700038B0 = (0x30, 0x80,
 * 0x30, 0x80) and 001F4E20(+0x100, D_700038B0, 5.0); 8 (0x80, 0x50, 0x30,
 * 0x80) and size 8.0; others nothing.
 * ---------------------------------------------------------------------- */
static void a17_8239D0(A17 *o, uint32_t self)
{
    if (a17_u8(o, self + 4) != 1) return;
    const int32_t kind = a17_s16(o, self + 0x94);
    if (kind < 5 || kind > 8) return;
    const uint32_t rgba[2][4] = {{0x30, 0x80, 0x30, 0x80}, {0x80, 0x50, 0x30, 0x80}};
    const unsigned k = kind == 8;
    for (unsigned i = 0; i < 4; ++i) a17_w32(o, D_700038B0 + 4 * i, rgba[k][i]);
    (void)a17_c_001F4E20(o, self + 0x100, D_700038B0, k ? K(8.0f) : K(5.0f));
}

int em_area17_overlay_008239D0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8239D0(&o, a0);
    return a17_end(&o);
}

/* The tail of most owners' state 1: the +0x4C method when 001B17A0(self)
 * is set. */
static void a17_method_if_drawn(A17 *o, uint32_t self)
{
    int32_t r = 0;
    (void)a17_c_001B17A0(o, self, &r);
    if (r != 0) (void)a17_method(o, self);
}

/* 0x823BA0's two groups and the flags 0xE / 0xF; the +0x1C object's +0x28 */
static void a17_arrival_flags(A17 *o, uint32_t self, uint32_t mode)
{
    const uint32_t obj = a17_u32(o, self + 0x1C);
    a17_w16(o, obj + 0x28, mode);
    (void)a17_c_0019C6F0(o, 0xE, 1);
    (void)a17_c_0019C6F0(o, 0xF, 1);
}

/* ----------------------------------------------------------------------
 * 0x823BA0 (decomp func_overlay_AREA17_00823B60) — [30], the arrival
 * (counter 0x2D). State 0: state 3 with flag 0x2D set; else after
 * 001B0FD0(self) is 0: 001C6380, state 1, +0 = 1, script 0x826A20. State
 * 1 by the step +5: 0 -> 1 and on; 1: a skip (001BA1F0 bit 1) -> 0x19;
 * once D_00810805 is set D_008106C0 = 001B6660(group 0x8263C0), +0x2E =
 * 0xFFFF, +5 2. 2: at D_00810805 == 2 001B6660(group 0x826420), 001F6B60,
 * D_00810805 = 3, the +0x1C object's +0x28 = 0x11E, 0019C6F0(0xE / 0xF,
 * 1); then 001BA1F0: bit 1 -> 0x19, else any bit -> 001FB0B0(0xB), +5 3.
 * 0x19: +5 3, +0x2E = 0xFFFF, the group 0x8263C0 if D_00810805 is 0, the
 * 001FB0B0(0xB) / group 0x826420 / 001F6B60 unless D_00810805 is 2,
 * D_00810805 = 4, the +0x1C object's +0x28 = 1, the flags, the player at
 * (156.5, 220, 300) heading 2.1118484, D_008105D0 = (116.4, 259, 324.1),
 * D_008105E0 = (156.5, 237, 300), copied to D_008101F0 / D_00810200
 * (00102948). 3: script 0x8270A0 at D_00810805 == 8 (+5 4). 4: +5 5 at
 * its end. Then 001C6380 and a17_method_if_drawn. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_823BA0(A17 *o, uint32_t self)
{
    const uint32_t talk = self + 0x1F0;
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;

    if (state == 2 || state == 3) {
        (void)a17_c_001AFC10(o, self);
        return;
    }
    if (state == 0) {
        (void)a17_c_001BA1C0(o, self, 0x2D, &r);
        if (r != 0) {
            a17_w8(o, self + 4, 3);
            return;
        }
        r = 0;
        (void)a17_c_001B0FD0(o, self, &r);
        if (r == 0) {
            (void)a17_c_001C6380(o, self);
            a17_w8(o, self + 4, 1);
            a17_w8(o, self, 1);
            (void)a17_c_001BA1A0(o, talk, A17_SCRIPT_826A20);
        }
        return;
    }
    if (state != 1) return;
    switch (a17_u8(o, self + 5)) {
    case 0:
        a17_w8(o, self + 5, 1);
        /* fall through */
    case 1:
        (void)a17_c_001BA1F0(o, self, &r);
        if (r & 2) a17_w8(o, self + 5, 0x19);
        if (a17_u8(o, D_00810805) != 0) {
            uint32_t g = 0;
            (void)a17_c_001B6660(o, A17_GROUP_8263C0, &g);
            a17_w32(o, D_008106C0, g);
            a17_w16(o, self + 0x2E, 0xFFFF);
            a17_w8(o, self + 5, 2);
        }
        break;
    case 2:
        if (a17_u8(o, D_00810805) == 2) {
            uint32_t g = 0;
            (void)a17_c_001B6660(o, A17_GROUP_826420, &g);
            (void)a17_c_001F6B60(o);
            a17_w8(o, D_00810805, 3);
            a17_arrival_flags(o, self, 0x11E);
        }
        (void)a17_c_001BA1F0(o, self, &r);
        if (r & 2) {
            a17_w8(o, self + 5, 0x19);
        } else if (r != 0) {
            a17_w8(o, self + 5, 3);
            (void)a17_c_001FB0B0(o, 0xB);
        }
        break;
    case 0x19: {
        a17_w8(o, self + 5, 3);
        a17_w16(o, self + 0x2E, 0xFFFF);
        uint32_t g = 0;
        if (a17_u8(o, D_00810805) == 0) {
            (void)a17_c_001B6660(o, A17_GROUP_8263C0, &g);
            a17_w32(o, D_008106C0, g);
        }
        if (a17_u8(o, D_00810805) != 2) {
            (void)a17_c_001FB0B0(o, 0xB);
            (void)a17_c_001B6660(o, A17_GROUP_826420, &g);
            (void)a17_c_001F6B60(o);
        }
        a17_w8(o, D_00810805, 4);
        a17_arrival_flags(o, self, 1);
        a17_w32(o, D_PLAYER_POS, K(156.5f));
        a17_w32(o, D_PLAYER_POS + 4, K(220.0f));
        a17_w32(o, D_PLAYER_POS + 8, K(300.0f));
        a17_w32(o, D_PLAYER_HEADING, K(2.1118484f));
        a17_w32(o, D_008105D0, K(116.4f));
        a17_w32(o, D_008105D0 + 4, K(259.0f));
        a17_w32(o, D_008105D0 + 8, K(324.1f));
        a17_w32(o, D_008105E0, K(156.5f));
        a17_w32(o, D_008105E0 + 4, K(237.0f));
        a17_w32(o, D_008105E0 + 8, K(300.0f));
        (void)a17_c_00102948(o, D_008101F0, D_008105D0);
        (void)a17_c_00102948(o, D_00810200, D_008105E0);
        a17_w8(o, self + 5, 3);
        break;
    }
    case 3:
        if (a17_u8(o, D_00810805) == 8) {
            (void)a17_c_001BA1A0(o, talk, A17_SCRIPT_8270A0);
            a17_w8(o, self + 5, 4);
        }
        break;
    case 4:
        (void)a17_c_001BA1F0(o, self, &r);
        if (r != 0) a17_w8(o, self + 5, 5);
        break;
    default:
        break;
    }
    (void)a17_c_001C6380(o, self);
    a17_method_if_drawn(o, self);
}

int em_area17_overlay_00823BA0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_823BA0(&o, a0);
    return a17_end(&o);
}

/* One circling step of 0x823F70: D_008105D0 / D_008101F0 x = mid x + r *
 * 0011E2A8(a), y = (mid y += 0.1), z = mid z + r * 0011DE90(a) with a =
 * 0.008726646 * work +0x10; then work +0x10 += 1. Returns 1 once the count
 * is not at most `limit`. */
static int a17_circle(A17 *o, uint32_t work, uint32_t limit)
{
    uint32_t s = 0;
    (void)a17_c_0011E2A8(o, A17_MUL(K(0.008726646f), a17_u32(o, work + 0x10)), &s);
    const uint32_t radius = a17_u32(o, A17_MID_969A80 + 12);
    const uint32_t x = A17_ADD(a17_u32(o, A17_MID_969A80), A17_MUL(radius, s));
    const uint32_t y = A17_ADD(a17_u32(o, A17_MID_969A80 + 4), K(0.1f));
    a17_w32(o, D_008105D0, x);
    a17_w32(o, D_008101F0, x);
    a17_w32(o, A17_MID_969A80 + 4, y);
    a17_w32(o, D_008105D0 + 4, y);
    a17_w32(o, D_008101F0 + 4, y);
    uint32_t c = 0;
    (void)a17_c_0011DE90(o, A17_MUL(K(0.008726646f), a17_u32(o, work + 0x10)), &c);
    const uint32_t r2 = a17_u32(o, A17_MID_969A80 + 12);
    const uint32_t z = A17_ADD(a17_u32(o, A17_MID_969A80 + 8), A17_MUL(r2, c));
    a17_w32(o, D_008105D0 + 8, z);
    a17_w32(o, D_008101F0 + 8, z);
    const uint32_t n = A17_ADD(a17_u32(o, work + 0x10), K(1.0f));
    a17_w32(o, work + 0x10, n);
    return !A17_LE(n, limit);
}

/* ----------------------------------------------------------------------
 * 0x823F70 (decomp func_overlay_AREA17_00823F30) — script 0x8270A0's op09
 * callback (self, the op09 record, the script's work block). Record +4 0:
 * the BSS 0x969A80 = the midpoint of the point 0x827520 and the player
 * (each sum divided by 2.0), 0x969A8C = 0011E748 of the squared horizontal
 * distance (a multiply-add), +4 1, work +0x10 = 0, D_008105E0 and
 * D_00810200 = the midpoint with y + 10. +4 1: a17_circle to 380, then
 * 001AEDE0(1, 1) and +4 2. +4 2: a17_circle; returns 1 past 600. Returns 0
 * otherwise.
 * ---------------------------------------------------------------------- */
static int32_t a17_823F70(A17 *o, uint32_t rec, uint32_t work)
{
    const uint32_t step = a17_u8(o, rec + 4);
    if (step == 0) {
        const uint32_t ax = a17_u32(o, A17_POINT_827520);
        const uint32_t px = a17_u32(o, D_PLAYER_POS);
        const uint32_t ay = a17_u32(o, A17_POINT_827520 + 4);
        const uint32_t py = a17_u32(o, D_PLAYER_POS + 4);
        const uint32_t az = a17_u32(o, A17_POINT_827520 + 8);
        const uint32_t sx = A17_ADD(ax, px);
        const uint32_t sy = A17_ADD(ay, py);
        const uint32_t dx = A17_SUB(ax, px);
        const uint32_t acc = A17_MUL(dx, dx);
        const uint32_t mx = A17_DIV(sx, K(2.0f));
        const uint32_t pz = a17_u32(o, D_PLAYER_POS + 8);
        a17_w32(o, A17_MID_969A80, mx);
        a17_w32(o, A17_MID_969A80 + 4, A17_DIV(sy, K(2.0f)));
        a17_w32(o, A17_MID_969A80 + 8, A17_DIV(A17_ADD(az, pz), K(2.0f)));
        const uint32_t dz = A17_SUB(az, pz);
        uint32_t d = 0;
        (void)a17_c_0011E748(o, A17_MADD(acc, dz, dz), &d);
        a17_w32(o, A17_MID_969A80 + 12, d);
        a17_w8(o, rec + 4, 1);
        a17_w32(o, work + 0x10, 0);
        const uint32_t x = a17_u32(o, A17_MID_969A80);
        const uint32_t y = A17_ADD(K(10.0f), a17_u32(o, A17_MID_969A80 + 4));
        const uint32_t z = a17_u32(o, A17_MID_969A80 + 8);
        a17_w32(o, D_008105E0, x);
        a17_w32(o, D_00810200, x);
        a17_w32(o, D_008105E0 + 4, y);
        a17_w32(o, D_00810200 + 4, y);
        a17_w32(o, D_008105E0 + 8, z);
        a17_w32(o, D_00810200 + 8, z);
        return 0;
    }
    if (step == 1) {
        if (a17_circle(o, work, K(380.0f))) {
            (void)a17_c_001AEDE0(o, 1, 1);
            a17_w8(o, rec + 4, 2);
        }
        return 0;
    }
    if (step == 2) return a17_circle(o, work, K(600.0f));
    return 0;
}

int em_area17_overlay_00823F70(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea17OverlayFault *fault)
{
    A17 o;
    (void)a0;
    if (!result || a17_begin(&o, h, fault)) return -1;
    const int32_t v = a17_823F70(&o, a1, a2);
    if (a17_end(&o)) return -1;
    *result = v;
    return 0;
}

/* 0x824280 (decomp func_overlay_AREA17_00824240): script 0x8270A0's other
 * op09 callback: 001EFD20(0x8000005C, 0x827520), 001EFD20(0x80000004,
 * 0x969A90), 001B6660(group 0x826500), returns 1. */
int em_area17_overlay_00824280(const EmArea17OverlayHooks *h, int32_t *result, EmArea17OverlayFault *fault)
{
    A17 o;
    uint32_t g = 0;
    if (!result || a17_begin(&o, h, fault)) return -1;
    (void)a17_c_001EFD20(&o, (int32_t)0x8000005Cu, A17_POINT_827520);
    (void)a17_c_001EFD20(&o, (int32_t)0x80000004u, A17_POINT_969A90);
    (void)a17_c_001B6660(&o, A17_GROUP_826500, &g);
    if (a17_end(&o)) return -1;
    *result = 1;
    return 0;
}

/* ----------------------------------------------------------------------
 * 0x8242D0 (decomp func_overlay_AREA17_00824290) — [39]. State 0:
 * 001C6380 after 001B0FD0(self) is 0. State 1: below 180 the halfword
 * +0x28 counts up, and the method (a17_method_if_drawn) runs only with
 * flag 0x2D, D_00810805 set or the count above 120; from 180 on it always
 * runs. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_8242D0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r == 0) (void)a17_c_001C6380(o, self);
    } else if (state == 1) {
        const int32_t n = a17_s16(o, self + 0x28);
        if (n < 180) {
            a17_w16(o, self + 0x28, (uint32_t)(n + 1));
            (void)a17_c_001BA1C0(o, self, 0x2D, &r);
            if (r == 0 && a17_u8(o, D_00810805) == 0 && a17_s16(o, self + 0x28) <= 120) return;
        }
        a17_method_if_drawn(o, self);
    } else if (state == 3) {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_008242D0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8242D0(&o, a0);
    return a17_end(&o);
}

/* 0x8243E0's change: state 2, the +0x1C object's state 2, the model
 * 001CA6E0(self, 001C6120(D_0028A59C, 0xD)), 001C62C0(self). */
static void a17_open_model(A17 *o, uint32_t self)
{
    int32_t m = 0;
    (void)a17_c_001C6120(o, a17_s32(o, D_0028A59C), 0xD, &m);
    (void)a17_c_001CA6E0(o, self, m);
    (void)a17_c_001C62C0(o, self);
}

/* ----------------------------------------------------------------------
 * 0x8243E0 (decomp func_overlay_AREA17_008243A0) — [31]. State 0: 001C6380
 * after 001B0FD0(self) is 0. State 1: flag 0x2D sets state 2, the +0x1C
 * object's state 2 and the model (a17_open_model); a halfword countdown
 * +0x28 that reaches 0 does the same with effect 0x80000013 at +0xB0 + (0,
 * 15, 0) (D_700038A0, w 1) between. States 1 / 2: a17_method_if_drawn.
 * State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_8243E0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r == 0) (void)a17_c_001C6380(o, self);
        return;
    }
    if (state == 3) {
        (void)a17_c_001AFC10(o, self);
        return;
    }
    if (state == 1) {
        (void)a17_c_001BA1C0(o, self, 0x2D, &r);
        if (r != 0) {
            a17_w8(o, self + 4, 2);
            a17_w8(o, a17_u32(o, self + 0x1C) + 4, 2);
            a17_open_model(o, self);
        }
        const int32_t n = a17_s16(o, self + 0x28);
        if (n != 0) {
            a17_w16(o, self + 0x28, (uint32_t)(n - 1));
            if (a17_s16(o, self + 0x28) == 0) {
                a17_w8(o, self + 4, 2);
                a17_w8(o, a17_u32(o, self + 0x1C) + 4, 2);
                a17_w32(o, D_700038A0, a17_u32(o, self + 0xB0));
                a17_w32(o, D_700038A0 + 4, A17_ADD(K(15.0f), a17_u32(o, self + 0xB4)));
                a17_w32(o, D_700038A0 + 8, a17_u32(o, self + 0xB8));
                a17_w32(o, D_700038A0 + 12, K(1.0f));
                (void)a17_c_001EFD20(o, (int32_t)0x80000013u, D_700038A0);
                a17_open_model(o, self);
            }
        }
    } else if (state != 2) {
        return;
    }
    a17_method_if_drawn(o, self);
}

int em_area17_overlay_008243E0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8243E0(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x824CF0 (decomp func_overlay_AREA17_00824CB0) — [42]. State 0: after
 * 001B0FD0(self) is 0: 001C6380, +0x1F0 = (0.4, 14.9, 0.25, 1) and
 * 001026A0(+0x1F0, +0xD0, +0x1F0). State 1: with D_00810806 == 1 the
 * halfword +0x28 counts and 001F5940(7, +0x1F0, 0) runs for the counts
 * 567..745 and 1106..1124; then 001B17A0 (result unused) and the method.
 * States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_824CF0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a17_c_001C6380(o, self);
        a17_w32(o, self + 0x1F0, K(0.4f));
        a17_w32(o, self + 0x1F4, K(14.9f));
        a17_w32(o, self + 0x1F8, K(0.25f));
        a17_w32(o, self + 0x1FC, K(1.0f));
        (void)a17_c_001026A0(o, self + 0x1F0, self + 0xD0, self + 0x1F0);
    } else if (state == 1) {
        if (a17_u8(o, D_00810806) == 1) {
            a17_w16(o, self + 0x28, a17_u16(o, self + 0x28) + 1);
            const int32_t n = a17_s16(o, self + 0x28);
            if ((n > 566 && n <= 745) || (n > 1105 && n <= 1124)) (void)a17_c_001F5940(o, 7, self + 0x1F0, 0);
        }
        (void)a17_c_001B17A0(o, self, &r);
        (void)a17_method(o, self);
    } else if (state == 2 || state == 3) {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_00824CF0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_824CF0(&o, a0);
    return a17_end(&o);
}

/* [40]'s and [41]'s method gate: 001B1B70 and the method unless
 * D_70003B92 and D_00810787 are both set. */
static void a17_lit_method(A17 *o, uint32_t self)
{
    if (a17_u8(o, D_70003B92) == 0 || a17_u8(o, D_00810787) == 0) {
        (void)a17_c_001B1B70(o, self);
        (void)a17_method(o, self);
    }
}

/* ----------------------------------------------------------------------
 * 0x824E40 (decomp func_overlay_AREA17_00824E00) — [40]. State 0: after
 * 001B0FD0(self) is 0: 001C6380 and six points +0x1F0 .. +0x240 (local
 * coordinates, w 1) each through 001026A0(point, +0xD0, point). State 1:
 * flag 0x2F gives the +0x114 object's halfword +0x8A = 0x7D00, 001C6380
 * and state 2; a17_lit_method; 001F5940(1, point, 0) for the six points.
 * State 2: the method. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_824E40(A17 *o, uint32_t self)
{
    static const float pts[6][3] = {{-88.567f, 161.814f, 60.85f}, {-93.098f, 161.814f, 53.966f},
                                    {-55.047f, 127.77f, 28.273f}, {-49.292f, 127.77f, 37.225f},
                                    {-17.464f, 96.156f, 17.702f}, {-24.076f, 96.156f, 7.621f}};
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r != 0) return;
        (void)a17_c_001C6380(o, self);
        for (unsigned k = 0; k < 6; ++k)
            for (unsigned i = 0; i < 3; ++i) a17_w32(o, self + 0x1F0 + 0x10 * k + 4 * i, K(pts[k][i]));
        for (unsigned k = 6; k-- > 0;) a17_w32(o, self + 0x1FC + 0x10 * k, K(1.0f)); /* +0x24C first */
        for (unsigned k = 0; k < 6; ++k)
            (void)a17_c_001026A0(o, self + 0x1F0 + 0x10 * k, self + 0xD0, self + 0x1F0 + 0x10 * k);
    } else if (state == 1) {
        (void)a17_c_001BA1C0(o, self, 0x2F, &r);
        if (r != 0) {
            a17_w16(o, a17_u32(o, self + 0x114) + 0x8A, 0x7D00);
            (void)a17_c_001C6380(o, self);
            a17_w8(o, self + 4, 2);
        }
        a17_lit_method(o, self);
        for (unsigned k = 0; k < 6; ++k) (void)a17_c_001F5940(o, 1, self + 0x1F0 + 0x10 * k, 0);
    } else if (state == 2) {
        (void)a17_method(o, self);
    } else if (state == 3) {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_00824E40(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_824E40(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8250E0 (decomp func_overlay_AREA17_008250A0) — [41]. State 0: 001C6380
 * after 001B0FD0(self) is 0. State 1: flag 0x2F sets state 3;
 * a17_lit_method. State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_8250E0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r == 0) (void)a17_c_001C6380(o, self);
    } else if (state == 1) {
        (void)a17_c_001BA1C0(o, self, 0x2F, &r);
        if (r != 0) a17_w8(o, self + 4, 3);
        a17_lit_method(o, self);
    } else if (state == 3) {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_008250E0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8250E0(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x825470 (decomp func_overlay_AREA17_00825430) — [38], the way to
 * AREA11. State 0: state 1, +0 = 1. State 1: step 0 starts script
 * 0x8284F0 (step 1) once D_00810835 is set; step 1 at its end
 * 001B0C60(0xB, 0, 3) (AREA11 entry 3) and state 3; then 001B17A0 (result
 * unused). State 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_825470(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        a17_w8(o, self + 4, 1);
        a17_w8(o, self, 1);
    } else if (state == 1) {
        const uint32_t step = a17_u8(o, self + 5);
        if (step == 0) {
            if (a17_u8(o, 0x00810835u) != 0) {
                (void)a17_c_001BA1A0(o, self + 0x1F0, 0x008284F0u);
                a17_w8(o, self + 5, 1);
            }
        } else if (step == 1) {
            (void)a17_c_001BA1F0(o, self, &r);
            if (r != 0) {
                (void)a17_c_001B0C60(o, 0xB, 0, 3);
                a17_w8(o, self + 4, 3);
            }
        }
        (void)a17_c_001B17A0(o, self, &r);
    } else if (state == 3) {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_00825470(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_825470(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x8260C0 (decomp func_overlay_AREA17_00826080) — the deferred group
 * 0x826280's [4]. State 0: 001C6380 after 001B1020(self, +0xD, -1, 0) is
 * 0. State 1: state 3 once D_00810787 is set; the method. Any other state:
 * 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_8260C0(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B1020(o, self, (int32_t)a17_u8(o, self + 0xD), -1, 0, &r);
        if (r == 0) (void)a17_c_001C6380(o, self);
    } else if (state == 1) {
        if (a17_u8(o, D_00810787) != 0) a17_w8(o, self + 4, 3);
        (void)a17_method(o, self);
    } else {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_008260C0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_8260C0(&o, a0);
    return a17_end(&o);
}

/* ----------------------------------------------------------------------
 * 0x826170 (decomp func_overlay_AREA17_00826130) — [37]. State 0: after
 * 001B0FD0(self) is 0: 001C6380, +0 = 1. State 1: once flag 0x2E is set
 * 001C6380, 001B1B70 and the method. States 2 / 3: 001AFC10.
 * ---------------------------------------------------------------------- */
static void a17_826170(A17 *o, uint32_t self)
{
    const uint32_t state = a17_u8(o, self + 4);
    int32_t r = 0;
    if (state == 0) {
        (void)a17_c_001B0FD0(o, self, &r);
        if (r == 0) {
            (void)a17_c_001C6380(o, self);
            a17_w8(o, self, 1);
        }
    } else if (state == 1) {
        (void)a17_c_001BA1C0(o, self, 0x2E, &r);
        if (r != 0) {
            (void)a17_c_001C6380(o, self);
            (void)a17_c_001B1B70(o, self);
            (void)a17_method(o, self);
        }
    } else if (state == 2 || state == 3) {
        (void)a17_c_001AFC10(o, self);
    }
}

int em_area17_overlay_00826170(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault)
{
    A17 o;
    if (a17_begin(&o, h, fault)) return -1;
    a17_826170(&o, a0);
    return a17_end(&o);
}
