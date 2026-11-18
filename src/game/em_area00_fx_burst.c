/* AREA00 lane A00FX, the six-slot particle burst: 001F1F60 (the node
 * callback of effect ids 0x80000037 and 0x80000038) and its helper
 * 001F1DD0. See em_area00_fx.h and docs/AREA00_FX.md.
 *
 * Both are byte-matched in the decomp (src/func_001F1F60.c,
 * src/func_001F1DD0.c); this follows the original instructions.
 *
 * The node's parent is +0x24 (the creature or the weapon that spawned it
 * through 001EFE00). State 0 takes six bone slots (001F1550(node, 6)), marks
 * every particle dead (age -1.0) and seeds the work block +0x1F0 (+0x20 the
 * parent's last point, +0x08 an LCG word from rand(), +0x14 the ring scale
 * 0.2). States 1 and 2 end with the parent (its +4 = 3). State 1 also emits:
 * on its first frame the node's matrix (+0xD0) is built from the parent's
 * +0xC0 (mode 0) or +0x70 (mode 1) through 001CD390; each frame 001F1DD0
 * starts one particle at the parent's +0x100 (mode 0: a second one at the
 * point twice as far along the last step). Every state-1 / state-2 frame
 * draws the live particles (001CD520 sprites; ages grow by 0.08 or 0.02 and
 * end past 1.0), and while the ring scale is below 1.5 the ring sprites
 * (001CFA60 / 001CFBE0: mode 0 one sprite, mode 1 four rotated ones). The
 * mode is the node's byte +0x0D; the LCG is work +0x0C, reseeded from +0x08
 * every frame. State 2 ends (state 3) when all 60 particles are dead and the
 * ring is done; state 3 frees the node. */
#include "game/em_area00_fx_internal.h"

#define F_M1 0xBF800000u
#define F_0P2 0x3E4CCCCDu
#define F_0P4 0x3ECCCCCDu
#define F_0P6 0x3F19999Au
#define F_2 0x40000000u
#define F_3 0x40400000u
#define F_48 0x42400000u
#define F_80 0x42A00000u
#define F_192 0x43400000u
#define F_1P5 0x3FC00000u
#define F_0P08 0x3DA3D70Au
#define F_0P02 0x3CA3D70Au
#define F_0P06 0x3D75C28Fu
#define F_0P04 0x3D23D70Au
#define F_110DEG 0x3FF5BE0Bu  /* 1.9198622 */
#define F_45DEG 0x3F490FDBu
#define F_135DEG 0x4016CBE4u
#define F_M45DEG 0xBF490FDBu
#define F_M135DEG 0xC016CBE4u

#define SPAD_3A20 0x70003A20u
#define SPAD_3A24 0x70003A24u
#define SPAD_38A0 0x700038A0u
#define SPAD_36A0 0x700036A0u
#define SPAD_36E0 0x700036E0u
#define SPAD_3710 0x70003710u
#define D_0025A0F0 0x0025A0F0u
#define D_0025A180 0x0025A180u

#define FX_00102760 0x00102760u
#define FX_00102870 0x00102870u
#define FX_00102A60 0x00102A60u
#define FX_00102B08 0x00102B08u
#define FX_00128250 0x00128250u
#define FX_001CD520 0x001CD520u

/* ---- 001F1DD0 ------------------------------------------------------------ */

int em_area00_fx_001F1DD0(S *s, u64 node, u64 work, u64 point)
{
    FxFrame fr = fx_enter(s, 0x001F1DD0u, 0x30);
    const u32 nd = (u32)node, w = (u32)work;
    const u32 mode = fx_lbu(s, nd + 0xDu);
    if (mode == 1)
        fx_sw(s, SPAD_3A20, F_0P6);
    else if (mode == 0)
        fx_sw(s, SPAD_3A20, F_0P4);
    fx_sw(s, w + 4u, fx_lw(s, w + 4u) + 1u);
    if (!((int32_t)fx_lw(s, w + 4u) < 10)) {
        fx_sw(s, w + 4u, 0);
        fx_sw(s, w, fx_lw(s, w) + 1u);
        if (!((int32_t)fx_lw(s, w) < 6)) fx_sw(s, w, 0);
    }
    {
        const u32 r = (u32)fx_call0(s, FX_00122BB8);
        const u32 u = fx_fdiv(fx_fcvt(r), FX_F_2POW31);
        fx_sw(s, SPAD_3A24, fx_fsub(fx_fmul(FX_F_2PI, u), FX_F_PI));
    }
    const u32 base = fx_lw(s, fx_lw(s, FX_D_00275B40) + (fx_lw(s, w) << 2));
    fx_00102948(s, base + (fx_lw(s, w + 4u) << 4), (u32)point);
    {
        const u32 sn = fx_callf(s, FX_0011E2A8, fx_lw(s, SPAD_3A24));
        const u32 t = fx_fmul(fx_lw(s, SPAD_3A20), sn);
        const u32 at = base + (fx_lw(s, w + 4u) << 4);
        fx_sw(s, at, fx_fadd(fx_lw(s, at), t));
    }
    {
        const u32 cs = fx_callf(s, FX_0011DE90, fx_lw(s, SPAD_3A24));
        const u32 t = fx_fmul(fx_lw(s, SPAD_3A20), cs);
        const u32 at = (fx_lw(s, w + 4u) << 4) + base;
        fx_sw(s, at + 4u, fx_fadd(fx_lw(s, at + 4u), t));
    }
    fx_sw(s, (fx_lw(s, w + 4u) << 2) + base + 0xA0u, 0);
    fx_leave(s, fr);
    return fx_result(s);
}

/* ---- 001F1F60 ------------------------------------------------------------ */

/* ((w >> 16) & 0xFFFF) / 65535 + 1e-4 of the LCG word at `lcg`, which
 * becomes w * 37 + 11. */
static u32 burst_unit(S *s, u32 lcg)
{
    const u32 v = fx_lw(s, lcg);
    const u32 k = fx_fdiv(fx_fcvt((u32)((int32_t)v >> 16) & 0xFFFFu), FX_F_65535);
    fx_sw(s, lcg, v * 37u + 11u);
    return fx_fadd(k, FX_F_1EM4);
}

/* 00128250(f12): v0 as the callee leaves it. */
static u64 burst_00128250(S *s, u32 f12)
{
    EmArea00FxRegs r = {0};
    u64 v0;
    fx_farg(&r, 12, f12);
    fx_call(s, FX_00128250, &r, &v0, NULL);
    return v0;
}

/* 001CD520(0, kind, point, tag, colour; f12 = f13 = size, f14). */
static void burst_sprite(S *s, u64 kind, u32 point, u64 tag, u64 colour, u32 size, u32 f14)
{
    EmArea00FxRegs r = {0};
    fx_arg(&r, R_A0, 0);
    fx_arg(&r, R_A1, kind);
    fx_arg(&r, R_A2, fx_sx(point));
    fx_arg(&r, R_A3, tag);
    fx_arg(&r, R_T0, colour);
    fx_farg(&r, 12, size);
    fx_farg(&r, 13, size);
    fx_farg(&r, 14, f14);
    fx_call(s, FX_001CD520, &r, NULL, NULL);
}

/* One live particle's sprite (mode 0 or 1) at `quad`, its age at `age`. */
static void burst_particle(S *s, u32 mode, u32 quad, u32 age)
{
    const u32 grey = mode == 1 ? F_80 : F_192;
    fx_sw(s, SPAD_38A0, grey);
    fx_sw(s, SPAD_38A0 + 4u, grey);
    fx_sw(s, SPAD_38A0 + 8u, grey);
    if (mode == 1) {
        const u32 a = fx_lw(s, age + 0xA0u);
        fx_sw(s, SPAD_38A0 + 0xCu, fx_fmul(grey, fx_fsub(FX_F_ONE, fx_fmul(a, a))));
        fx_sw(s, SPAD_3A20, fx_fadd(F_2, fx_fmul(F_3, fx_lw(s, age + 0xA0u))));
    } else {
        fx_sw(s, SPAD_38A0 + 0xCu, fx_fmul(F_48, fx_fsub(FX_F_ONE, fx_lw(s, age + 0xA0u))));
        fx_sw(s, SPAD_3A20, fx_fadd(F_2, fx_fmul(F_2, fx_lw(s, age + 0xA0u))));
    }
    u64 packed = burst_00128250(s, fx_lw(s, SPAD_38A0));
    packed |= fx_sx((u32)burst_00128250(s, fx_lw(s, SPAD_38A0 + 4u)) << 8);
    packed |= fx_sx((u32)burst_00128250(s, fx_lw(s, SPAD_38A0 + 8u)) << 16);
    packed |= fx_sx((u32)burst_00128250(s, fx_lw(s, SPAD_38A0 + 0xCu)) << 24);
    burst_sprite(s, 1, quad, UINT64_C(0x2004128555322090), packed, fx_lw(s, SPAD_3A20), 0);
    fx_sw(s, age + 0xA0u, fx_fadd(fx_lw(s, age + 0xA0u), mode == 1 ? F_0P02 : F_0P08));
}

int em_area00_fx_001F1F60(S *s, u64 node)
{
    FxFrame fr = fx_enter(s, 0x001F1F60u, 0x110);
    const u32 nd = (u32)node, p = nd + 0x1F0u, spB0 = s->sp + 0xB0u;
    const u32 ctx = fx_lw(s, nd + 0x24u);
    const u32 st = fx_lbu(s, nd + 4u);
    if (st == 3) {
        fx_call1(s, FX_001AFC10, node);
    } else if (st == 0) {
        u64 ok = 0;
        em_area00_fx_001F1550(s, node, 6, &ok);
        s->core.function = 0x001F1F60u;
        if (ok != 0) {
            for (u32 i = 0; i < 6; ++i) {
                const u32 lp = fx_lw(s, fx_lw(s, FX_D_00275B40) + 4u * i);
                for (u32 j = 0; j < 10; ++j) fx_sw(s, lp + 4u * j + 0xA0u, F_M1);
            }
            fx_00102948(s, p + 0x20u, ctx + 0x100u);
            fx_sw(s, p + 0x10u, 0);
            fx_sw(s, p, 0);
            fx_sw(s, p + 4u, 0);
            fx_sw(s, p + 8u, (u32)fx_call0(s, FX_00122BB8));
            fx_sw(s, p + 0x14u, F_0P2);
            fx_sb(s, nd + 4u, 1);
        }
    } else if (st == 1 || st == 2) {
        if (fx_lbu(s, ctx + 4u) == 3) {
            fx_sb(s, nd + 4u, 3);
            goto out;
        }
        if (st == 1) {
            if (fx_lw(s, p + 0x10u) == 0) {
                fx_sw(s, p + 0x10u, 1);
                const u32 mode = fx_lbu(s, nd + 0xDu);
                if (mode == 1 || mode == 0) {
                    fx_call1(s, FX_001029C0, fx_sx(nd + 0xD0u));
                    fx_call2(s, FX_00102760, fx_sx(SPAD_38A0), fx_sx(ctx + (mode == 1 ? 0x70u : 0xC0u)));
                }
                fx_call2(s, FX_001CD390, fx_sx(nd + 0xD0u), fx_sx(SPAD_38A0));
                fx_sw(s, nd + 0x100u, fx_lw(s, nd + 0xB0u));
                fx_sw(s, nd + 0x104u, fx_lw(s, nd + 0xB4u));
                fx_sw(s, nd + 0x108u, fx_lw(s, nd + 0xB8u));
            }
            em_area00_fx_001F1DD0(s, node, fx_sx(p), fx_sx(ctx + 0x100u));
            s->core.function = 0x001F1F60u;
            if (fx_lbu(s, nd + 0xDu) == 0) {
                fx_call3(s, FX_001028D0, fx_sx(SPAD_38A0), fx_sx(ctx + 0x100u), fx_sx(p + 0x20u));
                fx_call2f(s, FX_00102870, fx_sx(SPAD_38A0), fx_sx(SPAD_38A0), F_2);
                fx_call3(s, FX_001028B8, fx_sx(SPAD_38A0), fx_sx(SPAD_38A0), fx_sx(ctx + 0x100u));
                em_area00_fx_001F1DD0(s, node, fx_sx(p), fx_sx(SPAD_38A0));
                s->core.function = 0x001F1F60u;
            }
            fx_00102948(s, p + 0x20u, ctx + 0x100u);
        }
        /* the particles */
        u32 died = 0;
        fx_sw(s, p + 0xCu, fx_lw(s, p + 8u));
        for (u32 i = 0; i < 6 && !fx_latched(s); ++i) {
            const u32 r = burst_unit(s, p + 0xCu);
            const u32 life = fx_lw(s, fx_lw(s, FX_D_00275B40) + 4u * i);
            fx_sw(s, SPAD_3A24, r);
            for (u32 j = 0; j < 10 && !fx_latched(s); ++j) {
                const u32 age = life + 4u * j, quad = life + 16u * j;
                if (!fx_flt(fx_lw(s, age + 0xA0u), 0)) {
                    const u32 mode = fx_lbu(s, nd + 0xDu);
                    if (mode == 1 || mode == 0) burst_particle(s, mode, quad, age);
                }
                const u32 a = fx_lw(s, age + 0xA0u);
                if (!fx_fle(a, FX_F_ONE) || fx_feq(F_M1, a)) {
                    ++died;
                    fx_sw(s, age + 0xA0u, F_M1);
                }
            }
        }
        /* the ring */
        if (fx_flt(fx_lw(s, p + 0x14u), F_1P5)) {
            const u32 mode = fx_lbu(s, nd + 0xDu);
            if (mode == 0) {
                const u64 handle = fx_call1(s, FX_001CCF70, fx_sx(nd + 0x100u));
                const u32 r = burst_unit(s, p + 0xCu);
                {
                    EmArea00FxRegs rg = {0};
                    fx_arg(&rg, R_A0, fx_sx(spB0));
                    fx_arg(&rg, R_A1, fx_sx(nd + 0xD0u));
                    fx_farg(&rg, 12, fx_lw(s, p + 0x14u));
                    fx_farg(&rg, 13, r);
                    fx_call(s, FX_001CFA60, &rg, NULL, NULL);
                }
                {
                    EmArea00FxRegs rg = {0};
                    fx_arg(&rg, R_A0, handle);
                    fx_arg(&rg, R_A1, 1);
                    fx_arg(&rg, R_A2, fx_sx(D_0025A0F0));
                    fx_arg(&rg, R_A3, fx_sx(spB0));
                    fx_arg(&rg, R_T0, 0);
                    fx_call(s, FX_001CFBE0, &rg, NULL, NULL);
                }
                fx_sw(s, p + 0x14u, fx_fadd(fx_lw(s, p + 0x14u), F_0P06));
            } else if (mode == 1) {
                static const u32 turn[4] = {F_45DEG, F_135DEG, F_M45DEG, F_M135DEG};
                fx_call1(s, FX_001029C0, fx_sx(SPAD_36A0));
                fx_call2f(s, FX_00102B08, fx_sx(SPAD_36A0), fx_sx(SPAD_36A0), F_110DEG);
                for (u32 k = 0; k < 4 && !fx_latched(s); ++k) {
                    fx_call2f(s, FX_00102A60, fx_sx(SPAD_36E0), fx_sx(SPAD_36A0), turn[k]);
                    fx_call3(s, FX_001026D0, fx_sx(SPAD_36E0), fx_sx(nd + 0xD0u), fx_sx(SPAD_36E0));
                    const u64 handle = fx_call1(s, FX_001CCF70, fx_sx(SPAD_3710));
                    const u32 r = burst_unit(s, p + 0xCu);
                    {
                        EmArea00FxRegs rg = {0};
                        fx_arg(&rg, R_A0, fx_sx(spB0));
                        fx_arg(&rg, R_A1, fx_sx(SPAD_36E0));
                        fx_farg(&rg, 12, fx_lw(s, p + 0x14u));
                        fx_farg(&rg, 13, r);
                        fx_call(s, FX_001CFA60, &rg, NULL, NULL);
                    }
                    {
                        EmArea00FxRegs rg = {0};
                        fx_arg(&rg, R_A0, handle);
                        fx_arg(&rg, R_A1, 1);
                        fx_arg(&rg, R_A2, fx_sx(D_0025A180));
                        fx_arg(&rg, R_A3, fx_sx(spB0));
                        fx_arg(&rg, R_T0, 0);
                        fx_call(s, FX_001CFBE0, &rg, NULL, NULL);
                    }
                }
                fx_sw(s, p + 0x14u, fx_fadd(fx_lw(s, p + 0x14u), F_0P04));
            }
        }
        if (fx_lbu(s, nd + 4u) == 2) {
            if (died == 0x3C && !fx_flt(fx_lw(s, p + 0x14u), F_1P5)) fx_sb(s, nd + 4u, 3);
        } else if (fx_lbu(s, nd + 0xDu) == 1) {
            const u32 r = (u32)fx_call0(s, FX_00122BB8);
            const u32 size = fx_fadd(F_2, fx_fdiv(fx_fcvt(r), FX_F_2POW31));
            fx_sw(s, SPAD_3A20, size);
            burst_sprite(s, 2, ctx + 0x100u, UINT64_C(0x20041605113222CD), 0x609090u, size, F_2);
        }
    }
out:
    fx_leave(s, fr);
    return fx_result(s);
}
