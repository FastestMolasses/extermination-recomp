/* The AREA17 creature's spit: the launch 001370C0 (a class 2 node with the
 * behaviour 001373B0) and the spit's flight 001373B0. From the decomp C
 * (NEARMISS: body-correct), checked against the original instructions by
 * tools/test_area17_creature_reference.py. */
#include "em_area17_creature_local.h"

/* The velocity (+0xC0/+0xC4/+0xC8) of a spit node `n` from its yaw p+0x24
 * and pitch p+0x28 at speed 2.2 (sin yaw, sin pitch, cos yaw). */
static uint32_t a17c_velocity(A17C *o, uint32_t n, uint32_t p)
{
    uint32_t v;
    v = R(p + 0x24u);
    C(0011E2A8, v, &v);
    W(n + 0xC0u, MUL(0x400CCCCDu, v));
    v = R(p + 0x28u);
    C(0011E2A8, v, &v);
    W(n + 0xC4u, MUL(0x400CCCCDu, v));
    v = R(p + 0x24u);
    C(0011DE90, v, &v);
    W(n + 0xC8u, MUL(0x400CCCCDu, v));
    return 0;
}

/* 001370C0(bone, kind): a spit from the mouth bone ((0x275B40)->[bone]):
 * 001AFA90(2) gives the node (none: nothing); class byte +3 = 3, +0xD =
 * kind, behaviour 001373B0. The target p = n + 0x1F0 is the point (70,
 * 230, 270) for kind bit 2, else the player's position (0x810360); the
 * node starts 9 along the bone; the yaw p+0x24 is the bone's direction,
 * the pitch p+0x28 -4 degrees for kind bit 2, else the climb from the
 * node to 20 above the target over the flat distance (at most 45
 * degrees, never below 0). */
uint32_t a17c_001370C0(A17C *o, int32_t index, uint32_t kind)
{
    uint32_t n = 0, p, t, b, d, ang, v, x, z, w;
    int32_t steep;
    C(001AFA90, 2, &n);
    if (n == 0) return 0;
    WB(n + 3u, 3);
    steep = (int32_t)(kind & 0xFFu & 2u);
    WB(n + 0xDu, kind);
    p = n + 0x1F0u;
    W(n + 0x10u, 0x1373B0u);
    if (steep) {
        W(0x700038D0u, F(70.0f));
        W(0x700038D4u, F(230.0f));
        W(0x700038D8u, F(270.0f));
        W(0x700038DCu, F(1.0f));
    } else {
        C(00102948, 0x700038D0u, PLAYER_POS);
    }
    W(p, R(0x700038D0u));
    W(p + 4u, R(0x700038D4u));
    W(p + 8u, R(0x700038D8u));
    W(p + 0xCu, R(0x700038DCu));
    t = R(0x275B40u);
    v = R(t + (uint32_t)index * 4u);
    C(00102958, V36A0, v + 0x90u);
    W(0x700038A0u, F(9.0f));
    W(0x700038A4u, 0);
    W(0x700038A8u, 0);
    W(0x700038ACu, F(1.0f));
    C(001026A0, V38A0, V36A0, V38A0);
    W(0x700036D8u, 0);
    W(0x700036D4u, 0);
    W(0x700036D0u, 0);
    W(n + 0xB0u, R(0x700038A0u));
    W(n + 0xB4u, R(0x700038A4u));
    W(n + 0xB8u, R(0x700038A8u));
    C(00102948, V38B0, 0x700038D0u);
    W(0x700038B4u, 0);
    W(0x700038A4u, 0);
    C(001B15D0, V38A0, V38B0, &t);
    b = R(0x700038D4u);
    w = R(n + 0xB4u);
    b = ADD(F(20.0f), b);
    W(0x700038A4u, w);
    d = SUB(b, w);
    if (LT(d, 0)) d = 0;
    x = R(V36A0);
    z = R(V36A0 + 8u);
    C(0011E620, x, z, &v);
    C(001B1470, v, &v);
    W(p + 0x24u, v);
    if (steep) {
        W(p + 0x28u, 0xBD8EFA35u);
    } else {
        C(0011E620, d, t, &ang);
        C(001B1470, ang, &ang);
        W(p + 0x28u, ang);
        if (!LE(ang, 0x3F490FDBu)) W(p + 0x28u, 0x3F490FDBu);
    }
    SIB(a17c_velocity(o, n, p));
    return 0;
}

/* The flat distance from the spit's target to the node into 0x700038A0 /
 * 0x700038B0 (the node's height kept in `rise` as target y - node y). */
static uint32_t a17c_spit_range(A17C *o, uint32_t self, uint32_t p, uint32_t *rise, uint32_t *range)
{
    uint32_t a, b;
    C(00102948, V38A0, p);
    C(00102948, V38B0, self + 0xB0u);
    a = R(0x700038A4u);
    b = R(0x700038B4u);
    W(0x700038A4u, 0);
    *rise = SUB(a, b);
    W(0x700038B4u, 0);
    C(001B15D0, V38A0, V38B0, range);
    return 0;
}

/* 001373B0(self): the spit node's behaviour (p = self + 0x1F0).
 * State 0: the velocity x4 (00103230), the trail effect 001EFE00
 * (0x80000037) in +0x20, the last position +0xA0, the target's height
 * raised by 0..15. State 1 counts the flight p+0x10; from frame 10 to 300
 * (kind bit 1 clear) the yaw and pitch home on the target at 0.7 degrees
 * a frame; the node moves by its velocity; it bursts (state 2) when it
 * leaves the arena (269 from the centre for kind bit 2, else 190), when
 * it comes within 15 of the target after frame 20 (kind bit 2 clear), or
 * when 0019A570 finds a wall between the last and the new position (the
 * node is put at the hit point 0x700031B0); its matrix +0xD0 follows
 * (001C94B0). State 2 ends the trail, the burst effect 001EFD90
 * (0x8000006B) and the sound 0x449 or 0x44A. State 3 frees the node. */
uint32_t a17c_001373B0(A17C *o, uint32_t self)
{
    const uint32_t p = self + 0x1F0u;
    int32_t r = 0;
    uint32_t v, w, rise = 0, range = 0, x, z, e, limit;
    const uint32_t state = B(self + 4u);
    switch (state) {
    case 0:
        WB(self + 4u, state + 1u);
        W(p + 0x10u, 0);
        C(00103230, self + 0xC0u, self + 0xC0u, F(4.0f));
        C(001EFE00, (int32_t)0x80000037u, self, &r);
        W(self + 0x20u, (uint32_t)r);
        W(self + 0xA0u, R(self + 0xB0u));
        W(self + 0xA4u, R(self + 0xB4u));
        W(self + 0xA8u, R(self + 0xB8u));
        W(self + 0xACu, R(self + 0xBCu));
        C(00122BB8, &r);
        v = MUL(F(15.0f), DIV(A17C_CVT_S_W(a17c_sra((uint32_t)r, 16) & 0xFF), F(255.0f)));
        W(p + 4u, ADD(R(p + 4u), v));
        return 0;
    case 1:
        break;
    case 2:
        WB(self + 4u, state + 1u);
        e = R(self + 0x20u);
        WB(e + 4u, 2);
        C(001EFD90, (int32_t)0x8000006Bu, self + 0xB0u, self + 0xC0u);
        C(00122BB8, &r);
        C(001FBD50, self, (a17c_sra((uint32_t)r, 18) & 1) + 0x449, 0, F(450.0f), &r);
        return 0;
    case 3:
        C(001AFC10, self);
        return 0;
    default:
        return 0;
    }
    W(p + 0x10u, R(p + 0x10u) + 1u);
    SIB(a17c_spit_range(o, self, p, &rise, &range));
    v = R(p + 0x10u);
    if ((int32_t)v >= 0xA && (int32_t)v < 0x12D && !(B(self + 0xDu) & 1)) {
        x = R(p);
        z = R(p + 8u);
        C(001B1240, self + 0xB0u, x, z, &v);
        W(0x70003A20u, v);
        w = R(p + 0x24u);
        v = R(0x70003A20u);
        C(001B12B0, v, w, 0x3C482B17u, &v);
        W(p + 0x24u, v);
        C(0011E620, rise, range, &v);
        C(001B1470, v, &v);
        W(0x70003A20u, v);
        w = R(p + 0x28u);
        v = R(0x70003A20u);
        C(001B12B0, v, w, 0x3C482B17u, &v);
        W(p + 0x28u, v);
    }
    SIB(a17c_velocity(o, self, p));
    v = R(self + 0xC0u);
    W(self + 0xB0u, ADD(R(self + 0xB0u), v));
    v = R(self + 0xC4u);
    W(self + 0xB4u, ADD(R(self + 0xB4u), v));
    v = R(self + 0xC8u);
    W(self + 0xB8u, ADD(R(self + 0xB8u), v));
    C(00102948, V38A0, self + 0xB0u);
    W(0x700038A4u, 0);
    W(0x700038B0u, F(340.0f));
    W(0x700038B4u, 0);
    W(0x700038B8u, F(270.0f));
    W(0x700038BCu, F(1.0f));
    C(001B15D0, V38A0, V38B0, &v);
    W(0x70003A20u, v);
    if (B(self + 0xDu) & 2) {
        W(0x70003A24u, F(269.0f));
    } else {
        if ((int32_t)R(p + 0x10u) >= 0x14 && LE(range, F(15.0f))) {
            WB(self + 4u, 2);
            return 0;
        }
        W(0x70003A24u, F(190.0f));
    }
    v = R(0x70003A20u);
    limit = R(0x70003A24u);
    if (!LT(v, limit)) {
        WB(self + 4u, 2);
        return 0;
    }
    C(0019A570, self + 0xA0u, self + 0xB0u, 7, 0x20, &r);
    if (r != 0) {
        C(001031E0, self + 0xB0u, 0x700031B0u);
        WB(self + 4u, 2);
    }
    W(self + 0xA0u, R(self + 0xB0u));
    W(self + 0xA4u, R(self + 0xB4u));
    W(self + 0xA8u, R(self + 0xB8u));
    W(self + 0xACu, R(self + 0xBCu));
    W(0x700038A0u, 0);
    W(0x700038A4u, 0);
    W(0x700038A8u, 0);
    W(0x700038ACu, F(1.0f));
    C(001C94B0, self + 0xD0u, self + 0xB0u, V38A0, V38A0);
    return 0;
}
