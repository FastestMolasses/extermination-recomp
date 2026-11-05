/* AREA00 sub 2's creature: the per-frame pieces of states 1 and 2, the tilt
 * 0014E670, the bearing 0014E7A0 and the scale pulse 0014E890. From the
 * decomp C (src/func_*.c, byte-matched), checked against the original
 * instructions by tools/test_area00s2_creature_reference.py. */
#include "em_area00s2_creature_local.h"

/* 0014E670: the tilt, stepped by +7. Step 0 waits for the timer ctl+0x56;
 * at 0 the step byte on, ctl+0x58 = ctl+0x59, the timer drawn again
 * (0..127) and the turn speed ctl+0x3C = 0.5 (ctl+0x59 set) or 2.5; then
 * step 1 at once. Step 1 turns the float +0x78 of the record at +0x174
 * toward 0.6108653 (ctl+0x59 set) or 0 by ctl+0x3C degrees (001B12B0); on
 * arrival back to step 0 with ctl+0x59 = 1 - ctl+0x59. */
uint32_t a0s2_0014E670(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, target, speed, p, cur;
    const uint32_t st = B(self + 7u);
    if (st != 1) {
        if (st != 0) return 0;
        if (SH(ctl + 0x56u) != 0) return 0;
        WB(self + 7u, st + 1u);
        v = (uint32_t)SB(ctl + 0x59u);
        WB(ctl + 0x58u, v);
        C(00122BB8, &r);
        WH(ctl + 0x56u, ((uint32_t)r >> 17) & 0x7Fu);
        if (SB(ctl + 0x59u) != 0) W(ctl + 0x3Cu, F(0.5f));
        else W(ctl + 0x3Cu, F(2.5f));
    }
    target = SB(ctl + 0x59u) != 0 ? 0x3F1C61ABu : 0u;
    speed = R(ctl + 0x3Cu);
    p = R(self + 0x174u);
    cur = R(p + 0x78u);
    C(001B12B0, target, cur, DIV(MUL(F(3.1415927f), speed), F(180.0f)), &v);
    p = R(self + 0x174u);
    W(p + 0x78u, v);
    p = R(self + 0x174u);
    cur = R(p + 0x78u);
    if (EQ(target, cur)) {
        WB(self + 7u, 0);
        v = (uint32_t)SB(ctl + 0x59u);
        WB(ctl + 0x59u, 1u - v);
    }
    return 0;
}

/* 0014E7A0: the bearing. 0011E620(z, x) of the vector from the node
 * (+0xB0, +0xB8) to 0x810360 / 0x810368, wrapped by 001B1470, into ctl+0x38
 * and clamped to -pi/2 .. pi/2; then the node's position and 0x810360 are
 * copied to the scratchpad vectors 0x700038A0 / 0x700038B0 (00102948),
 * their +4 cleared, and their distance 001B15D0 goes to ctl+0x34. */
uint32_t a0s2_0014E7A0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    uint32_t pz, sz, sx, px, a;
    pz = R(PLAYER_POS + 8u);
    sz = R(self + 0xB8u);
    sx = R(self + 0xB0u);
    px = R(PLAYER_POS);
    C(0011E620, SUB(pz, sz), SUB(px, sx), &a);
    C(001B1470, a, &a);
    W(ctl + 0x38u, a);
    if (!LE(a, 0x3FC90FDBu)) W(ctl + 0x38u, 0x3FC90FDBu);
    else if (LT(a, 0xBFC90FDBu)) W(ctl + 0x38u, 0xBFC90FDBu);
    C(00102948, V38A0, self + 0xB0u);
    C(00102948, V38B0, PLAYER_POS);
    W(V38B0 + 4u, 0);
    W(V38A0 + 4u, 0);
    C(001B15D0, V38A0, V38B0, &a);
    W(ctl + 0x34u, a);
    return 0;
}

/* One fixed-point half of 0014E890: modff (0011E0A8) of `value` into the
 * frame local `ip`, then float_to_int (001281C0) of it into +0x8A and,
 * read again, +0x8C of the record at `field` (+0x13C or +0x140). */
static uint32_t fixed_pair(A0S2 *o, uint32_t self, uint32_t field, uint32_t ip, uint32_t value)
{
    int32_t n = 0;
    uint32_t dummy, p;
    C(0011E0A8, ip, value, &dummy);
    C(001281C0, R(ip), &n);
    p = R(self + field);
    WH(p + 0x8Au, (uint32_t)n);
    C(001281C0, R(ip), &n);
    p = R(self + field);
    WH(p + 0x8Cu, (uint32_t)n);
    return 0;
}

/* 0014E890: the scale pulse. While ctl+0x5C is set ctl+0x40 grows by 0.2
 * up to 2.0 (there ctl+0x5C is cleared); else it shrinks by 0.05 down to
 * 1.0. The integral part of 4096 * s goes to the halfwords +0x8A / +0x8C of
 * the record at +0x13C, that of 4096 / s to the record at +0x140 (the frame
 * local at sp - 4 holds the integral part). */
uint32_t a0s2_0014E890(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    const uint32_t ip = sp - 4u;
    uint32_t v;
    if (SB(ctl + 0x5Cu) != 0) {
        v = ADD(R(ctl + 0x40u), F(0.2f));
        W(ctl + 0x40u, v);
        if (!LT(v, F(2.0f))) {
            WB(ctl + 0x5Cu, 0);
            W(ctl + 0x40u, F(2.0f));
        }
    } else {
        v = SUB(R(ctl + 0x40u), F(0.05f));
        W(ctl + 0x40u, v);
        if (LE(v, F(1.0f))) W(ctl + 0x40u, F(1.0f));
    }
    v = R(ctl + 0x40u);
    SIB(fixed_pair(o, self, 0x13Cu, ip, MUL(F(4096.0f), v)));
    v = R(ctl + 0x40u);
    SIB(fixed_pair(o, self, 0x140u, ip, MUL(F(4096.0f), DIV(F(1.0f), v))));
    return 0;
}
