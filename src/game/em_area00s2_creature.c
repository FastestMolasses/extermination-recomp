/* AREA00 sub 2's creature (owner 0x825D70's record, group 0x827EF0): the
 * per-frame behaviour 0014D260, its set-up 0014D3B0 (state 0), the alive
 * state 0014D470 (state 1), the scripted state 0014E370 (state 2), the end
 * 0014E640 (state 3), the hit tally 0014E9C0, and the exported entries of
 * every translation. From the decomp C (src/func_*.c, byte-matched), checked
 * against the original instructions by
 * tools/test_area00s2_creature_reference.py. The node is `self`; `ctl` =
 * self + 0x1F0 is its control block. */
#include "em_area00s2_creature_local.h"

/* 0014D260(self): nothing while the byte 0x810803 is 1. The state byte +4
 * picks the handler (3, the end, ends the frame there; a state above 3 runs
 * none); then the countdowns ctl+0x5A (byte), +0x56 (halfword), +0x5B,
 * +0x5E (bytes), +0x52, +0x50 (halfwords) step toward 0, and 001F91C0
 * runs. */
uint32_t a0s2_0014D260(A0S2 *o, uint32_t self, uint32_t sp)
{
    const uint32_t ctl = self + 0x1F0u;
    int32_t n;
    if (B(FLAG803) == 1) return 0;
    switch (B(self + 4u)) {
    case 3:
        SIB(a0s2_0014E640(o, self));
        return 0;
    case 2:
        SIB(a0s2_0014E370(o, self, ctl, sp - 0x30u));
        break;
    case 1:
        SIB(a0s2_0014D470(o, self, ctl, sp - 0x30u));
        break;
    case 0:
        SIB(a0s2_0014D3B0(o, self, ctl));
        break;
    default:
        break;
    }
    if ((n = SB(ctl + 0x5Au)) != 0) WB(ctl + 0x5Au, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x56u)) != 0) WH(ctl + 0x56u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x5Bu)) != 0) WB(ctl + 0x5Bu, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x5Eu)) != 0) WB(ctl + 0x5Eu, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x52u)) != 0) WH(ctl + 0x52u, (uint32_t)(n - 1));
    if ((n = SH(ctl + 0x50u)) != 0) WH(ctl + 0x50u, (uint32_t)(n - 1));
    C(001F91C0, self);
    return 0;
}

/* 0014D3B0: state 0, the set-up. The state byte steps on; header 1, +0xD =
 * 0x80, ctl+0x20 = ctl+0x40 = 1.0, the record 0x275410 at +0x30, +0x5D =
 * 0x41, +0x58 from 0x28A6FC, +0x34 = 2000 when 0x81070A is set, else 1200;
 * 001B10B0(self, 0x9D, 0x9C), and when it returns 0: 001C63E0(self, 0),
 * the effect 001EFE00(2, self) into +0x20 and the move byte +5 = 4. */
uint32_t a0s2_0014D3B0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t v;
    const uint32_t state = B(self + 4u) + 1u;
    WB(self + 4u, state);
    WB(self, 1);
    WB(self + 0xDu, 0x80);
    W(ctl + 0x20u, F(1.0f));
    W(ctl + 0x40u, F(1.0f));
    W(self + 0x30u, 0x275410u);
    WB(self + 0x5Du, 0x41);
    v = R(0x28A6FCu);
    W(self + 0x58u, v);
    if (B(0x81070Au) != 0) WH(self + 0x34u, 0x7D0);
    else WH(self + 0x34u, 0x4B0);
    C(001B10B0, self, 0x9D, 0x9C, &r);
    if (r == 0) {
        C(001C63E0, self, 0);
        C(001EFE00, 2, self, &r);
        W(self + 0x20u, (uint32_t)r);
        WB(self + 5u, 4);
    }
    return 0;
}

/* The tail 0014D470 and 0014E370 share: the tilt 0014E670, the scale pulse
 * 0014E890, ctl+0x28 into the float +0x70 of the record at +0x138. */
static uint32_t pulse_and_tilt(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    uint32_t v, p;
    SIB(a0s2_0014E670(o, self, ctl));
    SIB(a0s2_0014E890(o, self, ctl, sp - 0x30u));
    v = R(ctl + 0x28u);
    p = R(self + 0x138u);
    W(p + 0x70u, v);
    return 0;
}

/* The clock and the closing calls of 0014D470 / 0014E370: the animation
 * clock 001C64F0 at the rate ctl+0x20 into ctl+0, 001C68C0, 001B17A0 and the
 * +0x4C method. */
static uint32_t clock_and_close(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    const uint32_t rate = R(ctl + 0x20u);
    C(001C64F0, self, rate, &r);
    W(ctl, (uint32_t)r);
    C(001C68C0, self);
    C(001B17A0, self, &r);
    (void)a0s2_method(o, self);
    return 0;
}

/* 0014D470: state 1. The bearing 0014E7A0; the move byte +5 picks 0014D5F0
 * (0), 0014D7C0 (1), 0014DC30 (2), 0014E050 (3) or 0014E2C0 (4); the header
 * is 2 while ctl+0x52 runs, else 1; with ctl+0x58 set bit 0x80 of +0x5D and
 * 001B4810, else that bit cleared; the tilt and pulse, the hit tally
 * 0014E9C0, then the clock and the closing calls. */
uint32_t a0s2_0014D470(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    uint32_t v;
    SIB(a0s2_0014E7A0(o, self, ctl));
    switch (B(self + 5u)) {
    case 4: SIB(a0s2_0014E2C0(o, self, ctl)); break;
    case 3: SIB(a0s2_0014E050(o, self, ctl)); break;
    case 2: SIB(a0s2_0014DC30(o, self, ctl)); break;
    case 1: SIB(a0s2_0014D7C0(o, self, ctl)); break;
    case 0: SIB(a0s2_0014D5F0(o, self, ctl)); break;
    default: break;
    }
    WB(self, SH(ctl + 0x52u) != 0 ? 2u : 1u);
    if (SB(ctl + 0x58u) != 0) {
        v = B(self + 0x5Du);
        WB(self + 0x5Du, v | 0x80u);
        C(001B4810, self);
    } else {
        v = B(self + 0x5Du);
        WB(self + 0x5Du, v & 0x7Fu);
    }
    SIB(pulse_and_tilt(o, self, ctl, sp));
    SIB(a0s2_0014E9C0(o, self, ctl));
    SIB(clock_and_close(o, self, ctl));
    return 0;
}

/* 0014E370: state 2. The move byte +5 picks 0014E420 (0) or 0014E4F0 (1);
 * then the tilt and pulse, the clock and the closing calls. */
uint32_t a0s2_0014E370(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp)
{
    switch (B(self + 5u)) {
    case 1: SIB(a0s2_0014E4F0(o, self, ctl)); break;
    case 0: SIB(a0s2_0014E420(o, self, ctl)); break;
    default: break;
    }
    SIB(pulse_and_tilt(o, self, ctl, sp));
    SIB(clock_and_close(o, self, ctl));
    return 0;
}

/* 0014E640: state 3, the end: 001B1190 of the byte +0x9A, then 001AFC10
 * frees the node. */
uint32_t a0s2_0014E640(A0S2 *o, uint32_t self)
{
    const uint32_t id = B(self + 0x9Au);
    C(001B1190, (int32_t)id);
    C(001AFC10, self);
    return 0;
}

/* 0014E9C0: the hit tally. Nothing without a hit (+0x36). While ctl+0x52
 * runs the hit is dropped (header 1). Else header 2, ctl+0x52 = 255, the
 * damage d = hit & 0xFFF (five times with bit 0x8000) adds to ctl+0x54 (or
 * starts it when ctl+0x5A is 0), ctl+0x5A = 25; bit 0x4000 with ctl+0x5B
 * clear arms ctl+0x5B = 60 and the effect 001EFE00(0x80000027, self). When
 * d reaches the count +0x34: +0x34 = 0, state 2 move 1, 001B4CF0, result 1.
 * Else +0x34 loses d; a 0x8000 hit gives state 2 move 0 (ctl+0x54 cleared,
 * result 1); ctl+0x54 at 25 or a 0x2000 hit clears ctl+0x54, sets ctl+0x5C
 * and plays 0x866; ctl+0x52 = 30 (bits 0x5000), 80 (0x2000) or 0; header
 * 1, the hit and ctl+0x50 cleared; with ctl+0x5D set ctl+0x20 gains 0.1
 * while at most 0.9; result 0. */
int32_t a0s2_0014E9C0(A0S2 *o, uint32_t self, uint32_t ctl)
{
    int32_t m, d, hp, r = 0;
    uint32_t v;
    if (SH(self + 0x36u) == 0) return 0;
    if (SH(ctl + 0x52u) != 0) {
        WB(self, 1);
        WH(self + 0x36u, 0);
        return 0;
    }
    WB(self, 2);
    WH(ctl + 0x52u, 0xFF);
    m = SH(self + 0x36u);
    d = (int16_t)(m & 0xFFF);
    if (m & 0x8000) d = (int16_t)(d * 5);
    if (SB(ctl + 0x5Au) != 0) WH(ctl + 0x54u, (uint32_t)(SH(ctl + 0x54u) + d));
    else WH(ctl + 0x54u, (uint32_t)d);
    WB(ctl + 0x5Au, 0x19);
    if (SB(ctl + 0x5Bu) == 0 && (SH(self + 0x36u) & 0x4000)) {
        WB(ctl + 0x5Bu, 0x3C);
        C(001EFE00, (int32_t)0x80000027u, self, &r);
    }
    hp = SH(self + 0x34u);
    if (!(d < hp)) {
        WH(self + 0x34u, 0);
        WB(self + 4u, 2);
        WB(self + 5u, 1);
        WB(self + 6u, 0);
        C(001B4CF0, self);
        return 1;
    }
    WH(self + 0x34u, (uint32_t)(hp - d));
    m = SH(self + 0x36u);
    if (m & 0x8000) {
        WB(self + 4u, 2);
        WB(self + 5u, 0);
        WB(self + 6u, 0);
        WH(ctl + 0x54u, 0);
        return 1;
    }
    if (SH(ctl + 0x54u) >= 0x19 || (m & 0x2000)) {
        WH(ctl + 0x54u, 0);
        WB(ctl + 0x5Cu, 1);
        C(001FBD50, self, 0x866, 0, F(300.0f), &r);
    }
    m = SH(self + 0x36u);
    if (m & 0x5000) WH(ctl + 0x52u, 0x1E);
    else if (m & 0x2000) WH(ctl + 0x52u, 0x50);
    else WH(ctl + 0x52u, 0);
    WB(self, 1);
    WH(self + 0x36u, 0);
    WH(ctl + 0x50u, 0);
    if (SB(ctl + 0x5Du) != 0) {
        v = R(ctl + 0x20u);
        if (LE(v, F(0.9f))) W(ctl + 0x20u, ADD(v, F(0.1f)));
    }
    return 0;
}

/* ---- exported entries ---------------------------------------------------- */
#define ENTRY(body) \
    A0S2 o; \
    if (a0s2_begin(&o, h, fault)) return -1; \
    (void)(body); \
    return a0s2_end(&o)

typedef EmArea00s2CreatureHooks Hooks;
typedef EmArea00s2CreatureFault Fault;

int em_area00s2_creature_0014D260(const Hooks *h, uint32_t a0, uint32_t sp, Fault *fault)
{ ENTRY(a0s2_0014D260(&o, a0, sp)); }
int em_area00s2_creature_0014D3B0(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014D3B0(&o, a0, a1)); }
int em_area00s2_creature_0014D470(const Hooks *h, uint32_t a0, uint32_t a1, uint32_t sp, Fault *fault)
{ ENTRY(a0s2_0014D470(&o, a0, a1, sp)); }
int em_area00s2_creature_0014D5F0(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014D5F0(&o, a0, a1)); }
int em_area00s2_creature_0014D7C0(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014D7C0(&o, a0, a1)); }
int em_area00s2_creature_0014DC30(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014DC30(&o, a0, a1)); }
int em_area00s2_creature_0014E050(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014E050(&o, a0, a1)); }
int em_area00s2_creature_0014E2C0(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014E2C0(&o, a0, a1)); }
int em_area00s2_creature_0014E370(const Hooks *h, uint32_t a0, uint32_t a1, uint32_t sp, Fault *fault)
{ ENTRY(a0s2_0014E370(&o, a0, a1, sp)); }
int em_area00s2_creature_0014E420(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014E420(&o, a0, a1)); }
int em_area00s2_creature_0014E4F0(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014E4F0(&o, a0, a1)); }
int em_area00s2_creature_0014E640(const Hooks *h, uint32_t a0, Fault *fault)
{ ENTRY(a0s2_0014E640(&o, a0)); }
int em_area00s2_creature_0014E670(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014E670(&o, a0, a1)); }
int em_area00s2_creature_0014E7A0(const Hooks *h, uint32_t a0, uint32_t a1, Fault *fault)
{ ENTRY(a0s2_0014E7A0(&o, a0, a1)); }
int em_area00s2_creature_0014E890(const Hooks *h, uint32_t a0, uint32_t a1, uint32_t sp, Fault *fault)
{ ENTRY(a0s2_0014E890(&o, a0, a1, sp)); }

int em_area00s2_creature_0014E9C0(const Hooks *h, uint32_t a0, uint32_t a1, int32_t *result, Fault *fault)
{
    A0S2 o;
    if (!result || a0s2_begin(&o, h, fault)) return -1;
    const int32_t r = a0s2_0014E9C0(&o, a0, a1);
    if (a0s2_end(&o)) return -1;
    *result = r;
    return 0;
}
