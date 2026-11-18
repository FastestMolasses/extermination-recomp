/* AREA21's Use classifier 001AAC00 with 001AA8D0 and 001AAA90
 * (em_area21_use.h: every function's behaviour). Translated from the
 * decomp's byte-identical C; loads and stores in the original's order. */
#include "game/em_area21_use.h"
#include "game/em_ee_float.h"
#include <string.h>

typedef struct { const EmArea21UseHooks *h; EmArea21UseFault *fault; } U;

static int latch(U *u, uint32_t address, int32_t code)
{
    if (u->fault && !u->fault->code) { u->fault->address = address; u->fault->code = code; }
    return -1;
}
static int load(U *u, uint32_t address, uint32_t size, uint32_t *out)
{
    uint8_t *p = u->h->bytes ? u->h->bytes(u->h->ctx, address, size, 0) : NULL;
    if (!p) return latch(u, address, EM_AREA21_USE_FAULT_BAD_ADDRESS);
    uint32_t v = 0;
    memcpy(&v, p, size);
    *out = v;
    return 0;
}
static int store(U *u, uint32_t address, uint32_t value)
{
    uint8_t *p = u->h->bytes ? u->h->bytes(u->h->ctx, address, 4, 1) : NULL;
    if (!p) return latch(u, address, EM_AREA21_USE_FAULT_BAD_ADDRESS);
    memcpy(p, &value, 4);
    return 0;
}
static int hook_f(U *u, uint32_t fn, int (*w)(void *, float, float *), uint32_t x, uint32_t *out)
{
    if (!w) return latch(u, fn, EM_AREA21_USE_FAULT_NULL_WORKER);
    float r = 0.0f;
    if (w(u->h->ctx, em_ee_float(x), &r) < 0) return latch(u, fn, EM_AREA21_USE_FAULT_WORKER_FAILED);
    *out = em_ee_bits(r);
    return 0;
}
static int transform(U *u, uint32_t out, uint32_t m, uint32_t v)
{
    if (!u->h->w_001026A0) return latch(u, 0x001026A0u, EM_AREA21_USE_FAULT_NULL_WORKER);
    if (u->h->w_001026A0(u->h->ctx, out, m, v) < 0)
        return latch(u, 0x001026A0u, EM_AREA21_USE_FAULT_WORKER_FAILED);
    return 0;
}
#define TRY(x) do { if ((x) < 0) return -1; } while (0)

enum {
    K_3 = 0x40400000u, K_3_5 = 0x40600000u, K_4 = 0x40800000u, K_8 = 0x41000000u,
    K_13 = 0x41500000u, K_12_6 = 0x4149999Au, K_PI = 0x40490FDBu, K_ONE = 0x3F800000u
};
#define SPAD 0x70003600u

/* sqrt(dx * dx + dz * dz) with the sum as MULA then MADD. */
static int planar(U *u, uint32_t dx, uint32_t dz, uint32_t *d)
{
    const uint32_t acc = em_ee_mula_bits(dx, dx);
    return hook_f(u, 0x0011E748u, u->h->w_0011E748, em_ee_madd_bits(acc, dz, dz), d);
}

static int t_001AA8D0(U *u, uint32_t a0, uint32_t e, uint32_t a1, uint32_t a2, int32_t *result)
{
    uint32_t y0, ey, ex, ez, x0, z0, d, side = 2;
    *result = 0;
    TRY(load(u, a0 + 0xA4, 4, &y0));
    TRY(load(u, e + 0xB4, 4, &ey));
    TRY(hook_f(u, 0x0011DF78u, u->h->w_0011DF78, em_ee_sub_bits(y0, ey), &d));
    if (!em_ee_c_le_bits(d, K_3)) return 0;
    TRY(load(u, e + 0xB0, 4, &ex));
    TRY(load(u, a0 + 0xA0, 4, &x0));
    TRY(load(u, a0 + 0xA8, 4, &z0));
    TRY(load(u, e + 0xB8, 4, &ez));
    TRY(planar(u, em_ee_sub_bits(x0, em_ee_add_bits(K_13, ex)), em_ee_sub_bits(z0, ez), &d));
    if (!em_ee_c_le_bits(d, K_3_5)) {
        side = 3;
        TRY(load(u, e + 0xB0, 4, &ex));
        TRY(load(u, a0 + 0xA0, 4, &x0));
        TRY(load(u, a0 + 0xA8, 4, &z0));
        TRY(load(u, e + 0xB8, 4, &ez));
        TRY(planar(u, em_ee_sub_bits(x0, em_ee_sub_bits(ex, K_13)), em_ee_sub_bits(z0, ez), &d));
        if (!em_ee_c_le_bits(d, K_3_5)) return 0;
    }
    TRY(load(u, e + 0xB0, 4, &ex));
    TRY(store(u, a1 + 0x0, side == 2 ? em_ee_add_bits(K_12_6, ex) : em_ee_sub_bits(ex, K_12_6)));
    TRY(load(u, a0 + 0xA4, 4, &y0));
    TRY(store(u, a1 + 0x4, y0));
    TRY(load(u, e + 0xB8, 4, &ez));
    TRY(store(u, a1 + 0x8, ez));
    TRY(store(u, a1 + 0xC, K_ONE));
    uint32_t yaw, wrapped;
    TRY(load(u, e + 0xC4, 4, &yaw));
    TRY(hook_f(u, 0x001B1470u, u->h->w_001B1470, em_ee_add_bits(K_PI, yaw), &wrapped));
    TRY(store(u, a2, wrapped));
    *result = (int32_t)side;
    return 0;
}

static int seed(U *u)
{
    TRY(store(u, SPAD + 0x0, 0));
    TRY(store(u, SPAD + 0x4, 0));
    TRY(store(u, SPAD + 0x8, K_8));
    return store(u, SPAD + 0xC, K_ONE);
}

static int t_001AAA90(U *u, uint32_t a0, uint32_t e, uint32_t a1, uint32_t a2, int32_t *result)
{
    uint32_t sx, sz, x0, z0, y0, ey, d;
    *result = 0;
    TRY(seed(u));
    TRY(transform(u, SPAD, e + 0xD0, SPAD));
    TRY(load(u, SPAD + 0x0, 4, &sx));
    TRY(load(u, a0 + 0xA0, 4, &x0));
    TRY(load(u, a0 + 0xA8, 4, &z0));
    TRY(load(u, SPAD + 0x8, 4, &sz));
    TRY(planar(u, em_ee_sub_bits(x0, sx), em_ee_sub_bits(z0, sz), &d));
    if (!em_ee_c_le_bits(d, K_8)) return 0;
    TRY(load(u, a0 + 0xA4, 4, &y0));
    TRY(load(u, e + 0xB4, 4, &ey));
    TRY(hook_f(u, 0x0011DF78u, u->h->w_0011DF78, em_ee_sub_bits(y0, ey), &d));
    if (!em_ee_c_le_bits(d, K_4)) return 0;
    TRY(seed(u));
    TRY(transform(u, a1, e + 0xD0, SPAD));
    uint32_t yaw, wrapped;
    TRY(load(u, e + 0xC4, 4, &yaw));
    TRY(hook_f(u, 0x001B1470u, u->h->w_001B1470, em_ee_add_bits(K_PI, yaw), &wrapped));
    TRY(store(u, a2, wrapped));
    *result = 1;
    return 0;
}

int em_area21_use_001AA8D0(const EmArea21UseHooks *h, uint32_t a0, uint32_t e, uint32_t a1,
                           uint32_t a2, int32_t *result, EmArea21UseFault *fault)
{
    U u = {h, fault};
    if (!h || !result) return -1;
    return t_001AA8D0(&u, a0, e, a1, a2, result);
}

int em_area21_use_001AAA90(const EmArea21UseHooks *h, uint32_t a0, uint32_t e, uint32_t a1,
                           uint32_t a2, int32_t *result, EmArea21UseFault *fault)
{
    U u = {h, fault};
    if (!h || !result) return -1;
    return t_001AAA90(&u, a0, e, a1, a2, result);
}

int em_area21_use_001AAC00(const EmArea21UseHooks *h, uint32_t a0, uint32_t a1, uint32_t a2,
                           int32_t *result, EmArea21UseFault *fault)
{
    U u = {h, fault};
    if (!h || !result) return -1;
    *result = 0;
    uint32_t gate, count, cursor;
    TRY(load(&u, 0x0028A9A0u, 2, &gate));
    if ((uint16_t)gate != 0) return 0;
    TRY(load(&u, 0x00275B84u, 2, &count));
    TRY(load(&u, 0x00275B7Cu, 4, &cursor));
    for (int32_t n = (int16_t)count; n != 0; --n, cursor += 4) {
        uint32_t e, tag, kind;
        int32_t r = 0;
        TRY(load(&u, cursor, 4, &e));
        TRY(load(&u, e, 1, &tag));
        if (tag != 1) continue;
        TRY(load(&u, e + 3, 1, &kind));
        if (kind == 0x4A) TRY(t_001AA8D0(&u, a0, e, a1, a2, &r));
        else if (kind == 0x4B) TRY(t_001AAA90(&u, a0, e, a1, a2, &r));
        else continue;
        if (r != 0) { *result = r; return 0; }
    }
    return 0;
}
