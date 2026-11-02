/* The AREA21 creature (AREA21 code group 0x82A600 [0]): the per-frame
 * behaviour 00137830, its states 0 (00137920), 1 (00137A50), 2 (00137C80)
 * and 3 (00137E80), its damage step 001381D0, its two rig followers
 * (node behaviour 00137EB0), and the exported entries of every
 * translation in em_area21_boot*.c. From the decomp C (src/func_*.c;
 * 00137EB0's is body-correct, not byte-matched), checked against the
 * original instructions by tools/test_area21_boot_reference.py. The node
 * is `self`; `ctl` = self + 0x1F0 is its control block. */
#include "em_area21_boot_local.h"

/* 00137830(self): the state byte +4 picks the handler (3, the release,
 * ends the frame there); then the byte countdowns ctl+0x42, +0x41, +0x40
 * and +0x44 step toward 0. */
uint32_t a21b_00137830(A21B *o, uint32_t self)
{
    const uint32_t ctl = self + 0x1F0u;
    int32_t n;
    switch (B(self + 4u)) {
    case 0: SIB(a21b_00137920(o, self, ctl)); break;
    case 1: SIB(a21b_00137A50(o, self, ctl)); break;
    case 2: SIB(a21b_00137C80(o, self, ctl)); break;
    case 3: SIB(a21b_00137E80(o, self)); return 0;
    default: break;
    }
    if ((n = SB(ctl + 0x42u)) != 0) WB(ctl + 0x42u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x41u)) != 0) WB(ctl + 0x41u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x40u)) != 0) WB(ctl + 0x40u, (uint32_t)(n - 1));
    if ((n = SB(ctl + 0x44u)) != 0) WB(ctl + 0x44u, (uint32_t)(n - 1));
    return 0;
}

/* 00137920: state 0, the set-up. The state byte steps up; header 1, the
 * placement record 0x2753A8, the clock rate ctl+0x28 = 1.0, the spawn
 * countdown ctl+0x32 = 1000, +0x5D = 0x81, +0x58 from 0x28A714, hit
 * points +0x34 = 3000 when 0x81070A is set, else 2000; the model
 * 001B10B0(self, 0xA3, 0xA2), and when it returns 0: 001C63E0(self, 0),
 * the position (102.904, 35, 161.986, 1) and two class-8 rig followers
 * (behaviour 00137EB0, +0x20 = self; +3 / +0xD = 0 / 0 and 4 / 1), the
 * second only when the first was made. */
uint32_t a21b_00137920(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t p = 0;
    WB(self + 4u, B(self + 4u) + 1u);
    WB(self, 1);
    W(self + 0x30u, 0x2753A8u);
    W(ctl + 0x28u, 0x3F800000u);
    WH(ctl + 0x32u, 0x3E8);
    WB(self + 0x5Du, 0x81);
    W(self + 0x58u, R(0x28A714u));
    if (B(HARD) != 0) WH(self + 0x34u, 0xBB8);
    else WH(self + 0x34u, 0x7D0);
    C(001B10B0, self, 0xA3, 0xA2, &r);
    if (r != 0) return 0;
    C(001C63E0, self, 0);
    W(self + 0xB0u, 0x42CDCEF3u);
    W(self + 0xB4u, 0x420C0000u);
    W(self + 0xB8u, 0x4321FC71u);
    W(self + 0xBCu, 0x3F800000u);
    C(001AFA90, 8, &p);
    if (p == 0) return 0;
    WB(p + 3u, 0);
    WB(p + 0xDu, 0);
    W(p + 0x10u, 0x137EB0u);
    W(p + 0x20u, self);
    C(001AFA90, 8, &p);
    if (p == 0) return 0;
    WB(p + 3u, 4);
    WB(p + 0xDu, 1);
    W(p + 0x10u, 0x137EB0u);
    W(p + 0x20u, self);
    return 0;
}

/* 00137A50: state 1, alive. Move byte +5: 0 the clip 0, then 1; 1 at the
 * clip event 0x1000 the clip 1, then 2; 2 the bearing ctl+0x2C to the
 * player and the turn +0xC4 by pi/40 toward it, the side countdown ctl+4
 * (at 0 the side +0xD flips and a new count: 10 + 0..255 on side 1, 10 +
 * 0..63 on side 0), and the spawn countdown ctl+0x32 (at 0 every second
 * time 580 + 0..511, else 30 + 0..127, and a small creature 0014D1E0
 * between +0xB0 and +0xC0, kind 2). Then the header (2 while ctl+0x40
 * runs, else 1), 001B4810, 001B5360, the damage 001381D0, the clock ctl+0
 * = 001C64F0(self, ctl+0x28), 00131ED0, 001C68C0, 001B17A0 and the +0x4C
 * method. */
uint32_t a21b_00137A50(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0, c;
    uint32_t v, x, z, side;
    const uint32_t st = B(self + 5u);
    switch (st) {
    case 0:
        WB(self + 5u, st + 1u);
        C(001C67E0, self, 0, 0, 0);
        break;
    case 1:
        if (R(ctl) & 0x1000u) {
            WB(self + 5u, st + 1u);
            C(001C67E0, self, 1, 0, 0);
        }
        break;
    case 2:
        x = R(PLAYER_POS);
        z = R(PLAYER_POS + 8u);
        C(001B1240, self + 0xB0u, x, z, &v);
        W(ctl + 0x2Cu, v);
        x = R(self + 0xC4u);
        C(001B12B0, R(ctl + 0x2Cu), x, F(0.07853981852531433f), &v);
        W(self + 0xC4u, v);
        v = R(ctl + 4u);
        if (v != 0) {
            W(ctl + 4u, v - 1u);
        } else {
            side = (1u - B(self + 0xDu)) & 0xFFu;
            WB(self + 0xDu, side);
            C(00122BB8, &r);
            if (side != 0) W(ctl + 4u, ((uint32_t)r >> 0x12 & 0xFFu) + 0xAu);
            else W(ctl + 4u, ((uint32_t)r >> 0x13 & 0x3Fu) + 0xAu);
        }
        if ((c = SH(ctl + 0x32u)) != 0) {
            WH(ctl + 0x32u, (uint32_t)(c - 1));
        } else {
            c = (int8_t)(SB(ctl + 0x43u) + 1);
            WB(ctl + 0x43u, (uint32_t)c);
            if (c >= 2) {
                WB(ctl + 0x43u, 0);
                C(00122BB8, &r);
                WH(ctl + 0x32u, ((uint32_t)r >> 0xD & 0x1FFu) + 0x244u);
            } else {
                C(00122BB8, &r);
                WH(ctl + 0x32u, ((uint32_t)r >> 0x12 & 0x7Fu) + 0x1Eu);
            }
            SIB(a21b_0014D1E0(o, self + 0xB0u, self + 0xC0u, 2));
        }
        break;
    default:
        break;
    }
    WB(self, SB(ctl + 0x40u) != 0 ? 2u : 1u);
    C(001B4810, self);
    C(001B5360, self);
    SIB(a21b_001381D0(o, self, ctl));
    C(001C64F0, self, R(ctl + 0x28u), &r);
    W(ctl, (uint32_t)r);
    C(00131ED0, self);
    C(001C68C0, self);
    C(001B17A0, self, &r);
    METHOD(self);
    return 0;
}

/* 00137C80: state 2, hurt or dying. Move byte +5: 0 the side +0xD = 0,
 * the clock rate ctl+0x28 = 1.0; with hit points left the clip 2 (blend
 * 5) and the cry 0x89A, else 0x81080F = 1, the clip 4, the effect
 * 001EFE00(0x80000046) kept at +0x20 and the cries 0x89B, 0x89C (all at
 * 500); 1 at the clip event 0x1000: with hit points back to state 1
 * (header 1, steps 0, ctl+4 = 0, the guard ctl+0x40 = 60 + 0..63, the
 * flash ctl+0x44 = -16 after a 0x8000 hit, the hit word cleared), else
 * step 2; 2 when 0x28A9A2 is set and 0x28A9A0 is 2: state 3, and with a
 * +0x24 word the node at +0x20 gets state 3 and +0x20 is cleared. Then
 * 001B5360, the clock ctl+0 = 001C64F0(self, ctl+0x28), 00131ED0,
 * 001C68C0, 001B17A0 and the +0x4C method. */
uint32_t a21b_00137C80(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t r = 0;
    uint32_t st = B(self + 5u);
    switch (st) {
    case 0:
        WB(self + 5u, st + 1u);
        WB(self + 0xDu, 0);
        W(ctl + 0x28u, F(1.0f));
        if (SH(self + 0x34u) != 0) {
            C(001C67E0, self, 2, F(5.0f), 0);
            C(001FBD50, self, 0x89A, 0, F(500.0f), &r);
        } else {
            WB(0x81080Fu, 1);
            C(001C67E0, self, 4, F(5.0f), 0);
            C(001EFE00, (int32_t)0x80000046u, self, &r);
            W(self + 0x20u, (uint32_t)r);
            C(001FBD50, self, 0x89B, 0, F(500.0f), &r);
            C(001FBD50, self, 0x89C, 0, F(500.0f), &r);
        }
        break;
    case 1:
        if (R(ctl) & 0x1000u) {
            if (SH(self + 0x34u) != 0) {
                WB(self, 1);
                WB(self + 4u, 1);
                WB(self + 5u, 0);
                WB(self + 6u, 0);
                W(ctl + 4u, 0);
                C(00122BB8, &r);
                WB(ctl + 0x40u, ((uint32_t)r >> 13 & 0x3Fu) + 0x3Cu);
                if (SH(self + 0x36u) & 0x8000) WB(ctl + 0x44u, 0xF0);
                WH(self + 0x36u, 0);
            } else {
                WB(self + 5u, st + 1u);
            }
        }
        break;
    case 2:
        if (B(0x28A9A2u) != 0 && SH(0x28A9A0u) == 2) {
            WB(self + 4u, 3);
            if (R(self + 0x24u) != 0) {
                WB(R(self + 0x20u) + 4u, 3);
                W(self + 0x20u, 0);
            }
        }
        break;
    default:
        break;
    }
    C(001B5360, self);
    C(001C64F0, self, R(ctl + 0x28u), &r);
    W(ctl, (uint32_t)r);
    C(00131ED0, self);
    C(001C68C0, self);
    C(001B17A0, self, &r);
    METHOD(self);
    return 0;
}

/* 00137E80: state 3, the release: 001B1190(+0x9A), 001AFC10(self). */
uint32_t a21b_00137E80(A21B *o, uint32_t self)
{
    C(001B1190, (int32_t)B(self + 0x9Au));
    C(001AFC10, self);
    return 0;
}

/* 00137EB0(self): a rig follower of the creature at +0x20. State +4: 0
 * the step up, the countdown ctl+4 = 0, the model D_0028A490[+0xD ? 0xA7
 * : 0xA8] (001CA6E0), its bone count 001C6150(+0x44) at +0xC; more bones
 * than the free count 0x275BCC: state 3; else one bone record 001AF780
 * per bone at +0x110, the count at +9, 001CB5B0(count) and 001C62C0.
 * 1: the bone 14 (side 1) or 4 (side 0) matrix of the creature copied
 * over the follower's first bone (0x275B40); the countdown steps down;
 * on side 0, with the creature's side set and the countdown at 0: the
 * countdown 6, the burst 001B3C30 at the point (5.62, 0.98, 0) of that
 * matrix (scale 0.8, rate 200, 20 on hard else 15) and the sound 0x164
 * (450); the creature's state 3 is copied; 001B17A0 and the +0x4C
 * method. 2 and 3: 001AFC10. */
uint32_t a21b_00137EB0(A21B *o, uint32_t self)
{
    const uint32_t ctl = self + 0x1F0u;
    int32_t r = 0, i;
    uint32_t st = B(self + 4u), other, idx, a, b, c, d, n, p = 0;
    switch (st) {
    case 0:
        WB(self + 4u, st + 1u);
        W(ctl + 4u, 0);
        C(001CA6E0, self, R(0x28A490u + 4u * (B(self + 0xDu) ? 0xA7u : 0xA8u)));
        C(001C6150, R(self + 0x44u), &r);
        WB(self + 0xCu, (uint32_t)r);
        i = SH(0x275BCCu);
        if (i < (int32_t)B(self + 0xCu)) {
            WB(self + 4u, 3);
            return 0;
        }
        for (i = 0, n = B(self + 0xCu); i < (int32_t)n; i++, n = B(self + 0xCu)) {
            C(001AF780, &p);
            W(self + 0x110u + 4u * (uint32_t)i, p);
        }
        WB(self + 9u, n);
        C(001CB5B0, (int32_t)B(self + 0xCu));
        C(001C62C0, self);
        break;
    case 1:
        idx = B(self + 0xDu) ? 0xEu : 4u;
        a = R(0x275B40u);
        other = R(self + 0x20u);
        a = R(a) + 0x90u;
        C(00102958, a, R(idx * 4u + other + 0x110u) + 0x90u);
        if ((a = R(ctl + 4u)) != 0) W(ctl + 4u, a - 1u);
        if (B(self + 0xDu) == 0 && B(other + 0xDu) != 0 && R(ctl + 4u) == 0) {
            W(ctl + 4u, 6);
            C(00102958, V36A0, R(R(0x275B40u)) + 0x90u);
            W(0x700038A0u, F(5.62f));
            W(0x700038A4u, F(0.98f));
            W(0x700038A8u, 0);
            W(0x700038ACu, F(1.0f));
            C(001026A0, V38A0, V36A0, V38A0);
            a = R(0x700038A0u);
            b = R(0x700038A4u);
            c = R(0x700038A8u);
            d = R(0x700038ACu);
            W(0x700036D0u, a);
            W(0x700036D4u, b);
            W(0x700036D8u, c);
            W(0x700036DCu, d);
            W(0x700038B0u, F(0.8f));
            W(0x700038B4u, F(0.8f));
            W(0x700038B8u, F(0.8f));
            W(0x700038BCu, F(1.0f));
            C(001B3C30, self, V36A0, V38B0, F(200.0f), B(HARD) ? F(20.0f) : F(15.0f));
            C(001FBD50, self, 0x164, 0, F(450.0f), &r);
        }
        if (B(other + 4u) == 3) WB(self + 4u, 3);
        C(001B17A0, self, &r);
        METHOD(self);
        break;
    case 2:
    case 3:
        C(001AFC10, self);
        break;
    default:
        break;
    }
    return 0;
}

/* 001381D0(self, ctl): the damage of a hit (the hit word +0x36: amount &
 * 0xFFF, x5 with bit 0x8000, x4 with 0x1000). With no hit: 0. During the
 * guard ctl+0x40 the hit is dropped (header 1). Else header 2, the guard
 * -1, the hit word loses bit 0x8000 while ctl+0x44 runs, the damage sum
 * ctl+0x30 (reset
 * unless ctl+0x42 still runs, which is set to 45), bit 0x4000 an effect
 * 0x80000027 at most every 60 frames (ctl+0x41), and the hit points +0x34
 * drop (at 0: the death effect 001B4CF0). No hit points left, a sum of
 * 160, or a 0x8000 hit: state 2 (hurt), the sum cleared, result 1. Else
 * the guard is 30 for 0x5000 hits, 60 for 0x2000, else 0; header 1, the
 * hit word cleared, result 0. */
int32_t a21b_001381D0(A21B *o, uint32_t self, uint32_t ctl)
{
    int32_t hit, amount, hp, f, r = 0;
    if (SH(self + 0x36u) == 0) return 0;
    if (SB(ctl + 0x40u) != 0) {
        WB(self, 1);
        WH(self + 0x36u, 0);
        return 0;
    }
    WB(self, 2);
    WB(ctl + 0x40u, 0xFF);
    hit = SH(self + 0x36u);
    amount = (int16_t)(hit & 0xFFF);
    if (SB(ctl + 0x44u) != 0) WH(self + 0x36u, (uint32_t)(hit & 0x7FFF));
    f = SH(self + 0x36u);
    if (f & 0x8000) amount = (int16_t)(amount * 5);
    else if (f & 0x1000) amount = (int16_t)(amount * 4);
    if (SB(ctl + 0x42u) != 0) {
        WH(ctl + 0x30u, (uint32_t)(SH(ctl + 0x30u) + amount));
    } else {
        WB(ctl + 0x42u, 0x2D);
        WH(ctl + 0x30u, (uint32_t)amount);
    }
    if (SB(ctl + 0x41u) == 0 && (SH(self + 0x36u) & 0x4000)) {
        WB(ctl + 0x41u, 0x3C);
        C(001EFE00, (int32_t)0x80000027u, self, &r);
    }
    hp = SH(self + 0x34u);
    if (hp <= amount) {
        WH(self + 0x34u, 0);
        C(001B4CF0, self);
    } else {
        WH(self + 0x34u, (uint32_t)(hp - amount));
    }
    if (SH(self + 0x34u) != 0 && SH(ctl + 0x30u) < 0xA0) {
        f = SH(self + 0x36u);
        if (!(f & 0x8000)) {
            if (f & 0x5000) WB(ctl + 0x40u, 0x1E);
            else if (f & 0x2000) WB(ctl + 0x40u, 0x3C);
            else WB(ctl + 0x40u, 0);
            WB(self, 1);
            WH(self + 0x36u, 0);
            return 0;
        }
    }
    WB(self + 4u, 2);
    WB(self + 5u, 0);
    WB(self + 6u, 0);
    WH(ctl + 0x30u, 0);
    return 1;
}

/* ---- exported entries ---------------------------------------------------- */
#define ENTRY(body) \
    A21B o; \
    if (a21b_begin(&o, h, fault)) return -1; \
    (void)(body); \
    return a21b_end(&o)

int em_area21_boot_0014C220(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault)
{ ENTRY(a21b_0014C220(&o, a0)); }
int em_area21_boot_0014C2E0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014C2E0(&o, a0, a1)); }
int em_area21_boot_0014C3A0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014C3A0(&o, a0, a1)); }
int em_area21_boot_0014C490(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014C490(&o, a0, a1)); }
int em_area21_boot_0014C680(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014C680(&o, a0, a1)); }
int em_area21_boot_0014C9D0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014C9D0(&o, a0, a1)); }
int em_area21_boot_0014CCC0(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault)
{ ENTRY(a21b_0014CCC0(&o, a0)); }
int em_area21_boot_0014CD00(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault)
{ ENTRY(a21b_0014CD00(&o, a0)); }
int em_area21_boot_0014CD30(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014CD30(&o, a0, a1)); }
int em_area21_boot_0014CDD0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_0014CDD0(&o, a0, a1)); }
int em_area21_boot_0014D1E0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, int32_t a2, EmArea21BootFault *fault)
{ ENTRY(a21b_0014D1E0(&o, a0, a1, a2)); }
int em_area21_boot_00137830(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault)
{ ENTRY(a21b_00137830(&o, a0)); }
int em_area21_boot_00137920(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_00137920(&o, a0, a1)); }
int em_area21_boot_00137A50(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_00137A50(&o, a0, a1)); }
int em_area21_boot_00137C80(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault)
{ ENTRY(a21b_00137C80(&o, a0, a1)); }
int em_area21_boot_00137E80(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault)
{ ENTRY(a21b_00137E80(&o, a0)); }
int em_area21_boot_00137EB0(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault)
{ ENTRY(a21b_00137EB0(&o, a0)); }

int em_area21_boot_001283D0(const EmArea21BootHooks *h, uint32_t a0, int32_t a1, uint32_t a2, int32_t *result, EmArea21BootFault *fault)
{
    A21B o;
    if (!result || a21b_begin(&o, h, fault)) return -1;
    const int32_t r = a21b_001283D0(&o, a0, a1, a2);
    if (a21b_end(&o)) return -1;
    *result = r;
    return 0;
}

int em_area21_boot_001381D0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea21BootFault *fault)
{
    A21B o;
    if (!result || a21b_begin(&o, h, fault)) return -1;
    const int32_t r = a21b_001381D0(&o, a0, a1);
    if (a21b_end(&o)) return -1;
    *result = r;
    return 0;
}
