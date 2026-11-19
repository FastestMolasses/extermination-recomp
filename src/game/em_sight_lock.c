/* em_sight_lock.c - see em_sight_lock.h. */
#include "game/em_sight_lock.h"

#include <string.h>

#include "game/em_status_draw.h"

#define NUMBER_BASE UINT32_C(0x008111D0) /* 001C5FB0's buffer */

typedef struct {
    EmSightLock *h;
    EmArea00HudVu vu;
} Run;

static int fail(EmSightLock *h, uint32_t fn, uint32_t address)
{
    if (!h->fault_function) {
        h->fault_function = fn ? fn : UINT32_MAX;
        h->fault_address = address;
    }
    return -1;
}

static uint8_t *lock_map(void *context, uint32_t address, uint32_t size, int write)
{
    Run *r = context;
    return r->h->map ? r->h->map(r->h->context, address, size, write) : NULL;
}

static int lock_call(void *context, EmAimFireReticleCall *c);

static void owner_init(EmAimFireReticle *o, Run *r, uint32_t sp)
{
    memset(o, 0, sizeof *o);
    o->context = r;
    o->map = lock_map;
    o->call = lock_call;
    o->sp = sp;
    o->vu = &r->vu;
}

static int nested(Run *r, EmAimFireReticleCall *c)
{
    EmAimFireReticle o;
    owner_init(&o, r, c->sp);
    const uint32_t style = (uint32_t)c->a[0], screen = (uint32_t)c->a[1], rgba = (uint32_t)c->a[2];
    const int status = c->function == 0x001DD2F0u ? em_aim_fire_reticle_001DD2F0(&o, style, screen, rgba)
                                                  : em_aim_fire_reticle_001DD600(&o, style, screen, rgba);
    if (status < 0) return fail(r->h, o.fault_function ? o.fault_function : c->function, o.fault_address);
    c->v0 = 0;
    return 0;
}

/* 001C5FB0(value, width, blank): the digits at D_008111D0, v0 = its address. */
static int number(Run *r, EmAimFireReticleCall *c)
{
    char text[8];
    if (!em_status_draw_001C5FB0(text, (int32_t)(uint32_t)c->a[0], (int32_t)(uint32_t)c->a[1],
                                 (int32_t)(uint32_t)c->a[2]))
        return fail(r->h, c->function, 0);
    const uint32_t n = (uint32_t)strlen(text) + 1u;
    uint8_t *d = lock_map(r, NUMBER_BASE, n, 1);
    if (!d) return fail(r->h, c->function, NUMBER_BASE);
    memcpy(d, text, n);
    c->v0 = NUMBER_BASE;
    return 0;
}

static int lock_call(void *context, EmAimFireReticleCall *c)
{
    Run *r = context;
    switch (c->function) {
    case 0x001DD2F0u: case 0x001DD600u:
        return nested(r, c);
    case 0x001C5FB0u:
        return number(r, c);
    case 0x001CBA50u: case 0x001281C0u:
        if (!r->h->call || r->h->call(r->h->context, c) < 0) return fail(r->h, c->function, 0);
        return 0;
    default:
        return fail(r->h, c->function, 0);
    }
}

int em_sight_lock_001DD170(EmSightLock *h, uint32_t sp, uint32_t style, uint32_t position, uint32_t kind,
                           uint32_t rgba, uint32_t value)
{
    if (!h || h->fault_function) return -1;
    Run r;
    memset(&r, 0, sizeof r);
    r.h = h;
    const uint8_t *cp = lock_map(&r, 0x00275670u, 4, 0);
    if (!cp) return fail(h, 0x001DD170u, 0x00275670u);
    const uint32_t ctx = (uint32_t)cp[0] | (uint32_t)cp[1] << 8 | (uint32_t)cp[2] << 16 | (uint32_t)cp[3] << 24;
    const uint8_t *fog = lock_map(&r, ctx + 0xA0u, 16, 0);
    if (!fog) return fail(h, 0x001DD170u, ctx + 0xA0u);
    memcpy(r.vu.vf[23], fog, 16);
    EmAimFireReticle o;
    owner_init(&o, &r, sp);
    if (em_aim_fire_reticle_001DD170(&o, style, position, kind, rgba, value) < 0)
        return fail(h, o.fault_function ? o.fault_function : 0x001DD170u, o.fault_address);
    return 0;
}
