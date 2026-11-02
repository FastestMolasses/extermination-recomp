/* em_aim_fire_burst.c - see em_aim_fire_burst.h. */
#include "game/em_aim_fire_burst.h"

#include <stdio.h>
#include <string.h>

#include "game/em_actor_pool.h"
#include "game/em_area02_misc.h"
#include "game/em_ee_float.h"
#include "game/em_aim_fire_runtime.h"
#include "game/em_level11_port.h"

/* The stack these nodes run on, private to the composition (0x7E000000..,
 * em_aim_fire_runtime's region): their frames hold a packet descriptor and
 * a clip matrix they write before reading. Not game RAM. */
#define BURST_SP 0x7E001400u

typedef struct {
    EmAimFireLive *live;
    uint32_t node;
    uint32_t word34;   /* the node's +0x34 word store (see finish_34) */
    int wrote34;
} Host;

static uint8_t *view(Host *h, uint32_t a, uint32_t n, int write)
{
    if (a == h->node + 0x34u) {
        if (!write || n != 4) return NULL;
        h->wrote34 = 1;
        return (uint8_t *)&h->word34;
    }
    return em_aim_fire_live_map(h->live, a, n, write);
}

/* Both nodes store a word at +0x34, the empty contact 001E49F0 / 001E4600;
 * its upper half is the record's halfword +0x36 (em_actor_pool.h). */
static void finish_34(Host *h)
{
    if (!h->wrote34) return;
    EmActor *a = em_aim_fire_pool_actor(h->node);
    if (!a) return;
    a->w34 = h->word34;
    a->h36 = (uint16_t)(h->word34 >> 16);
}

/* ---- 001E4CE0: em_area02_misc over the composition ---- */

static uint8_t *misc_view(void *ctx, uint32_t a, uint32_t n, int write) { return view(ctx, a, n, write); }

static int misc_call(void *ctx, EmArea02MiscCall *in)
{
    Host *h = ctx;
    EmAimFireTargetCall c = {0};
    if (in->na > sizeof c.a / sizeof c.a[0] || in->nf > sizeof c.f / sizeof c.f[0]) return -1;
    c.function = in->fn;
    c.sp = in->sp;
    c.na = in->na;
    c.nf = in->nf;
    memcpy(c.a, in->a, in->na * sizeof *c.a);
    memcpy(c.f, in->f, in->nf * sizeof *c.f);
    if (em_aim_fire_live_call(h->live, &c) < 0) return -1;
    in->v0 = c.v0;
    in->f0 = c.f0;
    return 0;
}

/* ---- 001E4610: em_level11_port over the composition ---- */

static uint8_t *l11_view(void *ctx, uint32_t a, uint32_t n) { return view(ctx, a, n, a == ((Host *)ctx)->node + 0x34u); }

static int forward(void *ctx, uint32_t fn, unsigned na, uint64_t a0, uint64_t a1, uint64_t a2, unsigned nf, float f0,
                   uint64_t *v0, uint32_t *f0_out)
{
    Host *h = ctx;
    EmAimFireTargetCall c = {0};
    c.function = fn;
    c.sp = BURST_SP;
    c.na = na;
    c.nf = nf;
    c.a[0] = a0;
    c.a[1] = a1;
    c.a[2] = a2;
    c.f[0] = em_ee_bits(f0);
    if (em_aim_fire_live_call(h->live, &c) < 0) return -1;
    if (v0) *v0 = c.v0;
    if (f0_out) *f0_out = c.f0;
    return 0;
}

#define ADDR(x) ((uint64_t)(int64_t)(int32_t)(x))
static int w_three(void *ctx, uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2)
{ return forward(ctx, fn, 3, ADDR(a0), ADDR(a1), ADDR(a2), 0, 0, NULL, NULL); }
static int w_001026A0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x001026A0u, a0, a1, a2); }
static int w_001028D0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x001028D0u, a0, a1, a2); }
static int w_00102918(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x00102918u, a0, a1, a2); }
static int w_00102C58(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x00102C58u, a0, a1, a2); }
static int w_001029C0(void *c, uint32_t a0) { return forward(c, 0x001029C0u, 1, ADDR(a0), 0, 0, 0, 0, NULL, NULL); }
static int w_001AFC10(void *c, uint32_t a0) { return forward(c, 0x001AFC10u, 1, ADDR(a0), 0, 0, 0, 0, NULL, NULL); }
static int w_f1(void *c, uint32_t fn, float f0, float *result)
{
    uint32_t bits = 0;
    if (forward(c, fn, 0, 0, 0, 0, 1, f0, NULL, &bits) < 0) return -1;
    if (result) *result = em_ee_float(bits);
    return 0;
}
static int w_0011DE90(void *c, float f0, float *result) { return w_f1(c, 0x0011DE90u, f0, result); }
static int w_0011E2A8(void *c, float f0, float *result) { return w_f1(c, 0x0011E2A8u, f0, result); }
static int w_00128250(void *c, float f0, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x00128250u, 0, 0, 0, 0, 1, f0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}
static int w_001B17A0(void *c, uint32_t a0, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001B17A0u, 1, ADDR(a0), 0, 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}
static int w_001CB5F0(void *c, uint32_t a0, int32_t n1, int32_t n2, uint32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CB5F0u, 3, ADDR(a0), ADDR(n1), ADDR(n2), 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (uint32_t)v0;
    return 0;
}
static int w_001CB900(void *c, uint32_t a0, int32_t n1, int32_t n2)
{ return forward(c, 0x001CB900u, 3, ADDR(a0), ADDR(n1), ADDR(n2), 0, 0, NULL, NULL); }
static int w_001CB950(void *c, uint32_t a0, int32_t n1, uint64_t q2, uint32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CB950u, 3, ADDR(a0), ADDR(n1), q2, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (uint32_t)v0;
    return 0;
}
static int w_001CCF70(void *c, uint32_t a0, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CCF70u, 1, ADDR(a0), 0, 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}
static int w_001CD370(void *c, int32_t n0, uint32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CD370u, 1, ADDR(n0), 0, 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (uint32_t)v0;
    return 0;
}
static int w_001CF470(void *c, uint32_t a0, uint32_t a1, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CF470u, 2, ADDR(a0), ADDR(a1), 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}

int em_aim_fire_burst_call(EmAimFireLive *live, EmAimFireTargetCall *f)
{
    if (f->function != EM_AIM_FIRE_BURST_CALLBACK && f->function != EM_AIM_FIRE_RING_CALLBACK) return 0;
    Host h = {live, (uint32_t)f->a[0], 0, 0};
    int rc;
    if (f->function == EM_AIM_FIRE_BURST_CALLBACK) {
        EmArea02Misc m;
        memset(&m, 0, sizeof m);
        m.call = misc_call;
        m.ctx = &h;
        m.sp = BURST_SP;
        rc = em_area02_misc_view_001E4CE0(&m, misc_view, h.node);
        if (rc < 0)
            fprintf(stderr, "aim/fire: 001E4CE0 fault code %d at %08X\n", (int)m.fault, (unsigned)m.fault_address);
    } else {
        EmLevel11PortHooks w;
        memset(&w, 0, sizeof w);
        w.ctx = &h;
        w.bytes = l11_view;
        w.w_001026A0 = w_001026A0;
        w.w_001028D0 = w_001028D0;
        w.w_00102918 = w_00102918;
        w.w_001029C0 = w_001029C0;
        w.w_00102C58 = w_00102C58;
        w.w_0011DE90 = w_0011DE90;
        w.w_0011E2A8 = w_0011E2A8;
        w.w_00128250 = w_00128250;
        w.w_001AFC10 = w_001AFC10;
        w.w_001B17A0 = w_001B17A0;
        w.w_001CB5F0 = w_001CB5F0;
        w.w_001CB900 = w_001CB900;
        w.w_001CB950 = w_001CB950;
        w.w_001CCF70 = w_001CCF70;
        w.w_001CD370 = w_001CD370;
        w.w_001CF470 = w_001CF470;
        EmLevel11PortFault fault = {0, 0};
        rc = em_level11_port_001E4610(&w, h.node, BURST_SP, &fault);
        if (rc < 0)
            fprintf(stderr, "aim/fire: 001E4610 fault code %d at %08X\n", (int)fault.code, (unsigned)fault.address);
    }
    if (rc < 0) return -1;
    finish_34(&h);
    return 1;
}
