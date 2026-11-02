/* AREA21's creature: the moves of state 1 (0014EEF0, 0014F390, 0014FBC0,
 * picked by the move byte +5 in 0014EDD0) and of state 2 (00150260,
 * 001504D0, 00150600, picked in 00150190). Each is stepped by the byte
 * +6. From the decomp C (src/func_*.c), checked against the original
 * instructions by tools/test_area21_creature_b_reference.py. */
#include "em_area21_creature_b_local.h"

/* The move table of 0014F390: rows of 0x44 bytes, one per pattern
 * ctl+0x66; `column` is the address of row 0's field. */
static uint32_t row_at(uint32_t column, int32_t pattern)
{
    return column + (uint32_t)pattern * 0x44u;
}

/* A random byte of 00122BB8's result (bits n..n+7) over 255.0. */
static uint32_t rand_unit(int32_t r, unsigned n)
{
    return DIV(CVT(((uint32_t)r >> n) & 0xFFu), F(255.0f));
}

/* 0014EEF0: move 0, the leap. The frame count ctl+4 steps on every call.
 * Step 0: the count, the heading +0xC4 and the tally ctl+0x52 cleared,
 * ctl+0x20 = 1.0; then step 1. Step 1: the sound flags ctl+0xC cleared,
 * the climb ctl+0x38 = 3.0 and ctl+0x40 = 1.0, a random drift ctl+0x2C in
 * [-0.5, 0.5] and random falls ctl+0x3C, ctl+0x48; the spawn point
 * 00150DA0, height +0xB4 = -120, clip 0 and the linked effect's +5 = 1.
 * Step 2: the drift moves +0xB0; ctl+0x38 / ctl+0x40 stop at -4 (their
 * falls cleared); ctl+0x64 = (ctl+0x40 < 0); at the top (ctl+0x38 first
 * below 0, bit 0x80 of ctl+0x67) half the time the spray 00150F40 and the
 * effect's +5 = 2; the cries 0x872 (falling, at or below 90) and 0x871
 * (rising, from -95; one in four also 0x873), each once; at the clip's
 * end (ctl+0 bit 0x1000) and at or below -160 the next step with a wait
 * ctl+8 of 30..93. Step 3: the wait; then step 1 again, unless the pattern
 * ctl+0x65 is 2, ctl+0x52 is below 200 and a one-in-four chance picks
 * move 2, or ctl+0x52 reached 200 or the count reached its limit (2400
 * frames for pattern 2, else 1200): then move 1. */
uint32_t a21b_0014EEF0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, limit;
    uint32_t v, link, st, q;
    v = R(ctl + 4u);
    W(ctl + 4u, v + 1u);
    st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(self + 0xC4u, 0);
        W(ctl + 4u, 0);
        W(ctl + 0x20u, F(1.0f));
        WH(ctl + 0x52u, 0);
        /* fall through */
    case 1:
        v = B(self + 6u);
        WB(self + 6u, v + 1u);
        W(ctl + 0xCu, 0);
        W(ctl + 0x38u, F(3.0f));
        W(ctl + 0x40u, F(1.0f));
        C(00122BB8, &r);
        W(ctl + 0x2Cu, SUB(rand_unit(r, 5), F(0.5f)));
        C(00122BB8, &r);
        q = rand_unit(r, 18);
        W(ctl + 0x3Cu, ADD(F(-0.018f), MUL(F(-0.002f), q)));
        C(00122BB8, &r);
        q = rand_unit(r, 12);
        W(ctl + 0x48u, ADD(F(-0.0065f), MUL(F(-0.004f), q)));
        WB(ctl + 0x64u, 0);
        WB(ctl + 0x67u, 0);
        SIB(a21b_00150DA0(o, self, ctl));
        W(self + 0xB4u, F(-120.0f));
        C(001C67E0, self, 0, 0, 0);
        link = R(self + 0x20u);
        if (link != 0) WB(link + 5u, 1);
        break;
    case 2:
        v = R(ctl + 0x2Cu);
        W(self + 0xB0u, ADD(R(self + 0xB0u), v));
        if (LT(R(ctl + 0x38u), F(-4.0f))) {
            W(ctl + 0x38u, F(-4.0f));
            W(ctl + 0x3Cu, 0);
        }
        if (LT(R(ctl + 0x40u), F(-4.0f))) {
            W(ctl + 0x40u, F(-4.0f));
            W(ctl + 0x48u, 0);
        }
        WB(ctl + 0x64u, LT(R(ctl + 0x40u), 0) ? 1u : 0u);
        if (LT(R(ctl + 0x38u), 0) && !(SB(ctl + 0x67u) & 0x80)) {
            WB(ctl + 0x67u, 0x80);
            C(00122BB8, &r);
            if (((uint32_t)r >> 15) & 1u) SIB(a21b_00150F40(o, self, ctl));
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 2);
        }
        if (LT(R(ctl + 0x38u), 0)) {
            v = R(ctl + 0xCu);
            if (!(v & 2u) && LE(R(self + 0xB4u), F(90.0f))) {
                W(ctl + 0xCu, v | 2u);
                C(001FBD50, self, 0x872, 0, F(1000.0f), &r);
            }
        } else {
            v = R(ctl + 0xCu);
            if (!(v & 1u) && !LT(R(self + 0xB4u), F(-95.0f))) {
                W(ctl + 0xCu, v | 1u);
                C(001FBD50, self, 0x871, 0, F(1000.0f), &r);
                C(00122BB8, &r);
                if ((((uint32_t)r >> 14) & 3u) == 0) C(001FBD50, self, 0x873, 0, F(800.0f), &r);
            }
        }
        if ((R(ctl) & 0x1000u) && LE(R(self + 0xB4u), F(-160.0f))) {
            v = B(self + 6u);
            WB(self + 6u, v + 1u);
            C(00122BB8, &r);
            W(ctl + 8u, (((uint32_t)r >> 14) & 0x3Fu) + 0x1Eu);
        }
        break;
    case 3:
        v = R(ctl + 8u);
        if (v != 0) {
            W(ctl + 8u, v - 1u);
            break;
        }
        WB(self + 6u, 1);
        if (SB(ctl + 0x65u) != 2) {
            limit = 0x4B0;
        } else {
            limit = 0x960;
            if (SH(ctl + 0x52u) < 200) {
                C(00122BB8, &r);
                if ((((uint32_t)r >> 7) & 3u) == 0) {
                    WB(self + 5u, 2);
                    WB(self + 6u, 0);
                    break;
                }
            }
        }
        if (SH(ctl + 0x52u) >= 200 || (int32_t)R(ctl + 4u) >= limit) {
            WB(self + 5u, 1);
            WB(self + 6u, 0);
        }
        break;
    default:
        break;
    }
    return 0;
}

/* 0014F390's turn-around of the swing: ctl+4 = 1, ctl+0x30 negated, the
 * swing speed ctl+0x34 = -1.4 times the pattern's column 0x3C. */
static uint32_t swing_back(A21B *o, uint32_t ctl)
{
    uint32_t v;
    int32_t k;
    W(ctl + 4u, 1);
    v = R(ctl + 0x30u);
    W(ctl + 0x30u, MUL(F(-1.0f), v));
    k = SB(ctl + 0x66u);
    v = R(row_at(0x245D3Cu, k));
    W(ctl + 0x34u, MUL(F(-1.4f), v));
    return 0;
}

/* 0014F390: move 1, the swing. Step 0: the counts ctl+4 / +8 cleared,
 * ctl+0x64 = 6, ctl+0x67 = 0, the pattern ctl+0x66 drawn from the row of
 * ctl+0x65 in the table 0x245EC0 (16 per row, a random column), its swing
 * (ctl+0x2C..+0x34) and climb (ctl+0x38..+0x48) set-up from the pattern
 * rows at 0x245D20, the heading +0xC4 (degrees to radians), the four
 * random part delays ctl+0x68.. (counts ctl+0x6C.. cleared), the linked
 * effect's +5 = 1, clip 1 and the start point +0xB0..+0xBC. Step 1: the
 * swing speed ctl+0x34 runs to its stop ctl+0x30 or starts once the height
 * passes the pattern's mark; the swing moves +0xB0; the climb stops at the
 * pattern's floor; at the top the effect's +5 = 2; the second swing
 * (ctl+0x40..+0x48) likewise; once (ctl+4) the swing turns back past 90 /
 * inside 110, or for pattern 2 the climb doubles past 140; the parts
 * 00151200; the cries 0x872 and 0x871 / 0x879 (flags ctl+8); at the clip's
 * end at or below -160 back to move 0 with the pattern's next ctl+0x65. */
uint32_t a21b_0014F390(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, k, n, i;
    uint32_t v, a, b, m, link, dv, f2;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(ctl + 4u, 0);
        W(ctl + 8u, 0);
        WB(ctl + 0x64u, 6);
        WB(ctl + 0x67u, 0);
        C(00122BB8, &r);
        k = SB(ctl + 0x65u);
        v = (uint32_t)SB(0x245EC0u + (uint32_t)k * 16u + (((uint32_t)r >> 12) & 0xFu));
        WB(ctl + 0x66u, v);
        k = SB(ctl + 0x66u);
        W(ctl + 0x2Cu, R(row_at(0x245D34u, k)));
        k = SB(ctl + 0x66u);
        W(ctl + 0x30u, R(row_at(0x245D38u, k)));
        W(ctl + 0x34u, 0);
        k = SB(ctl + 0x66u);
        W(ctl + 0x38u, R(row_at(0x245D40u, k)));
        k = SB(ctl + 0x66u);
        W(ctl + 0x3Cu, R(row_at(0x245D48u, k)));
        k = SB(ctl + 0x66u);
        W(ctl + 0x40u, R(row_at(0x245D4Cu, k)));
        k = SB(ctl + 0x66u);
        W(ctl + 0x44u, R(row_at(0x245D50u, k)));
        W(ctl + 0x48u, 0);
        k = SB(ctl + 0x66u);
        v = R(row_at(0x245D20u, k));
        W(self + 0xC4u, DIV(MUL(F(3.14159265f), v), F(180.0f)));
        for (i = 0; i < 4; i++) {
            WB(ctl + 0x6Cu + (uint32_t)i, 0);
            C(00122BB8, &r);
            WB(ctl + 0x68u + (uint32_t)i, ((uint32_t)r >> (i + 9)) & 7u);
        }
        link = R(self + 0x20u);
        if (link != 0) WB(link + 5u, 1);
        k = SB(ctl + 0x66u);
        v = R(row_at(0x245D24u, k));
        C(001C67E0, self, 1, 0, v);
        k = SB(ctl + 0x66u);
        W(self + 0xB0u, R(row_at(0x245D28u, k)));
        k = SB(ctl + 0x66u);
        W(self + 0xB4u, R(row_at(0x245D2Cu, k)));
        k = SB(ctl + 0x66u);
        W(self + 0xB8u, R(row_at(0x245D30u, k)));
        W(self + 0xBCu, F(1.0f));
        break;
    case 1:
        dv = R(ctl + 0x34u);
        if (!LE(dv, 0)) {
            a = R(ctl + 0x2Cu);
            b = R(ctl + 0x30u);
            if (!LT(a, b)) {
                W(ctl + 0x2Cu, b);
                W(ctl + 0x34u, 0);
            }
        } else if (LT(dv, 0)) {
            a = R(ctl + 0x2Cu);
            b = R(ctl + 0x30u);
            if (LE(a, b)) {
                W(ctl + 0x2Cu, b);
                W(ctl + 0x34u, 0);
            }
        } else {
            k = SB(ctl + 0x66u);
            a = R(self + 0xB4u);
            if (!LT(a, R(row_at(0x245D58u, k)))) W(ctl + 0x34u, R(row_at(0x245D3Cu, k)));
        }
        dv = R(ctl + 0x34u);
        a = R(ctl + 0x2Cu);
        v = ADD(a, dv);
        W(ctl + 0x2Cu, v);
        W(self + 0xB0u, ADD(R(self + 0xB0u), v));
        k = SB(ctl + 0x66u);
        a = R(ctl + 0x38u);
        m = R(row_at(0x245D44u, k));
        if (LT(a, m)) {
            W(ctl + 0x38u, m);
            W(ctl + 0x3Cu, 0);
        }
        if (LT(R(ctl + 0x38u), 0) && !(SB(ctl + 0x67u) & 0x80)) {
            WB(ctl + 0x67u, 0x80);
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 2);
        }
        dv = R(ctl + 0x48u);
        if (!LE(dv, 0)) {
            a = R(ctl + 0x40u);
            b = R(ctl + 0x44u);
            if (!LT(a, b)) {
                W(ctl + 0x40u, b);
                W(ctl + 0x48u, 0);
            }
        } else if (LT(dv, 0)) {
            a = R(ctl + 0x40u);
            b = R(ctl + 0x44u);
            if (LE(a, b)) {
                W(ctl + 0x40u, b);
                W(ctl + 0x48u, 0);
            }
        } else {
            k = SB(ctl + 0x66u);
            a = R(self + 0xB4u);
            if (!LT(a, R(row_at(0x245D5Cu, k)))) W(ctl + 0x48u, R(row_at(0x245D54u, k)));
        }
        n = (int32_t)R(ctl + 4u);
        if (n == 0 && SB(row_at(0x245D60u, SB(ctl + 0x66u))) == 2) {
            f2 = R(ctl + 0x2Cu);
            if (!LE(f2, 0) && !LT(R(ctl + 0x34u), 0) && !LT(R(self + 0xB0u), F(90.0f))) {
                SIB(swing_back(o, ctl));
            } else if (LT(f2, 0)) {
                if (LE(R(ctl + 0x34u), 0) && LE(R(self + 0xB0u), F(110.0f))) SIB(swing_back(o, ctl));
            }
        } else if (n == 0 && SB(ctl + 0x65u) == 2 && !LT(R(self + 0xB8u), F(140.0f))) {
            W(ctl + 4u, 1);
            v = R(ctl + 0x44u);
            W(ctl + 0x44u, MUL(F(-2.0f), v));
            k = SB(ctl + 0x66u);
            v = R(row_at(0x245D54u, k));
            W(ctl + 0x48u, MUL(F(-2.0f), v));
            k = SB(ctl + 0x66u);
            W(ctl + 0x34u, R(row_at(0x245D3Cu, k)));
        }
        SIB(a21b_00151200(o, self, ctl));
        if (LT(R(ctl + 0x38u), 0)) {
            v = R(ctl + 8u);
            if (!(v & 2u) && LE(R(self + 0xB4u), F(90.0f))) {
                W(ctl + 8u, v | 2u);
                C(001FBD50, self, 0x872, 0, F(1000.0f), &r);
            }
        } else {
            v = R(ctl + 8u);
            if (!(v & 1u) && !LT(R(self + 0xB4u), F(-95.0f))) {
                W(ctl + 8u, v | 1u);
                C(001FBD50, self, 0x871, 0, F(1000.0f), &r);
                C(001FBD50, self, 0x879, 0, F(800.0f), &r);
            }
        }
        if ((R(ctl) & 0x1000u) && LE(R(self + 0xB4u), F(-160.0f))) {
            WB(self + 5u, 0);
            WB(self + 6u, 0);
            WB(ctl + 0x64u, 0);
            W(self + 0xC4u, 0);
            k = SB(ctl + 0x66u);
            WB(ctl + 0x65u, (uint32_t)SB(row_at(0x245D60u, k)));
        }
        break;
    default:
        break;
    }
    return 0;
}

/* 0014FBC0: move 2, the lunge. Step 0: heading +0xC4 = pi, the counts
 * ctl+4 / +8 / +0xC cleared, ctl+0x64 = 6, the climb ctl+0x38 = 3.0 with
 * the fall ctl+0x3C, clip 2, the start point (100, -100, -80) and the
 * linked effect's +5 = 3. Step 1: the cry 0x871 once from the height -35;
 * the heading turns toward 0 by 2 degrees (001B12B0); at the clip's end
 * the next step, clip 3. Step 2: while ctl+4 is below the word 0x275428 and
 * the clip frame +0x3C is at most 106, the point on bone 4 is placed
 * (001026A0) and, at ctl+4 = 0, the bubbles 001EFD90(8) and the cry 0x878
 * once; every 0x27542C frames (ctl+8) ctl+4 steps on with the bubbles
 * 001EFD90(0x80000064) and the cry 0x87A, and at the second the player
 * (0x8102B0) is grabbed when 0021BE40 allows: flags, the turn toward the
 * player and the pull vector 0x810320; then the fall ctl+0x48 starts at
 * frame 118 and ctl+0x40 stops at -2; at the clip's end the next step with
 * clip 4 and the cry 0x871. Step 3: the settle (ctl+0x3C = -0.1 by frame
 * 58), the cry 0x872 once, and at the clip's end at or below -140 move 1. */
uint32_t a21b_0014FBC0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, n;
    uint32_t v, x, z, t, rec, link;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(self + 0xC4u, F(3.14159265f));
        W(ctl + 4u, 0);
        W(ctl + 8u, 0);
        W(ctl + 0xCu, 0);
        WB(ctl + 0x64u, 6);
        W(ctl + 0x38u, F(3.0f));
        W(ctl + 0x3Cu, F(-0.0295f));
        W(ctl + 0x34u, 0);
        W(ctl + 0x2Cu, 0);
        W(ctl + 0x48u, 0);
        W(ctl + 0x40u, 0);
        C(001C67E0, self, 2, 0, 0);
        W(self + 0xB0u, F(100.0f));
        W(self + 0xB4u, F(-100.0f));
        W(self + 0xB8u, F(-80.0f));
        W(self + 0xBCu, 0);
        link = R(self + 0x20u);
        if (link != 0) WB(link + 5u, 3);
        break;
    case 1:
        if (R(ctl + 0xCu) == 0 && !LT(R(self + 0xB4u), F(-35.0f))) {
            W(ctl + 0xCu, 1);
            C(001FBD50, self, 0x871, 0, F(1000.0f), &r);
        }
        v = R(self + 0xC4u);
        C(001B12B0, 0, v, F(0.034906585f), &v);
        W(self + 0xC4u, v);
        if (R(ctl) & 0x1000u) {
            v = B(self + 6u);
            WB(self + 6u, v + 1u);
            W(ctl + 0xCu, 0);
            W(self + 0xC4u, 0);
            W(ctl + 0x3Cu, 0);
            W(ctl + 0x38u, 0);
            C(001C67E0, self, 3, 0, 0);
        }
        break;
    case 2:
        n = (int32_t)R(ctl + 4u);
        if (n < (int32_t)R(0x275428u) && LE(R(self + 0x3Cu), F(106.0f))) {
            W(0x700038A0u, F(48.8871002f));
            W(0x700038A4u, F(10.4990997f));
            W(0x700038A8u, F(-1.44000006f));
            t = R(BONE_TABLE);
            W(0x700038ACu, F(1.0f));
            rec = R(t + 0x10u);
            C(00102958, V36A0, rec + 0x90u);
            C(001026A0, V38A0, V36A0, V38A0);
            if (R(ctl + 4u) == 0) {
                W(0x700038B8u, 0);
                W(0x700038B4u, 0);
                W(0x700038B0u, 0);
                W(0x700038BCu, F(1.0f));
                C(001EFD90, 8, V38A0, V38B0);
                if (R(ctl + 0xCu) == 0) {
                    W(ctl + 0xCu, 1);
                    C(001FBD50, self, 0x878, 0, F(800.0f), &r);
                }
            }
            v = R(ctl + 8u) + 1u;
            W(ctl + 8u, v);
            if (!((int32_t)v < (int32_t)R(0x27542Cu))) {
                v = R(ctl + 4u);
                W(ctl + 4u, v + 1u);
                W(ctl + 8u, 0);
                W(0x700038B0u, 0);
                W(0x700038B4u, 0);
                W(0x700038B8u, R(0x245EF0u));
                W(0x700038BCu, F(1.0f));
                C(001EFD90, (int32_t)0x80000064u, V38A0, V38B0);
                C(001FBD50, self, 0x87A, 0, F(800.0f), &r);
                if (R(ctl + 4u) == 2) {
                    C(0021BE40, 0x8102B0u, self, &r);
                    if (r == 0) {
                        v = B(0x8102B0u);
                        WB(0x8102B0u, v | 2u);
                        WB(0x8102BFu, 2);
                        x = R(PLAYER_POS);
                        z = R(PLAYER_POS + 8u);
                        C(001B1240, self + 0xB0u, x, z, &v);
                        W(S3A20, v);
                        C(001029C0, V36A0);
                        v = R(S3A20);
                        C(00102BB0, V36A0, V36A0, v);
                        W(0x810320u, 0);
                        W(0x810324u, 0);
                        W(0x810328u, F(1.0f));
                        W(0x81032Cu, F(1.0f));
                        C(001026A0, 0x810320u, V36A0, 0x810320u);
                    }
                }
            }
        }
        v = R(ctl + 0x40u);
        if (EQ(0, v)) {
            if (LE(R(self + 0x3Cu), F(118.0f))) W(ctl + 0x48u, F(-0.02f));
        } else if (LE(v, F(-2.0f))) {
            W(ctl + 0x40u, F(-2.0f));
            W(ctl + 0x48u, 0);
        }
        if (R(ctl) & 0x1000u) {
            v = B(self + 6u);
            WB(self + 6u, v + 1u);
            W(ctl + 0xCu, 0);
            W(ctl + 0x48u, 0);
            C(001C67E0, self, 4, 0, 0);
            C(001FBD50, self, 0x871, 0, F(1000.0f), &r);
        }
        break;
    case 3:
        if (EQ(0, R(ctl + 0x3Cu)) && LE(R(self + 0x3Cu), F(58.0f))) {
            W(ctl + 0x38u, 0);
            W(ctl + 0x3Cu, F(-0.1f));
        }
        if (R(ctl + 0xCu) == 0 && LT(R(ctl + 0x38u), 0) && LE(R(self + 0xB4u), F(40.0f))) {
            W(ctl + 0xCu, 1);
            C(001FBD50, self, 0x872, 0, F(1000.0f), &r);
        }
        if ((R(ctl) & 0x1000u) && LE(R(self + 0xB4u), F(-140.0f))) {
            WB(self + 5u, 1);
            WB(self + 6u, 0);
        }
        break;
    default:
        break;
    }
    return 0;
}

/* 00150260: state 2 move 0, the knock-back. Step 0: heading and ctl+4
 * cleared, ctl+0x64 = 6, the climb 2.5 with the falls ctl+0x3C / +0x48,
 * ctl+0x67 = 0, ctl+0x40 at most 0, the swing cleared, clip 6 from frame
 * 10; with hit points left and no kind bit 0x8000 the header 1 and the hit
 * +0x36 cleared; the cry 0x87D. Step 1: ctl+0x40 stops at -4; at the top
 * the effect's +5 = 2 (once, ctl+0x67); the cry 0x872 once at or below 88;
 * at the clip's end at or below -130 the motion stops and the next move:
 * back to state 1 with hit points left (no 0x8000), else move 2 (no hit
 * points) or move 1. */
uint32_t a21b_00150260(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, hp;
    uint32_t link;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(self + 0xC4u, 0);
        W(ctl + 4u, 0);
        WB(ctl + 0x64u, 6);
        W(ctl + 0x38u, F(2.5f));
        W(ctl + 0x3Cu, F(-0.04f));
        W(ctl + 0x48u, F(-0.025f));
        WB(ctl + 0x67u, 0);
        if (!LE(R(ctl + 0x40u), 0)) W(ctl + 0x40u, 0);
        W(ctl + 0x34u, 0);
        W(ctl + 0x2Cu, 0);
        C(001C67E0, self, 6, F(10.0f), 0);
        if (SH(self + 0x34u) != 0 && !(SH(self + 0x36u) & 0x8000)) {
            WB(self, 1);
            WH(self + 0x36u, 0);
        }
        C(001FBD50, self, 0x87D, 0, F(1000.0f), &r);
        break;
    case 1:
        if (LT(R(ctl + 0x40u), F(-4.0f))) {
            W(ctl + 0x40u, F(-4.0f));
            W(ctl + 0x34u, 0);
        }
        if (LT(R(ctl + 0x38u), 0) && SB(ctl + 0x67u) == 0) {
            WB(ctl + 0x67u, 1);
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 2);
        }
        if (R(ctl + 4u) == 0 && LT(R(ctl + 0x38u), 0) && LE(R(self + 0xB4u), F(88.0f))) {
            W(ctl + 4u, 1);
            C(001FBD50, self, 0x872, 0, F(1000.0f), &r);
        }
        if ((R(ctl) & 0x1000u) && LE(R(self + 0xB4u), F(-130.0f))) {
            WB(self + 6u, 0);
            W(ctl + 0x3Cu, 0);
            W(ctl + 0x38u, 0);
            W(ctl + 0x48u, 0);
            W(ctl + 0x40u, 0);
            hp = SH(self + 0x34u);
            if (hp != 0 && !(SH(self + 0x36u) & 0x8000)) {
                WB(self + 4u, 1);
                WB(self + 5u, 0);
                WH(self + 0x36u, 0);
            } else if (hp == 0) {
                WB(self + 5u, 2);
            } else {
                WB(self + 5u, 1);
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

/* 001504D0: state 2 move 1, the leap back in. Step 0: height -90,
 * ctl+0x64 = 6, the climb 3.5 with the fall -0.045, ctl+0x40 = -1, the
 * rest of the motion cleared, clip 5, the linked effect's +5 = 3 and the
 * cries 0x871, 0x87D. Step 1: at the clip's end at or below -130, back to
 * state 1 move 0 with the hit +0x36 and the motion cleared. */
uint32_t a21b_001504D0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t link;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        W(self + 0xB4u, F(-90.0f));
        WB(ctl + 0x64u, 6);
        W(ctl + 0x38u, F(3.5f));
        W(ctl + 0x3Cu, F(-0.045f));
        W(ctl + 0x40u, F(-1.0f));
        W(ctl + 0x48u, 0);
        W(ctl + 0x34u, 0);
        W(ctl + 0x2Cu, 0);
        C(001C67E0, self, 5, 0, 0);
        link = R(self + 0x20u);
        if (link != 0) WB(link + 5u, 3);
        C(001FBD50, self, 0x871, 0, F(1000.0f), &r);
        C(001FBD50, self, 0x87D, 0, F(1000.0f), &r);
        break;
    case 1:
        if ((R(ctl) & 0x1000u) && LE(R(self + 0xB4u), F(-130.0f))) {
            WB(self + 4u, 1);
            WB(self + 5u, 0);
            WB(self + 6u, 0);
            WH(self + 0x36u, 0);
            W(ctl + 0x3Cu, 0);
            W(ctl + 0x38u, 0);
            W(ctl + 0x48u, 0);
            W(ctl + 0x40u, 0);
        }
        break;
    default:
        break;
    }
    return 0;
}

/* 00150600: state 2 move 2, the end leap. Step 0: the byte 0x81080E = 1.
 * Step 1: once 0x81080E is 4, the motion set-up (climb 2.0, fall -0.0125,
 * ctl+0x40 = -1), the point (100, -85, -80), clip 7, the linked effect's
 * +5 = 3 and the cry 0x87E. Step 2: the cries 0x872 (falling, at or below
 * 40) and 0x871 (rising, from -35), each once (flags ctl+4); at the clip's
 * end at or below -130 the state 3 for the creature and its effect. */
uint32_t a21b_00150600(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v, link;
    const uint32_t st = B(self + 6u);
    switch (st) {
    case 0:
        WB(self + 6u, st + 1u);
        WB(0x81080Eu, 1);
        break;
    case 1:
        if (B(0x81080Eu) == 4) {
            WB(self + 6u, st + 1u);
            W(ctl + 4u, 0);
            WB(ctl + 0x64u, 6);
            W(ctl + 0x38u, F(2.0f));
            W(ctl + 0x3Cu, F(-0.0125f));
            W(ctl + 0x40u, F(-1.0f));
            W(ctl + 0x48u, 0);
            W(ctl + 0x34u, 0);
            W(ctl + 0x2Cu, 0);
            W(self + 0xB0u, F(100.0f));
            W(self + 0xB4u, F(-85.0f));
            W(self + 0xB8u, F(-80.0f));
            W(self + 0xBCu, 0);
            C(001C67E0, self, 7, 0, 0);
            link = R(self + 0x20u);
            if (link != 0) WB(link + 5u, 3);
            C(001FBD50, self, 0x87E, 0, F(500.0f), &r);
        }
        break;
    case 2:
        if (LT(R(ctl + 0x38u), 0)) {
            v = R(ctl + 4u);
            if (!(v & 2u) && LE(R(self + 0xB4u), F(40.0f))) {
                W(ctl + 4u, v | 2u);
                C(001FBD50, self, 0x872, 0, F(1000.0f), &r);
            }
        } else {
            v = R(ctl + 4u);
            if (!(v & 1u) && !LT(R(self + 0xB4u), F(-35.0f))) {
                W(ctl + 4u, v | 1u);
                C(001FBD50, self, 0x871, 0, F(1000.0f), &r);
            }
        }
        if ((R(ctl) & 0x1000u) && LE(R(self + 0xB4u), F(-130.0f))) {
            WB(self + 4u, 3);
            link = R(self + 0x20u);
            if (link != 0) WB(link + 4u, 3);
        }
        break;
    default:
        break;
    }
    return 0;
}
