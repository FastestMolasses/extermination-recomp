/* The AREA17 creature's moves (state 1, the move byte +5; the step byte
 * +6 runs each move): 0 the head sway 00134300, 1 the stalk 00134500,
 * 2 the beams 001348E0, 3 and 4 the spits 00134DC0 / 00134FF0, 5 the
 * lunge 001352E0, 6 the charge 00135870, 8 the entrance 00135D00. From the
 * decomp C, checked against the original instructions by
 * tools/test_area17_creature_reference.py. ctl = self + 0x1F0. */
#include "em_area17_creature_local.h"

/* Turn the heading +0xC4 toward the player (into ctl+0x24) by at most
 * `rate` (001B1240, 001B12B0): the shared tail of several moves. With
 * `reload` the heading's target is read back from ctl+0x24. */
static uint32_t a17c_turn(A17C *o, uint32_t self, uint32_t ctl, uint32_t rate, int reload)
{
    uint32_t x, z, v, h;
    x = R(PLAYER_POS);
    z = R(PLAYER_POS + 8u);
    C(001B1240, self + 0xB0u, x, z, &v);
    W(ctl + 0x24u, v);
    h = R(self + 0xC4u);
    if (reload) v = R(ctl + 0x24u);
    C(001B12B0, v, h, rate, &v);
    W(self + 0xC4u, v);
    return 0;
}

/* The bite of the lunge and the charge: once per move (ctl+4), at or
 * before the clip's frame `frame`, when 0021BE40(0x8102B0, self) lets it
 * and 001A7B80 finds the player in reach: the player's hit bit 2, the
 * damage 0x8104D4 (`hard` with 0x81070A set, else `soft`) and the push
 * 0x810320 = the flattened direction from the node to the player. */
static uint32_t a17c_bite(A17C *o, uint32_t self, uint32_t ctl, uint32_t frame, uint32_t hard, uint32_t soft,
                          int *hit)
{
    int32_t r = 0;
    uint32_t a, b, flags, mode;
    *hit = 0;
    if (R(ctl + 4u) != 0) return 0;
    if (!LE(R(self + 0x3Cu), frame)) return 0;
    C(0021BE40, 0x8102B0u, self, &r);
    if (r != 0) return 0;
    C(001A7B80, self, &r);
    if (r == 0) return 0;
    W(ctl + 4u, 1);
    flags = B(0x8102B0u);
    mode = B(0x81070Au);
    WB(0x8102B0u, flags | 2u);
    W(0x8104D4u, mode != 0 ? hard : soft);
    a = R(PLAYER_POS);
    b = R(self + 0xB0u);
    W(0x810320u, SUB(a, b));
    W(0x810324u, 0);
    a = R(PLAYER_POS + 8u);
    b = R(self + 0xB8u);
    W(0x810328u, SUB(a, b));
    C(00102760, 0x810320u, 0x810320u);
    *hit = 1;
    return 0;
}

/* The pace of the lunge's and the charge's runs: the speed ctl+0x2C is
 * held at 0.5 (above it: 0.5 and no gain), else gains 0.05 (ctl+0x30). */
static void a17c_pace(A17C *o, uint32_t ctl)
{
    if (LE(R(ctl + 0x2Cu), F(0.5f))) {
        W(ctl + 0x30u, 0x3D4CCCCDu);
    } else {
        W(ctl + 0x2Cu, F(0.5f));
        W(ctl + 0x30u, 0);
    }
}

/* The end of a move at its clip's end: move 0, ctl+0x5D = 0 and the
 * pause ctl+0x50 = `base` + 0..31. */
static uint32_t a17c_rest(A17C *o, uint32_t self, uint32_t ctl, uint32_t base)
{
    int32_t r = 0;
    if ((R(ctl) & 0x1000) == 0) return 0;
    WB(self + 5u, 0);
    WB(self + 6u, 0);
    WB(ctl + 0x5Du, 0);
    C(00122BB8, &r);
    WH(ctl + 0x50u, (a17c_sra((uint32_t)r, 11) & 0x1F) + base);
    return 0;
}

/* Step 0 of the beam and spit moves: without a part in `parts` of
 * ctl+0x5B straight to step 3, else ctl+0x5D = 1, the speeds cleared and
 * clip `clip`. */
static uint32_t a17c_open(A17C *o, uint32_t self, uint32_t ctl, uint32_t step, int32_t parts, int32_t clip)
{
    if ((SB(ctl + 0x5Bu) & parts) == 0) {
        WB(self + 6u, 3);
        return 0;
    }
    WB(self + 6u, step + 1u);
    WB(ctl + 0x5Du, 1);
    W(ctl + 0x2Cu, 0);
    W(ctl + 0x30u, 0);
    C(001C67E0, self, clip, F(5.0f), 0);
    return 0;
}

/* The step byte +6 advances (read again) and clip `clip` starts. */
static uint32_t a17c_next(A17C *o, uint32_t self, int32_t clip)
{
    WB(self + 6u, B(self + 6u) + 1u);
    C(001C67E0, self, clip, 0, 0);
    return 0;
}

/* 00134300 (move 0): step 0 clears the phase ctl+4 and the speeds
 * ctl+0x30 / +0x2C and starts clip 6 (blend 10) or, one time in 16, clip
 * 0 (blend 5). Step 1, while the clip word +0x2C is 0: in phase 1 the
 * value ctl+0x3C eases 3% toward 90 and, when ctl+0x5C is set, the haze
 * 001E04E0(self + 0xB0, alpha, value) is drawn with alpha
 * 255 * (90 - value) / 90; from 80 on the phase advances. In phase 0, at
 * the clip's frame 130 or before, phase 1 starts at 10 with the cry
 * 0x84B. Then at the clip's end or while ctl+0x56 runs, move 1. */
uint32_t a17c_00134300(A17C *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    int32_t r = 0, flags;
    uint32_t phase, cur, next, alpha_f, v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 0:
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        W(ctl + 0x30u, 0);
        W(ctl + 0x2Cu, 0);
        C(00122BB8, &r);
        if (a17c_sra((uint32_t)r, 12) & 0xF) C(001C67E0, self, 6, F(10.0f), 0);
        else C(001C67E0, self, 0, F(5.0f), 0);
        return 0;
    case 1:
        break;
    default:
        return 0;
    }
    flags = SH(self + 0x2Cu);
    if (((uint32_t)flags & 0xFFFF7FFFu) == 0) {
        phase = R(ctl + 4u);
        if (phase == 1) {
            cur = R(ctl + 0x3Cu);
            next = ADD(cur, MUL(F(0.03f), SUB(F(90.0f), cur)));
            W(ctl + 0x3Cu, next);
            alpha_f = DIV(MUL(F(255.0f), SUB(F(90.0f), next)), F(90.0f));
            C(001281C0, alpha_f, &r);
            if (SB(ctl + 0x5Cu) != 0) SIB(a17c_001E04E0(o, self + 0xB0u, r, next, sp - 0x40u));
            if (!LT(R(ctl + 0x3Cu), F(80.0f))) {
                v = R(ctl + 4u);
                W(ctl + 4u, v + 1u);
            }
        } else if (phase == 0 && !(flags & 0x8000) && LE(R(self + 0x3Cu), F(130.0f))) {
            W(ctl + 4u, phase + 1u);
            W(ctl + 0x3Cu, F(10.0f));
            C(001FBD50, self, 0x84B, 0, F(450.0f), &r);
        }
    }
    if ((R(ctl) & 0x1000) || SH(ctl + 0x56u) != 0) {
        WB(self + 5u, 1);
        WB(self + 6u, 0);
    }
    return 0;
}

/* 00134500 (move 1): step 0 sets the speed ctl+0x2C = 0.4, the duration
 * ctl+4 = 60..187 and clip 1, and aims ctl+0x24 at a random direction
 * (three times in four) or at the player. Step 1 counts the duration down
 * (at 0: move 0). With the player far from the arena centre (ctl+0x5C)
 * it heads for the centre: more than 45 degrees off it turns twice as fast toward the point
 * 62 ahead along the centre direction, else, with ctl+0x50 run out, it
 * picks the next move from the byte table 0x245190. Else it
 * re-aims at the player every 0..15 frames and, with ctl+0x50 run out,
 * goes for the charge (6) when the player is 50 or more away or one time
 * in two, else the lunge (5). Every step ends with the turn toward
 * ctl+0x24 at 2 (or 4) degrees a frame. */
uint32_t a17c_00134500(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t scale = F(2.0f), x, z, v, h, w;
    const uint32_t step = B(self + 6u);
    if (step == 1) {
        w = R(ctl + 4u);
        if (w == 0) {
            WB(self + 5u, 0);
            WB(self + 6u, 0);
        } else {
            W(ctl + 4u, w - 1u);
            if (SB(ctl + 0x5Cu) != 0) {
                W(0x700038A0u, F(340.0f));
                W(0x700038A4u, 0);
                W(0x700038A8u, F(270.0f));
                W(0x700038ACu, F(1.0f));
                x = R(PLAYER_POS);
                z = R(PLAYER_POS + 8u);
                C(001B1240, V38A0, x, z, &v);
                C(001B1470, ADD(F(3.1415927f), v), &v);
                W(0x70003A20u, v);
                x = R(self + 0xB0u);
                z = R(self + 0xB8u);
                C(001B1240, V38A0, x, z, &v);
                h = R(0x70003A20u);
                C(001B1470, SUB(v, h), &v);
                W(0x70003A24u, v);
                C(0011DF78, v, &v);
                if (!LE(v, F(0.7853982f))) {
                    scale = F(4.0f);
                    C(001029C0, V36A0);
                    v = R(0x70003A20u);
                    C(00102BB0, V36A0, V36A0, v);
                    W(0x700038B0u, 0);
                    W(0x700038B4u, 0);
                    W(0x700038B8u, F(62.0f));
                    W(0x700038BCu, F(1.0f));
                    C(001026A0, V38B0, V36A0, V38B0);
                    C(001028B8, V38A0, V38A0, V38B0);
                    x = R(0x700038A0u);
                    z = R(0x700038A8u);
                    C(001B1240, self + 0xB0u, x, z, &v);
                    W(ctl + 0x24u, v);
                } else if (SH(ctl + 0x50u) == 0) {
                    C(00122BB8, &r);
                    WB(self + 5u, B(0x245190u + (a17c_sra((uint32_t)r, 14) & 0xF)));
                    WB(self + 6u, 0);
                    W(ctl + 0x2Cu, 0);
                }
            } else {
                w = R(ctl + 8u);
                if (w != 0) {
                    W(ctl + 8u, w - 1u);
                } else {
                    C(00122BB8, &r);
                    W(ctl + 8u, a17c_sra((uint32_t)r, 17) & 0xF);
                    x = R(PLAYER_POS);
                    z = R(PLAYER_POS + 8u);
                    C(001B1240, self + 0xB0u, x, z, &v);
                    W(ctl + 0x24u, v);
                }
                if (SH(ctl + 0x50u) == 0) {
                    uint32_t next = 6;
                    if (LT(R(ctl + 0x34u), F(50.0f))) {
                        C(00122BB8, &r);
                        if (!(a17c_sra((uint32_t)r, 19) & 1)) next = 5;
                    }
                    WB(self + 5u, next);
                    WB(self + 6u, 0);
                }
            }
        }
    } else if (step == 0) {
        WB(self + 6u, step + 1u);
        W(ctl + 0x2Cu, 0x3ECCCCCDu);
        C(00122BB8, &r);
        W(ctl + 4u, (a17c_sra((uint32_t)r, 17) & 0x7F) + 0x3C);
        W(ctl + 8u, 0);
        C(001C67E0, self, 1, F(5.0f), 0);
        C(00122BB8, &r);
        if (a17c_sra((uint32_t)r, 14) & 3) {
            C(00122BB8, &r);
            v = DIV(A17C_CVT_S_W(a17c_sra((uint32_t)r, 17) & 0xFF), F(255.0f));
            C(001B1470, MUL(F(6.2831855f), v), &v);
            W(ctl + 0x24u, v);
        } else {
            x = R(PLAYER_POS);
            z = R(PLAYER_POS + 8u);
            C(001B1240, self + 0xB0u, x, z, &v);
            W(ctl + 0x24u, v);
        }
    }
    v = R(ctl + 0x24u);
    h = R(self + 0xC4u);
    C(001B12B0, v, h, DIV(MUL(F(3.1415927f), scale), F(180.0f)), &v);
    W(self + 0xC4u, v);
    return 0;
}

/* One beam of 001348E0: from the bone ((0x275B40)->[bone] + 0x90) the
 * point `ahead` along the bone (0x700038A0, kept in 0x700036D0), the
 * colour 0.8 grey, 001B3C30 with reach 200 and damage 20 (0x81070A set)
 * or 15, and the sound 0x164. */
static uint32_t a17c_beam(A17C *o, uint32_t self, uint32_t bone, uint32_t ahead_x, uint32_t ahead_y)
{
    int32_t r = 0;
    uint32_t x, y, z, w, table, node;
    table = R(0x275B40u);
    node = R(table + bone);
    C(00102958, V36A0, node + 0x90u);
    W(0x700038A0u, ahead_x);
    W(0x700038A4u, ahead_y);
    W(0x700038A8u, 0);
    W(0x700038ACu, F(1.0f));
    C(001026A0, V38A0, V36A0, V38A0);
    x = R(0x700038A0u);
    y = R(0x700038A4u);
    z = R(0x700038A8u);
    w = R(0x700038ACu);
    W(0x700036D0u, x);
    W(0x700036D4u, y);
    W(0x700036D8u, z);
    W(0x700036DCu, w);
    W(0x700038B0u, 0x3F4CCCCDu);
    W(0x700038B4u, 0x3F4CCCCDu);
    W(0x700038B8u, 0x3F4CCCCDu);
    W(0x700038BCu, F(1.0f));
    if (B(0x81070Au) != 0) C(001B3C30, self, V36A0, V38B0, F(200.0f), F(20.0f));
    else C(001B3C30, self, V36A0, V38B0, F(200.0f), F(15.0f));
    C(001FBD50, self, 0x164, 0, F(300.0f), &r);
    return 0;
}

/* 001348E0 (move 2, the beams; the parts bits ctl+0x5B & 3 are the two
 * emitters). Step 0: without an emitter straight to step 3; else
 * ctl+0x5D = 1, the speeds cleared, clip 0x11. Step 1, at the clip's
 * end: the emitters' countdowns ctl+4 (0..7) and ctl+8 (0..7), the
 * repeats ctl+0xC (0..1), clip 0x12 and the spin ctl+0x3C = +-1 degree a
 * frame by the side 001B1380 finds the player on. Step 2: each live
 * emitter fires every 11th frame (bit 1: bone 0xAC, 15 ahead; bit 2:
 * bone 0x84, 13 ahead and 0.5 down); at the clip's end it repeats or
 * goes on (clip 0x13); the heading spins. Step 3, at the clip's end:
 * move 0, ctl+0x5D = 0, the pause ctl+0x50 = 0..31. */
uint32_t a17c_001348E0(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, s;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 0:
        if ((SB(ctl + 0x5Bu) & 3) == 0) {
            WB(self + 6u, 3);
            return 0;
        }
        WB(self + 6u, step + 1u);
        WB(ctl + 0x5Du, 1);
        W(ctl + 0x2Cu, 0);
        W(ctl + 0x30u, 0);
        C(001C67E0, self, 0x11, F(5.0f), 0);
        return 0;
    case 1:
        if ((R(ctl) & 0x1000) == 0) return 0;
        WB(self + 6u, step + 1u);
        C(00122BB8, &r);
        W(ctl + 4u, a17c_sra((uint32_t)r, 13) & 7);
        C(00122BB8, &r);
        W(ctl + 8u, a17c_sra((uint32_t)r, 8) & 7);
        C(00122BB8, &r);
        W(ctl + 0xCu, a17c_sra((uint32_t)r, 13) & 1);
        C(001C67E0, self, 0x12, 0, 0);
        v = R(self + 0xC4u);
        C(001B1380, PLAYER_POS, self + 0xB0u, v, &r);
        W(ctl + 0x3Cu, r != 0 ? 0x3C8EFA35u : 0xBC8EFA35u);
        return 0;
    case 2:
        if (SB(ctl + 0x5Bu) & 1) {
            v = R(ctl + 4u);
            if (v != 0) {
                W(ctl + 4u, v - 1u);
            } else {
                W(ctl + 4u, 10);
                SIB(a17c_beam(o, self, 0xACu, F(15.0f), 0));
            }
        }
        if (SB(ctl + 0x5Bu) & 2) {
            v = R(ctl + 8u);
            if (v != 0) {
                W(ctl + 8u, v - 1u);
            } else {
                W(ctl + 8u, 10);
                SIB(a17c_beam(o, self, 0x84u, F(13.0f), F(-0.5f)));
            }
        }
        if (R(ctl) & 0x1000) {
            v = R(ctl + 0xCu);
            if (v != 0) {
                W(ctl + 0xCu, v - 1u);
                C(001C67E0, self, 0x12, 0, 0);
            } else {
                WB(self + 6u, B(self + 6u) + 1u);
                C(001C67E0, self, 0x13, 0, 0);
            }
        }
        s = R(ctl + 0x3Cu);
        v = R(self + 0xC4u);
        C(001B1470, ADD(v, s), &v);
        W(self + 0xC4u, v);
        return 0;
    case 3:
        if ((R(ctl) & 0x1000) == 0) return 0;
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        WB(ctl + 0x5Du, 0);
        C(00122BB8, &r);
        WH(ctl + 0x50u, a17c_sra((uint32_t)r, 11) & 0x1F);
        return 0;
    default:
        return 0;
    }
}

/* 00134DC0 (move 3, the spit; the parts bits ctl+0x5B & 0xC are the two
 * mouths). Step 0 opens clip 0x14; step 1 turns toward the player at 3
 * degrees a frame, cries 0x850 at the clip's frame 7 and goes on to clip
 * 0x15; step 2, at the clip event 5, spits from each live mouth
 * (001370C0 at bone 0x2A or 0x26, kind 1) with the cry 0x851, then clip
 * 0x16; step 3 rests. */
uint32_t a17c_00134DC0(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 0:
        SIB(a17c_open(o, self, ctl, step, 0xC, 0x14));
        break;
    case 1:
        SIB(a17c_turn(o, self, ctl, 0x3D567750u, 1));
        if (!(SH(self + 0x2Cu) & 0x8000) && EQ(F(7.0f), R(self + 0x3Cu)))
            C(001FBD50, self, 0x850, 0, F(450.0f), &r);
        if (R(ctl) & 0x1000) SIB(a17c_next(o, self, 0x15));
        break;
    case 2:
        if (B(ctl) == 5) {
            if (SB(ctl + 0x5Bu) & 4) SIB(a17c_001370C0(o, 0x2A, 1));
            if (SB(ctl + 0x5Bu) & 8) SIB(a17c_001370C0(o, 0x26, 1));
            C(001FBD50, self, 0x851, 0, F(450.0f), &r);
        }
        if (R(ctl) & 0x1000) SIB(a17c_next(o, self, 0x16));
        break;
    case 3:
        SIB(a17c_rest(o, self, ctl, 0));
        break;
    default:
        break;
    }
    return 0;
}

/* 00134FF0 (move 4, the spit volley). Step 0 opens clip 0x17; step 1, at
 * the clip's end, clears the shot ctl+4 and the count ctl+8 and starts
 * clip 0x18 or 0x19 (one time in two each); step 2 shoots once per clip
 * at its frame 35 (clip 0x18) or 39 (0x19): each live mouth spits
 * (001370C0 bone 0x2A kind 1, bone 0x26 kind 0) with the cry 0x851; at
 * the clip's end, one time in two and up to 6 clips, another clip, else
 * clip 0x1A; step 3 rests. Steps 1 and 2 (while ctl+4 is 0) turn toward
 * the player at 5 degrees a frame. */
uint32_t a17c_00134FF0(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    int run = 0;
    uint32_t lim, v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 0:
        SIB(a17c_open(o, self, ctl, step, 0xC, 0x17));
        break;
    case 1:
        run = 1;
        if ((R(ctl) & 0x1000) == 0) break;
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0);
        W(ctl + 8u, 0);
        C(00122BB8, &r);
        if (a17c_sra((uint32_t)r, 11) & 1) C(001C67E0, self, 0x18, 0, 0);
        else C(001C67E0, self, 0x19, 0, 0);
        break;
    case 2:
        if (R(ctl + 4u) == 0) {
            run = 1;
            lim = (((uint32_t)SH(self + 0x2Cu) & 0xFFFF7FFFu) == 0x18) ? 0x23 : 0x27;
            if (LE(R(self + 0x3Cu), A17C_CVT_S_W(lim))) {
                W(ctl + 4u, 1);
                if (SB(ctl + 0x5Bu) & 4) SIB(a17c_001370C0(o, 0x2A, 1));
                if (SB(ctl + 0x5Bu) & 8) SIB(a17c_001370C0(o, 0x26, 0));
                C(001FBD50, self, 0x851, 0, F(450.0f), &r);
            }
        }
        if ((R(ctl) & 0x1000) == 0) break;
        C(00122BB8, &r);
        if (a17c_sra((uint32_t)r, 5) & 1) {
            v = R(ctl + 8u) + 1u;
            W(ctl + 8u, v);
            if ((int32_t)v < 6) {
                W(ctl + 4u, 0);
                C(00122BB8, &r);
                if (a17c_sra((uint32_t)r, 14) & 1) C(001C67E0, self, 0x18, 0, 0);
                else C(001C67E0, self, 0x19, 0, 0);
                break;
            }
        }
        SIB(a17c_next(o, self, 0x1A));
        break;
    case 3:
        SIB(a17c_rest(o, self, ctl, 0));
        break;
    default:
        break;
    }
    if (run) SIB(a17c_turn(o, self, ctl, 0x3DB2B8C3u, 0));
    return 0;
}

/* 001352E0 (move 5, the lunge). Step 0: ctl+0x5D = 1, the speeds
 * cleared, clip 7. Step 1 (turning at 5 degrees a frame): the cry 0x84D
 * at the clip's frame 30, then clip 9 with the bite cleared. Step 2: the
 * cry 0x84E at frame 70; between frames 20 and 62 the run's pace, else
 * standing; the bite (frame 49, damage 30 / 25, then 001B55E0(self, 1));
 * at the clip's end clip 0xB (step 5) after a bite, else clip 8. Step 3
 * (turning at 2.5 degrees): the pace; at the clip's end clip 0xA, the
 * speeds and the bite cleared, the cry 0x84F. Step 4: the bite (frame
 * 37, damage 35 / 30, 001B55E0) and clip 0xC at the end. Step 5 rests
 * (60 + 0..31 frames). */
uint32_t a17c_001352E0(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    int hit = 0;
    uint32_t ang = 0, d, x, z, v, h;
    int steer = 0;
    switch (B(self + 6u)) {
    case 0:
        WB(self + 6u, B(self + 6u) + 1u);
        WB(ctl + 0x5Du, 1);
        W(ctl + 0x2Cu, 0);
        W(ctl + 0x30u, 0);
        C(001C67E0, self, 7, F(5.0f), 0);
        break;
    case 1:
        ang = F(5.0f);
        steer = 1;
        if (!(SH(self + 0x2Cu) & 0x8000) && EQ(F(30.0f), R(self + 0x3Cu)))
            C(001FBD50, self, 0x84D, 0, F(300.0f), &r);
        if (R(ctl) & 0x1000) {
            WB(self + 6u, B(self + 6u) + 1u);
            W(ctl + 4u, 0);
            C(001C67E0, self, 9, 0, 0);
        }
        break;
    case 2:
        if (EQ(F(70.0f), R(self + 0x3Cu))) C(001FBD50, self, 0x84E, 0, F(300.0f), &r);
        d = R(self + 0x3Cu);
        if (LE(d, F(62.0f)) && !LT(d, F(20.0f))) {
            a17c_pace(o, ctl);
        } else {
            W(ctl + 0x2Cu, 0);
            W(ctl + 0x30u, 0);
        }
        SIB(a17c_bite(o, self, ctl, F(49.0f), F(30.0f), F(25.0f), &hit));
        if (hit) C(001B55E0, self, 1);
        if (R(ctl) & 0x1000) {
            if (R(ctl + 4u) != 0) {
                WB(self + 6u, 5);
                C(001C67E0, self, 0xB, 0, 0);
            } else {
                SIB(a17c_next(o, self, 8));
            }
        }
        break;
    case 3:
        ang = F(2.5f);
        steer = 1;
        a17c_pace(o, ctl);
        if (R(ctl) & 0x1000) {
            WB(self + 6u, B(self + 6u) + 1u);
            W(ctl + 4u, 0);
            W(ctl + 0x2Cu, 0);
            W(ctl + 0x30u, 0);
            C(001C67E0, self, 0xA, 0, 0);
            C(001FBD50, self, 0x84F, 0, F(300.0f), &r);
        }
        break;
    case 4:
        SIB(a17c_bite(o, self, ctl, F(37.0f), F(35.0f), F(30.0f), &hit));
        if (hit) C(001B55E0, self, 1);
        if (R(ctl) & 0x1000) SIB(a17c_next(o, self, 0xC));
        break;
    case 5:
        SIB(a17c_rest(o, self, ctl, 0x3C));
        break;
    default:
        break;
    }
    if (steer) {
        x = R(PLAYER_POS);
        z = R(PLAYER_POS + 8u);
        C(001B1240, self + 0xB0u, x, z, &v);
        W(ctl + 0x24u, v);
        h = R(self + 0xC4u);
        C(001B12B0, v, h, DIV(MUL(F(3.1415927f), ang), F(180.0f)), &v);
        W(self + 0xC4u, v);
    }
    return 0;
}

/* The flattened distance from the player to the node (0x700038A0 /
 * 0x700038B0, 001B15D0), with the shot counter ctl+4 cleared first when
 * `clear` is set (00135870). */
static uint32_t a17c_reach(A17C *o, uint32_t self, uint32_t ctl, int clear, uint32_t *d)
{
    if (clear) W(ctl + 4u, 0);
    C(00102948, V38A0, PLAYER_POS);
    C(00102948, V38B0, self + 0xB0u);
    W(0x700038B4u, 0);
    W(0x700038A4u, 0);
    C(001B15D0, V38A0, V38B0, d);
    return 0;
}

/* 00135870 (move 6, the charge). Step 0: ctl+0x5D = 1, the speeds
 * cleared, clip 0xD. Step 1, at the clip's end: within 50 of the player
 * the strike (step 3, clip 0xF), else the run (gain 0.05, clip 0xE).
 * Step 2 counts the run (ctl+4) at speed up to 0.8 and strikes within 50
 * of the player, after 300 frames or with the player far from the arena
 * centre (ctl+0x5C). Step 3:
 * the cry 0x84E at the clip's frame 37; from frame 21 on the pace, else
 * standing; the bite (frame 30, damage 35 / 30); clip 0x10 at the end.
 * Step 4 rests (60 + 0..31 frames). Steps 1 and 2 turn toward the player
 * at 5 degrees a frame. */
uint32_t a17c_00135870(A17C *o, uint32_t self, uint32_t ctl)
{
    int hit = 0, turn = 0;
    uint32_t d, v;
    const uint32_t step = B(self + 6u);
    switch (step) {
    case 4:
        SIB(a17c_rest(o, self, ctl, 0x3C));
        break;
    case 3:
        if (EQ(F(37.0f), R(self + 0x3Cu))) {
            int32_t r = 0;
            C(001FBD50, self, 0x84E, 0, F(300.0f), &r);
        }
        if (!LT(R(self + 0x3Cu), F(21.0f))) {
            a17c_pace(o, ctl);
        } else {
            W(ctl + 0x2Cu, 0);
            W(ctl + 0x30u, 0);
        }
        SIB(a17c_bite(o, self, ctl, F(30.0f), F(35.0f), F(30.0f), &hit));
        if (R(ctl) & 0x1000) {
            WB(self + 6u, B(self + 6u) + 1u);
            W(ctl + 0x2Cu, 0);
            W(ctl + 0x30u, 0);
            C(001C67E0, self, 0x10, 0, 0);
        }
        break;
    case 2:
        v = R(ctl + 4u) + 1u;
        W(ctl + 4u, v);
        turn = 1;
        if (!LE(R(ctl + 0x2Cu), F(0.8f))) {
            W(ctl + 0x2Cu, F(0.8f));
            W(ctl + 0x30u, 0);
        }
        SIB(a17c_reach(o, self, ctl, 0, &d));
        if (LE(d, F(50.0f)) || (int32_t)R(ctl + 4u) >= 0x12C || SB(ctl + 0x5Cu) != 0) {
            WB(self + 6u, B(self + 6u) + 1u);
            W(ctl + 4u, 0);
            W(ctl + 0x2Cu, 0);
            W(ctl + 0x30u, 0);
            C(001C67E0, self, 0xF, 0, 0);
        }
        break;
    case 1:
        turn = 1;
        if ((R(ctl) & 0x1000) == 0) break;
        SIB(a17c_reach(o, self, ctl, 1, &d));
        if (LE(d, F(50.0f))) {
            WB(self + 6u, 3);
            C(001C67E0, self, 0xF, 0, 0);
        } else {
            WB(self + 6u, B(self + 6u) + 1u);
            W(ctl + 0x30u, 0x3D4CCCCDu);
            C(001C67E0, self, 0xE, 0, 0);
        }
        break;
    case 0:
        WB(self + 6u, step + 1u);
        WB(ctl + 0x5Du, 1);
        W(ctl + 0x2Cu, 0);
        W(ctl + 0x30u, 0);
        C(001C67E0, self, 0xD, F(5.0f), 0);
        break;
    default:
        break;
    }
    if (turn) SIB(a17c_turn(o, self, ctl, 0x3DB2B8C3u, 0));
    return 0;
}

/* 00135D00 (move 8, the entrance; ctl+0x52 = 100 keeps the header at 3).
 * Step 0 places the body at (340, 220, 270) facing -2.2 and opens clip
 * 0x14; step 1: the cry 0x850 at frame 7, clip 0x15 at the end; step 2:
 * at the clip event 5 both mouths spit (kind 3) with the cry 0x851, clip
 * 0x16 at the end; step 3: gain 0.05 and clip 1 (blend 5); step 4 walks
 * toward (423.5, 235) at 5 degrees a frame with the speed up to 0.4 and,
 * within 40 of (423, 235) and facing it, stops into clip 7; step 5: clip
 * 9 at the end; step 6: the cry 0x84E at frame 70, clip 0xB at the end;
 * step 7: the wait ctl+4 = 180 and clip 6 (blend 10); step 8 counts it
 * down and ends the move. Once the arena byte 0x810805 reaches 4 the move
 * ends at once. */
uint32_t a17c_00135D00(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, h, d;
    uint32_t step;
    WH(ctl + 0x52u, 100);
    step = B(self + 6u);
    switch (step) {
    case 0:
        WB(self + 6u, B(self + 6u) + 1u);
        W(self + 0xB0u, F(340.0f));
        W(self + 0xB4u, F(220.0f));
        W(self + 0xB8u, F(270.0f));
        W(self + 0xC4u, 0xC00CF77Du);
        W(ctl + 0x2Cu, 0);
        W(ctl + 0x30u, 0);
        C(001C67E0, self, 0x14, F(5.0f), 0);
        break;
    case 1:
        if (!(SH(self + 0x2Cu) & 0x8000) && EQ(F(7.0f), R(self + 0x3Cu)))
            C(001FBD50, self, 0x850, 0, F(450.0f), &r);
        if (R(ctl) & 0x1000) SIB(a17c_next(o, self, 0x15));
        break;
    case 2:
        if (B(ctl) == 5) {
            SIB(a17c_001370C0(o, 0x2A, 3));
            SIB(a17c_001370C0(o, 0x26, 3));
            C(001FBD50, self, 0x851, 0, F(450.0f), &r);
        }
        if (R(ctl) & 0x1000) SIB(a17c_next(o, self, 0x16));
        break;
    case 3:
        if ((R(ctl) & 0x1000) == 0) break;
        WB(self + 6u, step + 1u);
        W(ctl + 0x30u, 0x3D4CCCCDu);
        C(001C67E0, self, 1, F(5.0f), 0);
        break;
    case 4:
        C(001B1240, self + 0xB0u, F(423.5f), F(235.0f), &v);
        W(ctl + 0x24u, v);
        h = R(self + 0xC4u);
        v = R(ctl + 0x24u);
        C(001B12B0, v, h, 0x3DB2B8C3u, &v);
        W(self + 0xC4u, v);
        if (!LE(R(ctl + 0x2Cu), F(0.4f))) {
            W(ctl + 0x2Cu, F(0.4f));
            W(ctl + 0x30u, 0);
        }
        W(0x700038A0u, F(423.0f));
        W(0x700038A4u, 0);
        W(0x700038A8u, F(235.0f));
        W(0x700038ACu, F(1.0f));
        C(00102948, V38B0, self + 0xB0u);
        W(0x700038B4u, 0);
        C(001B15D0, V38A0, V38B0, &d);
        if (LE(d, F(40.0f))) {
            h = R(self + 0xC4u);
            v = R(ctl + 0x24u);
            if (EQ(h, v)) {
                WB(self + 6u, B(self + 6u) + 1u);
                W(ctl + 0x2Cu, 0);
                W(ctl + 0x30u, 0);
                C(001C67E0, self, 7, F(5.0f), 0);
            }
        }
        break;
    case 5:
        if ((R(ctl) & 0x1000) == 0) break;
        WB(self + 6u, step + 1u);
        C(001C67E0, self, 9, 0, 0);
        break;
    case 6:
        if (EQ(F(70.0f), R(self + 0x3Cu))) C(001FBD50, self, 0x84E, 0, F(450.0f), &r);
        if (R(ctl) & 0x1000) SIB(a17c_next(o, self, 0xB));
        break;
    case 7:
        if ((R(ctl) & 0x1000) == 0) break;
        WB(self + 6u, step + 1u);
        W(ctl + 4u, 0xB4);
        C(001C67E0, self, 6, F(10.0f), 0);
        break;
    case 8:
        v = R(ctl + 4u);
        if (v != 0) {
            W(ctl + 4u, v - 1u);
        } else {
            WB(self + 5u, 0);
            WB(self + 6u, 0);
            WH(ctl + 0x52u, 0);
        }
        break;
    default:
        break;
    }
    if (B(0x810805u) >= 4) {
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        WH(ctl + 0x52u, 0);
        W(ctl + 0x30u, 0);
        W(ctl + 0x2Cu, 0);
    }
    return 0;
}
