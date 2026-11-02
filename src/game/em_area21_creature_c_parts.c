/* The AREA21 creature's motion 00152F20, its spawn 00153290 and the spawned
 * pieces' behaviour 00153540. From the decomp C, checked against the
 * original instructions by tools/test_area21_creature_c_reference.py. */
#include "em_area21_creature_c_local.h"

/* 00152F20: the motion, by the motion mode ctl+0x53. Mode 0, free flight:
 * the fall speed ctl+0x30 gains ctl+0x34, the ground speed ctl+0x38 gains
 * ctl+0x3C, the height +0xB4 takes the fall speed and x / z the ground
 * speed along the heading +0xC4 (sin, cos). Mode 1, the clip's root
 * motion: for clip 0, 2, 3 or 4 (another clip ends the mode) the
 * per-frame stride tables 0x246040 / 0x246230 / 0x246410 / 0x246630 are
 * walked back from the frame (clip length 001C6160 - frame +0x3C) over the
 * whole frames the clock step ctl+0x28 crossed (modff into the frame
 * locals at sp - 8 and sp - 4), each stride moving x / z along the
 * heading; the walk wraps to the clip's last frame below 0. */
uint32_t a21c_00152F20(A21C *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    const uint32_t whole = sp - 8u, part = sp - 4u;
    uint32_t a, b, s, table, at, f20, f21, dummy;
    int32_t n = 0, kind;
    const int32_t mode = SB(ctl + 0x53u);
    if (mode == 0) {
        a = R(ctl + 0x34u);
        b = R(ctl + 0x30u);
        W(ctl + 0x30u, ADD(b, a));
        a = R(ctl + 0x3Cu);
        b = R(ctl + 0x38u);
        W(ctl + 0x38u, ADD(b, a));
        a = R(ctl + 0x30u);
        b = R(self + 0xB4u);
        W(self + 0xB4u, ADD(b, a));
        a = R(self + 0xC4u);
        C(0011E2A8, a, &s);
        a = R(ctl + 0x38u);
        b = R(self + 0xB0u);
        W(self + 0xB0u, ADD(b, MUL(a, s)));
        a = R(self + 0xC4u);
        C(0011DE90, a, &s);
        a = R(ctl + 0x38u);
        b = R(self + 0xB8u);
        W(self + 0xB8u, ADD(b, MUL(a, s)));
        return 0;
    }
    if (mode != 1) return 0;
    kind = SH(self + 0x2Cu);
    if (kind & 0x8000) return 0;
    switch (kind) {
    case 4: table = 0x246630u; break;
    case 3: table = 0x246410u; break;
    case 2: table = 0x246230u; break;
    case 0: table = 0x246040u; break;
    default:
        WB(ctl + 0x53u, 0);
        return 0;
    }
    C(001C6160, self, &n);
    a = R(self + 0x3Cu);
    f20 = SUB(CVT(n), a);
    C(0011E0A8, whole, f20, &dummy);
    a = R(ctl + 0x28u);
    C(0011E0A8, part, ADD(F(1.0f), SUB(f20, a)), &dummy);
    a = R(whole);
    b = R(part);
    a = SUB(a, b);
    W(whole, a);
    if (LT(a, 0)) return 0;
    do {
        int32_t index = 0;
        a = R(self + 0xC4u);
        C(0011E2A8, a, &f21);
        C(001281C0, f20, &index);
        at = table + (uint32_t)index * 4u;
        a = R(self + 0xB0u);
        b = R(at);
        W(self + 0xB0u, ADD(a, MUL(b, f21)));
        a = R(self + 0xC4u);
        C(0011DE90, a, &s);
        b = R(at);
        a = R(self + 0xB8u);
        f20 = SUB(f20, F(1.0f));
        W(self + 0xB8u, ADD(a, MUL(b, s)));
        if (LT(f20, 0)) {
            C(001C6160, self, &n);
            f20 = CVT((uint32_t)n - 1u);
        }
        a = SUB(R(whole), F(1.0f));
        W(whole, a);
    } while (!LE(a, 0));
    return 0;
}

/* 00153290: the burst's spawn. Six pieces (00153540) from the rows of
 * 0x246730 (bone, x, y, z, angle): each placed at its point in its bone's
 * matrix (the table behind 0x275B40, matrix at +0x90) and given the
 * velocity (0, 0, 0.75..1.1) turned by the fixed tilt and the heading
 * +0xC4 plus the row's angle; a refused allocation ends the spawn. Then
 * four sparks from the rows of 0x2467B0: the point in the bone's matrix
 * and the angles (-1.5533, heading + the row's angle) to 0014D1E0 and
 * the effect 001EFD90(3). */
uint32_t a21c_00153290(A21C *o, uint32_t self)
{
    int32_t r = 0;
    uint32_t row = 0x246730u, obj = 0, v, table, bone, c4, angle;
    for (int i = 0; i < 6; i++) {
        C(001AFA90, 2, &obj);
        if (obj == 0) return 0;
        WB(obj + 3u, 8);
        W(obj + 0x10u, 0x153540u);
        v = R(row + 4u);
        W(obj + 0xB0u, v);
        v = R(row + 8u);
        W(obj + 0xB4u, v);
        v = R(row + 0xCu);
        W(obj + 0xB8u, v);
        W(obj + 0xBCu, F(1.0f));
        v = R(row);
        table = R(BONE_TABLE);
        bone = R(table + v * 4u);
        C(00102958, V36A0, bone + 0x90u);
        C(001026A0, obj + 0xB0u, V36A0, obj + 0xB0u);
        C(001029C0, V36A0);
        C(00102B08, V36A0, V36A0, 0xBE71463Au);
        c4 = R(self + 0xC4u);
        angle = R(row + 0x10u);
        C(001B1470, ADD(c4, angle), &v);
        C(00102BB0, V36A0, V36A0, v);
        W(0x700038A0u, 0);
        W(0x700038A4u, 0);
        C0(00122BB8, &r);
        W(0x700038A8u, ADD(F(0.75f), MUL(0x3EB33333u, a21c_unit(r, 15))));
        W(0x700038ACu, F(1.0f));
        C(001026A0, obj + 0xC0u, V36A0, V38A0);
        row += 0x14u;
    }
    row = 0x2467B0u;
    for (int i = 0; i < 4; i++) {
        v = R(row + 4u);
        table = R(BONE_TABLE);
        W(0x700038A0u, v);
        v = R(row + 8u);
        W(0x700038A4u, v);
        v = R(row + 0xCu);
        W(0x700038A8u, v);
        W(0x700038ACu, F(1.0f));
        v = R(row);
        bone = R(table + v * 4u);
        C(00102958, V36A0, bone + 0x90u);
        C(001026A0, V38A0, V36A0, V38A0);
        W(0x700038B8u, 0);
        W(0x700038B4u, 0);
        W(0x700038B0u, 0);
        W(0x700038BCu, F(1.0f));
        W(0x700038B0u, 0xBFC6D3F2u);
        c4 = R(self + 0xC4u);
        angle = R(row + 0x10u);
        C(001B1470, ADD(c4, angle), &v);
        W(0x700038B4u, v);
        C(0014D1E0, V38A0, V38B0, 1);
        C(001EFD90, 3, V38A0, V38B0);
        row += 0x14u;
    }
    return 0;
}

/* Keep the pose +0xB0..0xBC as the last pose +0xA0..0xAC. */
static void a21c_keep_pose(A21C *o, uint32_t self)
{
    for (uint32_t k = 0; k < 0x10u; k += 4u) {
        const uint32_t v = R(self + 0xB0u + k);
        W(self + 0xA0u + k, v);
    }
}

/* 00153540: a spawned piece, by its state +4. 0: the velocity +0xC0 is
 * turned by 2.5 (00103230), the effect 001EFE00(0x80000037) kept in +0x20
 * and the pose kept. 1: the pose moves by the velocity; a hit of the
 * sweep 0019A570(last, pose, 7, 0x20) snaps it to the contact 0x700031B0
 * and ends the flight (2), as does x reaching 20 (moving down) or 180
 * (moving up); the pose is kept and its matrix +0xD0 built (001C94B0 with
 * the rotation (0, 0, 0, 1)). 2: the effect's state 2, the burst
 * 001EFD20(0x8000006B) at the pose and the cry 0x88A (300), then 3. 3:
 * the piece is freed (001AFC10). */
uint32_t a21c_00153540(A21C *o, uint32_t self)
{
    int32_t r = 0;
    uint32_t a, b, e;
    const uint32_t state = B(self + 4u);
    switch (state) {
    case 3:
        C(001AFC10, self);
        break;
    case 2:
        WB(self + 4u, state + 1u);
        e = R(self + 0x20u);
        WB(e + 4u, 2);
        C(001EFD20, (int32_t)0x8000006Bu, self + 0xB0u);
        C(001FBD50, self, 0x88A, 0, F(300.0f), &r);
        break;
    case 1:
        for (uint32_t k = 0; k < 0xCu; k += 4u) {
            a = R(self + 0xC0u + k);
            b = R(self + 0xB0u + k);
            W(self + 0xB0u + k, ADD(b, a));
        }
        C(0019A570, self + 0xA0u, self + 0xB0u, 7, 0x20, &r);
        if (r != 0) {
            C(001031E0, self + 0xB0u, 0x700031B0u);
            WB(self + 4u, 2);
        }
        if (LT(R(self + 0xC0u), 0)) {
            if (LE(R(self + 0xB0u), F(20.0f))) WB(self + 4u, 2);
        } else if (!LT(R(self + 0xB0u), F(180.0f))) {
            WB(self + 4u, 2);
        }
        a21c_keep_pose(o, self);
        W(0x700038A0u, 0);
        W(0x700038A4u, 0);
        W(0x700038A8u, 0);
        W(0x700038ACu, F(1.0f));
        C(001C94B0, self + 0xD0u, self + 0xB0u, V38A0, V38A0);
        break;
    case 0:
        WB(self + 4u, state + 1u);
        C(00103230, self + 0xC0u, self + 0xC0u, F(2.5f));
        C(001EFE00, (int32_t)0x80000037u, self, &r);
        W(self + 0x20u, (uint32_t)r);
        a21c_keep_pose(o, self);
        break;
    default:
        break;
    }
    return 0;
}
