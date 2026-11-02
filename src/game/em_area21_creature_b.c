/* AREA21's creature (AREA21 code group 0x82A540 [0]): the per-frame
 * behaviour 0014EC00, its set-up 0014ED10 (state 0), the alive states
 * 0014EDD0 (state 1) and 00150190 (state 2), the end 00150820 (state 3),
 * the bone clear 00150850, the hit tally 001513B0, and the exported entries
 * of every translation. From the decomp C (src/func_*.c), checked against
 * the original instructions by tools/test_area21_creature_b_reference.py.
 * The node is `self`; `ctl` = self + 0x1F0 is its control block. */
#include "em_area21_creature_b_local.h"

/* 0014EC00(self): the state byte +4 picks the handler (3, the end, ends
 * the frame there); then the countdowns ctl+0x61 (byte), +0x54 (halfword),
 * +0x62 and +0x60 (bytes) step toward 0, and 001F91C0 runs. */
uint32_t a21b_0014EC00(A21B *o, uint32_t self)
{
    const uint32_t ctl = self + 0x1F0u;
    int32_t n;
    switch (B(self + 4u)) {
    case 3:
        SIB(a21b_00150820(o, self));
        return 0;
    case 2:
        SIB(a21b_00150190(o, self, ctl));
        break;
    case 1:
        SIB(a21b_0014EDD0(o, self, ctl));
        break;
    case 0:
        SIB(a21b_0014ED10(o, self, ctl));
        break;
    default:
        break;
    }
    if ((n = SB(ctl + 0x61u)) != 0) WB(ctl + 0x61u, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x54u)) != 0) WH(ctl + 0x54u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x62u)) != 0) WB(ctl + 0x62u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x60u)) != 0) WB(ctl + 0x60u, (uint32_t)(n - 1));
    C(001F91C0, self);
    return 0;
}

/* 0014ED10: state 0, the set-up. The state byte steps on; header 1,
 * ctl+0x20 = 1.0, the placement record 0x275420 at +0x30, +0x5D = 0x81,
 * +0x58 from 0x28A6FC, hit points +0x34 = 1500 when 0x81070A is set, else
 * 1000; 001B10B0(self, 0x9D, 0x9C) loads the model, and when it returns 0,
 * 001C63E0(self, 0), the effect 001EFE00(6, self) into +0x20, +0xC0..+0xC8
 * cleared and +0xCC = 1.0. */
uint32_t a21b_0014ED10(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    const uint32_t state = B(self + 4u) + 1u;
    WB(self + 4u, state);
    WB(self, 1);
    W(ctl + 0x20u, F(1.0f));
    W(self + 0x30u, 0x275420u);
    WB(self + 0x5Du, 0x81);
    v = R(0x28A6FCu);
    W(self + 0x58u, v);
    if (B(0x81070Au) != 0) WH(self + 0x34u, 0x5DC);
    else WH(self + 0x34u, 0x3E8);
    C(001B10B0, self, 0x9D, 0x9C, &r);
    if (r == 0) {
        C(001C63E0, self, 0);
        C(001EFE00, 6, self, &r);
        W(self + 0x20u, (uint32_t)r);
        W(self + 0xC8u, 0);
        W(self + 0xC4u, 0);
        W(self + 0xC0u, 0);
        W(self + 0xCCu, F(1.0f));
    }
    return 0;
}

/* 0014EDD0: state 1. The move byte +5 picks 0014EEF0 (0), 0014F390 (1) or
 * 0014FBC0 (2); then 001B4810, the motion 00150900, the clamps 001509A0 and
 * the hit tally 001513B0; the header is 2 at or below the height -100
 * (+0xB4), else 1; the animation clock at the rate ctl+0x20 into ctl+0,
 * the bone clear 00150850, 001C68C0, 001B17A0 and the +0x4C method. */
uint32_t a21b_0014EDD0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    switch (B(self + 5u)) {
    case 2: SIB(a21b_0014FBC0(o, self, ctl)); break;
    case 1: SIB(a21b_0014F390(o, self, ctl)); break;
    case 0: SIB(a21b_0014EEF0(o, self, ctl)); break;
    default: break;
    }
    C(001B4810, self);
    SIB(a21b_00150900(o, self, ctl));
    SIB(a21b_001509A0(o, self, ctl));
    SIB(a21b_001513B0(o, self, ctl));
    v = R(self + 0xB4u);
    WB(self, LE(v, F(-100.0f)) ? 2u : 1u);
    v = R(ctl + 0x20u);
    C(001C64F0, self, v, &r);
    W(ctl, (uint32_t)r);
    SIB(a21b_00150850(o, self));
    C(001C68C0, self);
    C(001B17A0, self, &r);
    (void)a21b_method(o, self);
    return 0;
}

/* 00150190: state 2. The move byte +5 picks 00150260 then the hit tally
 * 001513B0 (0), 001504D0 (1) or 00150600 (2); then the motion 00150900,
 * the animation clock at the rate ctl+0x20 into ctl+0, the bone clear
 * 00150850, 001C68C0, 001B17A0 and the +0x4C method. */
uint32_t a21b_00150190(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    switch (B(self + 5u)) {
    case 2: SIB(a21b_00150600(o, self, ctl)); break;
    case 1: SIB(a21b_001504D0(o, self, ctl)); break;
    case 0:
        SIB(a21b_00150260(o, self, ctl));
        SIB(a21b_001513B0(o, self, ctl));
        break;
    default: break;
    }
    SIB(a21b_00150900(o, self, ctl));
    v = R(ctl + 0x20u);
    C(001C64F0, self, v, &r);
    W(ctl, (uint32_t)r);
    SIB(a21b_00150850(o, self));
    C(001C68C0, self);
    C(001B17A0, self, &r);
    (void)a21b_method(o, self);
    return 0;
}

/* 00150820: state 3, the end: 001B1190 of the byte +0x9A, then 001AFC10
 * frees the node. */
uint32_t a21b_00150820(A21B *o, uint32_t self)
{
    const uint32_t id = B(self + 0x9Au);
    C(001B1190, (int32_t)id);
    C(001AFC10, self);
    return 0;
}

/* 00150850: unless the move byte +5 is 0x63, the first six words of bone
 * 0's and bone 1's records are cleared (the table pointer and the record
 * pointer are fetched again for every word). */
uint32_t a21b_00150850(A21B *o, uint32_t self)
{
    uint32_t bone, i;
    if (B(self + 5u) == 0x63) return 0;
    for (bone = 0; bone < 2; bone++) {
        for (i = 0; i < 6; i++) {
            const uint32_t table = R(BONE_TABLE);
            const uint32_t record = R(table + 4u * bone);
            W(record + 4u * i, 0);
        }
    }
    return 0;
}

/* 001513B0: the hit tally. Nothing without a hit (+0x36). While the
 * grace ctl+0x60 runs the hit is dropped (header 1). Else header 2,
 * ctl+0x60 = -1, the damage d = hit & 0xFFF (five times with bit 0x8000)
 * adds to ctl+0x50 (or starts it when ctl+0x61 is 0), ctl+0x61 = 25; bit
 * 0x4000 with ctl+0x62 clear arms ctl+0x62 = 60 and the effect
 * 001EFE00(0x80000027, self); the hit points +0x34 lose d (at 0 the
 * effect 001B4CF0); ctl+0x52 adds d. In state 1 with the hit points gone,
 * 25 or more damage at once, or a 0xA000 kind bit: state 2, move 0, the
 * tally cleared, grace 30, result 1. Else the grace is 10 (bits 0x5000),
 * 30 (0x2000) or 0, header 1, the hit cleared, result 0. */
int32_t a21b_001513B0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t hit, d, hp, r = 0;
    if (SH(self + 0x36u) == 0) return 0;
    if (SB(ctl + 0x60u) != 0) {
        WB(self, 1);
        WH(self + 0x36u, 0);
        return 0;
    }
    WB(self, 2);
    WB(ctl + 0x60u, 0xFF);
    hit = SH(self + 0x36u);
    d = (int16_t)(hit & 0xFFF);
    if (hit & 0x8000) d = (int16_t)(d * 5);
    if (SB(ctl + 0x61u) != 0) WH(ctl + 0x50u, (uint32_t)(SH(ctl + 0x50u) + d));
    else WH(ctl + 0x50u, (uint32_t)d);
    WB(ctl + 0x61u, 0x19);
    if (SB(ctl + 0x62u) == 0 && (SH(self + 0x36u) & 0x4000)) {
        WB(ctl + 0x62u, 0x3C);
        C(001EFE00, (int32_t)0x80000027u, self, &r);
    }
    hp = SH(self + 0x34u);
    if (!(d < hp)) {
        WH(self + 0x34u, 0);
        C(001B4CF0, self);
    } else {
        WH(self + 0x34u, (uint32_t)(hp - d));
    }
    WH(ctl + 0x52u, (uint32_t)(SH(ctl + 0x52u) + d));
    if (B(self + 4u) == 1) {
        if (SH(self + 0x34u) == 0 || SH(ctl + 0x50u) >= 0x19 || (SH(self + 0x36u) & 0xA000)) {
            WB(self + 4u, 2);
            WB(self + 5u, 0);
            WB(self + 6u, 0);
            WH(ctl + 0x50u, 0);
            WB(ctl + 0x60u, 0x1E);
            return 1;
        }
    }
    hit = SH(self + 0x36u);
    if (hit & 0x5000) WB(ctl + 0x60u, 0xA);
    else if (hit & 0x2000) WB(ctl + 0x60u, 0x1E);
    else WB(ctl + 0x60u, 0);
    WB(self, 1);
    WH(self + 0x36u, 0);
    return 0;
}

/* ---- exported entries ---------------------------------------------------- */
#define ENTRY(body) \
    A21B o; \
    if (a21b_begin(&o, h, fault)) return -1; \
    (void)(body); \
    return a21b_end(&o)

int em_area21_creature_b_0014EC00(const EmArea21CreatureBHooks *h, uint32_t a0, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_0014EC00(&o, a0)); }
int em_area21_creature_b_0014ED10(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_0014ED10(&o, a0, a1)); }
int em_area21_creature_b_0014EDD0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_0014EDD0(&o, a0, a1)); }
int em_area21_creature_b_0014EEF0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_0014EEF0(&o, a0, a1)); }
int em_area21_creature_b_0014F390(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_0014F390(&o, a0, a1)); }
int em_area21_creature_b_0014FBC0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_0014FBC0(&o, a0, a1)); }
int em_area21_creature_b_00150190(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150190(&o, a0, a1)); }
int em_area21_creature_b_00150260(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150260(&o, a0, a1)); }
int em_area21_creature_b_001504D0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_001504D0(&o, a0, a1)); }
int em_area21_creature_b_00150600(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150600(&o, a0, a1)); }
int em_area21_creature_b_00150820(const EmArea21CreatureBHooks *h, uint32_t a0, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150820(&o, a0)); }
int em_area21_creature_b_00150850(const EmArea21CreatureBHooks *h, uint32_t a0, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150850(&o, a0)); }
int em_area21_creature_b_00150900(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150900(&o, a0, a1)); }
int em_area21_creature_b_001509A0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_001509A0(&o, a0, a1)); }
int em_area21_creature_b_00150DA0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150DA0(&o, a0, a1)); }
int em_area21_creature_b_00150F40(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00150F40(&o, a0, a1)); }
int em_area21_creature_b_00151200(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault)
{ ENTRY(a21b_00151200(&o, a0, a1)); }

int em_area21_creature_b_001513B0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, int32_t *result,
                                  EmArea21CreatureBFault *fault)
{
    A21B o;
    if (!result || a21b_begin(&o, h, fault)) return -1;
    const int32_t r = a21b_001513B0(&o, a0, a1);
    if (a21b_end(&o)) return -1;
    *result = r;
    return 0;
}
