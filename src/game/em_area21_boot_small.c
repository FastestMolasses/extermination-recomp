/* AREA21's small creature (node behaviour 0014C220) and the two shared
 * spawners: 001283D0 (a 0x12A5D0 node at a point, used by the other AREA21
 * creatures) and 0014D1E0 (a 0014C220 node between two points, used by
 * 00137A50 and two other AREA21 creatures). From the decomp C
 * (src/func_*.c), checked against the original instructions by
 * tools/test_area21_boot_reference.py. The node is `self`; `ctl` =
 * self + 0x1F0 is its control block. */
#include "em_area21_boot_local.h"

/* 001283D0(pos, kind, vel): while the live count at 0x700031F4 is at most
 * 20, a class-2 node from 001AFA90: +3 and +0x9A cleared, the point pos
 * (three words) at +0xB0; kind 1: +0xD = 12 and the vector vel at +0xC0,
 * else +0xD = 10 and +0xC0..+0xC8 zero; the behaviour 0x12A5D0 at +0x10,
 * the scale 1.5, 1.5, 1.5, 1.0 at +0x60; the count steps up; result 1.
 * Result 0 at the cap or without a node. */
int32_t a21b_001283D0(A21B *o, uint32_t pos, int32_t kind, uint32_t vel)
{
    uint32_t p = 0;
    if ((int32_t)R(0x700031F4u) > 0x14) return 0;
    C(001AFA90, 2, &p);
    if (p == 0) return 0;
    WB(p + 3u, 0);
    WB(p + 0x9Au, 0);
    W(p + 0xB0u, R(pos));
    W(p + 0xB4u, R(pos + 4u));
    W(p + 0xB8u, R(pos + 8u));
    if (kind == 1) {
        WB(p + 0xDu, 0xC);
        W(p + 0xC0u, R(vel));
        W(p + 0xC4u, R(vel + 4u));
        W(p + 0xC8u, R(vel + 8u));
    } else {
        WB(p + 0xDu, 0xA);
        W(p + 0xC0u, 0);
        W(p + 0xC4u, 0);
        W(p + 0xC8u, 0);
    }
    W(p + 0x10u, 0x12A5D0u);
    W(p + 0x60u, 0x3FC00000u);
    W(p + 0x64u, 0x3FC00000u);
    W(p + 0x68u, 0x3FC00000u);
    W(p + 0x6Cu, 0x3F800000u);
    W(0x700031F4u, R(0x700031F4u) + 1u);
    return 1;
}

/* 0014D1E0(from, to, kind): a class-2 node from 001AFA90 (none: nothing):
 * +3 = 8, +0xD = kind, the behaviour 0014C220 at +0x10, the vectors from
 * and to copied to +0xB0 / +0xC0. */
uint32_t a21b_0014D1E0(A21B *o, uint32_t from, uint32_t to, int32_t kind)
{
    uint32_t p = 0;
    C(001AFA90, 2, &p);
    if (p == 0) return 0;
    WB(p + 3u, 8);
    WB(p + 0xDu, (uint32_t)kind);
    W(p + 0x10u, 0x14C220u);
    C(00102948, p + 0xB0u, from);
    C(00102948, p + 0xC0u, to);
    return 0;
}

/* 0014C220(self): the state byte +4 picks the handler: 0 set-up
 * 0014C2E0, 1 alive 0014C3A0, 2 the cry 0014CCC0, 3 the release 0014CD00
 * (which ends the frame there); then the byte countdown ctl+0x41 steps
 * toward 0. */
uint32_t a21b_0014C220(A21B *o, uint32_t self)
{
    const uint32_t ctl = self + 0x1F0u;
    int32_t n;
    switch (B(self + 4u)) {
    case 0: SIB(a21b_0014C2E0(o, self, ctl)); break;
    case 1: SIB(a21b_0014C3A0(o, self, ctl)); break;
    case 2: SIB(a21b_0014CCC0(o, self)); break;
    case 3: SIB(a21b_0014CD00(o, self)); return 0;
    default: break;
    }
    if ((n = SB(ctl + 0x41u)) != 0) WB(ctl + 0x41u, (uint32_t)(n - 1));
    return 0;
}

/* 0014C2E0: state 0, the set-up. The state byte steps up; header 1, the
 * clock rate ctl+0x20 = 1.0, the budget ctl+0x18 = 240, the placement
 * record 0x275408, hit points +0x34 = 10 when 0x81070A is set, else 5,
 * the scale 0.5, 0.5, 0.5, 1.0 at +0x60, +0x58 from 0x28A720; the model
 * 001B10B0(self, 0xA6, 0xA5), and when it returns 0, 001C63E0(self, 0) and
 * the move byte +5 = +0xD. */
uint32_t a21b_0014C2E0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    WB(self + 4u, B(self + 4u) + 1u);
    WB(self, 1);
    W(ctl + 0x20u, 0x3F800000u);
    W(ctl + 0x18u, 0xF0);
    W(self + 0x30u, 0x275408u);
    if (B(HARD) != 0) WH(self + 0x34u, 0xA);
    else WH(self + 0x34u, 0x5);
    W(self + 0x60u, 0x3F000000u);
    W(self + 0x64u, 0x3F000000u);
    W(self + 0x68u, 0x3F000000u);
    W(self + 0x6Cu, 0x3F800000u);
    W(self + 0x58u, R(0x28A720u));
    C(001B10B0, self, 0xA6, 0xA5, &r);
    if (r == 0) {
        C(001C63E0, self, 0);
        WB(self + 5u, B(self + 0xDu));
    }
    return 0;
}

/* 0014C3A0: state 1, alive. The move byte +5 picks the move (0 0014C680,
 * 1 0014C9D0, 2 0014C490); the heading +0xC0 = 001B1470(-1 *
 * 0011E620(ctl+0x24, ctl+0x2C)); then 001B5360, the motion 0014CD30, the
 * attack 0014CDD0, the animation clock ctl+0 = 001C64F0(self, ctl+0x20),
 * 001C68C0, 001B17A0 and the +0x4C method. */
uint32_t a21b_0014C3A0(A21B *o, uint32_t self, uint32_t ctl)
{
    uint32_t a, b, v;
    int32_t r = 0;
    switch (B(self + 5u)) {
    case 0: SIB(a21b_0014C680(o, self, ctl)); break;
    case 1: SIB(a21b_0014C9D0(o, self, ctl)); break;
    case 2: SIB(a21b_0014C490(o, self, ctl)); break;
    default: break;
    }
    a = R(ctl + 0x24u);
    b = R(ctl + 0x2Cu);
    C(0011E620, a, b, &v);
    C(001B1470, MUL(F(-1.0f), v), &v);
    W(self + 0xC0u, v);
    C(001B5360, self);
    SIB(a21b_0014CD30(o, self, ctl));
    SIB(a21b_0014CDD0(o, self, ctl));
    C(001C64F0, self, R(ctl + 0x20u), &r);
    W(ctl, (uint32_t)r);
    C(001C68C0, self);
    C(001B17A0, self, &r);
    METHOD(self);
    return 0;
}

/* 0014C490: move 2, the rise. The budget ctl+0x18 = 240, the countdown
 * ctl+0x41 = -1; step +6: 0 the rate ctl+0x24 = 0.02, the turn +0xC0 =
 * (-pi/2, 0, 0, 0), the position one of the eight points 0x245C60 by the
 * random's bits 9..11, its height kept at ctl+0x38, and the effect
 * 001EFD20(4) four units above it; 1 when the height has risen 3.5, the
 * step and the rate change ctl+0x28 = -1e-4; 2 once the rate is at most
 * 0.002: back to move 1, the countdown 100, the effect 001EFD90(3) at the
 * position with the vector (-pi/2, 0, 0, 1) and the cry 0x874. */
uint32_t a21b_0014C490(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t st, v;
    W(ctl + 0x18u, 0xF0);
    WB(ctl + 0x41u, 0xFF);
    st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 0x24u, F(0.02f));
        W(self + 0xC0u, 0xBFC90FDBu);
        W(self + 0xC4u, 0);
        W(self + 0xC8u, 0);
        W(self + 0xCCu, 0);
        C(00122BB8, &r);
        C(00102948, self + 0xB0u, 0x245C60u + ((uint32_t)r >> 9 & 7u) * 0x10u);
        W(ctl + 0x38u, R(self + 0xB4u));
        C(00102948, V38A0, self + 0xB0u);
        W(0x700038A4u, ADD(R(0x700038A4u), F(4.0f)));
        C(001EFD20, 4, V38A0);
        break;
    case 1:
        v = R(ctl + 0x38u);
        if (!LT(R(self + 0xB4u), ADD(F(3.5f), v))) {
            WB(self + 6u, st + 1u);
            W(ctl + 0x28u, 0xB8D1B717u);
        }
        break;
    case 2:
        if (LE(R(ctl + 0x24u), F(0.002f))) {
            WB(self + 5u, 1);
            WB(self + 6u, 0);
            WB(ctl + 0x41u, 0x64);
            C(00102948, V38A0, self + 0xB0u);
            W(0x700038B8u, 0);
            W(0x700038B4u, 0);
            W(0x700038B0u, 0);
            W(0x700038BCu, F(1.0f));
            W(0x700038B0u, F(-1.5707964f));
            C(001EFD90, 3, V38A0, V38B0);
            C(001FBD50, self, 0x874, 0, F(500.0f), &r);
        }
        break;
    default:
        break;
    }
    return 0;
}

/* The random fraction -0.01 * ((r >> shift) & 0xFF) / 255 - bias. */
static uint32_t a21b_jitter(uint32_t scale, int32_t r, unsigned shift, uint32_t bias)
{
    return A21B_SUB(A21B_MUL(scale, A21B_DIV(A21B_CVT_S_W((uint32_t)r >> shift & 0xFFu), a21b_bits(255.0f))), bias);
}

/* 0014C680: move 0. Step +6: 0 (once) ctl+0x2C = 1.88; while the node's
 * height is below the player's plus 10, the angle -atan2(fabs(the
 * difference), the flat distance) (scratch 0x70003A20 / 0x70003A24) and,
 * when it is negative, ctl+0x24 = the y of the point (0, 0, 1.88) turned
 * by it (001029C0, 00102B08, 001026A0); then, as step 1: the turn
 * countdown ctl+0x14 (at 0 a random 0..15, the bearing ctl+0x34 to the
 * player and the turn +0xC4 by pi/40 toward it); while ctl+0x28 is not
 * negative a new ctl+0x28 = -0.01 * random / 255 - 0.035 when the height
 * difference to the player is above 6.5 and the flat distance at most
 * 100, else when 001B1560(self, player, 80 degrees) is 0. */
uint32_t a21b_0014C680(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t st, a, b, v, x, z;
    st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 0x2Cu, F(1.88f));
        C(00102948, V38A0, self + 0xB0u);
        C(00102948, V38B0, PLAYER_POS);
        b = R(0x700038B4u);
        a = R(0x700038A4u);
        b = ADD(b, F(10.0f));
        W(0x700038B4u, b);
        if (LT(a, b)) {
            C(0011DF78, SUB(a, b), &v);
            W(0x70003A20u, v);
            W(0x700038B4u, 0);
            W(0x700038A4u, 0);
            C(001B15D0, V38A0, V38B0, &v);
            W(0x70003A24u, v);
            a = R(0x70003A20u);
            b = R(0x70003A24u);
            C(0011E620, a, b, &v);
            v = MUL(F(-1.0f), v);
            W(0x70003A20u, v);
            if (LT(v, 0)) {
                C(001029C0, V36A0);
                C(00102B08, V36A0, V36A0, R(0x70003A20u));
                W(0x700038A0u, 0);
                W(0x700038A4u, 0);
                W(0x700038A8u, F(1.88f));
                W(0x700038ACu, F(1.0f));
                C(001026A0, V38A0, V36A0, V38A0);
                W(ctl + 0x24u, R(0x700038A4u));
            }
        }
        /* fall through */
    case 1:
        v = R(ctl + 0x14u);
        if (v != 0) {
            W(ctl + 0x14u, v - 1u);
        } else {
            C(00122BB8, &r);
            W(ctl + 0x14u, (uint32_t)r >> 15 & 0xFu);
            x = R(PLAYER_POS);
            z = R(PLAYER_POS + 8u);
            C(001B1240, self + 0xB0u, x, z, &v);
            W(ctl + 0x34u, v);
            x = R(self + 0xC4u);
            C(001B12B0, R(ctl + 0x34u), x, F(0.07853981852531433f), &v);
            W(self + 0xC4u, v);
        }
        if (LT(R(ctl + 0x28u), 0)) break;
        C(00102948, V38A0, self + 0xB0u);
        C(00102948, V38B0, PLAYER_POS);
        C(0011DF78, SUB(R(0x700038A4u), R(0x700038B4u)), &v);
        W(0x70003A20u, v);
        W(0x700038B4u, 0);
        W(0x700038A4u, 0);
        if (!LE(R(0x70003A20u), F(6.5f))) {
            C(001B15D0, V38A0, V38B0, &v);
            if (LE(v, F(100.0f))) goto reroll;
        }
        C(001B1560, self, V38B0, F(1.3962634801864624f), &r);
        if (r != 0) break;
    reroll:
        C(00122BB8, &r);
        W(ctl + 0x28u, a21b_jitter(F(-0.01f), r, 11, F(0.035f)));
        break;
    default:
        break;
    }
    return 0;
}

/* 0014C9D0: move 1. Step +6: 0 (once) ctl+4 = 0, ctl+0x24 = 3.0,
 * ctl+0x28 = -0.01 * random / 255 - 0.015, ctl+0x2C = 0.3 * random / 255
 * - 0.15, the budget ctl+0x18 = 600; then, as step 1: the budget is
 * cleared when 0x81080F is 1; while ctl+0x24 is negative: the turn
 * countdown ctl+0x14 (at 0 a random 0..7, the bearing ctl+0x34 and the
 * turn +0xC4 by 0.0611), ctl+0x24 held at -0.96 once it reaches it with
 * ctl+0x28 non-zero (ctl+0x28 then 0), and ctl+0x2C = the z of the point
 * (0, 0, 1.1) turned by 001B1470(atan2(fabs(the height difference to the
 * player), the flat distance)). */
uint32_t a21b_0014C9D0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t st, v, t, x, z;
    st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 4u, 0);
        W(ctl + 0x24u, F(3.0f));
        C(00122BB8, &r);
        W(ctl + 0x28u, a21b_jitter(F(-0.01f), r, 19, F(0.015f)));
        C(00122BB8, &r);
        W(ctl + 0x2Cu, a21b_jitter(F(0.3f), r, 12, F(0.15f)));
        W(ctl + 0x18u, 0x258);
        /* fall through */
    case 1:
        if (B(0x81080Fu) == 1) W(ctl + 0x18u, 0);
        if (!LT(R(ctl + 0x24u), 0)) break;
        v = R(ctl + 0x14u);
        if (v != 0) {
            W(ctl + 0x14u, v - 1u);
        } else {
            C(00122BB8, &r);
            W(ctl + 0x14u, (uint32_t)r >> 15 & 7u);
            x = R(PLAYER_POS);
            z = R(PLAYER_POS + 8u);
            C(001B1240, self + 0xB0u, x, z, &v);
            W(ctl + 0x34u, v);
            x = R(self + 0xC4u);
            C(001B12B0, R(ctl + 0x34u), x, F(0.06108652800321579f), &v);
            W(self + 0xC4u, v);
        }
        if (!EQ(R(ctl + 0x28u), 0) && LE(R(ctl + 0x24u), F(-0.96000004f))) {
            W(ctl + 0x24u, F(-0.96000004f));
            W(ctl + 0x28u, 0);
        }
        C(00102948, V38A0, PLAYER_POS);
        C(00102948, V38B0, self + 0xB0u);
        W(0x700038B4u, 0);
        W(0x700038A4u, 0);
        C(001B15D0, V38A0, V38B0, &v);
        W(0x70003A20u, v);
        x = R(self + 0xB4u);
        C(0011DF78, SUB(R(PLAYER_POS + 4u), x), &t);
        W(0x70003A24u, t);
        C(0011E620, t, R(0x70003A20u), &v);
        C(001B1470, v, &v);
        W(0x70003A20u, v);
        C(001029C0, V36A0);
        C(00102B08, V36A0, V36A0, R(0x70003A20u));
        W(0x700038A0u, 0);
        W(0x700038A4u, 0);
        W(0x700038A8u, F(1.1f));
        W(0x700038ACu, F(1.0f));
        C(001026A0, V38A0, V36A0, V38A0);
        W(ctl + 0x2Cu, R(0x700038A8u));
        break;
    default:
        break;
    }
    return 0;
}

/* 0014CCC0: state 2, the cry 0x875 (300), then state 3. */
uint32_t a21b_0014CCC0(A21B *o, uint32_t self)
{
    int32_t r = 0;
    C(001FBD50, self, 0x875, 0, F(300.0f), &r);
    WB(self + 4u, 3);
    return 0;
}

/* 0014CD00: state 3, the release: 001B1190(+0x9A), 001AFC10(self). */
uint32_t a21b_0014CD00(A21B *o, uint32_t self)
{
    C(001B1190, (int32_t)B(self + 0x9Au));
    C(001AFC10, self);
    return 0;
}

/* 0014CD30: the motion, unless ctl+0x40 is set: the height ctl+0x24 gains
 * the fall rate ctl+0x28 and the speed ctl+0x2C its change ctl+0x30; the
 * node rises by the height, and moves forward by the speed along the
 * heading +0xC4 (x by its sine, z by its cosine). */
uint32_t a21b_0014CD30(A21B *o, uint32_t self, uint32_t ctl)
{
    uint32_t h, s, t;
    if (SB(ctl + 0x40u) != 0) return 0;
    h = R(ctl + 0x28u);
    W(ctl + 0x24u, ADD(R(ctl + 0x24u), h));
    s = R(ctl + 0x30u);
    W(ctl + 0x2Cu, ADD(R(ctl + 0x2Cu), s));
    h = R(ctl + 0x24u);
    W(self + 0xB4u, ADD(R(self + 0xB4u), h));
    C(0011E2A8, R(self + 0xC4u), &t);
    s = R(ctl + 0x2Cu);
    W(self + 0xB0u, ADD(R(self + 0xB0u), MUL(s, t)));
    C(0011DE90, R(self + 0xC4u), &t);
    s = R(ctl + 0x2Cu);
    W(self + 0xB8u, ADD(R(self + 0xB8u), MUL(s, t)));
    return 0;
}

/* The landing: the effect D_00245CE0[+0xD] at the node with the vector
 * (0, 0, 0, 1), and state 2. */
static uint32_t a21b_land(A21B *o, uint32_t self)
{
    W(0x700038A8u, 0);
    W(0x700038A4u, 0);
    W(0x700038A0u, 0);
    W(0x700038ACu, 0x3F800000u);
    C(001EFD90, (int32_t)R(0x245CE0u + 4u * B(self + 0xDu)), self + 0xB0u, V38A0);
    WB(self + 4u, 2);
    return 0;
}

/* 0014CDD0: the attack. The budget ctl+0x18 counts down (at 0: state 2).
 * At most 5 high: the splash 001EFD20(0x8000005F) at the node raised to
 * 10, the cry 0x872 (800) and state 3. While ctl+0x41 is 0: the point
 * (0, 0, 4) turned by 001B2B10 plus the position against
 * 0019AD00(self, point, 6), and while ctl+0x24 is negative the position
 * lowered by 2 with the offset (0, -8, 0) against 0019AB20(self, point,
 * offset, 7): a hit lands (the effect D_00245CE0[+0xD], state 2). When
 * 0021BE40(0x8102B0, self) is 0 and 001A7B80(self) is not: land, the
 * byte 0x8102B0 gains bit 2 and 0x8104D4 / 0x8104DC = 20 / 25 when
 * 0x81070A is set, else 15 / 20. A hit +0x36 whose amount (& 0xFFF)
 * reaches the hit points +0x34 lands; a smaller one sets the header 1 and
 * is taken off +0x34. */
uint32_t a21b_0014CDD0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, wind;
    uint32_t v = R(ctl + 0x18u);
    if (v != 0) {
        W(ctl + 0x18u, v - 1u);
    } else {
        WB(self + 4u, 2);
        return 0;
    }
    if (LE(R(self + 0xB4u), F(5.0f))) {
        C(00102948, V38A0, self + 0xB0u);
        W(0x700038A4u, 0x41200000u);
        C(001EFD20, (int32_t)0x8000005Fu, V38A0);
        C(001FBD50, self, 0x872, 0, F(800.0f), &r);
        WB(self + 4u, 3);
        return 0;
    }
    if (SB(ctl + 0x41u) == 0) {
        W(0x700038A0u, 0);
        W(0x700038A4u, 0);
        W(0x700038A8u, 0x40800000u);
        W(0x700038ACu, 0x3F800000u);
        C(001B2B10, self, V38A0, V38A0);
        C(001028B8, V38A0, self + 0xB0u, V38A0);
        C(0019AD00, self, V38A0, 6, &r);
        if (r != 0) return a21b_land(o, self);
        if (LT(R(ctl + 0x24u), 0)) {
            W(0x700038A0u, 0);
            W(0x700038B0u, 0);
            W(0x700038A4u, 0xC0000000u);
            W(0x700038A8u, 0);
            W(0x700038ACu, 0x3F800000u);
            W(0x700038B4u, 0xC1000000u);
            W(0x700038B8u, 0);
            W(0x700038BCu, 0x3F800000u);
            C(001028B8, V38A0, V38A0, self + 0xB0u);
            C(0019AB20, self, V38A0, V38B0, 7, &r);
            if (r != 0) return a21b_land(o, self);
        }
    }
    C(0021BE40, 0x8102B0u, self, &r);
    if (r == 0) {
        C(001A7B80, self, &r);
        if (r != 0) {
            SIB(a21b_land(o, self));
            v = B(0x8102B0u);
            r = (int32_t)B(HARD);
            WB(0x8102B0u, v | 2u);
            if (r != 0) {
                W(0x8104D4u, 0x41A00000u);
                W(0x8104DCu, 0x41C80000u);
            } else {
                W(0x8104D4u, 0x41700000u);
                W(0x8104DCu, 0x41A00000u);
            }
            return 0;
        }
    }
    wind = SH(self + 0x36u);
    if (wind == 0) return 0;
    if (SH(self + 0x34u) <= (wind & 0xFFF)) return a21b_land(o, self);
    WB(self, 1);
    wind = SH(self + 0x36u) & 0xFFF;
    WH(self + 0x34u, (uint32_t)(SH(self + 0x34u) - wind));
    WH(self + 0x36u, 0);
    return 0;
}
