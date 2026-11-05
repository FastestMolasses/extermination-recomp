/* AREA00 sub 2's creature: the moves of state 1 (0014D5F0, 0014D7C0,
 * 0014DC30, 0014E050, 0014E2C0, picked by the move byte +5 in 0014D470)
 * and of state 2 (0014E420, 0014E4F0, picked in 0014E370). Each is stepped
 * by the byte +6. From the decomp C (src/func_*.c, byte-matched), checked
 * against the original instructions by
 * tools/test_area00s2_creature_reference.py. */
#include "em_area00s2_creature_local.h"

/* The clip number at `table` + 2 * index (a halfword). */
static int32_t clip_at(A0S2 *o, uint32_t table, uint32_t index)
{
    return SH(table + (index << 1));
}

/* The reaction 0014D7C0 and 0014DC30 share: bit 2 of the byte 0x8102B0,
 * the byte 0x8102BF = 2, the vector 0x810320 = (0x810360 - +0xB0, 0,
 * 0x810368 - +0xB8) and 00102760 over it. */
static uint32_t react(A0S2 *o, uint32_t self)
{
    uint32_t v, a;
    v = B(0x8102B0u);
    WB(0x8102B0u, v | 2u);
    WB(0x8102BFu, 2);
    v = R(PLAYER_POS);
    a = R(self + 0xB0u);
    W(AIM, SUB(v, a));
    W(AIM + 4u, 0);
    v = R(PLAYER_POS + 8u);
    a = R(self + 0xB8u);
    W(AIM + 8u, SUB(v, a));
    C(00102760, AIM, AIM);
    return 0;
}

/* 0014D5F0: move 0. Step 0: the step byte on, the wait ctl+4 cleared, the
 * heading goal ctl+0x24 from ctl+0x38, ctl+0x5D and the effect's +5
 * cleared, clip 2 (rate 5.0) unless the clip +0x2C (bit 15 aside) already
 * is 2. Step 1: the wait counts down, at 0 it is drawn again (0..127) and
 * the goal ctl+0x24 refreshed from ctl+0x38; ctl+0x28 turns toward ctl+0x24
 * (001B12B0, 2.5 degrees); when ctl+0x50 is 0: move 3 when ctl+0x34 is at
 * most 25 and the byte 0x8104E4 is not 1, else move 1 or 2 by a random bit;
 * the step byte cleared. Then, in every step: when ctl+0x5E is 0 it is
 * drawn again (60..123) and one of the four sounds at 0x245CF0 plays. */
uint32_t a0s2_0014D5F0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, a, link, id;
    const uint32_t st = B(self + 6u);
    if (st == 1) {
        v = R(ctl + 4u);
        if (v != 0) {
            W(ctl + 4u, v - 1u);
        } else {
            C(00122BB8, &r);
            W(ctl + 4u, ((uint32_t)r >> 14) & 0x7Fu);
            v = R(ctl + 0x38u);
            W(ctl + 0x24u, v);
        }
        a = R(ctl + 0x28u);
        v = R(ctl + 0x24u);
        C(001B12B0, v, a, 0x3D32B8C3u, &v);
        W(ctl + 0x28u, v);
        if (SH(ctl + 0x50u) == 0) {
            if (LE(R(ctl + 0x34u), F(25.0f)) && B(0x8104E4u) != 1) {
                WB(self + 5u, 3);
            } else {
                C(00122BB8, &r);
                WB(self + 5u, (((uint32_t)r >> 7) & 1u) ? 1u : 2u);
            }
            WB(self + 6u, 0);
        }
    } else if (st == 0) {
        WB(self + 6u, st + 1u);
        W(ctl + 4u, 0);
        v = R(ctl + 0x38u);
        W(ctl + 0x24u, v);
        WB(ctl + 0x5Du, 0);
        link = R(self + 0x20u);
        if (link != 0) WB(link + 5u, 0);
        if (((uint32_t)SH(self + 0x2Cu) & 0xFFFF7FFFu) != 2u) C(001C67E0, self, 2, F(5.0f), 0);
    }
    if (SB(ctl + 0x5Eu) == 0) {
        C(00122BB8, &r);
        WB(ctl + 0x5Eu, (((uint32_t)r >> 11) & 0x3Fu) + 0x3Cu);
        C(00122BB8, &r);
        id = R(0x245CF0u + ((((uint32_t)r >> 7) & 3u) << 2));
        C(001FBD50, self, (int32_t)id, 0, F(300.0f), &r);
    }
    return 0;
}

/* 0014D7C0: move 1. Step 0: the step byte on, the phase ctl+4 and the flag
 * ctl+0xC cleared, the timer ctl+8 = 90, ctl+0x20 = 0.525, ctl+0x5D and the
 * effect's +5 cleared, clip 3 (rate 5.0) and the sound 0x862. Step 1: at
 * the clip's end (ctl+0 bit 0x1000) the step byte on and the phase's clip
 * from 0x275418 (rate 0); a phase past 0 sets ctl+8 = 45. Step 2: the phase
 * gives the distance band of +0x3C (phase 0: 12..26, 1: 14..24, 2: 22..30);
 * inside it the effect's +5 = 3; with ctl+0xC clear, at or below the band's
 * low end the flag and the sound 0x865; inside the band, when 0021BE40
 * (0x8102B0, self) is 0 and 001A7B80(self) is not, the reaction, the word
 * 0x8104D4 = 25.0 (phase 2 or 0x81070A set) or 20.0, and 001B55E0(self, 1).
 * At the clip's end the phase steps on: at 3 the next step, ctl+0x5D = 1,
 * ctl+0x20 = 0.2, clip 7 and the effect's +5 cleared; else back to step 1
 * with the flag cleared and ctl+0x20 + 0.15. Step 3: at the clip's end
 * move 0 step 0, ctl+0x20 = 1.0, ctl+0x5D cleared and ctl+0x50 drawn
 * (0..255). Then, in every step: while ctl+8 runs it counts down and
 * ctl+0x28 turns toward ctl+0x38 (001B12B0, 1.5 degrees).
 *
 * A phase outside 0..2 in step 2 leaves the band's low end in a register
 * the function never set; the original then compares +0x3C with it when
 * ctl+0xC is clear. The native faults there (fail-stop): only this function
 * moves the phase, and step 2 is entered with it below 3. */
uint32_t a0s2_0014D7C0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, phase;
    uint32_t v, a, lo = 0, link;
    int inrange = 0, known = 1;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 3:
        if (!(R(ctl) & 0x1000u)) break;
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        W(ctl + 0x20u, F(1.0f));
        WB(ctl + 0x5Du, 0);
        C(00122BB8, &r);
        WH(ctl + 0x50u, ((uint32_t)r >> 12) & 0xFFu);
        break;
    case 2:
        phase = (int32_t)R(ctl + 4u);
        if (phase == 2 || phase == 1 || phase == 0) {
            const uint32_t hi = phase == 2 ? F(30.0f) : phase == 1 ? F(24.0f) : F(26.0f);
            lo = phase == 2 ? F(22.0f) : phase == 1 ? F(14.0f) : F(12.0f);
            v = R(self + 0x3Cu);
            if (LE(v, hi) && !LT(v, lo)) inrange = 1;
        } else {
            known = 0;
        }
        if (inrange) {
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 3);
        }
        if (R(ctl + 0xCu) == 0) {
            v = R(self + 0x3Cu);
            if (!known) return (uint32_t)a0s2_latch(o, 0x0014D7C0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
            if (LE(v, lo)) {
                W(ctl + 0xCu, 1);
                C(001FBD50, self, 0x865, 0, F(300.0f), &r);
            }
        }
        if (inrange) {
            C(0021BE40, 0x8102B0u, self, &r);
            if (r == 0) {
                C(001A7B80, self, &r);
                if (r != 0) {
                    SIB(react(o, self));
                    if (R(ctl + 4u) == 2u) W(0x8104D4u, 0x41C80000u);
                    else if (B(0x81070Au) != 0) W(0x8104D4u, 0x41C80000u);
                    else W(0x8104D4u, 0x41A00000u);
                    C(001B55E0, self, 1);
                }
            }
        }
        if (!(R(ctl) & 0x1000u)) break;
        phase = (int32_t)R(ctl + 4u) + 1;
        W(ctl + 4u, (uint32_t)phase);
        if (phase >= 3) {
            v = B(self + 6u);
            WB(self + 6u, v + 1u);
            WB(ctl + 0x5Du, 1);
            W(ctl + 0x20u, F(0.2f));
            C(001C67E0, self, 7, 0, 0);
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 0);
        } else {
            WB(self + 6u, 1);
            W(ctl + 0xCu, 0);
            v = R(ctl + 0x20u);
            W(ctl + 0x20u, ADD(v, F(0.15f)));
        }
        break;
    case 1:
        if (!(R(ctl) & 0x1000u)) break;
        WB(self + 6u, st + 1u);
        v = R(ctl + 4u);
        C(001C67E0, self, clip_at(o, 0x275418u, v), 0, 0);
        if (R(ctl + 4u) != 0) W(ctl + 8u, 0x2D);
        break;
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 4u, 0);
        W(ctl + 8u, 0x5A);
        W(ctl + 0xCu, 0);
        W(ctl + 0x20u, 0x3F066666u);
        WB(ctl + 0x5Du, 0);
        link = R(self + 0x20u);
        if (link != 0) WB(link + 5u, 0);
        C(001C67E0, self, 3, F(5.0f), 0);
        C(001FBD50, self, 0x862, 0, F(300.0f), &r);
        break;
    default:
        break;
    }
    v = R(ctl + 8u);
    if (v != 0) {
        W(ctl + 8u, v - 1u);
        a = R(ctl + 0x28u);
        v = R(ctl + 0x38u);
        C(001B12B0, v, a, 0x3CD67750u, &v);
        W(ctl + 0x28u, v);
    }
    return 0;
}

/* 0014DC30: move 2. Step 0: the step byte on, the flag ctl+8 cleared,
 * ctl+0x20 = 0.4, ctl+0x5D cleared, the side ctl+4 = 0 when ctl+0x38 is
 * below 0 else 1, the side's clip from 0x245D00 (rate 5.0) and the sound
 * 0x862. Step 1: with the flag clear, at or below 33 (+0x3C) the flag and
 * the sound 0x863; ctl+0x28 turns toward 0 (001B12B0, 2 degrees); once it
 * is 0 and the clip ended (ctl+0 bit 0x1000) the step byte on, the flag
 * cleared and the side's clip from 0x245D06 (rate 0). Step 2: on side 2,
 * or from 27 on (+0x3C) when 0021BE40(0x8102B0, self) is 0 and
 * 001A7B80(self) is not: the reaction, 0x8104D4 = 20.0 (0x81070A set) or
 * 15.0 and 001B55E0(self, 1); the effect's +5 = 2 inside 24..52, else 0;
 * with the flag clear, at or below 49 the flag and the sound 0x865; at the
 * clip's end the step byte on, ctl+0x20 = 0.2, ctl+0x5D = 1 and the side's
 * clip from 0x245D0C (rate 0). Step 3: at the clip's end move 0 step 0,
 * ctl+0x20 = 1.0, ctl+0x5D cleared and ctl+0x50 drawn (0..127 while +0x34
 * is below 241, else 0..255). */
uint32_t a0s2_0014DC30(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, a, link;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 3:
        if (!(R(ctl) & 0x1000u)) break;
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        W(ctl + 0x20u, F(1.0f));
        WB(ctl + 0x5Du, 0);
        if (SH(self + 0x34u) < 0xF1) {
            C(00122BB8, &r);
            WH(ctl + 0x50u, ((uint32_t)r >> 12) & 0x7Fu);
        } else {
            C(00122BB8, &r);
            WH(ctl + 0x50u, ((uint32_t)r >> 19) & 0xFFu);
        }
        break;
    case 2: {
        int spotted = 0;
        if (R(ctl + 4u) == 2u) {
            spotted = 1;
        } else if (!LT(R(self + 0x3Cu), F(27.0f))) {
            C(0021BE40, 0x8102B0u, self, &r);
            if (r == 0) {
                C(001A7B80, self, &r);
                spotted = r != 0;
            }
        }
        if (spotted) {
            SIB(react(o, self));
            if (B(0x81070Au) != 0) W(0x8104D4u, 0x41A00000u);
            else W(0x8104D4u, 0x41700000u);
            C(001B55E0, self, 1);
        }
        v = R(self + 0x3Cu);
        if (LE(v, F(52.0f)) && !LT(v, F(24.0f))) {
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 2);
        } else {
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 0);
        }
        if (R(ctl + 8u) == 0 && LE(R(self + 0x3Cu), F(49.0f))) {
            W(ctl + 8u, 1);
            C(001FBD50, self, 0x865, 0, F(300.0f), &r);
        }
        if (!(R(ctl) & 0x1000u)) break;
        v = B(self + 6u);
        WB(self + 6u, v + 1u);
        W(ctl + 0x20u, F(0.2f));
        WB(ctl + 0x5Du, 1);
        v = R(ctl + 4u);
        C(001C67E0, self, clip_at(o, 0x245D0Cu, v), 0, 0);
        break;
    }
    case 1:
        if (R(ctl + 8u) == 0 && LE(R(self + 0x3Cu), F(33.0f))) {
            W(ctl + 8u, 1);
            C(001FBD50, self, 0x863, 0, F(300.0f), &r);
        }
        a = R(ctl + 0x28u);
        C(001B12B0, 0, a, 0x3D0EFA35u, &v);
        W(ctl + 0x28u, v);
        if (!EQ(v, 0)) break;
        if (!(R(ctl) & 0x1000u)) break;
        v = B(self + 6u);
        WB(self + 6u, v + 1u);
        W(ctl + 8u, 0);
        v = R(ctl + 4u);
        C(001C67E0, self, clip_at(o, 0x245D06u, v), 0, 0);
        break;
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 8u, 0);
        W(ctl + 0x20u, 0x3ECCCCCDu);
        WB(ctl + 0x5Du, 0);
        if (LT(R(ctl + 0x38u), 0)) W(ctl + 4u, 0);
        else W(ctl + 4u, 1);
        v = R(ctl + 4u);
        C(001C67E0, self, clip_at(o, 0x245D00u, v), F(5.0f), 0);
        C(001FBD50, self, 0x862, 0, F(300.0f), &r);
        break;
    default:
        break;
    }
    return 0;
}

/* 0014E050: move 3. Step 0: the step byte on, ctl+0x20 = 0.5 and clip 0x14
 * (rate 5.0). Step 1: ctl+0x28 turns toward 0 (001B12B0, 2 degrees); once
 * it is 0 and the clip ended (ctl+0 bit 0x1000): the step byte on, clip
 * 0x15 (rate 0), bone record +0x140's matrix (+0x90) copied to the
 * scratchpad 0x700036A0, the point (10, 0, 0, 1) at 0x700038A0 taken
 * through it (001026A0), the vector 0x700038B0 = (0, 0, 0, 1), and for
 * t = 0, 1 .. 12 its +4 = 15 t degrees in radians and the effect
 * 001EFD90(3, 0x700038A0, 0x700038B0); then the sound 0x861. Step 2: at the
 * clip's end the step byte on and clip 0x16 (rate 0). Step 3: at the clip's
 * end move 0 step 0, ctl+0x20 = 1.0 and ctl+0x50 drawn (0..255, stored
 * twice). */
uint32_t a0s2_0014E050(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, a, t;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 3:
        if (!(R(ctl) & 0x1000u)) break;
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        W(ctl + 0x20u, F(1.0f));
        C(00122BB8, &r);
        v = ((uint32_t)r >> 19) & 0xFFu;
        WH(ctl + 0x50u, v);
        WH(ctl + 0x50u, v);
        break;
    case 2:
        if (!(R(ctl) & 0x1000u)) break;
        WB(self + 6u, st + 1u);
        C(001C67E0, self, 0x16, 0, 0);
        break;
    case 1:
        a = R(ctl + 0x28u);
        C(001B12B0, 0, a, 0x3D0EFA35u, &v);
        W(ctl + 0x28u, v);
        if (!EQ(v, 0)) break;
        if (!(R(ctl) & 0x1000u)) break;
        v = B(self + 6u);
        WB(self + 6u, v + 1u);
        C(001C67E0, self, 0x15, 0, 0);
        v = R(self + 0x140u);
        C(00102958, V36A0, v + 0x90u);
        W(V38A0, F(10.0f));
        W(V38A0 + 4u, 0);
        W(V38A0 + 8u, 0);
        W(V38A0 + 0xCu, F(1.0f));
        C(001026A0, V38A0, V36A0, V38A0);
        W(V38B0 + 8u, 0);
        W(V38B0 + 4u, 0);
        W(V38B0, 0);
        W(V38B0 + 0xCu, F(1.0f));
        t = 0;
        do {
            W(V38B0 + 4u, DIV(MUL(F(3.1415927f), MUL(F(15.0f), t)), F(180.0f)));
            C(001EFD90, 3, V38A0, V38B0);
            t = ADD(t, F(1.0f));
        } while (LT(t, F(13.0f)));
        C(001FBD50, self, 0x861, 0, F(300.0f), &r);
        break;
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 0x20u, F(0.5f));
        C(001C67E0, self, 0x14, F(5.0f), 0);
        break;
    default:
        break;
    }
    return 0;
}

/* 0014E2C0: move 4, the wait. Step 0: header 2, the step byte on, ctl+0x20
 * = 0, ctl+0x52 = 100 and clip 0 (rate 0). Step 1: ctl+0x52 held at 100;
 * when the byte 0x810803 is 2: header 1, move 0 step 0, ctl+0x20 = 1.0,
 * ctl+0x52 = 0 and ctl+0x50 = 180. */
uint32_t a0s2_0014E2C0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    uint32_t v;
    const uint32_t st = B(self + 6u);
    if (st == 1) {
        WH(ctl + 0x52u, 0x64);
        if (B(FLAG803) != 2) return 0;
        WB(self, 1);
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        W(ctl + 0x20u, F(1.0f));
        WH(ctl + 0x52u, 0);
        WH(ctl + 0x50u, 0xB4);
    } else if (st == 0) {
        WB(self, 2);
        v = B(self + 6u);
        WB(self + 6u, v + 1u);
        W(ctl + 0x20u, 0);
        WH(ctl + 0x52u, 0x64);
        C(001C67E0, self, 0, 0, 0);
    }
    return 0;
}

/* 0014E420: state 2 move 0. Step 0: the step byte on, ctl+0x28 = 0,
 * ctl+0x20 = 1.0, the tilt step +7 = 1, ctl+0x56 = 240, ctl+0x3C = 2.5,
 * ctl+0x59 / +0x58 cleared, clip 0x17 (rate 5.0) and the sound 0x867.
 * Step 1: ctl+0x56 held at 240; at the clip's end state 1, move 0, step
 * 0. */
uint32_t a0s2_0014E420(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t st = B(self + 6u);
    if (st == 1) {
        WH(ctl + 0x56u, 0xF0);
        if (!(R(ctl) & 0x1000u)) return 0;
        WB(self + 4u, 1);
        WB(self + 5u, 0);
        WB(self + 6u, 0);
    } else if (st == 0) {
        WB(self + 6u, st + 1u);
        W(ctl + 0x28u, 0);
        W(ctl + 0x20u, F(1.0f));
        WB(self + 7u, 1);
        WH(ctl + 0x56u, 0xF0);
        W(ctl + 0x3Cu, F(2.5f));
        WB(ctl + 0x59u, 0);
        WB(ctl + 0x58u, 0);
        C(001C67E0, self, 0x17, F(5.0f), 0);
        C(001FBD50, self, 0x867, 0, F(300.0f), &r);
    }
    return 0;
}

/* 0014E4F0: state 2 move 1. Step 0: the step byte on, ctl+0x28 = 0,
 * ctl+0x20 = 1.0, ctl+0x56 = 10000, the byte 0x810803 = 0x80 and clip 2
 * (rate 5.0). Step 1: ctl+0x56 held at 10000; when 0x810803 is 0x81: the
 * step byte on, clip 0x18 (rate 5.0), the effect's +4 = 3, the effect
 * 001EFE00(0x80000046, self) into +0x24 and the sound 0x868. Step 2: when
 * the byte 0x28A9A2 is set and the halfword 0x28A9A0 is 2: state 3, and the
 * record at +0x24 (when set) gets +4 = 3. */
uint32_t a0s2_0014E4F0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, p;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 2:
        if (SB(0x28A9A2u) == 0) break;
        if (SH(0x28A9A0u) != 2) break;
        WB(self + 4u, 3);
        p = R(self + 0x24u);
        if (p != 0) WB(p + 4u, 3);
        break;
    case 1:
        WH(ctl + 0x56u, 0x2710);
        if (B(FLAG803) != 0x81) break;
        v = B(self + 6u);
        WB(self + 6u, v + 1u);
        C(001C67E0, self, 0x18, F(5.0f), 0);
        p = R(self + 0x20u);
        if (p != 0) WB(p + 4u, 3);
        C(001EFE00, (int32_t)0x80000046u, self, &r);
        W(self + 0x24u, (uint32_t)r);
        C(001FBD50, self, 0x868, 0, F(300.0f), &r);
        break;
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 0x28u, 0);
        W(ctl + 0x20u, F(1.0f));
        WH(ctl + 0x56u, 0x2710);
        WB(FLAG803, 0x80);
        C(001C67E0, self, 2, F(5.0f), 0);
        break;
    default:
        break;
    }
    return 0;
}
