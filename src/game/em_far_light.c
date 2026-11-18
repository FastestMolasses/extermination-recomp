/* em_far_light.c - see em_far_light.h. */
#include "game/em_far_light.h"

#include <string.h>

#include "game/em_ee_float.h"
#include "game/em_effect_original.h"
#include "game/em_player_stage_workers.h"
#include "game/em_sdk_vu0.h"

typedef EmFarLight F;
typedef uint32_t u32;

#define D_00275670 0x00275670u   /* the render context pointer */
#define D_00275C08 0x00275C08u   /* the read-back list's count */
#define D_008172C0 0x008172C0u   /* the read-back list: 2 banks x 0x480 */
#define D_00251370 0x00251370u   /* the flare's position factors (7 floats) */
#define D_00251390 0x00251390u   /* the flare's sprite sizes (7 floats) */
#define D_007635C0 0x007635C0u   /* the effect page the flare is chained to */
#define SPR(o)     (0x70000000u + (o))

static const u32 VF0[4] = {0, 0, 0, 0x3F800000u};
#define F_ONE   0x3F800000u
#define F_TWO   0x40000000u

static int fault(F *f, int32_t code, u32 fn, u32 address)
{
    if (f->fault == EM_FAR_LIGHT_FAULT_NONE) {
        f->fault = code;
        f->fault_function = fn;
        f->fault_address = address;
    }
    return -1;
}

static uint8_t *at(F *f, u32 address, u32 size, int write, u32 fn)
{
    uint8_t *p = f->map ? f->map(f->ctx, address, size, write) : NULL;
    if (!p) fault(f, EM_FAR_LIGHT_FAULT_UNMAPPED, fn, address);
    return p;
}

static int ld32(F *f, u32 address, u32 *v, u32 fn)
{
    const uint8_t *p = at(f, address, 4, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 4);
    return 0;
}
static int st32(F *f, u32 address, u32 v, u32 fn)
{
    uint8_t *p = at(f, address, 4, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 4);
    return 0;
}
static int st64(F *f, u32 address, uint64_t v, u32 fn)
{
    uint8_t *p = at(f, address, 8, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 8);
    return 0;
}
static int ld128(F *f, u32 address, u32 v[4], u32 fn)
{
    const uint8_t *p = at(f, address & ~15u, 16, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 16);
    return 0;
}
static int st128(F *f, u32 address, const u32 v[4], u32 fn)
{
    uint8_t *p = at(f, address & ~15u, 16, 1, fn);
    if (!p) return -1;
    memcpy(p, v, 16);
    return 0;
}

#define TRY(x) do { if ((x) < 0) return -1; } while (0)
#define VU(f, x, fn) do { if ((x) != EM_EE_FLOAT_OK) return fault((f), EM_FAR_LIGHT_FAULT_UNMEASURED, (fn), 0); } while (0)

static int alive(F *f)
{
    if (!f) return -1;
    if (f->fault != EM_FAR_LIGHT_FAULT_NONE) return -1;
    if (!f->vu) return fault(f, EM_FAR_LIGHT_FAULT_WORKER, 0, 0);
    return 0;
}

/* vf28..vf31 = the camera rows at 0x70003AC0. */
static int camera_rows(F *f, u32 fn)
{
    for (unsigned k = 0; k < 4; ++k)
        TRY(ld128(f, SPR(0x3AC0) + 16 * k, f->vu->vf[28 + k], fn));
    return 0;
}

/* The projection shared by 001D72D0, 001D75E0 and 001CEFD0, with vf1
 * already loaded: vf2 = vf28 x + vf29 y + vf30 z + vf31; Q = 1 / vf2.w;
 * clip = vf2 (stored before the scale); vf2.xyz *= Q; vf2.w = vf23.z +
 * vf23.w * vf2.w, then min vf23.x and max 0; vf2 = ftoi4(vf2). */
static int project(F *f, u32 clip[4], u32 screen[4], u32 fn)
{
    EmFarLightVu *v = f->vu;
    VU(f, em_vu_vec_bits(EM_VU_MULABC, 15, 0, v->vf[28], v->vf[1], 0, NULL, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDABC, 15, 1, v->vf[29], v->vf[1], 0, v->acc, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDABC, 15, 2, v->vf[30], v->vf[1], 0, v->acc, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDBC, 15, 3, v->vf[31], VF0, 0, v->acc, v->vf[2]), fn);
    VU(f, em_vu_div_bits(VF0[3], v->vf[2][3], 3, 3, &v->q), fn);
    memcpy(clip, v->vf[2], 16);
    VU(f, em_vu_vec_bits(EM_VU_MULQ, 14, -1, v->vf[2], NULL, v->q, NULL, v->vf[2]), fn);
    VU(f, em_vu_vec_bits(EM_VU_MULABC, 1, 2, VF0, v->vf[23], 0, NULL, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDBC, 1, 3, v->vf[23], v->vf[2], 0, v->acc, v->vf[2]), fn);
    v->vf[2][3] = em_vu_min_bits(v->vf[2][3], v->vf[23][0]);
    v->vf[2][3] = em_vu_max_bits(v->vf[2][3], VF0[0]);
    for (unsigned k = 0; k < 4; ++k)
        v->vf[2][k] = em_vu_ftoi4_bits(v->vf[2][k]);
    memcpy(screen, v->vf[2], 16);
    return 0;
}

static int float_to_int(u32 bits) { return em_player_float_to_int(bits); }

/* ------------------------------------------------------------------ */

int em_far_light_001D73A0(F *f, int32_t a0, int32_t *v0)
{
    TRY(alive(f));
    const u32 fn = 0x001D73A0u;
    if (a0 < 0 || a0 >= 0x10) {
        *v0 = 0;
        return 0;
    }
    u32 count, ctx, bank, word;
    TRY(ld32(f, D_00275C08, &count, fn));
    TRY(ld32(f, D_00275670, &ctx, fn));
    TRY(ld32(f, ctx + 0x9Cu, &bank, fn));
    TRY(ld32(f, count * 0x48u + D_008172C0 + (1u - bank) * 0x480u + 8u, &word, fn));
    *v0 = (int32_t)word;
    return 0;
}

int em_far_light_001D7200(F *f, int32_t x, int32_t y, int32_t *v0)
{
    TRY(alive(f));
    const u32 fn = 0x001D7200u;
    u32 count, ctx, bank;
    TRY(ld32(f, D_00275C08, &count, fn));
    if ((int32_t)count >= 0x10) {
        *v0 = -1;
        return 0;
    }
    TRY(ld32(f, D_00275670, &ctx, fn));
    TRY(ld32(f, ctx + 0x9Cu, &bank, fn));
    TRY(ld32(f, D_00275C08, &count, fn));
    const u32 entry = D_008172C0 + bank * 0x480u + count * 0x48u;
    if (x < 0) x = 0;
    if (!em_ee_c_lt_bits(em_ee_cvt_s_w_bits((u32)x), 0x44000000u)) x = 0x1FF;   /* 512.0 */
    if (y < 0) y = 0;
    if (!(y < 0xE0)) y = 0xDF;
    TRY(st32(f, entry, em_ee_cvt_s_w_bits((u32)x), fn));
    TRY(st32(f, entry + 4, em_ee_cvt_s_w_bits((u32)y), fn));
    TRY(ld32(f, D_00275C08, &count, fn));
    TRY(st32(f, D_00275C08, count + 1u, fn));
    *v0 = (int32_t)count;
    return 0;
}

int em_far_light_001D72D0(F *f, const u32 pos[4], int32_t *v0)
{
    TRY(alive(f));
    const u32 fn = 0x001D72D0u;
    u32 count, clip[4], screen[4];
    TRY(ld32(f, D_00275C08, &count, fn));
    if ((int32_t)count >= 0x10) {
        *v0 = -1;
        return 0;
    }
    TRY(camera_rows(f, fn));
    memcpy(f->vu->vf[1], pos, 16);
    TRY(project(f, clip, screen, fn));
    if (em_ee_c_lt_bits(clip[3], 0u)) {
        *v0 = -1;
        return 0;
    }
    return em_far_light_001D7200(f, ((int32_t)screen[0] >> 4) - 0x700, ((int32_t)screen[1] >> 4) - 0x790, v0);
}

int em_far_light_001D75E0(F *f, int32_t idx, const u32 pos[4], uint64_t tex0, u32 rgba, u32 w, u32 h,
                          u32 *v0)
{
    TRY(alive(f));
    const u32 fn = 0x001D75E0u;
    u32 ctx, clip[4], screen[4];
    float ref[4], m[16];
    u32 rb[4] = {0x41800000u, 0x41800000u, 0x43F00000u, F_ONE};   /* 16, 16, 480, 1 */
    TRY(ld32(f, D_00275670, &ctx, fn));
    const uint8_t *mp = at(f, ctx + 0x2340u, 0x40, 0, fn);
    if (!mp) return -1;
    memcpy(m, mp, sizeof m);
    memcpy(ref, rb, sizeof ref);
    em_effect_original_001026A0(ref, m, ref);
    memcpy(rb, ref, sizeof rb);
    rb[0] = em_ee_div_bits(rb[0], rb[3]);
    rb[1] = em_ee_div_bits(rb[1], rb[3]);
    rb[0] = em_ee_sub_bits(rb[0], 0x45000000u);                    /* 2048 */
    rb[1] = em_ee_sub_bits(rb[1], 0x45000000u);
    rb[0] = em_ee_mul_bits(0x3F4CCCCDu, rb[0]);                    /* 0.8 */
    rb[1] = em_ee_mul_bits(0x3F000000u, rb[1]);                    /* 0.5 */
    w = em_ee_mul_bits(w, rb[0]);
    h = em_ee_mul_bits(h, rb[1]);

    TRY(camera_rows(f, fn));
    memcpy(f->vu->vf[1], pos, 16);
    TRY(project(f, clip, screen, fn));
    TRY(st128(f, SPR(0x3600), screen, fn));
    if (em_ee_c_lt_bits(clip[3], 0u)) {
        *v0 = 0;
        return 0;
    }
    u32 sx, sy;
    TRY(ld32(f, SPR(0x3600), &sx, fn));
    int32_t left = (int32_t)(sx - (u32)float_to_int(w));
    TRY(ld32(f, SPR(0x3600), &sx, fn));
    int32_t right = (int32_t)(sx + (u32)float_to_int(w));
    TRY(ld32(f, SPR(0x3604), &sy, fn));
    int32_t top = (int32_t)(sy - (u32)float_to_int(h));
    TRY(ld32(f, SPR(0x3604), &sy, fn));
    int32_t bottom = (int32_t)(sy + (u32)float_to_int(h));
    if (right < 0x7000 || left > 0x9000 || bottom < 0x7900 || top > 0x8700) {
        *v0 = 0;
        return 0;
    }
    TRY(st32(f, SPR(0x3670), 0u, fn));
    TRY(st32(f, SPR(0x3674), F_ONE, fn));
    TRY(st32(f, SPR(0x3678), F_ONE, fn));
    TRY(st32(f, SPR(0x367C), 0u, fn));
    TRY(st32(f, SPR(0x3A38), em_ee_mul_bits(F_TWO, w), fn));
    TRY(st32(f, SPR(0x3A3C), em_ee_mul_bits(F_TWO, h), fn));
    u32 a, d;
    if (left < 0x7000) {
        TRY(ld32(f, SPR(0x3A38), &d, fn));
        TRY(ld32(f, SPR(0x3670), &a, fn));
        TRY(st32(f, SPR(0x3670), em_ee_add_bits(a, em_ee_div_bits(em_ee_cvt_s_w_bits((u32)(0x7000 - left)), d)), fn));
        left = 0x7000;
    }
    if (right > 0x9000) {
        TRY(ld32(f, SPR(0x3A38), &d, fn));
        TRY(ld32(f, SPR(0x3678), &a, fn));
        TRY(st32(f, SPR(0x3678), em_ee_sub_bits(a, em_ee_div_bits(em_ee_cvt_s_w_bits((u32)(right - 0x9000)), d)), fn));
        right = 0x9000;
    }
    if (top < 0x7900) {
        TRY(ld32(f, SPR(0x3A3C), &d, fn));
        TRY(ld32(f, SPR(0x3674), &a, fn));
        TRY(st32(f, SPR(0x3674), em_ee_sub_bits(a, em_ee_div_bits(em_ee_cvt_s_w_bits((u32)(0x7900 - top)), d)), fn));
        top = 0x7900;
    }
    if (bottom > 0x8700) {
        TRY(ld32(f, SPR(0x3A3C), &d, fn));
        TRY(ld32(f, SPR(0x367C), &a, fn));
        TRY(st32(f, SPR(0x367C), em_ee_add_bits(a, em_ee_div_bits(em_ee_cvt_s_w_bits((u32)(bottom - 0x8700)), d)), fn));
        bottom = 0x8700;
    }

    /* the channel cursor: the packet's first quadword is its DMA tag */
    const u32 cursor = ctx + (u32)idx * 4u + 0x10u;
    u32 pkt;
    TRY(ld32(f, cursor, &pkt, fn));
    uint8_t *p = at(f, pkt, 0x90, 1, fn);
    if (!p) return -1;
    p[3] = 0x10;
    memset(p + 4, 0, 4);
    p[0] = 8;
    p[1] = 0;
    TRY(st32(f, cursor, pkt + 0x90u, fn));
    const u32 zero[4] = {0, 0, 0, 0};
    TRY(st128(f, pkt + 0x10, zero, fn));
    TRY(st32(f, pkt + 0x1C, 0x50000007u, fn));
    TRY(st64(f, pkt + 0x20, UINT64_C(0x602B400000008001), fn));
    TRY(st64(f, pkt + 0x28, 0x424216u, fn));
    const u32 dl = pkt + 0x30u;
    u32 s8, sc, t0, t1, t2, t3;
    TRY(st64(f, dl + 0x00, tex0, fn));
    TRY(st32(f, dl + 0x10, rgba & 0xFFu, fn));
    TRY(st32(f, dl + 0x14, rgba >> 8 & 0xFFu, fn));
    TRY(st32(f, dl + 0x18, rgba >> 16 & 0xFFu, fn));
    TRY(st32(f, dl + 0x1C, rgba >> 24 & 0xFFu, fn));
    TRY(ld32(f, SPR(0x3670), &t0, fn));
    TRY(st32(f, dl + 0x20, t0, fn));
    TRY(ld32(f, SPR(0x3674), &t1, fn));
    TRY(st32(f, dl + 0x24, t1, fn));
    TRY(st32(f, dl + 0x28, F_ONE, fn));
    TRY(st32(f, dl + 0x30, (u32)left, fn));
    TRY(st32(f, dl + 0x34, (u32)top, fn));
    TRY(ld32(f, SPR(0x3608), &s8, fn));
    TRY(st32(f, dl + 0x38, s8, fn));
    TRY(ld32(f, SPR(0x360C), &sc, fn));
    TRY(st32(f, dl + 0x3C, sc, fn));
    TRY(ld32(f, SPR(0x3678), &t2, fn));
    TRY(st32(f, dl + 0x40, t2, fn));
    TRY(ld32(f, SPR(0x367C), &t3, fn));
    TRY(st32(f, dl + 0x44, t3, fn));
    TRY(st32(f, dl + 0x48, F_ONE, fn));
    TRY(st32(f, dl + 0x50, (u32)right, fn));
    TRY(st32(f, dl + 0x54, (u32)bottom, fn));
    TRY(ld32(f, SPR(0x3608), &s8, fn));
    TRY(st32(f, dl + 0x58, s8, fn));
    TRY(ld32(f, SPR(0x360C), &sc, fn));
    TRY(st32(f, dl + 0x5C, sc, fn));
    *v0 = pkt + 0x10u;
    return 0;
}

int em_far_light_001CF320(F *f, u32 xy, u32 rgba, uint64_t tex0, u32 size)
{
    TRY(alive(f));
    const u32 fn = 0x001CF320u;
    const u32 hx = (u32)float_to_int(em_ee_mul_bits(0x42400000u, size)) * 0x10u;   /* 48 */
    const u32 hy = (u32)float_to_int(em_ee_mul_bits(0x41C00000u, size)) * 0x10u;   /* 24 */
    u32 v;
    if (!f->w_001CB5F0 || f->w_001CB5F0(f->ctx, D_007635C0, 0xFFF000, 8, &v) < 0)
        return fault(f, EM_FAR_LIGHT_FAULT_WORKER, 0x001CB5F0u, 0);
    const u32 zero[4] = {0, 0, 0, 0};
    u32 c[4], x, y;
    TRY(st128(f, v, zero, fn));
    TRY(st32(f, v + 0xC, 0x50000007u, fn));
    TRY(st64(f, v + 0x10, UINT64_C(0x602B400000008001), fn));
    TRY(st64(f, v + 0x18, 0x424216u, fn));
    TRY(st64(f, v + 0x20, tex0, fn));
    for (unsigned k = 0; k < 4; ++k) {
        TRY(ld32(f, rgba + 4 * k, &c[k], fn));
        TRY(st32(f, v + 0x30 + 4 * k, c[k], fn));
    }
    TRY(st32(f, v + 0x40, 0, fn));
    TRY(st32(f, v + 0x44, 0, fn));
    TRY(st32(f, v + 0x48, F_ONE, fn));
    TRY(ld32(f, xy, &x, fn));
    TRY(st32(f, v + 0x50, x + hx, fn));
    TRY(ld32(f, xy + 4, &y, fn));
    TRY(st32(f, v + 0x54, y + hy, fn));
    TRY(st32(f, v + 0x58, 0xFFFFFFu, fn));
    TRY(st32(f, v + 0x5C, 0, fn));
    TRY(st32(f, v + 0x60, F_ONE, fn));
    TRY(st32(f, v + 0x64, F_ONE, fn));
    TRY(st32(f, v + 0x68, F_ONE, fn));
    TRY(ld32(f, xy, &x, fn));
    TRY(st32(f, v + 0x70, x - hx, fn));
    TRY(ld32(f, xy + 4, &y, fn));
    TRY(st32(f, v + 0x74, y - hy, fn));
    TRY(st32(f, v + 0x78, 0xFFFFFFu, fn));
    TRY(st32(f, v + 0x7C, 0, fn));
    return 0;
}

int em_far_light_001CEFD0(F *f, const u32 pos[4], const u32 rgba[4])
{
    TRY(alive(f));
    const u32 fn = 0x001CEFD0u;
    EmFarLightVu *v = f->vu;
    u32 factor[7], size[7], ctx, clip[4], screen[4];
    const uint8_t *t = at(f, D_00251370, 0x40, 0, fn);   /* both tables, 0x20 apart */
    if (!t) return -1;
    memcpy(factor, t, sizeof factor);
    memcpy(size, t + 0x20, sizeof size);
    TRY(ld32(f, D_00275670, &ctx, fn));
    const u32 m = ctx + (2u << 6) + 0x2240u;              /* 001CD370(2) */
    for (unsigned k = 0; k < 4; ++k)
        TRY(ld128(f, m + 16 * k, v->vf[24 + k], fn));
    memcpy(v->vf[1], pos, 16);
    VU(f, em_vu_vec_bits(EM_VU_MULABC, 15, 0, v->vf[24], v->vf[1], 0, NULL, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDABC, 15, 1, v->vf[25], v->vf[1], 0, v->acc, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDABC, 15, 2, v->vf[26], v->vf[1], 0, v->acc, v->acc), fn);
    VU(f, em_vu_vec_bits(EM_VU_MADDBC, 15, 3, v->vf[27], VF0, 0, v->acc, v->vf[2]), fn);
    /* the clip test of x, y, z against |w| on DAZ'd magnitudes */
    for (unsigned k = 0; k < 4; ++k)
        if (em_eei_exp(v->vf[2][k]) == 0xFFu)
            return fault(f, EM_FAR_LIGHT_FAULT_UNMEASURED, fn, 0);
    u32 flags = 0, wmag = em_eei_daz(v->vf[2][3]) & 0x7FFFFFFFu;
    for (unsigned k = 0; k < 3; ++k) {
        const u32 x = em_eei_daz(v->vf[2][k]);
        if ((x & 0x7FFFFFFFu) > wmag)
            flags |= ((x >> 31) ? 2u : 1u) << (2 * k);
    }
    v->clip = ((v->clip << 6) | flags) & 0xFFFFFFu;
    if ((v->clip & 0x3Fu) != 0)
        return 0;

    TRY(camera_rows(f, fn));
    TRY(project(f, clip, screen, fn));
    TRY(st128(f, SPR(0x3600), screen, fn));
    u32 fog, sx, sy, r, k;
    TRY(ld32(f, SPR(0x360C), &fog, fn));
    if ((int32_t)fog < 0) return 0;
    TRY(ld32(f, SPR(0x360C), &fog, fn));
    if ((int32_t)fog >= 0x100) return 0;
    TRY(ld32(f, SPR(0x3600), &sx, fn));
    TRY(ld32(f, SPR(0x3604), &sy, fn));
    TRY(st32(f, SPR(0x3600), em_ee_cvt_s_w_bits((u32)(((int32_t)sx >> 4) - 0x800)), fn));
    TRY(st32(f, SPR(0x3604), em_ee_cvt_s_w_bits((u32)(((int32_t)sy >> 4) - 0x800)), fn));
    TRY(ld32(f, SPR(0x3600), &sx, fn));
    TRY(ld32(f, SPR(0x3604), &sy, fn));
    const u32 sq = em_ee_madd_bits(em_ee_mula_bits(sx, sx), sy, sy);
    if (!f->w_0011E748 || f->w_0011E748(f->ctx, sq, &r) < 0)
        return fault(f, EM_FAR_LIGHT_FAULT_WORKER, 0x0011E748u, 0);
    r = em_ee_div_bits(r, 0x43C4EC4Fu);                            /* 393.84616 */
    if (!em_ee_c_le_bits(r, F_ONE)) r = F_ONE;
    r = em_ee_sub_bits(F_ONE, r);
    TRY(st32(f, SPR(0x3A20), r, fn));
    u32 col[4];
    VU(f, em_sdk_vu0_00102900(col, rgba, r), fn);                  /* into 0x70003610 */
    for (k = 0; k < 4; ++k)
        col[k] = em_vu_ftoi0_bits(col[k]);                         /* 00102990 in place */
    TRY(st128(f, SPR(0x3610), col, fn));
    for (int32_t i = 0; i < 7; ++i) {
        TRY(ld32(f, SPR(0x3600), &sx, fn));
        TRY(st32(f, SPR(0x3A20), factor[i], fn));
        TRY(st32(f, SPR(0x3A24), size[i], fn));
        TRY(st32(f, SPR(0x3620), (u32)(float_to_int(em_ee_mul_bits(sx, factor[i])) + 0x800) << 4, fn));
        TRY(ld32(f, SPR(0x3604), &sy, fn));
        TRY(st32(f, SPR(0x3624), (u32)(float_to_int(em_ee_mul_bits(sy, factor[i])) + 0x800) << 4, fn));
        const uint64_t tex0 = (i & 1) ? UINT64_C(0x20045F0599421EB8) : UINT64_C(0x20045DA599421EA8);
        TRY(em_far_light_001CF320(f, SPR(0x3620), SPR(0x3610), tex0, size[i]));
    }
    if (!f->w_001CB900 || f->w_001CB900(f->ctx, D_007635C0, 0xFFF000, 2) < 0)
        return fault(f, EM_FAR_LIGHT_FAULT_WORKER, 0x001CB900u, 0);
    return 0;
}
