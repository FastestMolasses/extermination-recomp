/* The AREA21 creature's moves (state 1, the move byte +5): 00151940 (0, the
 * watch), 00151E50 (1, the turn), 00152040 (2, the swipe), 00152330 (3,
 * the lunge), 00152660 (4, the volley), 00152930 (5, the burst) and
 * 00152B60 (9, the entrance), plus the alert all three attacks share. From
 * the decomp C, checked against the original instructions by
 * tools/test_area21_creature_c_reference.py. */
#include "em_area21_creature_c_local.h"

#define PI 0x40490FDBu

/* The alert (00151940 / 00152040 / 00152330, one shape): the player's
 * flags 0x8102B0 |= 2 and 0x8102BF = 2, the shake 0x8104D4 = `hard` when
 * 0x81070A is set, else `easy`; the push: the bearing from the node to the
 * player 001B1240 into 0x70003A20, the yaw matrix 0x700036A0 of it, the
 * vector 0x810320 = (0, 0, 1, 1) turned by it, and 001B55E0(self, 1). */
uint32_t a21c_alert(A21C *o, uint32_t self, uint32_t hard, uint32_t easy)
{
    uint32_t flags, mode, x, z, v;
    flags = B(0x8102B0u);
    mode = B(0x81070Au);
    WB(0x8102B0u, flags | 2u);
    WB(0x8102BFu, 2);
    W(0x8104D4u, mode != 0 ? hard : easy);
    x = R(PLAYER_POS);
    z = R(PLAYER_POS + 8u);
    C(001B1240, self + 0xB0u, x, z, &v);
    W(SPAD_ANGLE, v);
    C(001029C0, V36A0);
    v = R(SPAD_ANGLE);
    C(00102BB0, V36A0, V36A0, v);
    W(0x810320u, 0);
    W(0x810324u, 0);
    W(0x810328u, F(1.0f));
    W(0x81032Cu, F(1.0f));
    C(001026A0, 0x810320u, V36A0, 0x810320u);
    C(001B55E0, self, 1);
    return 0;
}

/* The alert's gate: 0021BE40(0x8102B0, self) clear and 001A7B80(self)
 * set. */
static uint32_t a21c_maybe_alert(A21C *o, uint32_t self, uint32_t hard, uint32_t easy)
{
    int32_t r = 0;
    C(0021BE40, 0x8102B0u, self, &r);
    if (r != 0) return 0;
    C(001A7B80, self, &r);
    if (r == 0) return 0;
    SIB(a21c_alert(o, self, hard, easy));
    return 0;
}

/* 00151940: move 0, the watch. Step 0 clears the motion (ctl+0x53 = 1),
 * clip 0 at speed 10 and the effect's byte +5 = 10. Step 1: a random turn
 * rate into ctl+0x20; with the heading +0xC4 within about +-89 degrees
 * the bounds bit 4 (ctl+0x55) asks for the turn, and with z (+0xB8) below
 * 120 a new random goal heading is drawn when the old one is reached
 * (else the goal is 0); outside that arc bounds bit 8 asks for the turn and
 * the goal is drawn around pi. The alert; the heading turns toward the
 * goal; then the move:
 * the turn (1), the burst (5: no burst pause and the far flag), the volley
 * or the burst (pause ctl+0x42 over and the count 0x700031F4 below 20), or
 * the lunge (3: ctl+0x57 and ctl+0x58 clear, 001B1560 within 85 degrees
 * and the player within 90). */
uint32_t a21c_00151940(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, burst, pause;
    uint32_t a, v, flag = 0, move;
    const uint32_t step = B(self + 6u);
    if (step == 0) {
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        WB(ctl + 0x53u, 1);
        W(ctl + 0x20u, 0);
        W(ctl + 0x34u, 0);
        W(ctl + 0x30u, 0);
        W(ctl + 0x3Cu, 0);
        W(ctl + 0x38u, 0);
        C(001C67E0, self, 0, F(10.0f), 0);
        v = R(self + 0x20u);
        if (v) WB(v + 5u, 0xA);
        return 0;
    }
    if (step != 1) return 0;
    C0(00122BB8, &r);
    W(ctl + 0x20u, ADD(0x3C0EFA35u, MUL(0x3C8EFA35u, a21c_unit(r, 4))));
    a = R(self + 0xC4u);
    if (!LE(a, 0xBFC6D3F2u) && LT(a, 0x3FC6D3F2u)) {
        if (SB(ctl + 0x55u) & 4) flag = 1;
        if (!LT(R(self + 0xB8u), F(120.0f))) {
            W(ctl + 0x2Cu, 0);
        } else if (EQ(a, R(ctl + 0x2Cu))) {
            C0(00122BB8, &r);
            W(SPAD_ANGLE, ADD(0x3EC90FDBu, MUL(0x3EC90FDBu, a21c_unit(r, 19))));
            if (!LT(R(self + 0xC4u), 0)) {
                v = R(SPAD_ANGLE);
                W(SPAD_ANGLE, MUL(v, F(-1.0f)));
            }
            v = R(SPAD_ANGLE);
            W(ctl + 0x2Cu, v);
        }
    } else {
        if (SB(ctl + 0x55u) & 8) flag = 1;
        if (EQ(a, R(ctl + 0x2Cu))) {
            C0(00122BB8, &r);
            W(SPAD_ANGLE, ADD(0x3EC90FDBu, MUL(0x3EC90FDBu, a21c_unit(r, 15))));
            if (LT(R(self + 0xC4u), 0)) {
                v = R(SPAD_ANGLE);
                W(SPAD_ANGLE, MUL(v, F(-1.0f)));
            }
            v = R(SPAD_ANGLE);
            C(001B1470, ADD(PI, v), &v);
            W(ctl + 0x2Cu, v);
        }
    }
    SIB(a21c_maybe_alert(o, self, 0x41C80000u, 0x41A00000u));
    {
        const uint32_t heading = R(self + 0xC4u);
        const uint32_t rate = R(ctl + 0x20u);
        const uint32_t goal = R(ctl + 0x2Cu);
        C(001B12B0, goal, heading, rate, &v);
        W(self + 0xC4u, v);
    }
    if (flag) {
        move = 1;
    } else {
        burst = SH(ctl + 0x44u);
        if (burst == 0 && SB(ctl + 0x58u) != 0) {
            move = 5;
        } else {
            pause = SH(ctl + 0x42u);
            if (pause == 0 && a21c_s32(o, 0x700031F4u) < 0x14) {
                if (burst != 0) {
                    move = 4;
                } else {
                    C0(00122BB8, &r);
                    move = (a21c_sra((uint32_t)r, 18) & 1) ? 4u : 5u;
                }
            } else {
                if (SB(ctl + 0x57u) != 0) return 0;
                if (SB(ctl + 0x58u) != 0) return 0;
                C(001B1560, self, PLAYER_POS, 0x3FBDE44Eu, &r);
                if (r == 0) return 0;
                C(001B15D0, self + 0xB0u, PLAYER_POS, &v);
                if (!LE(v, F(90.0f))) return 0;
                move = 3;
            }
        }
    }
    WB(self + 5u, move);
    WB(self + 6u, 0);
    return 0;
}

/* 00151E50: move 1, the turn. Step 0: ctl+0x53 = 2, the goal heading 0
 * (near side, +0xB8 at most 150) or pi, the rate = the wrapped difference
 * / 50, clip 1 at speed 5, the effect's byte +5 = 10 and the cry 0x88D.
 * Step 1, while the clip runs to frame 80: the heading turns; at the
 * clip's end facing the goal, the turn ends in the swipe (2: not near and
 * the player at least 100 away, flattened) or back to the watch. */
uint32_t a21c_00151E50(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, a, b;
    const uint32_t step = B(self + 6u);
    if (step == 1) {
        if (SH(self + 0x2Cu) & 0x8000) return 0;
        if (!LE(R(self + 0x3Cu), F(80.0f))) return 0;
        {
            const uint32_t heading = R(self + 0xC4u);
            const uint32_t rate = R(ctl + 0x20u);
            const uint32_t goal = R(ctl + 0x2Cu);
            C(001B12B0, goal, heading, rate, &v);
            W(self + 0xC4u, v);
        }
        if ((R(ctl) & 0x1000u) == 0) return 0;
        a = R(self + 0xC4u);
        b = R(ctl + 0x2Cu);
        if (!EQ(a, b)) return 0;
        C(00102948, V38A0, PLAYER_POS);
        C(00102948, V38B0, self + 0xB0u);
        W(0x700038B4u, 0);
        W(0x700038A4u, 0);
        if (SB(ctl + 0x58u) == 0) {
            C(001B15D0, V38A0, V38B0, &v);
            if (!LT(v, F(100.0f))) {
                WB(self + 5u, 2);
                WB(self + 6u, 0);
                return 0;
            }
        }
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        WB(ctl + 0x57u, 0);
        return 0;
    }
    if (step != 0) return 0;
    WB(self + 6u, step + 1u);
    WB(ctl + 0x53u, 2);
    if (LE(R(self + 0xB8u), F(150.0f))) W(ctl + 0x2Cu, 0);
    else W(ctl + 0x2Cu, PI);
    a = R(self + 0xC4u);
    b = R(ctl + 0x2Cu);
    C(0011DF78, SUB(a, b), &v);
    W(ctl + 0x20u, DIV(v, F(50.0f)));
    C(001C67E0, self, 1, F(5.0f), 0);
    v = R(self + 0x20u);
    if (v) WB(v + 5u, 0xA);
    C(001FBD50, self, 0x88D, 0, F(500.0f), &r);
    return 0;
}

/* 00152040: move 2, the swipe. Step 0: the counter ctl+4 cleared,
 * ctl+0x53 = 1, the goal heading 0 (+0xB8 below 150) or pi, clip 2 at
 * speed 5, the effect's byte +5 = 10. Step 1: the cry 0x88E once by frame
 * 52; the heading turns at 0.0698; at the clip's end facing the goal, clip
 * 3. Step 2: the alert (25 / 20); at the clip's end clip 4. Step 3: at the
 * clip's end back to the watch. */
uint32_t a21c_00152040(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, a, b;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 3:
        if (R(ctl) & 0x1000u) {
            WB(self + 5u, 0);
            WB(self + 6u, 0);
        }
        break;
    case 2:
        SIB(a21c_maybe_alert(o, self, 0x41C80000u, 0x41A00000u));
        if (R(ctl) & 0x1000u) {
            WB(self + 6u, B(self + 6u) + 1u);
            C(001C67E0, self, 4, 0, 0);
        }
        break;
    case 1:
        if ((SH(self + 0x2Cu) & 0x8000) == 0 && R(ctl + 4u) == 0 && LE(R(self + 0x3Cu), F(52.0f))) {
            W(ctl + 4u, 1);
            C(001FBD50, self, 0x88E, 0, F(500.0f), &r);
        }
        a = R(self + 0xC4u);
        b = R(ctl + 0x2Cu);
        C(001B12B0, b, a, 0x3D8EFA35u, &v);
        W(self + 0xC4u, v);
        if ((R(ctl) & 0x1000u) == 0) break;
        b = R(ctl + 0x2Cu);
        a = R(self + 0xC4u);
        if (!EQ(a, b)) break;
        WB(self + 6u, B(self + 6u) + 1u);
        C(001C67E0, self, 3, 0, 0);
        break;
    case 0:
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        WB(ctl + 0x53u, 1);
        if (LT(R(self + 0xB8u), F(150.0f))) W(ctl + 0x2Cu, 0);
        else W(ctl + 0x2Cu, PI);
        C(001C67E0, self, 2, F(5.0f), 0);
        v = R(self + 0x20u);
        if (v) WB(v + 5u, 0xA);
        break;
    default:
        break;
    }
    return 0;
}

/* 00152330: move 3, the lunge. Step 0: ctl+0x53 = 2, the clock rate
 * ctl+0x28 = 2.0, the side ctl+0x56 = 001B1380(player, position, heading)
 * picks clip 0xF or 0xC (speed 5), the effect's byte +5 cleared. Step 1,
 * at the clip's end: rate 1.0 and clip 0x10 or 0xD. Step 2: by frame 9 the
 * alert (30 / 25); at the clip's end clip 0x11 (effect byte 11) or 0xE
 * (effect byte 12) and the cry 0x88B. Step 3, at the clip's end: back to
 * the watch, one more lunge counted in ctl+0x57. */
uint32_t a21c_00152330(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 3:
        if (R(ctl) & 0x1000u) {
            WB(self + 5u, 0);
            WB(self + 6u, 0);
            WB(ctl + 0x57u, (uint32_t)(SB(ctl + 0x57u) + 1));
        }
        break;
    case 2:
        if (LE(R(self + 0x3Cu), F(9.0f))) SIB(a21c_maybe_alert(o, self, 0x41F00000u, 0x41C80000u));
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, B(self + 6u) + 1u);
        if (SB(ctl + 0x56u) != 0) {
            C(001C67E0, self, 0x11, 0, 0);
            v = R(self + 0x20u);
            if (v) WB(v + 5u, 0xB);
        } else {
            C(001C67E0, self, 0xE, 0, 0);
            v = R(self + 0x20u);
            if (v) WB(v + 5u, 0xC);
        }
        C(001FBD50, self, 0x88B, 0, F(500.0f), &r);
        break;
    case 1:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, step + 1u);
        W(ctl + 0x28u, F(1.0f));
        if (SB(ctl + 0x56u) != 0) C(001C67E0, self, 0x10, 0, 0);
        else C(001C67E0, self, 0xD, 0, 0);
        break;
    case 0:
        WB(self + 6u, step + 1u);
        WB(ctl + 0x53u, 2);
        W(ctl + 0x28u, F(2.0f));
        v = R(self + 0xC4u);
        C(001B1380, PLAYER_POS, self + 0xB0u, v, &r);
        WB(ctl + 0x56u, (uint32_t)r);
        if (SB(ctl + 0x56u) != 0) C(001C67E0, self, 0xF, F(5.0f), 0);
        else C(001C67E0, self, 0xC, F(5.0f), 0);
        v = R(self + 0x20u);
        if (v) WB(v + 5u, 0);
        break;
    default:
        break;
    }
    return 0;
}

/* 00152660: move 4, the volley. Step 0: the counters ctl+4 / +8 / +0xC
 * cleared, rate 2.0, ctl+0x53 = 2, clip 0x12 at speed 5, the effect's byte
 * +5 cleared. Step 1, at the clip's end: clip 0x13. Step 2, at the clip's
 * end each frame: the cry 0x88C once; when the random wait ctl+4 runs out
 * a random point (x within +-2.5, y 17, z 57..62) is moved by 001B2B10
 * and offset by the position, handed to 001283D0 with the node's +0xC0
 * vector, and the cry 0x888 (300); after five points clip 0x14. Step 3,
 * at the clip's end: back to the watch, rate 1.0 and a random volley
 * pause ctl+0x42 of 600..1111. */
uint32_t a21c_00152660(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, n;
    uint32_t v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 3:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        W(ctl + 0x28u, F(1.0f));
        C0(00122BB8, &r);
        WH(ctl + 0x42u, (uint32_t)((a21c_sra((uint32_t)r, 7) & 0x1FF) + 0x258));
        break;
    case 2:
        if ((R(ctl) & 0x1000u) == 0) break;
        if (R(ctl + 0xCu) == 0) {
            W(ctl + 0xCu, 1);
            C(001FBD50, self, 0x88C, 0, F(500.0f), &r);
        }
        n = a21c_s32(o, ctl + 4u);
        if (n != 0) {
            W(ctl + 4u, (uint32_t)(n - 1));
            break;
        }
        C0(00122BB8, &r);
        W(ctl + 4u, (uint32_t)(a21c_sra((uint32_t)r, 3) & 0xF));
        W(0x700038A8u, 0);
        W(0x700038A4u, 0);
        W(0x700038A0u, 0);
        W(0x700038ACu, F(1.0f));
        W(0x700038A4u, F(17.0f));
        C0(00122BB8, &r);
        W(0x700038A0u, SUB(MUL(F(5.0f), a21c_unit(r, 18)), F(2.5f)));
        C0(00122BB8, &r);
        W(0x700038A8u, ADD(F(57.0f), MUL(F(5.0f), a21c_unit(r, 13))));
        C(001B2B10, self, V38A0, V38A0);
        C(001028B8, V38A0, self + 0xB0u, V38A0);
        C(001283D0, V38A0, 1, self + 0xC0u, &r);
        C(001FBD50, self, 0x888, 0, F(300.0f), &r);
        n = a21c_s32(o, ctl + 8u) + 1;
        W(ctl + 8u, (uint32_t)n);
        if (n >= 5) {
            WB(self + 6u, B(self + 6u) + 1u);
            C(001C67E0, self, 0x14, 0, 0);
        }
        break;
    case 1:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, step + 1u);
        C(001C67E0, self, 0x13, 0, 0);
        break;
    case 0:
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        W(ctl + 8u, 0);
        W(ctl + 0xCu, 0);
        W(ctl + 0x28u, F(2.0f));
        WB(ctl + 0x53u, 2);
        C(001C67E0, self, 0x12, F(5.0f), 0);
        v = R(self + 0x20u);
        if (v) WB(v + 5u, 0);
        break;
    default:
        break;
    }
    return 0;
}

/* 00152930: move 5, the burst. Step 0: the counter ctl+4 cleared,
 * ctl+0x53 = 2, rate 1.0, clip 5 at speed 5, the effect's byte +5
 * cleared, the cry 0x88F. Steps 1 / 2 each wait for the clip's end: clip 6 (cry
 * 0x890) and clip 7. Step 3: by frame 50, once, the spawn 00153290 with
 * the cries 0x886, 0x889; at the clip's end clip 8 and the cry 0x890. Step 4,
 * at the clip's end: back to the watch with a random burst pause
 * ctl+0x44 of 100..611. */
uint32_t a21c_00152930(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 4:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        C0(00122BB8, &r);
        WH(ctl + 0x44u, (uint32_t)((a21c_sra((uint32_t)r, 16) & 0x1FF) + 0x64));
        break;
    case 3:
        if (R(ctl + 4u) == 0 && LE(R(self + 0x3Cu), F(50.0f))) {
            W(ctl + 4u, 1);
            SIB(a21c_00153290(o, self));
            C(001FBD50, self, 0x886, 0, F(500.0f), &r);
            C(001FBD50, self, 0x889, 0, F(500.0f), &r);
        }
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, B(self + 6u) + 1u);
        C(001C67E0, self, 8, 0, 0);
        C(001FBD50, self, 0x890, 0, F(500.0f), &r);
        break;
    case 2:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, step + 1u);
        C(001C67E0, self, 7, 0, 0);
        break;
    case 1:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, step + 1u);
        C(001C67E0, self, 6, 0, 0);
        C(001FBD50, self, 0x890, 0, F(500.0f), &r);
        break;
    case 0:
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        WB(ctl + 0x53u, 2);
        W(ctl + 0x28u, F(1.0f));
        C(001C67E0, self, 5, F(5.0f), 0);
        v = R(self + 0x20u);
        if (v) WB(v + 5u, 0);
        C(001FBD50, self, 0x88F, 0, F(500.0f), &r);
        break;
    default:
        break;
    }
    return 0;
}

/* 00152B60: move 9, the entrance. Step 0: header 2, the node placed at
 * (100, 35, 95, 1) with +0xC0 = (0, 0, 0, 1), clip 0x18. Step 1, at the clip's
 * end: the counter ctl+4 cleared and clip 0x19. Step 2: the cry 0x88D once
 * by frame 150; at the clip's end header 1, back to the watch with the
 * volley pause ctl+0x42 = 600 and the effect's byte +5 cleared. */
uint32_t a21c_00152B60(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 2:
        if (R(ctl + 4u) == 0 && LE(R(self + 0x3Cu), F(150.0f))) {
            W(ctl + 4u, 1);
            C(001FBD50, self, 0x88D, 0, F(500.0f), &r);
        }
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self, 1);
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        WH(ctl + 0x42u, 0x258);
        v = R(self + 0x20u);
        if (v) WB(v + 5u, 0);
        break;
    case 1:
        if ((R(ctl) & 0x1000u) == 0) break;
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        C(001C67E0, self, 0x19, 0, 0);
        break;
    case 0:
        WB(self + 6u, step + 1u);
        WB(self, 2);
        W(self + 0xC8u, 0);
        W(self + 0xC4u, 0);
        W(self + 0xC0u, 0);
        W(self + 0xCCu, F(1.0f));
        W(self + 0xB0u, F(100.0f));
        W(self + 0xB4u, F(35.0f));
        W(self + 0xB8u, F(95.0f));
        W(self + 0xBCu, F(1.0f));
        C(001C67E0, self, 0x18, 0, 0);
        break;
    default:
        break;
    }
    return 0;
}
