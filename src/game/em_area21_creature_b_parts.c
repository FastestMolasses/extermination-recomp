/* AREA21's creature: the per-frame pieces. The motion 00150900, the arena
 * clamps 001509A0, the spawn point 00150DA0, the spray 00150F40 and the
 * part bursts 00151200. From the decomp C (src/func_*.c; 00150F40's is a
 * body-correct NEARMISS), checked against the original instructions by
 * tools/test_area21_creature_b_reference.py. */
#include "em_area21_creature_b_local.h"

/* A random byte of 00122BB8's result (bits n..n+7) over 255.0. */
static uint32_t rand_unit(int32_t r, unsigned n)
{
    return DIV(CVT(((uint32_t)r >> n) & 0xFFu), F(255.0f));
}

/* 00150900: unless ctl+0x63 is set, the climb ctl+0x38 gains its fall
 * ctl+0x3C and the speed ctl+0x40 gains ctl+0x48; the height +0xB4 gains
 * the climb, and the position +0xB0 / +0xB8 moves by the speed along the
 * heading ctl+0x28 (its sine 0011E2A8 and cosine 0011DE90). */
uint32_t a21b_00150900(A21B *o, uint32_t self, uint32_t ctl)
{
    uint32_t a, b, s;
    if (SB(ctl + 0x63u) != 0) return 0;
    b = R(ctl + 0x3Cu);
    a = R(ctl + 0x38u);
    W(ctl + 0x38u, ADD(a, b));
    b = R(ctl + 0x48u);
    a = R(ctl + 0x40u);
    W(ctl + 0x40u, ADD(a, b));
    b = R(ctl + 0x38u);
    a = R(self + 0xB4u);
    W(self + 0xB4u, ADD(a, b));
    a = R(ctl + 0x28u);
    C(0011E2A8, a, &s);
    a = R(ctl + 0x40u);
    b = R(self + 0xB0u);
    W(self + 0xB0u, ADD(b, MUL(a, s)));
    a = R(ctl + 0x28u);
    C(0011DE90, a, &s);
    a = R(ctl + 0x40u);
    b = R(self + 0xB8u);
    W(self + 0xB8u, ADD(b, MUL(a, s)));
    return 0;
}

/* 001509A0: the arena. Unless ctl+0x64 bit 2: the flat distance from
 * (100, 150) to the position, beyond the radius (420 for the pattern
 * ctl+0x65 = 2, else 345), puts the position back on the circle (the
 * bearing 001B1240, a rotation, 001026A0 / 001028B8). Unless bit 4: for
 * patterns other than 2, z at most 350 (a clamp there ends it), then
 * x at least 250 (pattern != 0) or at most -50 (pattern 0), and unless
 * bit 1 a z below 10 is raised to the slope 10 - (x - 195) or
 * 10 - |x - 5| (0011DF78) staged at 0x70003A20; for pattern 2, z at most
 * -70 and x raised to 5 + (z - 10) below 5, or lowered to 195 + |z - 10|
 * above 195. */
uint32_t a21b_001509A0(A21B *o, uint32_t self, uint32_t ctl)
{
    uint32_t lim, d, v, x, z, a, t;
    int32_t sel;
    if (!(SB(ctl + 0x64u) & 2)) {
        lim = SB(ctl + 0x65u) == 2 ? F(420.0f) : F(345.0f);
        C(00102948, V38A0, self + 0xB0u);
        W(0x700038A4u, 0);
        W(0x700038B0u, F(100.0f));
        W(0x700038B4u, 0);
        W(0x700038B8u, F(150.0f));
        W(0x700038BCu, F(1.0f));
        C(001B15D0, V38A0, V38B0, &d);
        if (!LE(d, lim)) {
            x = R(0x700038A0u);
            z = R(0x700038A8u);
            C(001B1240, V38B0, x, z, &v);
            W(S3A20, v);
            C(001029C0, V36A0);
            v = R(S3A20);
            C(00102BB0, V36A0, V36A0, v);
            W(0x700038A0u, 0);
            W(0x700038A4u, 0);
            W(0x700038A8u, lim);
            W(0x700038ACu, F(1.0f));
            C(001026A0, V38A0, V36A0, V38A0);
            C(001028B8, V38A0, V38B0, V38A0);
            W(self + 0xB0u, R(0x700038A0u));
            W(self + 0xB8u, R(0x700038A8u));
        }
    }
    if (SB(ctl + 0x64u) & 4) return 0;
    sel = SB(ctl + 0x65u);
    if (sel != 2) {
        if (!LE(R(self + 0xB8u), F(350.0f))) {
            W(self + 0xB8u, F(350.0f));
            return 0;
        }
        if (sel != 0) {
            if (LT(R(self + 0xB0u), F(250.0f))) W(self + 0xB0u, F(250.0f));
            if (SB(ctl + 0x64u) & 1) return 0;
            if (!LT(R(self + 0xB8u), F(10.0f))) return 0;
            v = R(self + 0xB0u);
            W(S3A20, SUB(F(10.0f), SUB(v, F(195.0f))));
        } else {
            if (!LE(R(self + 0xB0u), F(-50.0f))) W(self + 0xB0u, F(-50.0f));
            if (SB(ctl + 0x64u) & 1) return 0;
            if (!LT(R(self + 0xB8u), F(10.0f))) return 0;
            v = R(self + 0xB0u);
            C(0011DF78, SUB(v, F(5.0f)), &t);
            W(S3A20, SUB(F(10.0f), t));
        }
        a = R(self + 0xB8u);
        t = R(S3A20);
        if (LT(a, t)) W(self + 0xB8u, t);
        return 0;
    }
    if (!LE(R(self + 0xB8u), F(-70.0f))) W(self + 0xB8u, F(-70.0f));
    v = R(self + 0xB0u);
    if (LT(v, F(5.0f))) {
        z = R(self + 0xB8u);
        W(S3A20, ADD(F(5.0f), SUB(z, F(10.0f))));
        a = R(self + 0xB0u);
        t = R(S3A20);
        if (LT(a, t)) W(self + 0xB0u, t);
    } else if (!LE(v, F(195.0f))) {
        z = R(self + 0xB8u);
        C(0011DF78, SUB(z, F(10.0f)), &t);
        W(S3A20, ADD(F(195.0f), t));
        a = R(self + 0xB0u);
        t = R(S3A20);
        if (!LE(a, t)) W(self + 0xB0u, t);
    }
    return 0;
}

/* 00150DA0: a random spawn point. Patterns other than 2 (ctl+0x65): z =
 * 423 * u - 270, an offset 150 * u at 0x70003A20, x = 250 + offset
 * (pattern != 0) or -200 + offset; pattern 2: x = -245 + 690 * u and
 * z = -270 + 200 * u (each u a fresh random byte / 255). */
uint32_t a21b_00150DA0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    if (SB(ctl + 0x65u) != 2) {
        C(00122BB8, &r);
        W(self + 0xB8u, SUB(MUL(F(423.0f), rand_unit(r, 13)), F(270.0f)));
        C(00122BB8, &r);
        W(S3A20, MUL(F(150.0f), rand_unit(r, 19)));
        if (SB(ctl + 0x65u) != 0) {
            v = R(S3A20);
            W(self + 0xB0u, ADD(F(250.0f), v));
        } else {
            v = R(S3A20);
            W(self + 0xB0u, ADD(F(-200.0f), v));
        }
    } else {
        C(00122BB8, &r);
        W(self + 0xB0u, ADD(F(-245.0f), MUL(F(690.0f), rand_unit(r, 14))));
        C(00122BB8, &r);
        W(self + 0xB8u, ADD(F(-270.0f), MUL(F(200.0f), rand_unit(r, 17))));
    }
    return 0;
}

/* 00150F40: the spray, four jets. Each jet's row of the table 0x245F00
 * (64 bytes per pattern ctl+0x65, 16 per jet: bone, x, y, z) places a point
 * on its bone (001026A0); the jet's yaw 0x700038B4 starts at 0 (pattern 2),
 * +pi/2 (pattern 0) or -pi/2, and is pulled toward the bearing to the
 * player (001B1240) within 30 degrees (001B1470 wraps); then the jet
 * 0014D1E0(point, angles, 0) and the drops 001EFD90(3). Last the cry
 * 0x874. */
uint32_t a21b_00150F40(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, k;
    uint32_t i, row, t, bone, rec, x, z, v, a, w;
    for (i = 0; i < 4; i++) {
        row = 0x245F00u + 16u * i;
        k = SB(ctl + 0x65u);
        t = R(BONE_TABLE);
        W(0x700038A0u, R(row + (uint32_t)k * 64u + 4u));
        k = SB(ctl + 0x65u);
        W(0x700038A4u, R(row + (uint32_t)k * 64u + 8u));
        k = SB(ctl + 0x65u);
        W(0x700038A8u, R(row + (uint32_t)k * 64u + 0xCu));
        W(0x700038ACu, F(1.0f));
        k = SB(ctl + 0x65u);
        bone = R(row + (uint32_t)k * 64u);
        rec = R(t + bone * 4u);
        C(00102958, V36A0, rec + 0x90u);
        C(001026A0, V38A0, V36A0, V38A0);
        W(0x700038B8u, 0);
        W(0x700038B4u, 0);
        W(0x700038B0u, 0);
        W(0x700038BCu, F(1.0f));
        k = SB(ctl + 0x65u);
        if (k != 2) W(0x700038B4u, k != 0 ? F(-1.5707964f) : F(1.5707964f));
        x = R(PLAYER_POS);
        z = R(PLAYER_POS + 8u);
        C(001B1240, V38A0, x, z, &v);
        W(S3A20, v);
        a = R(0x700038B4u);
        C(001B1470, ADD(F(0.5235988f), a), &w);
        t = R(S3A20);
        if (LT(w, t)) {
            a = R(0x700038B4u);
            C(001B1470, ADD(F(0.5235988f), a), &v);
            W(0x700038B4u, v);
        } else {
            a = R(0x700038B4u);
            C(001B1470, SUB(a, F(0.5235988f)), &w);
            t = R(S3A20);
            if (LE(w, t)) {
                W(0x700038B4u, t);
            } else {
                a = R(0x700038B4u);
                C(001B1470, SUB(a, F(0.5235988f)), &v);
                W(0x700038B4u, v);
            }
        }
        C(0014D1E0, V38A0, V38B0, 0);
        C(001EFD90, 3, V38A0, V38B0);
    }
    C(001FBD50, self, 0x874, 0, F(500.0f), &r);
    return 0;
}

/* 00151200: the part bursts. When the position is inside the area
 * 001B1EA0(0, +0xB0, 0x246000, 4), each of the four parts with fewer than
 * two bursts (ctl+0x6C + i) counts its delay ctl+0x68 + i down, or at 0
 * bursts: the count steps on, a random delay 0..31, the point of the row
 * 0x245FC0 + 16 * i (bone, x, y, z) placed on its bone, the drops
 * 001EFD90(5), the burst 001283D0(point, 0, +0xC0) and the cry 0x876. */
uint32_t a21b_00151200(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, n;
    uint32_t i, p, x, t, bone, rec;
    C(001B1EA0, 0, self + 0xB0u, 0x246000u, 4, &r);
    if (r == 0) return 0;
    for (i = 0; i < 4; i++) {
        p = 0x245FC0u + 16u * i;
        if (SB(ctl + 0x6Cu + i) >= 2) continue;
        if (SB(ctl + 0x68u + i) != 0) {
            n = SB(ctl + 0x68u + i);
            WB(ctl + 0x68u + i, (uint32_t)(n - 1));
            continue;
        }
        n = SB(ctl + 0x6Cu + i);
        WB(ctl + 0x6Cu + i, (uint32_t)(n + 1));
        C(00122BB8, &r);
        WB(ctl + 0x68u + i, ((uint32_t)r >> 14) & 0x1Fu);
        x = R(p + 4u);
        t = R(BONE_TABLE);
        W(0x700038A0u, x);
        W(0x700038A4u, R(p + 8u));
        W(0x700038A8u, R(p + 0xCu));
        W(0x700038ACu, F(1.0f));
        bone = R(p);
        rec = R(t + bone * 4u);
        C(00102958, V36A0, rec + 0x90u);
        C(001026A0, V38A0, V36A0, V38A0);
        W(0x700038B8u, 0);
        W(0x700038B4u, 0);
        W(0x700038B0u, 0);
        W(0x700038BCu, F(1.0f));
        C(001EFD90, 5, V38A0, V38B0);
        C(001283D0, V38A0, 0, self + 0xC0u, &r);
        C(001FBD50, self, 0x876, 0, F(500.0f), &r);
    }
    return 0;
}
