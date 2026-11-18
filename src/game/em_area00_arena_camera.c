/*
 * em_area00_arena_camera.c — 00194DB0 (the AREA00 boss arena camera) and
 * 0018C0C0 (the target copy), translated from the decomp's byte-matched C
 * (src/func_00194DB0.c, src/func_0018C0C0.c). See em_area00_arena_camera.h
 * and docs/AREA00_OVERLAY_PORT.md "The arena camera".
 *
 * Arithmetic is the EE's (em_ee_float.h) on raw bit patterns; the
 * operations, their operands and their order are the original's (one
 * product, one quotient, two products for the eye; no fused forms).
 */
#include "em_area00_arena_camera.h"

#include <string.h>

#include "em_ee_float.h"

#define D_008105D0 0x008105D0u
#define D_008105E0 0x008105E0u

typedef struct AC {
    const EmA00ArenaCamHooks *h;
    uint32_t *fault;
    int failed;
} AC;

static int ac_fail(AC *o, uint32_t address)
{
    if (!o->failed) {
        o->failed = 1;
        if (o->fault) *o->fault = address;
    }
    return -1;
}

static uint32_t ac_u32(AC *o, uint32_t address)
{
    if (o->failed) return 0;
    uint8_t *p = (address & 3u) ? NULL : o->h->bytes(o->h->ctx, address, 4);
    if (!p) { ac_fail(o, address); return 0; }
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

static uint16_t ac_u16(AC *o, uint32_t address)
{
    if (o->failed) return 0;
    uint8_t *p = (address & 1u) ? NULL : o->h->bytes(o->h->ctx, address, 2);
    if (!p) { ac_fail(o, address); return 0; }
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}

static void ac_w32(AC *o, uint32_t address, uint32_t value)
{
    if (o->failed) return;
    uint8_t *p = (address & 3u) ? NULL : o->h->bytes(o->h->ctx, address, 4);
    if (!p) { ac_fail(o, address); return; }
    memcpy(p, &value, 4);
}

#define K(x) em_ee_bits(x)
#define LT(a, b) em_ee_c_lt_bits((a), (b))
#define LE(a, b) em_ee_c_le_bits((a), (b))

/* A float leaf: f0 = fn(f12). */
static uint32_t ac_leaf(AC *o, int (*fn)(void *, float, float *), uint32_t address, uint32_t x)
{
    if (o->failed) return 0;
    float r = 0.0f;
    if (!fn || fn(o->h->ctx, em_ee_float(x), &r) < 0) { ac_fail(o, address); return 0; }
    return em_ee_bits(r);
}

static uint32_t ac_001B12B0(AC *o, uint32_t goal, uint32_t current, uint32_t rate)
{
    if (o->failed) return 0;
    float r = 0.0f;
    if (!o->h->w_001B12B0 ||
        o->h->w_001B12B0(o->h->ctx, em_ee_float(goal), em_ee_float(current), em_ee_float(rate), &r) < 0) {
        ac_fail(o, 0x001B12B0u);
        return 0;
    }
    return em_ee_bits(r);
}

static void ac_0018C0C0(AC *o, uint32_t cam)
{
    if (o->failed) return;
    if (!o->h->w_00102948 || o->h->w_00102948(o->h->ctx, D_008105E0, cam + 0x20u) < 0)
        ac_fail(o, 0x00102948u);
}

static int ac_begin(AC *o, const EmA00ArenaCamHooks *h, uint32_t *fault)
{
    o->h = h;
    o->fault = fault;
    o->failed = 0;
    if (fault) *fault = 0;
    if (!h || !h->bytes) { o->failed = 1; return -1; }
    return 0;
}

int em_a00_arena_camera_0018C0C0(const EmA00ArenaCamHooks *h, uint32_t cam, uint32_t *fault_address)
{
    AC o;
    if (ac_begin(&o, h, fault_address)) return -1;
    ac_0018C0C0(&o, cam);
    return o.failed ? -1 : 0;
}

int em_a00_arena_camera_00194DB0(const EmA00ArenaCamHooks *h, uint32_t cam, uint32_t ent,
                                 uint32_t *fault_address)
{
    AC o;
    if (ac_begin(&o, h, fault_address)) return -1;

    /* The player's z picks the eye radius and, clamped to [-1657, -1497],
     * the goal yaw: delta = pi * (z' + 1497) / -160. */
    const uint32_t pitch = ac_u32(&o, ent + 0xA8u);
    const uint32_t scale = LT(pitch, K(-1610.0f)) ? K(94.0f) : K(100.0f);
    uint32_t hi = pitch;
    if (!LE(hi, K(-1497.0f)))
        hi = K(-1497.0f);
    else if (LT(hi, K(-1657.0f)))
        hi = K(-1657.0f);
    hi = em_ee_add_bits(hi, K(1497.0f));
    const uint32_t delta = em_ee_div_bits(em_ee_mul_bits(K(3.1415927f), hi), K(-160.0f));

    uint32_t goal;
    if (LT(pitch, K(-1577.0f))) {
        /* the far half: pi/2 + (pi - delta) within [1.9198622, 2.792527] */
        goal = em_ee_add_bits(K(1.5707964f), em_ee_sub_bits(K(3.1415927f), delta));
        if (!LE(goal, K(2.792527f)))
            goal = K(2.792527f);
        else if (LT(goal, K(1.9198622f)))
            goal = K(1.9198622f);
    } else {
        /* the near half: (pi - delta) - pi/2 within [0.34906587, 1.2217306] */
        goal = em_ee_sub_bits(em_ee_sub_bits(K(3.1415927f), delta), K(1.5707964f));
        if (!LE(goal, K(1.2217306f)))
            goal = K(1.2217306f);
        else if (LT(goal, K(0.34906587f)))
            goal = K(0.34906587f);
    }

    /* The yaw +0xAC eases toward the goal at pi/900 a tick; in player state
     * 3 only while the wrapped difference exceeds pi/4. */
    if ((int32_t)ac_u32(&o, ent + 0x230u) == 3) {
        const uint32_t wrapped = ac_leaf(&o, o.h->w_001B1470, 0x001B1470u,
                                         em_ee_sub_bits(goal, ac_u32(&o, cam + 0xACu)));
        const uint32_t mag = ac_leaf(&o, o.h->w_0011DF78, 0x0011DF78u, wrapped);
        if (o.failed) return -1;
        if (!LE(mag, K(0.7853982f)))
            ac_w32(&o, cam + 0xACu, ac_001B12B0(&o, goal, ac_u32(&o, cam + 0xACu), K(0.0034906587f)));
    } else {
        ac_w32(&o, cam + 0xACu, ac_001B12B0(&o, goal, ac_u32(&o, cam + 0xACu), K(0.0034906587f)));
    }
    if (o.failed) return -1;

    /* The eye on the circle of radius `scale` round (-142, -1577). */
    const uint32_t s = ac_leaf(&o, o.h->w_0011E2A8, 0x0011E2A8u, ac_u32(&o, cam + 0xACu));
    if (o.failed) return -1;
    ac_w32(&o, cam + 0x10u, em_ee_add_bits(K(-142.0f), em_ee_mul_bits(scale, s)));
    const uint32_t c = ac_leaf(&o, o.h->w_0011DE90, 0x0011DE90u, ac_u32(&o, cam + 0xACu));
    if (o.failed) return -1;
    ac_w32(&o, cam + 0x18u, em_ee_add_bits(K(-1577.0f), em_ee_mul_bits(scale, c)));

    /* The eye height by +0x5A bit 0; the target's y at most -90 and its z
     * within [-1617, -1537]. */
    ac_w32(&o, cam + 0x14u, (ac_u16(&o, cam + 0x5Au) & 1u) ? K(-3.0f) : K(-13.0f));
    if (!LE(ac_u32(&o, cam + 0x20u), K(-90.0f)))
        ac_w32(&o, cam + 0x20u, K(-90.0f));
    if (LT(ac_u32(&o, cam + 0x28u), K(-1617.0f)))
        ac_w32(&o, cam + 0x28u, K(-1617.0f));
    else if (!LE(ac_u32(&o, cam + 0x28u), K(-1537.0f)))
        ac_w32(&o, cam + 0x28u, K(-1537.0f));
    if (o.failed) return -1;

    ac_0018C0C0(&o, cam);
    if (o.failed) return -1;
    if (!o.h->w_0018D7B0 || o.h->w_0018D7B0(o.h->ctx, cam, 5) < 0) return ac_fail(&o, 0x0018D7B0u);
    if (!o.h->w_0018C6A0 || o.h->w_0018C6A0(o.h->ctx, cam + 0x10u, D_008105D0, 1.0f) < 0)
        return ac_fail(&o, 0x0018C6A0u);
    const uint32_t y = ac_u32(&o, cam + 0x14u);
    if (o.failed) return -1;
    if (!o.h->w_0018C4B0 || o.h->w_0018C4B0(o.h->ctx, D_008105D0, em_ee_float(y), 1.0f) < 0)
        return ac_fail(&o, 0x0018C4B0u);
    return 0;
}
