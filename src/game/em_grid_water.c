/* em_grid_water.c - see em_grid_water.h. */
#include "game/em_grid_water.h"

#include <string.h>

#include "game/em_ee_float.h"
#include "game/em_player_stage_workers.h"

typedef EmGridWater G;
typedef uint32_t u32;

#define D_00275670 0x00275670u   /* the render context pointer */
#define D_0081E0F0 0x0081E0F0u   /* the 16 x 16 surface table (0x18-byte records) */
#define TABLE_BYTES 0x1800u
#define SPR(o)     (0x70000000u + (o))

#define F_ZERO  0x00000000u
#define F_ONE   0x3F800000u

static int fault(G *g, int32_t code, u32 fn, u32 address)
{
    if (g->fault == EM_GRID_WATER_FAULT_NONE) {
        g->fault = code;
        g->fault_function = fn;
        g->fault_address = address;
    }
    return -1;
}

static uint8_t *at(G *g, u32 address, u32 size, int write, u32 fn)
{
    uint8_t *p = g->map ? g->map(g->ctx, address, size, write) : NULL;
    if (!p) fault(g, EM_GRID_WATER_FAULT_UNMAPPED, fn, address);
    return p;
}

static int ld32(G *g, u32 address, u32 *v, u32 fn)
{
    const uint8_t *p = at(g, address, 4, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 4);
    return 0;
}
static int ld64(G *g, u32 address, uint64_t *v, u32 fn)
{
    const uint8_t *p = at(g, address, 8, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 8);
    return 0;
}
static int st8(G *g, u32 address, uint8_t v, u32 fn)
{
    uint8_t *p = at(g, address, 1, 1, fn);
    if (!p) return -1;
    *p = v;
    return 0;
}
static int st16(G *g, u32 address, uint16_t v, u32 fn)
{
    uint8_t *p = at(g, address, 2, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 2);
    return 0;
}
static int st32(G *g, u32 address, u32 v, u32 fn)
{
    uint8_t *p = at(g, address, 4, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 4);
    return 0;
}
static int st64(G *g, u32 address, uint64_t v, u32 fn)
{
    uint8_t *p = at(g, address, 8, 1, fn);
    if (!p) return -1;
    memcpy(p, &v, 8);
    return 0;
}
static int ld128(G *g, u32 address, u32 v[4], u32 fn)
{
    const uint8_t *p = at(g, address & ~15u, 16, 0, fn);
    if (!p) return -1;
    memcpy(v, p, 16);
    return 0;
}
static int st128(G *g, u32 address, const u32 v[4], u32 fn)
{
    uint8_t *p = at(g, address & ~15u, 16, 1, fn);
    if (!p) return -1;
    memcpy(p, v, 16);
    return 0;
}

#define TRY(x) do { if ((x) < 0) return -1; } while (0)

static int alive(G *g)
{
    if (!g) return -1;
    if (g->fault != EM_GRID_WATER_FAULT_NONE) return -1;
    return 0;
}

static int hook(G *g, int ok, u32 address)
{
    return ok ? 0 : fault(g, EM_GRID_WATER_FAULT_WORKER, address, 0);
}

/* The channel's cursor slot: ctx + 0x10 + 4 chan. */
static int cursor_slot(G *g, int32_t chan, u32 *slot, u32 fn)
{
    u32 ctx;
    TRY(ld32(g, D_00275670, &ctx, fn));
    *slot = ctx + 0x10u + (u32)chan * 4u;
    return 0;
}

/* The CNT tag every row starts with (byte +3 = 0x10, word +4 = 0, half +0 =
 * qwc; byte +2 is not written), the cursor advanced by `advance`; *q gets
 * the tag + 0x10. */
static int row_tag(G *g, u32 slot, uint16_t qwc, u32 advance, u32 *q, u32 fn)
{
    u32 cur;
    TRY(ld32(g, slot, &cur, fn));
    TRY(st8(g, cur + 3u, 0x10, fn));
    TRY(ld32(g, slot, &cur, fn));
    TRY(st32(g, cur + 4u, 0, fn));
    TRY(ld32(g, slot, &cur, fn));
    TRY(st16(g, cur, qwc, fn));
    TRY(ld32(g, slot, &cur, fn));
    TRY(st32(g, slot, cur + advance, fn));
    *q = cur + 0x10u;
    return 0;
}

/* ------------------------------------------------------------------ */

int em_grid_water_001E1760(G *g, int32_t chan)
{
    TRY(alive(g));
    const u32 fn = 0x001E1760u;
    u32 ctx;
    uint64_t tex0;
    TRY(hook(g, g->w_001D6E60 && g->w_001D6E60(g->ctx, chan, 0x258000, 8, 8) >= 0, 0x001D6E60u));
    TRY(hook(g, g->w_001D1F80 && g->w_001D1F80(g->ctx, chan, 0, 7) >= 0, 0x001D1F80u));
    TRY(hook(g, g->w_001D1FF0 && g->w_001D1FF0(g->ctx, chan, 1) >= 0, 0x001D1FF0u));
    TRY(ld32(g, D_00275670, &ctx, fn));
    TRY(ld64(g, ctx + 0x1E0u, &tex0, fn));
    TRY(hook(g, g->w_001D6F60 && g->w_001D6F60(g->ctx, chan, tex0, 0x80) >= 0, 0x001D6F60u));
    TRY(em_grid_water_001E10A0(g));
    TRY(em_grid_water_001E13E0(g, chan));
    TRY(hook(g, g->w_001D1F20 && g->w_001D1F20(g->ctx, chan) >= 0, 0x001D1F20u));
    return 0;
}

/* Record (i, j) of the table: x +0, y +4, z +8, w +0xC, u +0x10, v +0x14. */
#define REC(t, i, j) ((t) + ((u32)(i) * 16u + (u32)(j)) * 0x18u)
static u32 rf(const uint8_t *p, u32 off) { u32 v; memcpy(&v, p + off, 4); return v; }
static void wf(uint8_t *p, u32 off, u32 v) { memcpy(p + off, &v, 4); }

int em_grid_water_001E10A0(G *g)
{
    TRY(alive(g));
    const u32 fn = 0x001E10A0u;
    uint8_t *t = at(g, D_0081E0F0, TABLE_BYTES, 1, fn);
    if (!t) return -1;
    for (int i = 0; i < 16; ++i) {
        const int im1 = (i - 1) & 0xF, ip1 = (i + 1) & 0xF;
        const u32 row_v = em_ee_div_bits(em_ee_cvt_s_w_bits((u32)i), 0x41800000u);   /* i / 16 */
        for (int j = 0; j < 16; ++j) {
            uint8_t *c = REC(t, i, j);
            u32 sum = F_ZERO;
            sum = em_ee_add_bits(sum, rf(REC(t, im1, j), 8));
            sum = em_ee_add_bits(sum, rf(REC(t, ip1, j), 8));
            sum = em_ee_add_bits(sum, rf(REC(t, i, (j - 1) & 0xF), 8));
            sum = em_ee_add_bits(sum, rf(REC(t, i, (j + 1) & 0xF), 8));
            sum = em_ee_sub_bits(sum, em_ee_add_bits(0x3E800000u, em_ee_mul_bits(0x40800000u, rf(c, 8))));
            const u32 w = em_ee_mul_bits(rf(c, 0xC), 0x3F760419u);                   /* 0.961 */
            wf(c, 0xC, w);
            wf(c, 0xC, em_ee_add_bits(w, em_ee_mul_bits(0x3B102DE0u, sum)));          /* 0.0022 */
            const u32 col_u = em_ee_div_bits(em_ee_cvt_s_w_bits((u32)j), 0x41800000u);
            wf(c, 0x10, em_ee_add_bits(col_u, em_ee_mul_bits(0x3CF5C28Fu,             /* 0.03 */
                                                              em_ee_sub_bits(rf(c, 8), rf(REC(t, i, (j + 1) & 0xF), 8)))));
            wf(c, 0x14, em_ee_add_bits(row_v, em_ee_mul_bits(0x3CF5C28Fu,
                                                              em_ee_sub_bits(rf(c, 8), rf(REC(t, ip1, j), 8)))));
        }
    }
    int32_t r;
    TRY(hook(g, g->w_00122BB8 && g->w_00122BB8(g->ctx, &r) >= 0, 0x00122BB8u));
    const int a = (r >> 6) & 0xF;
    TRY(hook(g, g->w_00122BB8 && g->w_00122BB8(g->ctx, &r) >= 0, 0x00122BB8u));
    const int b = (r >> 6) & 0xF;
    static const struct { int di, dj; u32 k; } kick[9] = {
        {0, 0, 0x3F99999Au},                                            /* 1.2 */
        {0, -1, 0x3F666667u}, {0, 1, 0x3F666667u}, {-1, 0, 0x3F666667u}, {1, 0, 0x3F666667u},
        {-1, -1, 0x3F0A3D71u}, {-1, 1, 0x3F0A3D71u}, {1, -1, 0x3F0A3D71u}, {1, 1, 0x3F0A3D71u},
    };
    for (unsigned k = 0; k < 9; ++k) {
        uint8_t *c = REC(t, (b + kick[k].di) & 0xF, (a + kick[k].dj) & 0xF);
        wf(c, 0xC, em_ee_add_bits(rf(c, 0xC), kick[k].k));
    }
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j) {
            uint8_t *c = REC(t, i, j);
            wf(c, 8, em_ee_add_bits(rf(c, 8), rf(c, 0xC)));
        }
    return 0;
}

static int32_t to_int(u32 bits) { return em_player_float_to_int(bits); }

/* 30720 + 16 c (12.4 screen coordinates around 2048). */
static u32 screen(u32 c) { return em_ee_add_bits(0x46F00000u, em_ee_mul_bits(0x41800000u, c)); }

/* One vertex of 001E13E0: ST at q, RGBAQ at q + 0x10, XYZ2 at q + 0x20. */
static int water_vertex(G *g, u32 q, u32 s, u32 t, int32_t x, int32_t y, u32 fn)
{
    TRY(st32(g, q + 0x0u, s, fn));
    TRY(st32(g, q + 0x4u, t, fn));
    TRY(st32(g, q + 0x8u, F_ONE, fn));
    TRY(st32(g, q + 0x10u, 0x40, fn));
    TRY(st32(g, q + 0x14u, 0x40, fn));
    TRY(st32(g, q + 0x18u, 0x40, fn));
    TRY(st32(g, q + 0x1Cu, 0x80, fn));
    TRY(st32(g, q + 0x20u, (u32)x, fn));
    TRY(st32(g, q + 0x24u, (u32)y, fn));
    TRY(st32(g, q + 0x28u, 0, fn));
    TRY(st32(g, q + 0x2Cu, 0, fn));
    return 0;
}

int em_grid_water_001E13E0(G *g, int32_t chan)
{
    TRY(alive(g));
    const u32 fn = 0x001E13E0u;
    const uint8_t *t = at(g, D_0081E0F0, TABLE_BYTES, 0, fn);
    if (!t) return -1;
    u32 slot;
    TRY(cursor_slot(g, chan, &slot, fn));
    for (int i = 0; i < 16; ++i) {
        const int k = (i + 1) & 0xF;
        u32 q;
        TRY(cursor_slot(g, chan, &slot, fn));
        TRY(row_tag(g, slot, 0x68, 0x690u, &q, fn));
        const u32 vbias = i == 0xF ? F_ONE : F_ZERO;
        const u32 bias = em_ee_cvt_s_w_bits(i == 0xF ? 0x1000u : 0u);
        TRY(st128(g, q, (const u32[4]){0, 0, 0, 0}, fn));
        TRY(st32(g, q + 0xCu, 0x50000067u, fn));
        TRY(st64(g, q + 0x10u, UINT64_C(0x600A400000008011), fn));
        TRY(st64(g, q + 0x18u, UINT64_C(0x512512), fn));
        q += 0x20u;
        for (int j = 0; j < 16; ++j) {
            const uint8_t *a = REC(t, i, j), *b = REC(t, k, j);
            TRY(water_vertex(g, q, rf(a, 0x10), rf(a, 0x14), to_int(screen(rf(a, 0))), to_int(screen(rf(a, 4))), fn));
            TRY(water_vertex(g, q + 0x30u, rf(b, 0x10), em_ee_add_bits(vbias, rf(b, 0x14)),
                             to_int(screen(rf(b, 0))), to_int(em_ee_add_bits(bias, screen(rf(b, 4)))), fn));
            q += 0x60u;
        }
        const uint8_t *row = REC(t, i, 0), *next = REC(t, k, 0);
        TRY(water_vertex(g, q, em_ee_add_bits(F_ONE, rf(row, 0x10)), rf(row, 0x14), 0x8800,
                         to_int(screen(rf(row, 4))), fn));
        TRY(water_vertex(g, q + 0x30u, em_ee_add_bits(F_ONE, rf(next, 0x10)), em_ee_add_bits(vbias, rf(next, 0x14)),
                         0x8800, to_int(em_ee_add_bits(bias, screen(rf(next, 4)))), fn));
    }
    return 0;
}

/* 93.75 n - 46.875 */
static u32 grid_coord(int n) { return em_ee_sub_bits(em_ee_mul_bits(0x42BB8000u, em_ee_cvt_s_w_bits((u32)n)), 0x423B8000u); }

int em_grid_water_001E17E0(G *g, int32_t chan, u32 *v0)
{
    TRY(alive(g));
    const u32 fn = 0x001E17E0u;
    if (!g->vu) return fault(g, EM_GRID_WATER_FAULT_WORKER, fn, 0);
    EmFarLightVu *v = g->vu;
    u32 slot, first, ctx;
    TRY(cursor_slot(g, chan, &slot, fn));
    TRY(ld32(g, slot, &first, fn));
    TRY(ld32(g, D_00275670, &ctx, fn));
    const u32 m = ctx + (2u << 6) + 0x2240u;                 /* 001CD370(2) */
    for (unsigned k = 0; k < 4; ++k)
        TRY(ld128(g, m + 16 * k, v->vf[24 + k], fn));
    for (unsigned k = 0; k < 4; ++k)
        TRY(ld128(g, SPR(0x3AC0) + 16 * k, v->vf[28 + k], fn));
    const u32 height = 0x41200000u;                          /* 10 */
    for (int i = -8; i < 8; ++i) {
        const u32 z[2] = {grid_coord(i), grid_coord(i + 1)};
        u32 q;
        TRY(cursor_slot(g, chan, &slot, fn));
        TRY(row_tag(g, slot, 0x22, 0x230u, &q, fn));
        TRY(st128(g, q, (const u32[4]){0, 0, 0, 0}, fn));
        TRY(st32(g, q + 0xCu, 0x50000021u, fn));
        TRY(st64(g, q + 0x10u, UINT64_C(0x2002400000008010), fn));
        TRY(st64(g, q + 0x18u, UINT64_C(0x44), fn));
        q += 0x20u;
        int32_t adc = 0;
        for (int j = -8; j < 8; ++j) {
            const u32 x = grid_coord(j);
            for (unsigned h = 0; h < 2; ++h) {
                const u32 out = q + 0x10u * h;
                u32 clip[4], scr[4];
                adc >>= 1;
                v->vf[1][0] = x;
                v->vf[1][1] = height;
                v->vf[1][2] = z[h];
                v->vf[1][3] = F_ONE;
                if (em_far_light_vu_clip(v) < 0)
                    return fault(g, EM_GRID_WATER_FAULT_UNMEASURED, fn, 0);
                if ((v->clip & 0x3Fu) != 0) adc |= 4;
                if (em_far_light_vu_project(v, clip, scr) < 0)
                    return fault(g, EM_GRID_WATER_FAULT_UNMEASURED, fn, 0);
                TRY(st128(g, out, scr, fn));
                if (adc != 0) TRY(st32(g, out + 0xCu, 0xFFFFu, fn));
            }
            q += 0x20u;
        }
    }
    *v0 = first;
    return 0;
}
