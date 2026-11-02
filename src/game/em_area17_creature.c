/* The AREA17 creature (AREA17 code group 0x8263C0 [0]): the per-frame
 * behaviour 00133E30, its states 0 (00133FD0), 1 (00134090), 2 (00136140)
 * and 3 (00136600), the shared per-frame pieces 00136630 / 001366E0, the
 * dying sequence 00136240 / 00136490, and the exported entries of every
 * translation. From the decomp C (src/func_*.c), checked against the
 * original instructions by tools/test_area17_creature_reference.py.
 * The node is `self`; `ctl` = self + 0x1F0 is its control block. */
#include "em_area17_creature_local.h"

/* 00133E30(self): the distance and heading to the player (0x810360) from
 * the node, flattened, into ctl+0x34 / ctl+0x38; the state byte +4 picks
 * the handler (3 ends the frame there); then the countdowns ctl+0x58,
 * +0x5E (bytes) and +0x52, +0x56, +0x50 (halfwords) step toward 0, and
 * 001F91C0, 001B0D80 run. */
uint32_t a17c_00133E30(A17C *o, uint32_t self, uint32_t sp)
{
    const uint32_t ctl = self + 0x1F0u;
    uint32_t v, x, z;
    int32_t n;
    C(00102948, V38A0, self + 0xB0u);
    C(00102948, V38B0, PLAYER_POS);
    W(0x700038B4u, 0);
    W(0x700038A4u, 0);
    C(001B15D0, V38A0, V38B0, &v);
    W(ctl + 0x34u, v);
    x = R(0x700038B0u);
    z = R(0x700038B8u);
    C(001B1240, V38A0, x, z, &v);
    W(ctl + 0x38u, v);
    switch (B(self + 4u)) {
    case 3:
        SIB(a17c_00136600(o, self));
        return 0;
    case 2:
        SIB(a17c_00136140(o, self, ctl));
        break;
    case 1:
        SIB(a17c_00134090(o, self, ctl, sp - 0x30u));
        break;
    case 0:
        SIB(a17c_00133FD0(o, self, ctl));
        break;
    default:
        break;
    }
    if ((n = SB(ctl + 0x58u)) != 0) WB(ctl + 0x58u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x5Eu)) != 0) WB(ctl + 0x5Eu, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x52u)) != 0) WH(ctl + 0x52u, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x56u)) != 0) WH(ctl + 0x56u, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x50u)) != 0) WH(ctl + 0x50u, (uint32_t)(n - 1));
    C(001F91C0, self);
    C(001B0D80, self);
    return 0;
}

/* 00133FD0: state 0, the set-up. The state byte steps to 1; header 1,
 * +0xD = 0x80, ctl+0x20 = 1.0, ctl+0x5B = 0x1F (every part intact), the
 * placement record 0x2753A0, +0x5D = 0x81, +0x58 from 0x28A704, hit points
 * +0x34 = 2000 when 0x81070A is set, else 1000; 001B10B0(self, 0x9C, 0x9B)
 * loads the model, and when it returns 0, 001C63E0(self, 0) and the move
 * byte +5 = 8 (the entrance). */
uint32_t a17c_00133FD0(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t state = B(self + 4u) + 1u;
    uint32_t v;
    WB(self + 4u, state);
    WB(self, 1);
    WB(self + 0xDu, 0x80);
    W(ctl + 0x20u, F(1.0f));
    WB(ctl + 0x5Bu, 0x1F);
    W(self + 0x30u, 0x2753A0u);
    WB(self + 0x5Du, 0x81);
    v = R(0x28A704u);
    W(self + 0x58u, v);
    if (B(0x81070Au) != 0) WH(self + 0x34u, 0x7D0);
    else WH(self + 0x34u, 0x3E8);
    C(001B10B0, self, 0x9C, 0x9B, &r);
    if (r == 0) {
        C(001C63E0, self, 0);
        WB(self + 5u, 8);
    }
    return 0;
}

/* 00134090: state 1, alive. ctl+0x5C = 0 while the player is within 94 of
 * the point (340, 270), else 1; the move byte +5 picks the move (7 has
 * none); the header is 3 while ctl+0x52 runs, else 1; at the clip event 9
 * one of the ten cries 0x841..0x84A; then the parts, damage, motion and
 * arena pieces, the animation clock, the shared node steps and the +0x4C
 * method; last 001DA6A0 of the word 0x275B44. */
uint32_t a17c_00134090(A17C *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    uint32_t x, y, z, w, v;
    int32_t r = 0;
    x = R(PLAYER_POS);
    y = R(PLAYER_POS + 4u);
    z = R(PLAYER_POS + 8u);
    w = R(PLAYER_POS + 0xCu);
    W(0x700038B0u, F(340.0f));
    W(0x700038A0u, x);
    W(0x700038A4u, y);
    W(0x700038A8u, z);
    W(0x700038ACu, w);
    W(0x700038B4u, 0);
    W(0x700038A4u, 0);
    W(0x700038B8u, F(270.0f));
    W(0x700038BCu, F(1.0f));
    C(001B15D0, V38A0, V38B0, &v);
    WB(ctl + 0x5Cu, LE(v, F(94.0f)) ? 0u : 1u);
    switch (B(self + 5u)) {
    case 0: SIB(a17c_00134300(o, self, ctl, sp - 0x30u)); break;
    case 1: SIB(a17c_00134500(o, self, ctl)); break;
    case 2: SIB(a17c_001348E0(o, self, ctl)); break;
    case 3: SIB(a17c_00134DC0(o, self, ctl)); break;
    case 4: SIB(a17c_00134FF0(o, self, ctl)); break;
    case 5: SIB(a17c_001352E0(o, self, ctl)); break;
    case 6: SIB(a17c_00135870(o, self, ctl)); break;
    case 8: SIB(a17c_00135D00(o, self, ctl)); break;
    default: break;
    }
    WB(self, SH(ctl + 0x52u) != 0 ? 3u : 1u);
    if (B(ctl) == 9) {
        C(00122BB8, &r);
        C(001FBD50, self, a17c_sra((uint32_t)r, 16) % 10 + 0x841, 0, F(450.0f), &r);
    }
    SIB(a17c_00136CB0(o, self, ctl));
    SIB(a17c_001368D0(o, self, ctl));
    SIB(a17c_00136630(o, self, ctl));
    SIB(a17c_001366E0(o, self, ctl));
    v = R(ctl + 0x20u);
    C(001C64F0, self, v, &r);
    W(ctl, (uint32_t)r);
    C(00131ED0, self);
    C(001C68C0, self);
    C(001B17A0, self, &r);
    if (a17c_method(o, self)) return 0;
    v = R(0x275B44u);
    C(001DA6A0, v);
    return 0;
}

/* 00136140: state 2, dying. The move byte +5 picks the fall (0) or the
 * end (1); the cry at clip event 9 (300); then 001B5360, the motion
 * pieces, the animation clock at rate 1.0 and the shared node steps. */
uint32_t a17c_00136140(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    switch (B(self + 5u)) {
    case 1: SIB(a17c_00136490(o, self, ctl)); break;
    case 0: SIB(a17c_00136240(o, self, ctl)); break;
    default: break;
    }
    if (B(ctl) == 9) {
        C(00122BB8, &r);
        C(001FBD50, self, a17c_sra((uint32_t)r, 16) % 10 + 0x841, 0, F(300.0f), &r);
    }
    C(001B5360, self);
    SIB(a17c_00136630(o, self, ctl));
    SIB(a17c_001366E0(o, self, ctl));
    C(001C64F0, self, F(1.0f), &r);
    W(ctl, (uint32_t)r);
    C(00131ED0, self);
    C(001C68C0, self);
    C(001B17A0, self, &r);
    (void)a17c_method(o, self);
    return 0;
}

/* 00136240: the fall. Step 0: with no hit points (+0x34) and neither
 * kind bit 0xA000 in +0x36, straight on to move 1; else the step
 * advances, ctl+0x20 = 1.0, the heading +0xC4 turns to the player, and
 * clip 0x1B (kind bit 0x8000: ctl+0x2C = -1, cry 0x856) or 0x1C (cry
 * 0x854 or 0x855). Both steps then: in clip 0x1B before frame 130 a backward
 * slide (ctl+0x2C < 0) gets the drag 0.03, a forward one stops; at the
 * clip's end the creature gets up when it has hit points left (state 1,
 * 120 frames of grace, cry 0x84C one time in 16), else move 1. */
uint32_t a17c_00136240(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, kind;
    uint32_t x, z, v;
    switch (B(self + 6u)) {
    case 0:
        if (SH(self + 0x34u) == 0 && (SH(self + 0x36u) & 0xA000) == 0) {
            WB(self + 5u, B(self + 5u) + 1u);
            return 0;
        }
        WB(self + 6u, B(self + 6u) + 1u);
        W(ctl + 0x20u, F(1.0f));
        x = R(PLAYER_POS);
        z = R(PLAYER_POS + 8u);
        C(001B1240, self + 0xB0u, x, z, &v);
        W(self + 0xC4u, v);
        W(ctl + 0x30u, 0);
        W(ctl + 0x2Cu, 0);
        if (SH(self + 0x36u) & 0x8000) {
            W(ctl + 0x2Cu, F(-1.0f));
            C(001C67E0, self, 0x1B, F(5.0f), 0);
            C(001FBD50, self, 0x856, 0, F(450.0f), &r);
        } else {
            C(001C67E0, self, 0x1C, F(5.0f), 0);
            C(00122BB8, &r);
            C(001FBD50, self, (r & 1) + 0x854, 0, F(450.0f), &r);
        }
        /* fall through */
    case 1:
        kind = SH(self + 0x2Cu);
        if ((kind & 0x8000) == 0 && kind == 0x1B && LE(R(self + 0x3Cu), F(130.0f))) {
            const uint32_t c = R(ctl + 0x2Cu);
            if (!EQ(c, 0)) {
                if (LE(c, 0)) {
                    W(ctl + 0x30u, 0x3CF5C28Fu);
                } else {
                    W(ctl + 0x2Cu, 0);
                    W(ctl + 0x30u, 0);
                }
            }
        }
        if (R(ctl) & 0x1000) {
            if (SH(self + 0x34u) != 0) {
                WB(self + 4u, 1);
                WB(self + 5u, 0);
                WB(self + 6u, 0);
                WB(self, 1);
                WB(ctl + 0x5Du, 0);
                WH(self + 0x36u, 0);
                WH(ctl + 0x52u, 0x78);
                WH(ctl + 0x50u, 0);
                WH(ctl + 0x56u, 0x12C);
                W(ctl + 0x2Cu, 0);
                C(00122BB8, &r);
                if ((a17c_sra((uint32_t)r, 17) & 0xF) == 0) C(001FBD50, self, 0x84C, 0, F(450.0f), &r);
            } else {
                WB(self + 5u, 1);
                WB(self + 6u, 0);
                W(ctl + 0x30u, 0);
                W(ctl + 0x2Cu, 0);
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

/* 00136490: the end. Step 0: ctl+0x20 = 1.0 and the arena byte 0x810805
 * = 8; step 1, once that byte is 9: the body is placed at (340, 270)
 * facing 0, clip 0x1D, the effect 001EFE00(0x80000046) kept in +0x24 and
 * the cries 0x857, 0x858; step 2 waits for the clip's end; step 3, once
 * 0x28A9A2 is set and 0x28A9A0 is 2: state 3 and the effect's state 3. */
uint32_t a17c_00136490(A17C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t step = B(self + 6u);
    uint32_t e;
    switch (step) {
    case 3:
        if (SB(0x28A9A2u) == 0 || SH(0x28A9A0u) != 2) break;
        WB(self + 4u, 3);
        e = R(self + 0x24u);
        if (e) {
            WB(e + 4u, 3);
            W(self + 0x24u, 0);
        }
        break;
    case 2:
        if (R(ctl) & 0x1000) WB(self + 6u, step + 1u);
        break;
    case 1:
        if (B(0x810805u) != 9) break;
        WB(self + 6u, step + 1u);
        W(self + 0xC4u, 0);
        W(self + 0xB0u, F(340.0f));
        W(self + 0xB8u, F(270.0f));
        W(ctl + 0x30u, 0);
        W(ctl + 0x2Cu, 0);
        C(001C67E0, self, 0x1D, F(5.0f), 0);
        C(001EFE00, (int32_t)0x80000046u, self, &r);
        W(self + 0x24u, (uint32_t)r);
        C(001FBD50, self, 0x857, 0, F(450.0f), &r);
        C(001FBD50, self, 0x858, 0, F(450.0f), &r);
        break;
    case 0:
        WB(self + 6u, step + 1u);
        W(ctl + 0x20u, F(1.0f));
        WB(0x810805u, 8);
        break;
    default:
        break;
    }
    return 0;
}

/* 00136600: state 3, gone: 001B1190 of the slot byte +0x9A, then the node
 * is freed (001AFC10). */
uint32_t a17c_00136600(A17C *o, uint32_t self)
{
    C(001B1190, (int32_t)B(self + 0x9Au));
    C(001AFC10, self);
    return 0;
}

/* 00136630: unless ctl+0x59 is set, the airborne step: the fall speed
 * ctl+0x28 loses 0.1, the ground speed ctl+0x2C gains ctl+0x30, the
 * height +0xB4 takes the fall speed and x / z the ground speed along the
 * heading +0xC4 (sin, cos). */
uint32_t a17c_00136630(A17C *o, uint32_t self, uint32_t ctl)
{
    uint32_t a, b, s;
    if (SB(ctl + 0x59u) != 0) return 0;
    a = R(ctl + 0x28u);
    W(ctl + 0x28u, ADD(a, 0xBDCCCCCDu));
    a = R(ctl + 0x30u);
    b = R(ctl + 0x2Cu);
    W(ctl + 0x2Cu, ADD(b, a));
    a = R(ctl + 0x28u);
    b = R(self + 0xB4u);
    W(self + 0xB4u, ADD(b, a));
    a = R(self + 0xC4u);
    C(0011E2A8, a, &s);
    a = R(ctl + 0x2Cu);
    b = R(self + 0xB0u);
    W(self + 0xB0u, ADD(b, MUL(a, s)));
    a = R(self + 0xC4u);
    C(0011DE90, a, &s);
    a = R(ctl + 0x2Cu);
    b = R(self + 0xB8u);
    W(self + 0xB8u, ADD(b, MUL(a, s)));
    return 0;
}

/* 001366E0: the arena wall and the floor. ctl+0x5A = 0; outside 62 of
 * the arena's centre (340, 270), flattened, the node is put back on the
 * circle of radius 62 toward it (bit 1); while falling (ctl+0x28 < 0) a
 * hit of 001B3440(self, self + 0xB0, (0, -15, 0, 1), 0.7853982) stops
 * the fall (bit 2). */
uint32_t a17c_001366E0(A17C *o, uint32_t self, uint32_t ctl)
{
    uint32_t v, x, z;
    int32_t r = 0;
    WB(ctl + 0x5Au, 0);
    C(00102948, V38A0, self + 0xB0u);
    W(0x700038A4u, 0);
    W(0x700038B0u, F(340.0f));
    W(0x700038B4u, 0);
    W(0x700038B8u, F(270.0f));
    W(0x700038BCu, F(1.0f));
    C(001B15D0, V38A0, V38B0, &v);
    if (!LE(v, F(62.0f))) {
        WB(ctl + 0x5Au, (uint32_t)(SB(ctl + 0x5Au) | 1));
        x = R(0x700038A0u);
        z = R(0x700038A8u);
        C(001B1240, V38B0, x, z, &v);
        W(0x70003A20u, v);
        C(001029C0, V36A0);
        v = R(0x70003A20u);
        C(00102BB0, V36A0, V36A0, v);
        W(0x700038A0u, 0);
        W(0x700038A4u, 0);
        W(0x700038A8u, F(62.0f));
        W(0x700038ACu, F(1.0f));
        C(001026A0, V38A0, V36A0, V38A0);
        C(001028B8, V38A0, V38B0, V38A0);
        W(self + 0xB0u, R(0x700038A0u));
        W(self + 0xB8u, R(0x700038A8u));
    }
    if (LT(R(ctl + 0x28u), 0)) {
        W(0x700038A0u, 0);
        W(0x700038A4u, F(-15.0f));
        W(0x700038A8u, 0);
        W(0x700038ACu, F(1.0f));
        C(001B3440, self, self + 0xB0u, V38A0, 0x3F490FDBu, &r);
        if (r != 0) {
            W(ctl + 0x28u, 0);
            WB(ctl + 0x5Au, (uint32_t)(SB(ctl + 0x5Au) | 2));
        }
    }
    return 0;
}

/* ---- exported entries ---------------------------------------------------- */
#define ENTRY(body) \
    A17C o; \
    if (a17c_begin(&o, h, fault)) return -1; \
    (void)(body); \
    return a17c_end(&o)

int em_area17_creature_00133E30(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t sp, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00133E30(&o, a0, sp)); }
int em_area17_creature_00133FD0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00133FD0(&o, a0, a1)); }
int em_area17_creature_00134090(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00134090(&o, a0, a1, sp)); }
int em_area17_creature_00134300(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00134300(&o, a0, a1, sp)); }
int em_area17_creature_00134500(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00134500(&o, a0, a1)); }
int em_area17_creature_001348E0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001348E0(&o, a0, a1)); }
int em_area17_creature_00134DC0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00134DC0(&o, a0, a1)); }
int em_area17_creature_00134FF0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00134FF0(&o, a0, a1)); }
int em_area17_creature_001352E0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001352E0(&o, a0, a1)); }
int em_area17_creature_00135870(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00135870(&o, a0, a1)); }
int em_area17_creature_00135D00(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00135D00(&o, a0, a1)); }
int em_area17_creature_00136140(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136140(&o, a0, a1)); }
int em_area17_creature_00136240(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136240(&o, a0, a1)); }
int em_area17_creature_00136490(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136490(&o, a0, a1)); }
int em_area17_creature_00136600(const EmArea17CreatureHooks *h, uint32_t a0, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136600(&o, a0)); }
int em_area17_creature_00136630(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136630(&o, a0, a1)); }
int em_area17_creature_001366E0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001366E0(&o, a0, a1)); }
int em_area17_creature_00136C40(const EmArea17CreatureHooks *h, int32_t a0, uint32_t a1, uint32_t a2, float a3, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136C40(&o, a0, a1, a2, a17c_bits(a3))); }
int em_area17_creature_00136CB0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136CB0(&o, a0, a1)); }
int em_area17_creature_00136F20(const EmArea17CreatureHooks *h, uint32_t a0, int32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_00136F20(&o, a0, a1)); }
int em_area17_creature_001370C0(const EmArea17CreatureHooks *h, int32_t a0, uint32_t a1, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001370C0(&o, a0, a1)); }
int em_area17_creature_001373B0(const EmArea17CreatureHooks *h, uint32_t a0, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001373B0(&o, a0)); }
int em_area17_creature_001E00F0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t a2, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001E00F0(&o, a0, a1, a2)); }
int em_area17_creature_001E04E0(const EmArea17CreatureHooks *h, uint32_t a0, int32_t a1, float a2, uint32_t sp, EmArea17CreatureFault *fault)
{ ENTRY(a17c_001E04E0(&o, a0, a1, a17c_bits(a2), sp)); }

int em_area17_creature_001368D0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea17CreatureFault *fault)
{
    A17C o;
    if (!result || a17c_begin(&o, h, fault)) return -1;
    const int32_t r = a17c_001368D0(&o, a0, a1);
    if (a17c_end(&o)) return -1;
    *result = r;
    return 0;
}

int em_area17_creature_001E0240(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t sp, int32_t *result, EmArea17CreatureFault *fault)
{
    A17C o;
    if (!result || a17c_begin(&o, h, fault)) return -1;
    const int32_t r = a17c_001E0240(&o, a0, a1, a2, a3, sp);
    if (a17c_end(&o)) return -1;
    *result = r;
    return 0;
}
