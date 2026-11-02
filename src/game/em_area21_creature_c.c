/* The AREA21 creature (AREA21 code group 0x82A5A0 [0]): the per-frame
 * behaviour 00151590, its states 0 (001516B0), 1 (00151770), 2 (00152CC0)
 * and 3 (00152E80), the shared per-frame pieces 00152EB0 / 00153180 /
 * 00153770, and the exported entries of every translation. From the decomp
 * C (src/func_*.c), checked against the original instructions by
 * tools/test_area21_creature_c_reference.py. The node is `self`; `ctl` =
 * self + 0x1F0 is its control block. */
#include "em_area21_creature_c_local.h"

/* 00151590(self): the state byte +4 picks the handler (3 ends the frame
 * there); then the countdowns ctl+0x51, +0x52 (bytes), +0x42 (halfword),
 * +0x50 (byte) and +0x44 (halfword) step toward 0, and 001F91C0 runs. */
uint32_t a21c_00151590(A21C *o, uint32_t self, uint32_t sp)
{
    const uint32_t ctl = self + 0x1F0u;
    int32_t n;
    switch (B(self + 4u)) {
    case 3:
        SIB(a21c_00152E80(o, self));
        return 0;
    case 2:
        SIB(a21c_00152CC0(o, self, ctl));
        break;
    case 1:
        SIB(a21c_00151770(o, self, ctl, sp - 0x30u));
        break;
    case 0:
        SIB(a21c_001516B0(o, self, ctl));
        break;
    default:
        break;
    }
    if ((n = SB(ctl + 0x51u)) != 0) WB(ctl + 0x51u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x52u)) != 0) WB(ctl + 0x52u, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x42u)) != 0) WH(ctl + 0x42u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x50u)) != 0) WB(ctl + 0x50u, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x44u)) != 0) WH(ctl + 0x44u, (uint32_t)(n - 1));
    C(001F91C0, self);
    return 0;
}

/* 001516B0: state 0, the set-up. The state byte steps on; header 1, +0xD =
 * 0x80, the placement record 0x275430 in +0x30, ctl+0x28 = 1.0, +0x5D =
 * 0x81, +0x58 from 0x28A708, hit points +0x34 = 3000 when 0x81070A is set,
 * else 2000; 001B10B0(self, 0xA0, 0x9F) loads the model, and when it
 * returns 0: 001C63E0(self, 0), the effect 001EFE00(9, self) kept in +0x20
 * and the move byte +5 = 9. */
uint32_t a21c_001516B0(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t state = B(self + 4u) + 1u;
    uint32_t v;
    WB(self + 4u, state);
    WB(self, 1);
    WB(self + 0xDu, 0x80);
    W(self + 0x30u, 0x275430u);
    W(ctl + 0x28u, F(1.0f));
    WB(self + 0x5Du, 0x81);
    v = R(0x28A708u);
    W(self + 0x58u, v);
    if (B(0x81070Au) != 0) WH(self + 0x34u, 0xBB8);
    else WH(self + 0x34u, 0x7D0);
    C(001B10B0, self, 0xA0, 0x9F, &r);
    if (r == 0) {
        C(001C63E0, self, 0);
        C(001EFE00, 9, self, &r);
        W(self + 0x20u, (uint32_t)r);
        WB(self + 5u, 9);
    }
    return 0;
}

/* 00151770: state 1, alive. ctl+0x58 = 1 while the word 0x810364 (the y of
 * the vector at 0x810360) is not below 75; the move byte +5 picks the move (6, 7, 8 have none); at
 * the clip events 9 / 8 the cries 0x883 / 0x884 (random) or 0x885; then
 * the damage step 00153770, 001B4810, the motion 00152F20, the bounds
 * 00153180, the animation clock at the rate ctl+0x28, the bone reset
 * 00152EB0, the shared node steps and the +0x4C method. */
uint32_t a21c_00151770(A21C *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    int32_t r = 0;
    uint32_t v, tag;
    v = R(0x810364u);
    WB(ctl + 0x58u, LT(v, F(75.0f)) ? 0u : 1u);
    switch (B(self + 5u)) {
    case 0: SIB(a21c_00151940(o, self, ctl)); break;
    case 1: SIB(a21c_00151E50(o, self, ctl)); break;
    case 2: SIB(a21c_00152040(o, self, ctl)); break;
    case 3: SIB(a21c_00152330(o, self, ctl)); break;
    case 4: SIB(a21c_00152660(o, self, ctl)); break;
    case 5: SIB(a21c_00152930(o, self, ctl)); break;
    case 9: SIB(a21c_00152B60(o, self, ctl)); break;
    default: break;
    }
    tag = B(ctl);
    if (tag == 9) {
        C0(00122BB8, &r);
        C(001FBD50, self, (a21c_sra((uint32_t)r, 13) & 1) + 0x883, 0, F(500.0f), &r);
    } else if (tag == 8) {
        C(001FBD50, self, 0x885, 0, F(500.0f), &r);
    }
    SIB(a21c_00153770(o, self, ctl));
    C(001B4810, self);
    SIB(a21c_00152F20(o, self, ctl, sp - 0x30u));
    SIB(a21c_00153180(o, self, ctl));
    v = R(ctl + 0x28u);
    C(001C64F0, self, v, &r);
    W(ctl, (uint32_t)r);
    SIB(a21c_00152EB0(o, self));
    C(001C68C0, self);
    C(001B17A0, self, &r);
    (void)a21c_method(o, self);
    return 0;
}

/* 00152CC0: state 2, the defeat, on the move byte +5. Step 0: the clock
 * rate ctl+0x28 = 1.0, the speeds ctl+0x30..0x3C cleared, clip 0x16 with
 * cry 0x893 (hit points left and kind bit 0x8000 in +0x36) or clip 0x17
 * with cry 0x894, and the +0x20 effect's byte +5 cleared. Step 1, at the
 * clip's end: back to state 1 with a random pause in ctl+0x50 when hit
 * points are left, else step 2, the arena byte 0x81080E = 2 and the
 * effect's state 3. Step 2: state 3 once 0x81080E is 3. Then always the
 * animation clock, 00152EB0, the shared node steps and the +0x4C method. */
uint32_t a21c_00152CC0(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t step = B(self + 5u);
    uint32_t p, v;
    switch (step) {
    case 2:
        if (B(0x81080Eu) == 3) WB(self + 4u, 3);
        break;
    case 1:
        if ((R(ctl) & 0x1000u) == 0) break;
        if (SH(self + 0x34u) != 0) {
            WB(self + 4u, 1);
            WB(self + 5u, 0);
            WB(self + 6u, 0);
            C0(00122BB8, &r);
            WB(ctl + 0x50u, (uint32_t)((a21c_sra((uint32_t)r, 13) & 0x3F) + 0x3C));
        } else {
            WB(self + 5u, step + 1u);
            WB(0x81080Eu, 2);
            p = R(self + 0x20u);
            if (p) WB(p + 4u, 3);
        }
        break;
    case 0:
        WB(self + 5u, step + 1u);
        W(ctl + 0x28u, F(1.0f));
        W(ctl + 0x34u, 0);
        W(ctl + 0x30u, 0);
        W(ctl + 0x3Cu, 0);
        W(ctl + 0x38u, 0);
        if (SH(self + 0x34u) != 0 && (SH(self + 0x36u) & 0x8000)) {
            C(001C67E0, self, 0x16, F(5.0f), 0);
            C(001FBD50, self, 0x893, 0, F(500.0f), &r);
        } else {
            C(001C67E0, self, 0x17, F(5.0f), 0);
            C(001FBD50, self, 0x894, 0, F(500.0f), &r);
        }
        p = R(self + 0x20u);
        if (p) WB(p + 5u, 0);
        break;
    default:
        break;
    }
    v = R(ctl + 0x28u);
    C(001C64F0, self, v, &r);
    W(ctl, (uint32_t)r);
    SIB(a21c_00152EB0(o, self));
    C(001C68C0, self);
    C(001B17A0, self, &r);
    (void)a21c_method(o, self);
    return 0;
}

/* 00152E80: state 3, gone: 001B1190 of the slot byte +0x9A, then the node
 * is freed (001AFC10). */
uint32_t a21c_00152E80(A21C *o, uint32_t self)
{
    C(001B1190, (int32_t)B(self + 0x9Au));
    C(001AFC10, self);
    return 0;
}

/* 00152EB0: unless the move byte +5 is 9 or 99, the first six words of the
 * record behind the first entry of the bone table (0x275B40) are cleared,
 * the pointers read again for each word. */
uint32_t a21c_00152EB0(A21C *o, uint32_t self)
{
    const uint32_t move = B(self + 5u);
    if (move == 9 || move == 99) return 0;
    for (uint32_t k = 0; k < 6u; k++) {
        const uint32_t table = R(BONE_TABLE);
        const uint32_t p = R(table);
        W(p + 4u * k, 0);
    }
    return 0;
}

/* One axis of 00153180: clamp the word at `at` into the limits `lo_word`
 * / `hi_word` (fires when above the first, or below the second), OR-ing
 * `lo_bit` / `hi_bit` into ctl+0x55. */
static void a21c_clamp(A21C *o, uint32_t ctl, uint32_t at, uint32_t lo_word, uint32_t hi_word,
                       uint32_t lo_bit, uint32_t hi_bit)
{
    const uint32_t x = R(at);
    if (!LE(x, R(lo_word))) {
        WB(ctl + 0x55u, (uint32_t)SB(ctl + 0x55u) | lo_bit);
        W(at, R(lo_word));
    } else if (LT(x, R(hi_word))) {
        WB(ctl + 0x55u, (uint32_t)SB(ctl + 0x55u) | hi_bit);
        W(at, R(hi_word));
    }
}

/* 00153180: the bounds. ctl+0x55 = 0; unless ctl+0x54 is set, x (+0xB0)
 * is held within the limits 0x246720 / 0x246724 (bits 1 / 2), z (+0xB8)
 * within 0x246728 / 0x24672C (bits 4 / 8), and the height +0xB4 is at
 * least 35. */
uint32_t a21c_00153180(A21C *o, uint32_t self, uint32_t ctl)
{
    WB(ctl + 0x55u, 0);
    if (SB(ctl + 0x54u) != 0) return 0;
    a21c_clamp(o, ctl, self + 0xB0u, 0x246720u, 0x246724u, 1, 2);
    a21c_clamp(o, ctl, self + 0xB8u, 0x246728u, 0x24672Cu, 4, 8);
    if (LT(R(self + 0xB4u), F(35.0f))) W(self + 0xB4u, F(35.0f));
    return 0;
}

/* 00153770: the damage step; returns 1 when the creature is knocked down.
 * Nothing without a hit (+0x36); with the pause ctl+0x50 running the hit
 * is dropped (header 1). Else header 2, ctl+0x50 = -1, and the step is the
 * hit's low 12 bits (five times with bit 0x8000); bit 0x2000 adds it to
 * (or with ctl+0x51 clear, sets) the stun total ctl+0x40 and ctl+0x51 =
 * the byte 0x275438; bit 0x4000 with ctl+0x52 clear: ctl+0x52 = 60 and the
 * effect 0x80000027. Hit points +0x34 at or below the step: they are
 * cleared, state 2, and 001B4CF0. Else they lose the step; a stun total
 * at the limit 0x27543A (outside move 1) or bit 0x8000 also means state
 * 2 (the total cleared); otherwise the pause ctl+0x50 by the hit's class
 * (0x5000: 30, 0x2000: 60, else 0), header 1 and the hit cleared. */
int32_t a21c_00153770(A21C *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, flags, step, left;
    if (SH(self + 0x36u) == 0) return 0;
    if (SB(ctl + 0x50u) != 0) {
        WB(self, 1);
        WH(self + 0x36u, 0);
        return 0;
    }
    WB(self, 2);
    WB(ctl + 0x50u, 0xFF);
    flags = SH(self + 0x36u);
    step = (int16_t)(flags & 0xFFF);
    if (flags & 0x8000) step = (int16_t)(uint16_t)((uint32_t)step * 5u);
    if (flags & 0x2000) {
        if (SB(ctl + 0x51u) != 0) WH(ctl + 0x40u, (uint32_t)(SH(ctl + 0x40u) + step));
        else WH(ctl + 0x40u, (uint32_t)step);
        WB(ctl + 0x51u, (uint32_t)SB(0x275438u));
    }
    if (SB(ctl + 0x52u) == 0 && (SH(self + 0x36u) & 0x4000)) {
        WB(ctl + 0x52u, 0x3C);
        C(001EFE00, (int32_t)0x80000027u, self, &r);
    }
    left = SH(self + 0x34u);
    if (!(step < left)) {
        WH(self + 0x34u, 0);
        WB(self + 4u, 2);
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        C(001B4CF0, self);
        return 1;
    }
    WH(self + 0x34u, (uint32_t)(left - step));
    int calm = B(self + 5u) == 1;
    if (!calm) {
        const int32_t total = SH(ctl + 0x40u);
        calm = total < SH(0x27543Au);
    }
    if (calm) {
        flags = SH(self + 0x36u);
        if ((flags & 0x8000) == 0) {
            if (flags & 0x5000) WB(ctl + 0x50u, 0x1E);
            else if (flags & 0x2000) WB(ctl + 0x50u, 0x3C);
            else WB(ctl + 0x50u, 0);
            WB(self, 1);
            WH(self + 0x36u, 0);
            return 0;
        }
    }
    WB(self + 4u, 2);
    WB(self + 5u, 0);
    WB(self + 6u, 0);
    WH(ctl + 0x40u, 0);
    return 1;
}

/* ---- exported entries ---------------------------------------------------- */
#define ENTRY(body) \
    A21C o; \
    if (a21c_begin(&o, h, fault)) return -1; \
    (void)(body); \
    return a21c_end(&o)

int em_area21_creature_c_00151590(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t sp, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00151590(&o, a0, sp)); }
int em_area21_creature_c_001516B0(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_001516B0(&o, a0, a1)); }
int em_area21_creature_c_00151770(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00151770(&o, a0, a1, sp)); }
int em_area21_creature_c_00151940(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00151940(&o, a0, a1)); }
int em_area21_creature_c_00151E50(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00151E50(&o, a0, a1)); }
int em_area21_creature_c_00152040(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152040(&o, a0, a1)); }
int em_area21_creature_c_00152330(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152330(&o, a0, a1)); }
int em_area21_creature_c_00152660(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152660(&o, a0, a1)); }
int em_area21_creature_c_00152930(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152930(&o, a0, a1)); }
int em_area21_creature_c_00152B60(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152B60(&o, a0, a1)); }
int em_area21_creature_c_00152CC0(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152CC0(&o, a0, a1)); }
int em_area21_creature_c_00152E80(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152E80(&o, a0)); }
int em_area21_creature_c_00152EB0(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152EB0(&o, a0)); }
int em_area21_creature_c_00152F20(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00152F20(&o, a0, a1, sp)); }
int em_area21_creature_c_00153180(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00153180(&o, a0, a1)); }
int em_area21_creature_c_00153290(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00153290(&o, a0)); }
int em_area21_creature_c_00153540(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault)
{ ENTRY(a21c_00153540(&o, a0)); }

int em_area21_creature_c_00153770(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea21CreatureCFault *fault)
{
    A21C o;
    if (!result || a21c_begin(&o, h, fault)) return -1;
    const int32_t r = a21c_00153770(&o, a0, a1);
    if (a21c_end(&o)) return -1;
    *result = r;
    return 0;
}
