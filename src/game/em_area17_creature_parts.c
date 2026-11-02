/* The AREA17 creature's parts: the damage step 001368D0 (hit points, the
 * five breakable parts of ctl+0x5B and the death), the part glows
 * 00136CB0 / 00136C40 and the part-break effect 00136F20. From the decomp
 * C, checked against the original instructions by
 * tools/test_area17_creature_reference.py. ctl = self + 0x1F0. */
#include "em_area17_creature_local.h"

/* The sign extension of the low halfword. */
static int32_t a17c_half(uint32_t v) { return (int16_t)(uint16_t)v; }

/* The break of one part (001368D0): the part bits keep `keep`, the hit
 * word +0x36 gains bit 0x8000, the effect 001EFE00(`effect`, self) and
 * the break 00136F20(self, `which`). */
static uint32_t a17c_break(A17C *o, uint32_t self, uint32_t ctl, int32_t parts, int32_t keep, uint32_t effect,
                           int32_t which)
{
    int32_t r = 0;
    WB(ctl + 0x5Bu, (uint32_t)(parts & keep));
    WH(self + 0x36u, (uint32_t)SH(self + 0x36u) | 0xFFFF8000u);
    C(001EFE00, (int32_t)effect, self, &r);
    SIB(a17c_00136F20(o, self, which));
    return 0;
}

/* 001368D0(self, ctl): the damage of a hit (the hit word +0x36: amount
 * & 0xFFF, x5 with bit 0x8000; bit 0x4000 an effect at most every 60
 * frames). With no hit: 0. During the grace ctl+0x52 the hit is dropped.
 * Else the header is 2, the grace 255 frames, the damage sum ctl+0x54
 * (reset unless ctl+0x58 still runs, which is set to 25) and the hit
 * points +0x34 drop (at 0: the break effect 00136F20(self, 0)). Parts
 * break by hit points: bit 0x10 below 401 (the effect 0x80000058 at the
 * bone of table offset 0x6C, whose three turn halfwords are cleared), bit 2 below 301, bit 4 below 151,
 * bit 1 below 51. No hit points left, 25 damage in the window, or a kind
 * bit 0xA000: state 2 (dying), result 1. Else the grace is 30 frames for
 * kind 0x5000 hits, else 0, the header 1, the hit word cleared, result 0. */
int32_t a17c_001368D0(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t hit, delta, hp, parts, r = 0;
    uint32_t t, bone;
    if (SH(self + 0x36u) == 0) return 0;
    if (SH(ctl + 0x52u) != 0) {
        WH(self + 0x36u, 0);
        return 0;
    }
    WB(self, 2);
    WH(ctl + 0x52u, 0xFF);
    hit = SH(self + 0x36u);
    delta = a17c_half((uint32_t)hit & 0xFFF);
    if (hit & 0x8000) delta = a17c_half((uint32_t)delta * 5u);
    if (SB(ctl + 0x58u) != 0) WH(ctl + 0x54u, (uint32_t)(SH(ctl + 0x54u) + delta));
    else WH(ctl + 0x54u, (uint32_t)delta);
    WB(ctl + 0x58u, 0x19);
    if (SB(ctl + 0x5Eu) == 0 && (SH(self + 0x36u) & 0x4000)) {
        WB(ctl + 0x5Eu, 0x3C);
        C(001EFE00, (int32_t)0x80000027u, self, &r);
    }
    hp = SH(self + 0x34u);
    if (delta < hp) {
        WH(self + 0x34u, (uint32_t)(hp - delta));
    } else {
        WH(self + 0x34u, 0);
        SIB(a17c_00136F20(o, self, 0));
    }
    parts = SB(ctl + 0x5Bu);
    if ((parts & 0x10) && SH(self + 0x34u) < 0x191) {
        WB(ctl + 0x5Bu, (uint32_t)(parts & 0xF));
        WH(self + 0x36u, (uint32_t)SH(self + 0x36u) | 0xFFFF8000u);
        W(0x700038A0u, F(2.0f));
        W(0x700038A4u, F(4.25f));
        W(0x700038A8u, 0);
        t = R(0x275B40u);
        W(0x700038ACu, F(1.0f));
        bone = R(t + 0x6Cu);
        C(00102958, V36A0, bone + 0x90u);
        C(001026A0, V38A0, V36A0, V38A0);
        t = R(0x275B40u);
        bone = R(t + 0x6Cu);
        WH(bone + 0x88u, 0);
        t = R(0x275B40u);
        bone = R(t + 0x6Cu);
        WH(bone + 0x8Au, 0);
        t = R(0x275B40u);
        bone = R(t + 0x6Cu);
        WH(bone + 0x8Cu, 0);
        C(001EFD90, (int32_t)0x80000058u, V38A0, self + 0xC0u);
    }
    parts = SB(ctl + 0x5Bu);
    if ((parts & 2) && SH(self + 0x34u) < 0x12D) SIB(a17c_break(o, self, ctl, parts, 0x1D, 0x80000055u, 2));
    parts = SB(ctl + 0x5Bu);
    if ((parts & 4) && SH(self + 0x34u) < 0x97) SIB(a17c_break(o, self, ctl, parts, 0x1B, 0x80000057u, 3));
    parts = SB(ctl + 0x5Bu);
    if ((parts & 1) && SH(self + 0x34u) < 0x33) SIB(a17c_break(o, self, ctl, parts, 0x1E, 0x80000056u, 1));
    if (SH(self + 0x34u) == 0 || SH(ctl + 0x54u) >= 0x19) {
        hit = 0xA000;
    } else {
        hit = SH(self + 0x36u);
        if ((hit & 0xA000) == 0) {
            WH(ctl + 0x52u, (hit & 0x5000) ? 0x1E : 0);
            WB(self, 1);
            WH(self + 0x36u, 0);
            return 0;
        }
    }
    WB(self + 4u, 2);
    WB(self + 5u, 0);
    WB(self + 6u, 0);
    WH(ctl + 0x54u, 0);
    return 1;
}

/* 00136C40(bone, point, colour, size): the point from the bone
 * ((0x275B40)->[bone] + 0x90) in place, then the glow 001F4E20. */
uint32_t a17c_00136C40(A17C *o, int32_t index, uint32_t point, uint32_t colour, uint32_t size)
{
    const uint32_t t = R(0x275B40u);
    const uint32_t node = R(t + (uint32_t)index * 4u);
    C(001026A0, point, node + 0x90u, point);
    C(001F4E20, point, colour, size);
    return 0;
}

/* One glow of 00136CB0: the offset (x, y, z) at bone `bone`, the colour
 * (red, green, blue, 0x80) and the size. */
static uint32_t a17c_glow(A17C *o, int32_t bone, uint32_t x, uint32_t y, uint32_t z, uint32_t red, uint32_t green,
                          uint32_t blue, uint32_t size)
{
    W(0x700038A0u, x);
    W(0x700038A4u, y);
    W(0x700038A8u, z);
    W(0x700038ACu, F(1.0f));
    W(0x700038B0u, red);
    W(0x700038B4u, green);
    W(0x700038B8u, blue);
    W(0x700038BCu, 0x80);
    SIB(a17c_00136C40(o, bone, V38A0, V38B0, size));
    return 0;
}

/* 00136CB0(self, ctl): the glows of the intact parts: one at bone 0xF
 * while bit 0x10 of ctl+0x5B is clear, then one at each
 * live part (bit 1 bone 0x19, bit 2 bone 3, bit 4 bone 0xD). The first
 * glow has the colour (0x80, 0x50, 0x30) and size 8, the others (0x30,
 * 0x80, 0x30) and size 5. */
uint32_t a17c_00136CB0(A17C *o, uint32_t self, uint32_t ctl)
{
    (void)self;
    if (!(SB(ctl + 0x5Bu) & 0x10))
        SIB(a17c_glow(o, 0xF, 0x3FD78D50u, 0x4067DF3Bu, 0x3E841893u, 0x80, 0x50, 0x30, F(8.0f)));
    if (SB(ctl + 0x5Bu) & 1)
        SIB(a17c_glow(o, 0x19, 0x409147AEu, 0x3D072B02u, 0xBD656042u, 0x30, 0x80, 0x30, F(5.0f)));
    if (SB(ctl + 0x5Bu) & 2)
        SIB(a17c_glow(o, 3, 0x40177CEEu, 0x40B12F1Bu, 0x403CAC08u, 0x30, 0x80, 0x30, F(5.0f)));
    if (SB(ctl + 0x5Bu) & 4)
        SIB(a17c_glow(o, 0xD, 0x409E147Bu, 0x3D54FDF4u, 0xBCBC6A7Fu, 0x30, 0x80, 0x30, F(5.0f)));
    return 0;
}

/* 00136F20(self, which): the break effect 001EFD90(0x80000036) of part
 * `which` (0 bone 0xF, 1 bone 0x19, 2 bone 3, 3 bone 0xD) at
 * the part's point, moving with the node (+0xC0). The original's other
 * selectors read a register no caller sets; they fail-stop here. */
uint32_t a17c_00136F20(A17C *o, uint32_t self, int32_t which)
{
    static const uint32_t point[4][3] = {
        {0x3FD78D50u, 0x4067DF3Bu, 0x3E841893u}, {0x409147AEu, 0x3D072B02u, 0xBD656042u},
        {0x40177CEEu, 0x40B12F1Bu, 0x403CAC08u}, {0x409E147Bu, 0x3D54FDF4u, 0xBCBC6A7Fu}};
    static const uint32_t bone[4] = {0xF, 0x19, 3, 0xD};
    uint32_t t, node;
    if (which < 0 || which > 3) return (uint32_t)a17c_latch(o, 0x00136F20u, EM_AREA17_CREATURE_FAULT_WORKER_FAILED);
    W(0x700038A0u, point[which][0]);
    W(0x700038A4u, point[which][1]);
    W(0x700038A8u, point[which][2]);
    W(0x700038ACu, F(1.0f));
    t = R(0x275B40u);
    node = R(t + bone[which] * 4u);
    C(001026A0, V38A0, node + 0x90u, V38A0);
    C(001EFD90, (int32_t)0x80000036u, 0x70003600u, self + 0xC0u);
    return 0;
}
