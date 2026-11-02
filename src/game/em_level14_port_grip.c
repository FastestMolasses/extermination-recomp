/* The last three slots of 0015B130's player table, by the module's hook
 * design: +5 0x0D 00167C80 (the drop onto a ledge from surface 0x33), +5 0x16
 * 0016BC40 and +5 0x17 0016BF80 (the overhead grip on surface 0x3C) with
 * their pieces 0016BB80 (the feeler), 0016BE40 (the speed's ease) and the
 * reaction 00224600. docs/LEVEL14_PORT.md section 2b. Ground truth: the
 * original instructions (the test runs every entry against them). */
#include "em_level14_port_internal.h"

#define F_ONE 0x3F800000u
#define F_ZERO 0x00000000u
#define F_4 0x40800000u
#define F_8 0x41000000u
#define F_16 0x41800000u
#define F_22_5 0x41B40000u
#define F_23 0x41B80000u
#define F_30 0x41F00000u
#define F_40 0x42200000u
#define F_46 0x42380000u
#define F_100 0x42C80000u
#define F_300 0x43960000u
#define F_PI 0x40490FDBu
#define F_PI_8 0x3EC90FDBu
#define F_0_15 0x3E19999Au
#define F_0_01 0x3C23D70Au

#define HIT_POINT_Y 0x700031B4u
#define HIT_RECORD 0x700031D0u

/* ------------------------------------------------------------------------
 * 00167C80 (p): +5 0x0D, by +6. 0: +6 = 1, +7 = 0, 001749A0(p, 0x94 for kind
 * 0x1B else 0x95, 0, 8.0). 1: +0x200 bit 0x8000 clear -> +6 = 2. 2: the step
 * sounds at the clip's thresholds by +7: kind 0x1B 24, 5; others 99 (00182430
 * (p, 2)), 69, 48, 23, 2; each next is 00182AB0, the last also +6 = 3 and
 * 001FBD50(p, 0x119, 0, 300). 3: on +0x200 bit 0x1000: kind 0x1C turns +0xC4
 * by pi; +0xD0 = identity turned by +0xC0, +0xB0 = the skeleton's node 1 +0xC0
 * plus +0xD0 * (0, -10.4, 4.3), +0xBC = 1, +0xD0's translation = +0xB0, +0x2F1
 * = 0, 0017FF80(p, 0), +5 = 0xE, +6 = 0, +0x1F0 = 0x1D.
 * ---------------------------------------------------------------------- */
int em_level14_port_00167C80(const H14 *h, uint32_t p, F14 *fault)
{
    L14 o;
    if (l14_begin(&o, h, fault)) return -1;
    uint32_t st = l14_u8(&o, p + 6);
    if (st == 0) {
        l14_w8(&o, p + 6, st + 1u);
        l14_w8(&o, p + 7, 0);
        l14_c_001749A0(&o, p, l14_u8(&o, p + 0x1F0) == 0x1B ? 0x94 : 0x95, 0, F_8);
    } else if (st == 1) {
        if (!(l14_u32(&o, p + 0x200) & 0x8000u)) l14_w8(&o, p + 6, st + 1u);
    } else if (st == 2) {
        if (l14_u8(&o, p + 0x1F0) == 0x1B) {
            uint32_t sub = l14_u8(&o, p + 7);
            if (sub == 0) {
                if (L14_LE(l14_u32(&o, p + 0x3C), 0x41C00000u)) {
                    l14_w8(&o, p + 7, sub + 1u);
                    l14_c_00182AB0(&o, p);
                }
            } else if (sub == 1) {
                if (L14_LE(l14_u32(&o, p + 0x3C), 0x40A00000u)) {
                    l14_w8(&o, p + 6, st + 1u);
                    if (l14_c_00182AB0(&o, p)) return -1;
                    l14_c_001FBD50(&o, p, 0x119, 0, F_300);
                }
            }
            return l14_end(&o);
        }
        uint32_t sub = l14_u8(&o, p + 7);
        static const uint32_t kLimit[4] = { 0x428A0000u /* 69 */, 0x42400000u /* 48 */, 0x41B80000u /* 23 */,
                                            0x40000000u /* 2 */ };
        if (sub == 0) {
            if (L14_LE(l14_u32(&o, p + 0x3C), 0x42C60000u)) {
                l14_w8(&o, p + 7, sub + 1u);
                l14_c_00182430(&o, p, 2);
            }
        } else if (sub >= 1 && sub <= 3) {
            if (L14_LE(l14_u32(&o, p + 0x3C), kLimit[sub - 1u])) {
                l14_w8(&o, p + 7, sub + 1u);
                l14_c_00182AB0(&o, p);
            }
        } else if (sub == 4) {
            if (L14_LE(l14_u32(&o, p + 0x3C), kLimit[3])) {
                l14_w8(&o, p + 6, st + 1u);
                if (l14_c_00182AB0(&o, p)) return -1;
                l14_c_001FBD50(&o, p, 0x119, 0, F_300);
            }
        }
    } else if (st == 3) {
        if (!(l14_u32(&o, p + 0x200) & 0x1000u)) return l14_end(&o);
        if (l14_u8(&o, p + 0x1F0) == 0x1C) {
            uint32_t r = 0;
            if (l14_c_001B1470(&o, L14_ADD(F_PI, l14_u32(&o, p + 0xC4)), &r)) return -1;
            l14_w32(&o, p + 0xC4, r);
        }
        if (l14_c_001029C0(&o, p + 0xD0)) return -1;
        if (l14_c_00102C58(&o, p + 0xD0, p + 0xD0, p + 0xC0)) return -1;
        l14_w32(&o, 0x700038A0u, 0);
        l14_w32(&o, 0x700038A4u, 0xC1266666u);
        l14_w32(&o, 0x700038A8u, 0x4089999Au);
        l14_w32(&o, 0x700038ACu, 0);
        if (l14_c_001026A0(&o, 0x700038B0u, p + 0xD0, 0x700038A0u)) return -1;
        const uint32_t nodes = l14_u32(&o, 0x00275B40u);
        if (l14_c_001028B8(&o, p + 0xB0, l14_u32(&o, nodes + 4u) + 0xC0u, 0x700038B0u)) return -1;
        l14_w32(&o, p + 0xBC, F_ONE);
        if (l14_c_00102918(&o, p + 0xD0, p + 0xD0, p + 0xB0)) return -1;
        l14_w8(&o, p + 0x2F1, 0);
        if (l14_c_0017FF80(&o, p, F_ZERO)) return -1;
        l14_w8(&o, p + 5, 0xE);
        l14_w8(&o, p + 6, 0);
        l14_w8(&o, p + 0x1F0, 0x1D);
    }
    return l14_end(&o);
}

/* ------------------------------------------------------------------------
 * 0016BB80 (p) -> 1 / 0: the feeler. 001031E0(0x700038A0, p + 0xB0),
 * 0x700038A4 += 40, 0019A570(p + 0xB0, 0x700038A0, 4, 0): a hit on a record
 * of byte +0x1A = 0x3C copies its +0x34 / +0x38 / +0x3C to +0x2E0 / +0x2E4 /
 * +0x2E8 -> 1.
 * ---------------------------------------------------------------------- */
int em_level14_port_0016BB80(const H14 *h, uint32_t p, int32_t *result, F14 *fault)
{
    L14 o;
    int32_t r = 0;
    int32_t out = 0;
    if (!result || l14_begin(&o, h, fault)) return -1;
    if (l14_c_001031E0(&o, 0x700038A0u, p + 0xB0)) return -1;
    l14_w32(&o, 0x700038A4u, L14_ADD(l14_u32(&o, 0x700038A4u), F_40));
    if (l14_c_0019A570(&o, p + 0xB0, 0x700038A0u, 4, 0, &r)) return -1;
    if (r != 0) {
        const uint32_t rec = l14_u32(&o, HIT_RECORD);
        if (l14_u8(&o, rec + 0x1A) == 0x3C) {
            l14_w32(&o, p + 0x2E0, l14_u32(&o, rec + 0x34));
            l14_w32(&o, p + 0x2E4, l14_u32(&o, l14_u32(&o, HIT_RECORD) + 0x38));
            l14_w32(&o, p + 0x2E8, l14_u32(&o, l14_u32(&o, HIT_RECORD) + 0x3C));
            out = 1;
        }
    }
    if (l14_end(&o)) return -1;
    *result = out;
    return 0;
}

/* ------------------------------------------------------------------------
 * 0016BC40 (p): +5 0x16, by +6. 0: +0xC4 = 001B12B0(+0x218, +0xC4, pi/8); at
 * +0x218: area 0x11 -> +6 = 0x63, D_00810835 = 1; else +6 += 1, 001749A0(p,
 * 0x17F, 0, 8), 00182870(p, 0), and 0016BB80: +0x254 = 0x700031B4 - 22.5,
 * +0x258 = that - +0xB4; a miss +0x254 = +0xB4, +0x258 = 0. 1: +0x200 bit
 * 0x8000 set: +0xB4 += +0x258 / 8; clear: +6 = 2, 0016BAE0(p, 1), D_00275B08
 * = 0, +0x25F = 1, +0xB4 = +0x254. 2: +0x3C <= 46 -> +6 = 3, 001FBD50(p,
 * 0x134, 0, 300). 3: on bit 0x1000 +5 = 0x17, +6 = 0, +0x1F0 = 0x2A.
 * ---------------------------------------------------------------------- */
int em_level14_port_0016BC40(const H14 *h, uint32_t p, F14 *fault)
{
    L14 o;
    if (l14_begin(&o, h, fault)) return -1;
    uint32_t st = l14_u8(&o, p + 6);
    if (st == 0) {
        uint32_t r = 0;
        const uint32_t cur = l14_u32(&o, p + 0xC4), goal = l14_u32(&o, p + 0x218);
        if (l14_c_001B12B0(&o, goal, cur, F_PI_8, &r)) return -1;
        l14_w32(&o, p + 0xC4, r);
        if (L14_EQ(r, l14_u32(&o, p + 0x218))) {
            if (l14_u8(&o, 0x00810700u) == 0x11) {
                l14_w8(&o, p + 6, 0x63);
                l14_w8(&o, 0x00810835u, 1);
            } else {
                l14_w8(&o, p + 6, l14_u8(&o, p + 6) + 1u);
                if (l14_c_001749A0(&o, p, 0x17F, 0, F_8)) return -1;
                if (l14_c_00182870(&o, p, 0)) return -1;
                int32_t found = 0;
                if (em_level14_port_0016BB80(h, p, &found, fault)) return -1;
                if (found) {
                    const uint32_t t = L14_SUB(l14_u32(&o, HIT_POINT_Y), F_22_5);
                    l14_w32(&o, p + 0x254, t);
                    l14_w32(&o, p + 0x258, L14_SUB(t, l14_u32(&o, p + 0xB4)));
                } else {
                    l14_w32(&o, p + 0x254, l14_u32(&o, p + 0xB4));
                    l14_w32(&o, p + 0x258, 0);
                }
            }
        }
    } else if (st == 1) {
        if (!(l14_u32(&o, p + 0x200) & 0x8000u)) {
            l14_w8(&o, p + 6, st + 1u);
            if (l14_c_0016BAE0(&o, p, 1)) return -1;
            l14_w32(&o, 0x00275B08u, 0);
            l14_w8(&o, p + 0x25F, 1);
            l14_w32(&o, p + 0xB4, l14_u32(&o, p + 0x254));
        } else {
            const uint32_t step = L14_DIV(l14_u32(&o, p + 0x258), F_8);
            l14_w32(&o, p + 0xB4, L14_ADD(l14_u32(&o, p + 0xB4), step));
        }
    } else if (st == 2) {
        if (L14_LE(l14_u32(&o, p + 0x3C), F_46)) {
            l14_w8(&o, p + 6, st + 1u);
            l14_c_001FBD50(&o, p, 0x134, 0, F_300);
        }
    } else if (st == 3) {
        if (l14_u32(&o, p + 0x200) & 0x1000u) {
            l14_w8(&o, p + 5, 0x17);
            l14_w8(&o, p + 6, 0);
            l14_w8(&o, p + 0x1F0, 0x2A);
        }
    }
    return l14_end(&o);
}

/* ------------------------------------------------------------------------
 * 0016BE40 (p): the two four-word tables D_00248650 (lo) and D_00248660 (hi)
 * go to the frame, 001751A0(p), then by +0x24C: 0: below lo[+0x23F] +0x38
 * rises by lo / 16, clamped to lo; 1: above hi[+0x23F] it falls by an eighth
 * of the gap, clamped to hi.
 * ---------------------------------------------------------------------- */
int em_level14_port_0016BE40(const H14 *h, uint32_t p, uint32_t sp, F14 *fault)
{
    L14 o;
    if (l14_begin(&o, h, fault)) return -1;
    const uint32_t frame = sp - 0x40u, lo = frame + 0x20u, hi = frame + 0x30u;
    uint8_t q[16];
    l14_q(&o, 0x00248650u, q);
    l14_wq(&o, lo, q);
    l14_q(&o, 0x00248660u, q);
    l14_wq(&o, hi, q);
    if (l14_c_001751A0(&o, p)) return -1;
    const int32_t mode = l14_s32(&o, p + 0x24C);
    if (mode == 0) {
        const uint32_t row = l14_u8(&o, p + 0x23F), cur = l14_u32(&o, p + 0x38);
        uint32_t limit = l14_u32(&o, lo + 4u * row);
        if (L14_LT(cur, limit)) {
            l14_w32(&o, p + 0x38, L14_ADD(cur, L14_DIV(limit, F_16)));
            const uint32_t row2 = l14_u8(&o, p + 0x23F), next = l14_u32(&o, p + 0x38);
            limit = l14_u32(&o, lo + 4u * row2);
            if (!L14_LE(next, limit)) l14_w32(&o, p + 0x38, limit);
        }
    } else if (mode == 1) {
        const uint32_t row = l14_u8(&o, p + 0x23F), cur = l14_u32(&o, p + 0x38);
        uint32_t limit = l14_u32(&o, hi + 4u * row);
        if (L14_LT(limit, cur)) {
            l14_w32(&o, p + 0x38, L14_SUB(cur, L14_DIV(L14_SUB(cur, limit), F_8)));
            const uint32_t row2 = l14_u8(&o, p + 0x23F), next = l14_u32(&o, p + 0x38);
            limit = l14_u32(&o, hi + 4u * row2);
            if (L14_LT(next, limit)) l14_w32(&o, p + 0x38, limit);
        }
    }
    return l14_end(&o);
}

/* ------------------------------------------------------------------------
 * 00224600 (p) -> 0 / 1 / 2: the reaction on the grip, by +7. 0 with +0x224,
 * +0x22C and bit 1 of +0xF all clear -> 0; else 001B61C0(0, 0xC0, 5, 1), the
 * sounds 0x152 / 0x153 with 0021C350 / 0021C270 for the nonzero floats, then
 * +7 = 0x1E (+0x220 <= 0 with +0xF = 0x63 or +0x234 = 1, after 0021C200), 0xA
 * (the other deaths), 0x14 (+0x228 >= 100 with D_008106F1), else 1; bit 1 of
 * +0xF clears +0xF. The rest play the clips; 2 on the two terminal changes.
 * ---------------------------------------------------------------------- */
int em_level14_port_00224600(const H14 *h, uint32_t p, int32_t *result, F14 *fault)
{
    L14 o;
    int32_t out = 1;
    if (!result || l14_begin(&o, h, fault)) return -1;
    const uint32_t st = l14_u8(&o, p + 7);
    switch (st) {
    case 0:
        if (L14_EQ(l14_u32(&o, p + 0x224), F_ZERO) && L14_EQ(l14_u32(&o, p + 0x22C), F_ZERO) &&
            !(l14_u8(&o, p + 0xF) & 2u)) {
            out = 0;
            break;
        }
        if (l14_c_001B61C0(&o, 0, 0xC0, 5, 1)) return -1;
        if (!L14_EQ(l14_u32(&o, p + 0x224), F_ZERO)) {
            if (l14_c_001FBD50(&o, p, 0x152, 0, F_300)) return -1;
            if (l14_c_0021C350(&o, p)) return -1;
        }
        if (!L14_EQ(l14_u32(&o, p + 0x22C), F_ZERO)) {
            if (l14_c_001FBD50(&o, p, 0x153, 0, F_300)) return -1;
            if (l14_c_0021C270(&o, p)) return -1;
        }
        if (L14_LE(l14_u32(&o, p + 0x220), F_ZERO)) {
            if (l14_u8(&o, p + 0xF) == 0x63 || l14_u8(&o, p + 0x234) == 1) {
                if (l14_c_0021C200(&o, p)) return -1;
                l14_w8(&o, p + 7, 0x1E);
            } else {
                l14_w8(&o, p + 7, 0xA);
            }
        } else if (!L14_LT(l14_u32(&o, p + 0x228), F_100) && l14_u8(&o, 0x008106F1u) != 0) {
            l14_w8(&o, p + 7, 0x14);
        } else {
            l14_w8(&o, p + 7, l14_u8(&o, p + 7) + 1u);
        }
        if (l14_u8(&o, p + 0xF) & 2u) l14_w8(&o, p + 0xF, 0);
        break;
    case 1:
        l14_w8(&o, p + 7, st + 1u);
        if (l14_u8(&o, p + 0xF) & 2u) {
            if (l14_c_001749A0(&o, p, 0x185, 0, F_4)) return -1;
            l14_w8(&o, p + 0xF, 0);
        } else {
            int32_t r = 0;
            if (l14_c_00122BB8(&o, &r)) return -1;
            if (l14_c_001749A0(&o, p, (r & 1) ? 0x185 : 0x184, 0, F_4)) return -1;
        }
        break;
    case 2:
    case 0x17:
        if (l14_u32(&o, p + 0x200) & 0x1000u) {
            l14_w8(&o, p + 7, 0);
            if (l14_c_001749A0(&o, p, 0x180, 0, F_8)) return -1;
            l14_w16(&o, p + 0x20E, 0x3C);
        }
        break;
    case 0xA:
        l14_w8(&o, p + 7, st + 1u);
        l14_c_001749A0(&o, p, 0x187, 0, F_8);
        break;
    case 0xB:
        if (l14_u32(&o, p + 0x200) & 0x1000u) {
            l14_w8(&o, p + 6, 0x14);
            l14_w8(&o, p + 7, 0);
            out = 2;
        }
        break;
    case 0x14:
        l14_w8(&o, p + 7, st + 1u);
        l14_c_001749A0(&o, p, 0x184, 0, F_4);
        break;
    case 0x15:
        if (L14_LE(l14_u32(&o, p + 0x3C), F_30)) {
            l14_w8(&o, p + 7, st + 1u);
            l14_c_0021C120(&o, p);
        }
        break;
    case 0x16: {
        int32_t r = 0;
        if (l14_c_0021C190(&o, p, &r)) return -1;
        if (r != 0) l14_w8(&o, p + 7, l14_u8(&o, p + 7) + 1u);
        break;
    }
    case 0x1E:
        l14_w8(&o, p + 7, st + 1u);
        if (l14_c_001749A0(&o, p, 0x186, 0, F_8)) return -1;
        l14_w32(&o, p + 0x21C, 0);
        l14_w32(&o, p + 0x2E4, 0);
        l14_w8(&o, p + 0x25F, 2);
        break;
    case 0x1F:
        if (!(l14_u32(&o, p + 0x200) & 0x8000u)) {
            l14_w8(&o, p + 7, st + 1u);
            l14_w32(&o, 0x00275B08u, 1);
        }
        break;
    case 0x20:
        if (l14_u32(&o, p + 0x200) & 0x1000u) {
            l14_w8(&o, p + 7, st + 1u);
        } else {
            uint32_t g = l14_u32(&o, 0x00275B40u);
            const uint32_t last = l14_u32(&o, p + 0x21C);
            const uint32_t y = l14_u32(&o, l14_u32(&o, g) + 8u);
            l14_w32(&o, p + 0x38, L14_SUB(y, last));
            l14_w32(&o, p + 0x21C, l14_u32(&o, l14_u32(&o, l14_u32(&o, 0x00275B40u)) + 8u));
            if (l14_c_00178B90(&o, p, 1)) return -1;
            g = l14_u32(&o, 0x00275B40u);
            const uint32_t past = l14_u32(&o, p + 0x2E4);
            const uint32_t x = l14_u32(&o, l14_u32(&o, g) + 4u);
            l14_w32(&o, p + 0x2EC, L14_SUB(x, past));
            l14_w32(&o, p + 0x2E4, l14_u32(&o, l14_u32(&o, l14_u32(&o, 0x00275B40u)) + 4u));
            if (L14_LT(l14_u32(&o, p + 0x2EC), 0xC0800000u)) l14_w32(&o, p + 0x2EC, 0xC0800000u);
            const uint32_t fall = l14_u32(&o, p + 0x2EC);
            l14_w32(&o, p + 0xB4, L14_ADD(l14_u32(&o, p + 0xB4), fall));
            int32_t r = 0;
            l14_c_00175900(&o, p, 1, &r);
        }
        break;
    case 0x21: {
        if (l14_c_00179880(&o, p, p + 0x2EC)) return -1;
        int32_t r = 0;
        if (l14_c_00175900(&o, p, 1, &r)) return -1;
        if (r != 0) {
            if (l14_c_00182870(&o, p, 1)) return -1;
            if (l14_c_001FBD50(&o, p, 0x156, 0, F_300)) return -1;
            l14_w8(&o, p + 4, 2);
            l14_w8(&o, p + 5, 3);
            l14_w8(&o, p + 6, 0);
            l14_w8(&o, p + 0x1F0, 0x3F);
            out = 2;
        }
        break;
    }
    default:
        break;
    }
    if (l14_end(&o)) return -1;
    *result = out;
    return 0;
}

/* The walk along the surface in 0016BF80's states 2..4: +0xB0 / +0xB8 by
 * +0x2E0 / +0x2E8 times +0x38, +0xB4 = 0x700031B4 - 22.5. */
static void grip_step(L14 *o, uint32_t p)
{
    for (uint32_t k = 0; k < 2; ++k) {
        const uint32_t step = l14_u32(o, p + 0x2E0 + 8u * k), speed = l14_u32(o, p + 0x38);
        const uint32_t at = l14_u32(o, p + 0xB0 + 8u * k);
        l14_w32(o, p + 0xB0 + 8u * k, L14_ADD(at, L14_MUL(step, speed)));
    }
    l14_w32(o, p + 0xB4, L14_SUB(l14_u32(o, HIT_POINT_Y), F_22_5));
}
/* The queued sound's stop: a live handle +0x31B goes to 0011A070, then +0x31B
 * = -1, +0x31A = 0. */
static int grip_stop(L14 *o, uint32_t p)
{
    const int32_t snd = l14_s8(o, p + 0x31B);
    if (snd == -1) return 0;
    if (l14_c_0011A070(o, snd)) return -1;
    l14_w8(o, p + 0x31B, 0xFF);
    l14_w8(o, p + 0x31A, 0);
    return 0;
}

/* ------------------------------------------------------------------------
 * 0016BF80 (p, sp): +5 0x17, by +6. 0: +6 = 1, +7 = 0, +0x2E4 = +0x38 = 0,
 * 001749A0(p, 0x180, 0, 8), +0x28 = 0. 1: 00224600 = 0: (area 0x15 with
 * 0x70003B8D) or D_00810E74 & 0x70003B76 -> +6 = 5; else 0016BE40, and a moving
 * +0x38 -> +6 += 1, 001FBD50(p, 0x136, 0, 300). 2: 00224600 = 2 or +0x25F =
 * 2: nothing; else 0016BB80, |+0x2E4| >= 0.01 -> +6 += 1, the loop sound 0x135
 * (+0x31B, +0x31A = 1, +0x31C = 0x135), else 0016BE40; the step. 3: the same
 * guard stops the sound; else 0016BB80, |+0x2E4| < 0.01 -> +6 += 1, the sound
 * stops, 0x136; else 0016BE40 and the sound's restart; the step. 4: the guard;
 * a miss -> +6 = 5 when 00224600 gave 0; +0x38 falls by 0.15 (at 0, the same
 * +6 = 5); the step. 5: +6 = 6, +0x1F0 = 0x2B, 001749A0(p, 0x181, 0, 8). 6:
 * bit 0x8000 clear -> 7. 7: +0x3C <= 23 -> 8, D_00275B08 = 1, sound 0x134. 8:
 * on bit 0x1000 -> 9, +0x2EC = 0, 00174A50(p, 4). 9: 00179880(p, p + 0x2EC),
 * 00175900(p, 1) nonzero: +5 = +6 = +0x1F0 = 0, 00182870(p, 1). 20: 0021D2E0
 * (p, 0x78, 1). 00224600 takes the record: its callers' a0 still holds it.
 * ---------------------------------------------------------------------- */
int em_level14_port_0016BF80(const H14 *h, uint32_t p, uint32_t sp, F14 *fault)
{
    L14 o;
    if (l14_begin(&o, h, fault)) return -1;
    const uint32_t st = l14_u8(&o, p + 6), fsp = sp - 0x30u;
    int32_t rc = 0, found = 0;
    uint32_t r = 0;
    switch (st) {
    case 0:
        l14_w8(&o, p + 6, st + 1u);
        l14_w8(&o, p + 7, 0);
        l14_w32(&o, p + 0x2E4, 0);
        l14_w32(&o, p + 0x38, 0);
        if (l14_c_001749A0(&o, p, 0x180, 0, F_8)) return -1;
        l14_w16(&o, p + 0x28, 0);
        break;
    case 1:
        if (em_level14_port_00224600(h, p, &rc, fault)) return -1;
        if (rc != 0) break;
        if ((l14_u8(&o, 0x00810700u) == 0x15 && l14_u8(&o, 0x70003B8Du) != 0) ||
            (l14_u16(&o, 0x00810E74u) & l14_u16(&o, 0x70003B76u))) {
            l14_w8(&o, p + 6, 5);
            break;
        }
        if (em_level14_port_0016BE40(h, p, fsp, fault)) return -1;
        if (!L14_EQ(l14_u32(&o, p + 0x38), F_ZERO)) {
            l14_w8(&o, p + 6, l14_u8(&o, p + 6) + 1u);
            l14_c_001FBD50(&o, p, 0x136, 0, F_300);
        }
        break;
    case 2:
        if (em_level14_port_00224600(h, p, &rc, fault)) return -1;
        if (rc == 2 || l14_u8(&o, p + 0x25F) == 2) break;
        if (em_level14_port_0016BB80(h, p, &found, fault)) return -1;
        if (l14_c_0011DF78(&o, l14_u32(&o, p + 0x2E4), &r)) return -1;
        if (!L14_LT(r, F_0_01)) {
            l14_w8(&o, p + 6, l14_u8(&o, p + 6) + 1u);
            if (l14_c_001FBD50(&o, p, 0x135, 0, F_300)) return -1;
            l14_w8(&o, p + 0x31B, (uint32_t)o.sound_result);
            l14_w8(&o, p + 0x31A, 1);
            l14_w16(&o, p + 0x31C, 0x135);
        } else if (em_level14_port_0016BE40(h, p, fsp, fault)) {
            return -1;
        }
        grip_step(&o, p);
        break;
    case 3:
        if (em_level14_port_00224600(h, p, &rc, fault)) return -1;
        if (rc == 2 || l14_u8(&o, p + 0x25F) == 2) {
            grip_stop(&o, p);
            break;
        }
        if (em_level14_port_0016BB80(h, p, &found, fault)) return -1;
        if (l14_c_0011DF78(&o, l14_u32(&o, p + 0x2E4), &r)) return -1;
        if (L14_LT(r, F_0_01)) {
            l14_w8(&o, p + 6, l14_u8(&o, p + 6) + 1u);
            if (grip_stop(&o, p)) return -1;
            if (l14_c_001FBD50(&o, p, 0x136, 0, F_300)) return -1;
        } else {
            if (em_level14_port_0016BE40(h, p, fsp, fault)) return -1;
            if (l14_u8(&o, p + 0x31A) != 0 && l14_u16(&o, p + 0x31C) == 0x5DD && l14_s8(&o, p + 0x31B) == -1) {
                if (l14_c_001FBD50(&o, p, 0x135, 0, F_300)) return -1;
                l14_w8(&o, p + 0x31B, (uint32_t)o.sound_result);
            }
        }
        grip_step(&o, p);
        break;
    case 4:
        if (em_level14_port_00224600(h, p, &rc, fault)) return -1;
        if (rc == 2 || l14_u8(&o, p + 0x25F) == 2) break;
        if (em_level14_port_0016BB80(h, p, &found, fault)) return -1;
        if (!found) {
            if (rc == 0) l14_w8(&o, p + 6, 5);
            break;
        }
        {
            const uint32_t speed = l14_u32(&o, p + 0x38);
            if (L14_LT(F_0_15, speed)) {
                l14_w32(&o, p + 0x38, L14_SUB(speed, F_0_15));
            } else {
                l14_w32(&o, p + 0x38, 0);
                if (rc == 0) l14_w8(&o, p + 6, 5);
            }
        }
        grip_step(&o, p);
        break;
    case 5:
        l14_w8(&o, p + 6, st + 1u);
        l14_w8(&o, p + 0x1F0, 0x2B);
        l14_c_001749A0(&o, p, 0x181, 0, F_8);
        break;
    case 6:
        if (!(l14_u32(&o, p + 0x200) & 0x8000u)) l14_w8(&o, p + 6, st + 1u);
        break;
    case 7:
        if (L14_LE(l14_u32(&o, p + 0x3C), F_23)) {
            l14_w8(&o, p + 6, st + 1u);
            l14_w32(&o, 0x00275B08u, 1);
            l14_c_001FBD50(&o, p, 0x134, 0, F_300);
        }
        break;
    case 8:
        if (l14_u32(&o, p + 0x200) & 0x1000u) {
            l14_w8(&o, p + 6, st + 1u);
            l14_w32(&o, p + 0x2EC, 0);
            l14_c_00174A50(&o, p, F_4);
        }
        break;
    case 9: {
        if (l14_c_00179880(&o, p, p + 0x2EC)) return -1;
        int32_t on = 0;
        if (l14_c_00175900(&o, p, 1, &on)) return -1;
        if (on != 0) {
            l14_w8(&o, p + 5, 0);
            l14_w8(&o, p + 6, 0);
            l14_w8(&o, p + 0x1F0, 0);
            l14_c_00182870(&o, p, 1);
        }
        break;
    }
    case 20:
        l14_c_0021D2E0(&o, p, 0x78, 1);
        break;
    default:
        break;
    }
    return l14_end(&o);
}
