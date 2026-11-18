/* em_haze_ring.c - see em_haze_ring.h. */
#include "game/em_haze_ring.h"

#include <string.h>

#include "game/em_ee_float.h"
#include "game/em_player_stage_workers.h"

typedef EmHazeRing H;
typedef uint32_t u32;

#define D_00275670 0x00275670u   /* the render context pointer */
#define D_0026E9A0 0x0026E9A0u   /* 001E76A0's style words (style[3]: the centre alpha) */
#define D_00253530 0x00253530u   /* 001E0750's six corner offsets (x, y) */
#define SPR(o)     (0x70000000u + (o))

static const u32 VF0[4] = {0, 0, 0, 0x3F800000u};
#define F_ONE    0x3F800000u
#define F_ZERO   0x00000000u
#define F_PI     0x40490FDBu
#define F_TWO    0x40000000u
#define F_64     0x42800000u
#define F_300    0x43960000u
#define F_75     0x42960000u
#define F_30     0x41F00000u
#define F_256    0x43800000u
#define F_8      0x41000000u
#define F_2_M31  0x30000000u     /* 2^-31 */
#define F_S_SPAN 0x39CCCCCDu     /* 0.000390625 */
#define F_S_BASE 0xB94CCCCDu     /* -0.0001953125 */
#define F_T_SPAN 0x3A6A0EA1u     /* 0.00089285715 */
#define F_T_BASE 0xB9EA0EA1u     /* -0.00044642857 */
#define F_2_M13  0x39000000u     /* 1 / 8192 */
#define F_S_OFF  0xC0600000u     /* -3.5 */
#define F_2_M12  0x39800000u     /* 1 / 4096 */
#define F_T_OFF  0xC0F20000u     /* -7.5625 */

static int fault(H *h, int32_t code, u32 fn, u32 address)
{
    if (h->fault == EM_HAZE_RING_FAULT_NONE) {
        h->fault = code;
        h->fault_function = fn;
        h->fault_address = address;
    }
    return -1;
}

static uint8_t *at(H *h, u32 address, u32 size, int write, u32 fn)
{
    uint8_t *p = h->map ? h->map(h->ctx, address, size, write) : NULL;
    if (!p) fault(h, EM_HAZE_RING_FAULT_UNMAPPED, fn, address);
    return p;
}

static int ld8(H *h, u32 address, u32 *v, u32 fn)
{
    const uint8_t *p = at(h, address, 1, 0, fn);
    if (!p) return -1;
    *v = *p;
    return 0;
}
static int st8(H *h, u32 address, u32 v, u32 fn)
{
    uint8_t *p = at(h, address, 1, 1, fn);
    if (!p) return -1;
    *p = (uint8_t)v;
    return 0;
}
static int st16(H *h, u32 address, u32 v, u32 fn)
{
    uint8_t *p = at(h, address, 2, 1, fn);
    if (!p) return -1;
    const uint16_t x = (uint16_t)v;
    memcpy(p, &x, 2);
    return 0;
}
static int ld32(H *h, u32 address, u32 *v, u32 fn)
{
    const uint8_t *p = at(h, address, 4, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 4);
    return 0;
}
static int st32(H *h, u32 address, u32 v, u32 fn)
{
    uint8_t *p = at(h, address, 4, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 4);
    return 0;
}
static int st64(H *h, u32 address, uint64_t v, u32 fn)
{
    uint8_t *p = at(h, address, 8, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 8);
    return 0;
}
static int ld128(H *h, u32 address, u32 v[4], u32 fn)
{
    const uint8_t *p = at(h, address & ~15u, 16, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 16);
    return 0;
}
static int st128(H *h, u32 address, const u32 v[4], u32 fn)
{
    uint8_t *p = at(h, address & ~15u, 16, 1, fn);
    if (!p) return -1;
    memcpy(p, v, 16);
    return 0;
}

#define TRY(x) do { if ((x) < 0) return -1; } while (0)
#define VUOK(h, x, fn) do { if ((x) != EM_EE_FLOAT_OK) return fault((h), EM_HAZE_RING_FAULT_UNMEASURED, (fn), 0); } while (0)

static int alive(H *h)
{
    if (!h) return -1;
    if (h->fault != EM_HAZE_RING_FAULT_NONE) return -1;
    if (!h->vu) return fault(h, EM_HAZE_RING_FAULT_WORKER, 0, 0);
    return 0;
}

/* A hook: NULL or failing latches WORKER at its address. */
#define HOOK(h, name, fn, ...) do { \
    if (!(h)->name) return fault((h), EM_HAZE_RING_FAULT_WORKER, (fn), 0); \
    if ((h)->name((h)->ctx, __VA_ARGS__) < 0) return fault((h), EM_HAZE_RING_FAULT_WORKER, (fn), 0); \
} while (0)

/* ------------------------------------------------------------------ */

/* 001E0750 (frame 0xA0): the clip test, the projection and the six
 * vertices, in the original instructions' order. */
int em_haze_ring_001E0750(H *h, const u32 pos[4], u32 style, u32 sp)
{
    TRY(alive(h));
    const u32 fn = 0x001E0750u;
    const u32 csp = sp - 0xA0u;   /* the callees' stack pointer */
    EmFarLightVu *v = h->vu;
    u32 ctx;

    /* m = 001CD370(0): the context + 0x2240; vf24..vf27 its rows */
    TRY(ld32(h, D_00275670, &ctx, fn));
    const u32 m = ctx + 0x2240u;
    for (unsigned k = 0; k < 4; ++k)
        TRY(ld128(h, m + 16u * k, v->vf[24 + k], fn));
    memcpy(v->vf[1], pos, 16);
    if (em_far_light_vu_clip(v) < 0)
        return fault(h, EM_HAZE_RING_FAULT_UNMEASURED, fn, 0);
    if ((v->clip & 0x3Fu) != 0)
        return 0;

    u32 alpha;
    TRY(ld32(h, style + 0xCu, &alpha, fn));
    const u32 centre[4] = {0x80u, 0x80u, 0x80u, alpha};
    const u32 edge[4] = {0x80u, 0x80u, 0x80u, 0u};

    for (unsigned k = 0; k < 4; ++k)
        TRY(ld128(h, SPR(0x3AC0) + 16u * k, v->vf[28 + k], fn));
    memcpy(v->vf[1], pos, 16);
    VUOK(h, em_vu_vec_bits(EM_VU_MULABC, 15, 0, v->vf[28], v->vf[1], 0, NULL, v->acc), fn);
    VUOK(h, em_vu_vec_bits(EM_VU_MADDABC, 15, 1, v->vf[29], v->vf[1], 0, v->acc, v->acc), fn);
    VUOK(h, em_vu_vec_bits(EM_VU_MADDABC, 15, 2, v->vf[30], v->vf[1], 0, v->acc, v->acc), fn);
    VUOK(h, em_vu_vec_bits(EM_VU_MADDBC, 15, 3, v->vf[31], VF0, 0, v->acc, v->vf[2]), fn);
    VUOK(h, em_vu_div_bits(VF0[3], v->vf[2][3], 3, 3, &v->q), fn);
    u32 clip[4];
    memcpy(clip, v->vf[2], 16);                               /* the stack copy */
    VUOK(h, em_vu_vec_bits(EM_VU_MULQ, 12, -1, v->vf[2], NULL, v->q, NULL, v->vf[2]), fn);
    /* vf3 = the 128-bit GPR the zero float was moved into: x = 0 and y = its
     * sign extension; z and w (the GPR's upper half) are not modelled. */
    v->vf[3][0] = F_ZERO;
    v->vf[3][1] = 0;
    VUOK(h, em_vu_vec_bits(EM_VU_SUBBC, 1, 0, v->vf[2], v->vf[3], 0, NULL, v->vf[2]), fn);
    VUOK(h, em_vu_div_bits(VF0[3], v->vf[2][3], 3, 3, &v->q), fn);
    VUOK(h, em_vu_vec_bits(EM_VU_MULQ, 2, -1, v->vf[2], NULL, v->q, NULL, v->vf[2]), fn);
    VUOK(h, em_vu_vec_bits(EM_VU_MULABC, 1, 2, VF0, v->vf[23], 0, NULL, v->acc), fn);
    VUOK(h, em_vu_vec_bits(EM_VU_MADDBC, 1, 3, v->vf[23], v->vf[2], 0, v->acc, v->vf[2]), fn);
    v->vf[2][3] = em_vu_min_bits(v->vf[2][3], v->vf[23][0]);
    v->vf[2][3] = em_vu_max_bits(v->vf[2][3], VF0[0]);
    for (unsigned k = 0; k < 4; ++k)
        v->vf[2][k] = em_vu_ftoi4_bits(v->vf[2][k]);
    TRY(st128(h, SPR(0x3600), v->vf[2], fn));

    const u32 scale = em_ee_mul_bits(F_8, em_ee_div_bits(F_256, clip[3]));
    u32 corner[12];
    const uint8_t *cp = at(h, D_00253530, 0x30, 0, fn);
    if (!cp) return -1;
    memcpy(corner, cp, sizeof corner);

    /* the CNT tag, the DIRECT word and the GIF tag at the channel-3 cursor */
    u32 cur;
    TRY(ld32(h, D_00275670, &ctx, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st8(h, cur + 3u, 0x10u, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st32(h, cur + 4u, 0, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st16(h, cur, 0x14u, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st32(h, ctx + 0x1Cu, cur + 0x150u, fn));
    static const u32 zero[4] = {0, 0, 0, 0};
    TRY(st128(h, cur + 0x10u, zero, fn));
    TRY(st32(h, cur + 0x1Cu, 0x50000013u, fn));
    TRY(st64(h, cur + 0x20u, (uint64_t)0x302EC000u << 32 | 0x8006u, fn));
    TRY(st64(h, cur + 0x28u, 0x421u, fn));
    u32 vtx = cur + 0x30u;

    for (unsigned i = 0; i < 6; ++i) {
        int32_t r;
        HOOK(h, w_00122BB8, 0x00122BB8u, csp, &r);
        u32 t = em_ee_mul_bits(F_2_M31, em_ee_cvt_s_w_bits((u32)r));
        t = em_ee_add_bits(F_S_BASE, em_ee_mul_bits(F_S_SPAN, t));
        const u32 jitter_s = em_ee_mul_bits(scale, t);
        HOOK(h, w_00122BB8, 0x00122BB8u, csp, &r);
        t = em_ee_mul_bits(F_2_M31, em_ee_cvt_s_w_bits((u32)r));
        t = em_ee_add_bits(F_T_BASE, em_ee_mul_bits(F_T_SPAN, t));
        const u32 jitter_t = em_ee_mul_bits(scale, t);

        TRY(st128(h, vtx, i == 0 ? centre : edge, fn));
        u32 sx, sy, sz, wx, wy;
        TRY(ld32(h, SPR(0x3600), &sx, fn));
        TRY(st32(h, vtx + 0x20u, (u32)em_player_float_to_int(
                     em_ee_add_bits(em_ee_cvt_s_w_bits(sx), em_ee_mul_bits(scale, corner[2 * i]))), fn));
        TRY(ld32(h, SPR(0x3604), &sy, fn));
        TRY(st32(h, vtx + 0x24u, (u32)em_player_float_to_int(
                     em_ee_add_bits(em_ee_cvt_s_w_bits(sy), em_ee_mul_bits(scale, corner[2 * i + 1]))), fn));
        TRY(ld32(h, SPR(0x3608), &sz, fn));
        TRY(st32(h, vtx + 0x28u, sz, fn));
        TRY(st32(h, vtx + 0x2Cu, 0, fn));
        TRY(ld32(h, vtx + 0x20u, &wx, fn));
        TRY(st32(h, vtx + 0x10u, em_ee_add_bits(jitter_s, em_ee_add_bits(F_S_OFF,
                     em_ee_mul_bits(F_2_M13, em_ee_cvt_s_w_bits(wx)))), fn));
        TRY(ld32(h, vtx + 0x24u, &wy, fn));
        TRY(st32(h, vtx + 0x14u, em_ee_add_bits(jitter_t, em_ee_add_bits(F_T_OFF,
                     em_ee_mul_bits(F_2_M12, em_ee_cvt_s_w_bits(wy)))), fn));
        TRY(st32(h, vtx + 0x18u, F_ONE, fn));
        vtx += 0x30u;
    }
    return 0;
}

/* 001E0AC0 (frame 0x70): the 64-step ring. */
int em_haze_ring_001E0AC0(H *h, u32 p, u32 style, u32 fx, u32 fy, u32 sp)
{
    TRY(alive(h));
    const u32 fn = 0x001E0AC0u;
    const u32 csp = sp - 0x70u;
    u32 ctx, buf, cur;
    TRY(ld32(h, D_00275670, &ctx, fn));
    TRY(ld32(h, ctx + 0x1Cu, &buf, fn));
    HOOK(h, w_001D1F80, 0x001D1F80u, csp, 3, 2, 1);
    HOOK(h, w_001D7510, 0x001D7510u, csp, 3, 0, 0);
    HOOK(h, w_001D1FF0, 0x001D1FF0u, csp, 3, 2);
    for (int32_t i = 0; i < 0x40; ++i) {
        u32 a = em_ee_mul_bits(F_PI, em_ee_cvt_s_w_bits((u32)i));
        a = em_ee_mul_bits(F_TWO, a);
        a = em_ee_sub_bits(em_ee_div_bits(a, F_64), F_PI);
        u32 s, c, q[4];
        HOOK(h, w_0011DE90, 0x0011DE90u, csp, a, &s);
        const u32 da = em_ee_mul_bits(fx, s);
        HOOK(h, w_0011E2A8, 0x0011E2A8u, csp, a, &c);
        const u32 db = em_ee_mul_bits(fy, c);
        TRY(ld32(h, p, &q[0], fn));
        q[0] = em_ee_add_bits(q[0], da);
        TRY(ld32(h, p + 4u, &q[1], fn));
        q[1] = em_ee_add_bits(q[1], db);
        TRY(ld32(h, p + 8u, &q[2], fn));
        TRY(ld32(h, p + 0xCu, &q[3], fn));
        TRY(em_haze_ring_001E0750(h, q, style, csp));
    }
    TRY(ld32(h, D_00275670, &ctx, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st8(h, cur + 3u, 0x60u, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st32(h, cur + 4u, 0, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st16(h, cur, 0, fn));
    TRY(ld32(h, ctx + 0x1Cu, &cur, fn));
    TRY(st32(h, ctx + 0x1Cu, cur + 0x10u, fn));
    HOOK(h, w_001CAAC0, 0x001CAAC0u, csp, p, buf);
    return 0;
}

/* 001E76A0 (frame 0x20): the node's state machine. */
int em_haze_ring_001E76A0(H *h, u32 self, u32 sp)
{
    TRY(alive(h));
    const u32 fn = 0x001E76A0u;
    const u32 csp = sp - 0x20u;
    u32 state;
    TRY(ld8(h, self + 4u, &state, fn));
    if (state == 3 || state == 2) {
        HOOK(h, w_001AFC10, 0x001AFC10u, csp, self);
        return 0;
    }
    if (state == 0) {
        TRY(st8(h, self + 4u, 1u, fn));
        TRY(st32(h, self + 0x20u, F_300, fn));
    } else if (state != 1) {
        return 0;
    }
    HOOK(h, w_001028B8, 0x001028B8u, csp, self + 0xB0u, self + 0xB0u, self + 0xC0u);
    u32 timer;
    TRY(ld32(h, self + 0x20u, &timer, fn));
    timer = em_ee_sub_bits(timer, F_ONE);
    TRY(st32(h, self + 0x20u, timer, fn));
    if (em_ee_c_le_bits(timer, F_ZERO))
        TRY(st8(h, self + 4u, 3u, fn));
    return em_haze_ring_001E0AC0(h, self + 0xB0u, D_0026E9A0, F_75, F_30, csp);
}
