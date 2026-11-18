/* em_haze_ring_live.c - see em_haze_ring_live.h. */
#include "game/em_haze_ring_live.h"

#include <stdio.h>
#include <string.h>

#include "game/em_haze_ring.h"

/* The VU0 registers 001E0750 writes (vf1, vf2, vf3.xy, vf24..vf31, ACC, Q,
 * CLIP) are loaded by it before use, so only vf23 (the fog ramp it reads as
 * its caller left it) and vf0 need a value at entry. */
static EmFarLightVu s_vu;

static uint8_t *map(void *ctx, uint32_t address, uint32_t size, int write)
{
    EmArea01Runtime *r = ctx;
    return r->host.bytes ? r->host.bytes(r->host.ctx, address, size, write) : NULL;
}

static int call(EmArea01Runtime *r, uint32_t sp, EmArea01Call *c)
{
    c->sp = sp - EM_AREA01_RUNTIME_NEST;
    return em_area01_runtime_call(r, c);
}

static int w_001028B8(void *ctx, uint32_t sp, uint32_t d, uint32_t a, uint32_t b)
{
    EmArea01Call c = {.function = 0x001028B8u, .na = 3, .a = {d, a, b}};
    return call(ctx, sp, &c);
}
static int w_001AFC10(void *ctx, uint32_t sp, uint32_t self)
{
    EmArea01Call c = {.function = 0x001AFC10u, .na = 1, .a = {self}};
    return call(ctx, sp, &c);
}
static int chan3(void *ctx, uint32_t sp, uint32_t fn, unsigned na, int32_t a0, int32_t a1, int32_t a2)
{
    EmArea01Call c = {.function = fn, .na = na};
    c.a[0] = (uint64_t)(int64_t)a0;
    c.a[1] = (uint64_t)(int64_t)a1;
    c.a[2] = (uint64_t)(int64_t)a2;
    return call(ctx, sp, &c);
}
static int w_001D1F80(void *ctx, uint32_t sp, int32_t a0, int32_t a1, int32_t a2)
{
    return chan3(ctx, sp, 0x001D1F80u, 3, a0, a1, a2);
}
static int w_001D7510(void *ctx, uint32_t sp, int32_t a0, int32_t a1, int32_t a2)
{
    return chan3(ctx, sp, 0x001D7510u, 3, a0, a1, a2);
}
static int w_001D1FF0(void *ctx, uint32_t sp, int32_t a0, int32_t a1)
{
    return chan3(ctx, sp, 0x001D1FF0u, 2, a0, a1, 0);
}
static int trig(void *ctx, uint32_t sp, uint32_t fn, uint32_t x, uint32_t *result)
{
    EmArea01Call c = {.function = fn, .nf = 1};
    c.f[0] = x;
    const int rc = call(ctx, sp, &c);
    if (rc == 0) *result = c.f0;
    return rc;
}
static int w_0011DE90(void *ctx, uint32_t sp, uint32_t x, uint32_t *result)
{
    return trig(ctx, sp, 0x0011DE90u, x, result);
}
static int w_0011E2A8(void *ctx, uint32_t sp, uint32_t x, uint32_t *result)
{
    return trig(ctx, sp, 0x0011E2A8u, x, result);
}
static int w_00122BB8(void *ctx, uint32_t sp, int32_t *result)
{
    EmArea01Call c = {.function = 0x00122BB8u};
    const int rc = call(ctx, sp, &c);
    if (rc == 0) *result = (int32_t)c.v0;
    return rc;
}
static int w_001CAAC0(void *ctx, uint32_t sp, uint32_t pos, uint32_t payload)
{
    EmArea01Call c = {.function = 0x001CAAC0u, .na = 2, .a = {pos, payload}};
    return call(ctx, sp, &c);
}

int em_haze_ring_live_001E76A0(EmArea01Runtime *r, uint32_t sp, uint32_t node, uint32_t *where, int32_t *code)
{
    EmHazeRing h;
    memset(&h, 0, sizeof h);
    h.ctx = r;
    h.map = map;
    h.vu = &s_vu;
    h.w_001028B8 = w_001028B8;
    h.w_001AFC10 = w_001AFC10;
    h.w_001D1F80 = w_001D1F80;
    h.w_001D7510 = w_001D7510;
    h.w_001D1FF0 = w_001D1FF0;
    h.w_0011DE90 = w_0011DE90;
    h.w_0011E2A8 = w_0011E2A8;
    h.w_00122BB8 = w_00122BB8;
    h.w_001CAAC0 = w_001CAAC0;
    memcpy(s_vu.vf[0], (const uint32_t[4]){0, 0, 0, 0x3F800000u}, 16);
    /* vf23 (open item, docs/AREA21_PORT.md "Haze ring"): 001E0750 reads it
     * as the logic walk left it. Measured in the original at every 001E0750
     * entry of g77 rows 5922..6281 (57600 calls, 900 rings): constant within
     * a ring; the fog quadword ctx +0xA0 in 676 rings, another owner's
     * leftover (a matrix row) in 224. The port starts every ring from ctx
     * +0xA0. vf23 shapes only the fog lane: ACC.w, vf2.w and the scratch
     * word 0x7000360C (001E0750 copies only the x, y, z words into its
     * packet); a later read of that word before its next store would differ. */
    {
        uint32_t ctx = 0;
        const uint8_t *word = map(r, 0x00275670u, 4, 0);
        if (word) memcpy(&ctx, word, 4);
        const uint8_t *fog = word ? map(r, ctx + 0xA0u, 16, 0) : NULL;
        if (!fog) {
            *where = 0x001E0750u;
            *code = EM_HAZE_RING_FAULT_UNMAPPED;
            return -1;
        }
        memcpy(s_vu.vf[23], fog, 16);
    }
    const int rc = em_haze_ring_001E76A0(&h, node, sp);
    if (rc < 0) {
        fprintf(stderr, "em_haze_ring: fault %d in %08X at %08X\n", (int)h.fault, (unsigned)h.fault_function,
                (unsigned)h.fault_address);
        *where = h.fault_address ? h.fault_address : h.fault_function;
        *code = h.fault;
    }
    return rc;
}
